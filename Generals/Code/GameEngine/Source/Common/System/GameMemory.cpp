/*
**	Command & Conquer Generals(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

////////////////////////////////////////////////////////////////////////////////
//																																						//
//  (c) 2001-2003 Electronic Arts Inc.																				//
//																																						//
////////////////////////////////////////////////////////////////////////////////

// FILE: Memory.cpp 
//-----------------------------------------------------------------------------
//                                                                          
//                       Westwood Studios Pacific.                          
//                                                                          
//                       Confidential Information                           
//                Copyright (C) 2001 - All Rights Reserved                  
//                                                                          
//-----------------------------------------------------------------------------
//
// Project:   RTS3
//
// File name: Memory.cpp
//
// Created:   Steven Johnson, August 2001
//
// Desc:      Memory manager
//
// ----------------------------------------------------------------------------

#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

// SYSTEM INCLUDES 

// USER INCLUDES 
#include "Common/GameMemory.h"
#include "Common/CriticalSection.h"
#include "Common/Errors.h"
#include "Common/GlobalData.h"
#include "Common/PerfTimer.h"
#ifdef MEMORYPOOL_DEBUG
#include "GameClient/ClientRandomValue.h"
#endif
#ifdef MEMORYPOOL_STACKTRACE
	#include "Common/StackDump.h"
#endif

#ifdef MEMORYPOOL_DEBUG
DECLARE_PERF_TIMER(MemoryPoolDebugging)
DECLARE_PERF_TIMER(MemoryPoolInitFilling)
#endif

#ifdef _INTERNAL
// for occasional debugging...
//#pragma optimize("", off)
//#pragma MESSAGE("************************************** WARNING, optimization disabled for debugging purposes")
#endif


// ----------------------------------------------------------------------------
// DEFINES 
// ----------------------------------------------------------------------------

/** 
	define MPSB_DLINK to add a backlink to MemoryPoolSingleBlock; this makes it 
	faster to free raw DMA blocks. 
	@todo verify this speedup is enough to be worth the extra space
*/
#define MPSB_DLINK

#ifdef MEMORYPOOL_DEBUG

	/**
		if you define MEMORYPOOL_INTENSE_VERIFY, we do intensive verifications after
		nearly every memory operation. this is OFF by default, since it slows down 
		things a lot, but is worth turning on for really obscure memory corruption issues.
	*/
	#ifndef MEMORYPOOL_INTENSE_VERIFY
		#define NO_MEMORYPOOL_INTENSE_VERIFY
	#endif

	/**
		if you define MEMORYPOOL_CHECK_BLOCK_OWNERSHIP, we do lots of calls to verify
		that a block actually belongs to the pool it is called with. this is great
		for debugging, but can be realllly slow, so is off by default.
	*/
	#ifndef MEMORYPOOL_CHECK_BLOCK_OWNERSHIP
		#define NO_MEMORYPOOL_CHECK_BLOCK_OWNERSHIP
	#endif

	static const char* FREE_SINGLEBLOCK_TAG_STRING			= "FREE_SINGLEBLOCK_TAG_STRING";
	const Short SINGLEBLOCK_MAGIC_COOKIE								= 12345;
	const Int GARBAGE_FILL_VALUE												= 0xdeadbeef;

	// flags for m_debugFlags
	enum
	{
		IGNORE_LEAKS		= 0x0001
	};

	// in debug mode (but not internal), save stacktraces too
	#if !defined(MEMORYPOOL_CHECKPOINTING) && defined(MEMORYPOOL_STACKTRACE) && defined(_DEBUG)
		#define MEMORYPOOL_SINGLEBLOCK_GETS_STACKTRACE
	#endif

	#define USE_FILLER_VALUE

	const Int MAX_INIT_FILLER_COUNT = 8;
	#ifdef USE_FILLER_VALUE
		static UnsignedInt s_initFillerValue = 0xf00dcafe; // will be replaced, should never be this value at runtime
		static void calcFillerValue(Int index)
		{
			s_initFillerValue = (index & 3) << 1;
			s_initFillerValue |= 0x01;
			s_initFillerValue |= (~(s_initFillerValue << 4)) & 0xf0;
			s_initFillerValue |= (s_initFillerValue << 8);
			s_initFillerValue |= (s_initFillerValue << 16);
			DEBUG_LOG(("Setting MemoryPool initFillerValue to %08x (index %d)\n",s_initFillerValue,index));
		}
	#endif

#endif

#ifdef MEMORYPOOL_BOUNDINGWALL

	#define WALLCOUNT (2)	// default setting of 8 requires 4*4*2==32 extra bytes PER BLOCK
	#define WALLSIZE	(WALLCOUNT * sizeof(Int))

#endif

#ifdef MEMORYPOOL_STACKTRACE

	#define MEMORYPOOL_STACKTRACE_SIZE				(20)
	#define MEMORYPOOL_STACKTRACE_SKIP_SIZE		(6)
	#define MEMORYPOOL_STACKTRACE_SIZE_BYTES	(MEMORYPOOL_STACKTRACE_SIZE * sizeof(void*))

#endif

// ----------------------------------------------------------------------------
// PRIVATE DATA 
// ----------------------------------------------------------------------------

#ifdef MEMORYPOOL_BOUNDINGWALL

	static Int theBoundingWallPattern = 0xbabeface;

#endif

#ifdef MEMORYPOOL_STACKTRACE

	/** the max number levels to dump in the stacktrace. a variable rather than
		constant so that you can fiddle with it in the debugger if desired, to
		get shorter or longer dumps. (you can't go longer than MEMORYPOOL_STACKTRACE_SIZE
		in any event. */
	static Int theStackTraceDepth = 16;

#endif

#ifdef MEMORYPOOL_DEBUG

	static Int theTotalSystemAllocationInBytes = 0;
	static Int thePeakSystemAllocationInBytes = 0;
	static Int theTotalLargeBlocks = 0;
	static Int thePeakLargeBlocks = 0;
	Int theTotalDMA = 0;
	Int thePeakDMA = 0;
	Int theWastedDMA = 0;
	Int thePeakWastedDMA = 0;

#define NO_INTENSE_DMA_BOOKKEEPING
#ifdef INTENSE_DMA_BOOKKEEPING
	struct UsedNPeak
	{
		Int used, peak, waste, peakwaste;
		UsedNPeak() : used(0), peak(0), waste(0), peakwaste(0) { }
	};
	typedef std::map< const char*, UsedNPeak, std::less<const char*> > UsedNPeakMap;
	static UsedNPeakMap TheUsedNPeakMap;
	static Int doingIntenseDMA = 0;
#endif


#endif

static Bool thePreMainInitFlag = false;
static Bool theMainInitFlag = false;

// ----------------------------------------------------------------------------
// PRIVATE PROTOTYPES 
// ----------------------------------------------------------------------------

/// @todo srj -- make this work for 8
#define MEM_BOUND_ALIGNMENT 4

static Int roundUpMemBound(Int i);
static void *sysAllocate(Int numBytes);
static void *sysAllocateDoNotZero(Int numBytes);
static void sysFree(void* p);
static void memset32(void* ptr, Int value, Int bytesToFill);
#ifdef MEMORYPOOL_STACKTRACE
static void doStackDumpOutput(const char* m);
static void doStackDump(void **stacktrace, int size);
#endif
static void preMainInitMemoryManager();

// ----------------------------------------------------------------------------
// PRIVATE FUNCTIONS 
// ----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
/** round up to the nearest multiple of MEM_BOUND_ALIGNMENT */
static Int roundUpMemBound(Int i)
{
	return (i + (MEM_BOUND_ALIGNMENT-1)) & ~(MEM_BOUND_ALIGNMENT-1);
}

//-----------------------------------------------------------------------------
/**
	identical to sysAllocateDoNotZero, except that the memory block returned
	is filled to all-zero-bytes.
*/
static void* sysAllocate(Int numBytes)
{
	void* p = ::GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, numBytes);
	if (!p)
		throw ERROR_OUT_OF_MEMORY;
#ifdef MEMORYPOOL_DEBUG
	{
		USE_PERF_TIMER(MemoryPoolDebugging)
		theTotalSystemAllocationInBytes += ::GlobalSize(p);
		if (thePeakSystemAllocationInBytes < theTotalSystemAllocationInBytes)
			thePeakSystemAllocationInBytes = theTotalSystemAllocationInBytes;
	}
#endif
	return p;
}

//-----------------------------------------------------------------------------
/** 
	this is the low-level allocator that we use to request memory from the OS.
	all (repeat, all) memory allocations in this module should ultimately
	go thru this routine (or sysAllocate).

	note: throws ERROR_OUT_OF_MEMORY on failure; never returns null
*/
static void* sysAllocateDoNotZero(Int numBytes)
{
	void* p = ::GlobalAlloc(GMEM_FIXED, numBytes);
	if (!p)
		throw ERROR_OUT_OF_MEMORY;
#ifdef MEMORYPOOL_DEBUG
	{
		USE_PERF_TIMER(MemoryPoolDebugging)
		#ifdef USE_FILLER_VALUE
		{
			USE_PERF_TIMER(MemoryPoolInitFilling)
			::memset32(p, s_initFillerValue, ::GlobalSize(p));
		}
		#endif
		theTotalSystemAllocationInBytes += ::GlobalSize(p);
		if (thePeakSystemAllocationInBytes < theTotalSystemAllocationInBytes)
			thePeakSystemAllocationInBytes = theTotalSystemAllocationInBytes;
	}
#endif
	return p;
}

//-----------------------------------------------------------------------------
/** 
	the counterpart to sysAllocate / sysAllocateDoNotZero; used to free blocks
	allocated by them. it is OK to pass null here (it will just be ignored).
*/
static void sysFree(void* p)
{
	if (p)
	{
#ifdef MEMORYPOOL_DEBUG
		{
			USE_PERF_TIMER(MemoryPoolDebugging)
			::memset32(p, GARBAGE_FILL_VALUE, ::GlobalSize(p));
			theTotalSystemAllocationInBytes -= ::GlobalSize(p);
		}
#endif
		::GlobalFree(p);
	}
}

// ----------------------------------------------------------------------------
/** 
	fills memory with a 32-bit value (note: assumes the ptr is 4-byte-aligned)
*/
static void memset32(void* ptr, Int value, Int bytesToFill)
{
	Int wordsToFill = bytesToFill>>2;
	bytesToFill -= (wordsToFill<<2);

	Int *p = (Int*)ptr;
	for (++wordsToFill; --wordsToFill; )
		*p++ = value;

	Byte *b = (Byte *)p;
	for (++bytesToFill; --bytesToFill; )
		*b++ = (Byte)value;
}

#ifdef MEMORYPOOL_STACKTRACE
// ----------------------------------------------------------------------------
/**
	This is just a convenience routine that dumps output from the StackDump module
	to our normal debug log file, with a little massaging for formatting.
*/
static void doStackDumpOutput(const char* m)
{
	const char *PREPEND = "STACKTRACE";
	if (*m == 0 || strcmp(m, "\n") == 0)
	{
		DEBUG_LOG((m));
	}
	else
	{
		// Note - I am moving the prepend to the end, as this allows double clicking in the 
		// output window to open the file in VisualStudio.  jba.
		DEBUG_LOG(("%s,    %s",m, PREPEND));
	}
}
#endif

#ifdef MEMORYPOOL_STACKTRACE
// ----------------------------------------------------------------------------
/**
	dump the given stacktrace to the debug log.
*/
static void doStackDump(void **stacktrace, int size)
{
	::doStackDumpOutput("Allocation Stack Trace:");
	::doStackDumpOutput("\n");
	// ::StackDumpFromAddresses(stacktrace, size, ::doStackDumpOutput);
}
#endif

// ----------------------------------------------------------------------------
// PRIVATE TYPES 
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
#ifdef MEMORYPOOL_CHECKPOINTING
/**
	This is a auxiliary record that we allocate in debug modes (actually, checkpoint modes)
	to retain extra information about blocks; there is a one-to-one correspondence
	between this record and a block allocation. The interesting bit about this record is that
	we don't deallocate it when the corresponding block is freed; we retain it so we can
	later provide information about what blocks were freed when, etc. Yes, this does chew
	up a lot of memory! That's why it's debug-mode only; the presumption is that developers
	machines have boatloads of RAM. (Note that we *do* free these when resetCheckpoints() is called.)
	Note also that we directly allocate/free these with sysAllocate/sysFree, so ctors/dtors
	are never executed, nor would virtual functions work -- I know, it's a little evil.
*/
class BlockCheckpointInfo
{
private:
	BlockCheckpointInfo				*m_next;										///< next checkpoint in this pool/dma
	const char								*m_debugLiteralTagString;		///< the tagstring for the block
	Int												m_allocCheckpoint;					///< when it was allocated
	Int												m_freeCheckpoint;						///< when it was freed (-1 if still in existence)
	Int												m_blockSize;								///< logical size of the block
#ifdef MEMORYPOOL_STACKTRACE
	void*											m_stacktrace[MEMORYPOOL_STACKTRACE_SIZE];		///< stacktrace of when block was allocated
#endif

	~BlockCheckpointInfo() {};

public:

	BlockCheckpointInfo *getNext();	///< return next checkpointinfo for this pool/dma

	void debugSetFreepoint(Int f);	///< set the checkpoint at which the block was freed.

#ifdef MEMORYPOOL_STACKTRACE
	void **getStacktraceInfo();			///< return a ptr to the allocation stacktrace info.
#endif
	
	static BlockCheckpointInfo *addToList(
		BlockCheckpointInfo **pHead,
		const char *debugLiteralTagString,
		Int allocCheckpoint,
		Int blockSize
	);
	
	static void freeList(BlockCheckpointInfo **pHead);

	Bool shouldBeInReport(Int flags, Int startCheckpoint, Int endCheckpoint);

	static void doBlockCheckpointReport( BlockCheckpointInfo *bi, const char *poolName, 
									Int flags, Int startCheckpoint, Int endCheckpoint );
};

#endif

// ----------------------------------------------------------------------------
/**
	This is the fundamental allocation unit; when you allocate via (say) MemoryPool::allocateBlock,
	this is what is being allocated for you. (Of course, you don't see the private fields.)
	For the most part, we allocate big chunks of these in a monolithic Blob and subdivide 
	from there. (However, we occasionally allocate these individually, for large blocks.)

	Note also that we directly allocate/free these with sysAllocate/sysFree, so ctors/dtors
	are never executed, nor would virtual functions work -- I know, it's a little evil.
*/
class MemoryPoolSingleBlock 
{
private:

	MemoryPoolBlob				*m_owningBlob;			///< will be NULL if the single block was allocated via sysAllocate()
	MemoryPoolSingleBlock	*m_nextBlock;				///< if m_owningBlob is nonnull, this points to next free (unallocated) block in the blob; if m_owningBlob is null, this points to the next used (allocated) raw block in the pool.
#ifdef MPSB_DLINK
	MemoryPoolSingleBlock	*m_prevBlock;				///< if m_owningBlob is nonnull, this points to prev free (unallocated) block in the blob; if m_owningBlob is null, this points to the prev used (allocated) raw block in the pool.
#endif
#ifdef MEMORYPOOL_CHECKPOINTING
	BlockCheckpointInfo		*m_checkpointInfo;	///< ptr to the checkpointinfo for this block
#endif
#ifdef MEMORYPOOL_BOUNDINGWALL
	Int										m_wallPattern;			///< unique seed value for the bounding-walls for this block
#endif
#ifdef MEMORYPOOL_DEBUG
	const char						*m_debugLiteralTagString;	///< ptr to the tagstring for this block.
	Int										m_logicalSize;						///< logical size of block (not including overhead, walls, etc.)
	Int										m_wastedSize;							///< if allocated via DMA, the "wasted" bytes
	Short									m_magicCookie;						///< magic value used to verify that the block is one of ours (as opposed to random pointer)
	Short									m_debugFlags;							///< misc flags
	#ifdef MEMORYPOOL_SINGLEBLOCK_GETS_STACKTRACE
	void*									m_stacktrace[MEMORYPOOL_STACKTRACE_SIZE];		///< stacktrace of when block was allocated (if not checkpointing)
	#endif
#endif

private:

	void* getUserDataNoDbg();	
#ifdef MEMORYPOOL_BOUNDINGWALL
	void debugFillInWalls();	
#endif

public:
	
	static Int calcRawBlockSize(Int logicalSize);
	static MemoryPoolSingleBlock *rawAllocateSingleBlock(MemoryPoolSingleBlock **pRawListHead, Int logicalSize, MemoryPoolFactory *owningFactory DECLARE_LITERALSTRING_ARG2);
	void removeBlockFromList(MemoryPoolSingleBlock **pHead);

	void initBlock(Int logicalSize, MemoryPoolBlob *owningBlob, MemoryPoolFactory *owningFactory DECLARE_LITERALSTRING_ARG2);
	void* getUserData();
	static MemoryPoolSingleBlock *recoverBlockFromUserData(void* pUserData);
	MemoryPoolBlob *getOwningBlob();

	MemoryPoolSingleBlock *getNextFreeBlock();
	void setNextFreeBlock(MemoryPoolSingleBlock *b);
	MemoryPoolSingleBlock *getNextRawBlock();
	void setNextRawBlock(MemoryPoolSingleBlock *b);

#ifdef MEMORYPOOL_DEBUG
	void debugIgnoreLeaksForThisBlock();
	const char *debugGetLiteralTagString();
	Int debugGetLogicalSize();
	Int debugGetWastedSize();
	void debugSetWastedSize(Int waste);
	void debugVerifyBlock();
	void debugMarkBlockAsFree();
	Bool debugCheckUnderrun();
	Bool debugCheckOverrun();
	Int debugSingleBlockReportLeak(const char* owner);
#endif	// MEMORYPOOL_DEBUG
#ifdef MEMORYPOOL_CHECKPOINTING
	BlockCheckpointInfo *debugGetCheckpointInfo();
	void debugSetCheckpointInfo(BlockCheckpointInfo *bi);
	void debugResetCheckpoint();
#endif

};
// ----------------------------------------------------------------------------
class MemoryPoolBlob 
{
private:
	MemoryPool							*m_owningPool;				///< the pool that owns this blob
	MemoryPoolBlob					*m_nextBlob;					///< next blob in this pool
	MemoryPoolBlob					*m_prevBlob;					///< prev blob in this pool
	MemoryPoolSingleBlock		*m_firstFreeBlock;		///< ptr to first available block in this blob
	Int											m_usedBlocksInBlob;		///< total allocated blocks in this blob
	Int											m_totalBlocksInBlob;	///< total blocks in this blob (allocated + available)
	char										*m_blockData;					///< ptr to the blocks (really a MemoryPoolSingleBlock*)

public:

	MemoryPoolBlob();
	~MemoryPoolBlob();

	void initBlob(MemoryPool *owningPool, Int allocationCount);

	void addBlobToList(MemoryPoolBlob **ppHead, MemoryPoolBlob **ppTail);
	void removeBlobFromList(MemoryPoolBlob **ppHead, MemoryPoolBlob **ppTail);
	MemoryPoolBlob *getNextInList();
	Bool hasAnyFreeBlocks();

	MemoryPoolSingleBlock *allocateSingleBlock(DECLARE_LITERALSTRING_ARG1);
	void freeSingleBlock(MemoryPoolSingleBlock *block);

	MemoryPool *getOwningPool();
	Int getFreeBlockCount();
	Int getUsedBlockCount();
	Int getTotalBlockCount();

#ifdef MEMORYPOOL_DEBUG
	void debugMemoryVerifyBlob();
	Int debugBlobReportLeaks(const char* owner);
	Bool debugIsBlockInBlob(void *pBlock);
#endif
#ifdef MEMORYPOOL_CHECKPOINTING
	void debugResetCheckpoints();
#endif

};

// ----------------------------------------------------------------------------
// PUBLIC DATA 
// ----------------------------------------------------------------------------

MemoryPoolFactory *TheMemoryPoolFactory = NULL;
DynamicMemoryAllocator *TheDynamicMemoryAllocator = NULL;

// ----------------------------------------------------------------------------
// INLINES 
// ----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_CHECKPOINTING
inline BlockCheckpointInfo *BlockCheckpointInfo::getNext() { return m_next; }
inline void BlockCheckpointInfo::debugSetFreepoint(Int f) { DEBUG_ASSERTCRASH(m_freeCheckpoint == -1, ("already have a freepoint")); m_freeCheckpoint = f; }
#ifdef MEMORYPOOL_STACKTRACE
inline void **BlockCheckpointInfo::getStacktraceInfo() { return m_stacktrace; }
#endif
#endif

// ----------------------------------------------------------------------------
/**
	return a ptr to the user-data area of the block (ie, the part the enduser can deal with).
	this call does NO debug verification and is for internal use of class MemoryPoolSingleBlock only.
*/
inline void* MemoryPoolSingleBlock::getUserDataNoDbg()
{
	char* p = ((char*)this) + sizeof(MemoryPoolSingleBlock);
	#ifdef MEMORYPOOL_BOUNDINGWALL
	p += WALLSIZE;
	#endif
	return (void*)p;
}

/**
	return a ptr to the user-data area of the block (ie, the part the enduser can deal with).
	this call verifies that the block is valid in debug mode.
*/
inline void* MemoryPoolSingleBlock::getUserData()
{
	// yes, verify the block in this case for plain debug mode (not intense-verify mode)
	#ifdef MEMORYPOOL_DEBUG
	debugVerifyBlock();
	#endif
	return getUserDataNoDbg();
}

/**
	given a desired logical block size, calculate the physical size needed for each
	MemoryPoolSingleBlock (including overhead, etc.)
*/
inline /*static*/ Int MemoryPoolSingleBlock::calcRawBlockSize(Int logicalSize) 
{ 
	Int s = ::roundUpMemBound(logicalSize) + sizeof(MemoryPoolSingleBlock);
	#ifdef MEMORYPOOL_BOUNDINGWALL
	s += WALLSIZE*2;
	#endif
	return s;
}

/**
	accessor
*/
inline MemoryPoolBlob *MemoryPoolSingleBlock::getOwningBlob() 
{ 
	return m_owningBlob; 
}

/**
	return the next free block in this blob. this call assumes that the block
	in question belongs to a blob, and will assert if not.
*/
inline MemoryPoolSingleBlock *MemoryPoolSingleBlock::getNextFreeBlock() 
{ 
	DEBUG_ASSERTCRASH(m_owningBlob != NULL, ("must be called on blob block")); 
	return m_nextBlock; 
}

/**
	set the next free block in this blob. this call assumes that both blocks
	in question belongs to a blob, but will NOT assert if not, since it may be 
	called when the blocks are in an inconsistent state.
*/
inline void MemoryPoolSingleBlock::setNextFreeBlock(MemoryPoolSingleBlock *b) 
{ 
	//DEBUG_ASSERTCRASH(m_owningBlob != NULL && b->m_owningBlob != NULL, ("must be called on blob block")); 
	// don't check the 'b' block -- we need to call this before 'b' is fully initialized.
	DEBUG_ASSERTCRASH(m_owningBlob != NULL, ("must be called on blob block")); 
	this->m_nextBlock = b;
#ifdef MPSB_DLINK
	if (b) {
		b->m_prevBlock = this;
	}
#endif
}

/**
	return the next raw block in this dma. this call assumes that the block
	in question does NOT belong to a blob, and will assert if not.
*/
inline MemoryPoolSingleBlock *MemoryPoolSingleBlock::getNextRawBlock()
{ 
	DEBUG_ASSERTCRASH(m_owningBlob == NULL, ("must be called on raw block")); 
	return m_nextBlock; 
}

/**
	set the next raw block in this dma. this call assumes that the blocks
	in question do NOT belong to a blob, and will assert if not.
*/
inline void MemoryPoolSingleBlock::setNextRawBlock(MemoryPoolSingleBlock *b) 
{ 
	DEBUG_ASSERTCRASH(m_owningBlob == NULL && (!b || b->m_owningBlob == NULL), ("must be called on raw block")); 
	m_nextBlock = b; 
#ifdef MPSB_DLINK
	if (b)
		b->m_prevBlock = this;
#endif
}

#ifdef MEMORYPOOL_DEBUG
inline void MemoryPoolSingleBlock::debugIgnoreLeaksForThisBlock()
{
	//USE_PERF_TIMER(MemoryPoolDebugging) not worth it
	m_debugFlags |= IGNORE_LEAKS;
}
/**
	accessor
*/
inline const char *MemoryPoolSingleBlock::debugGetLiteralTagString() 
{ 
	//USE_PERF_TIMER(MemoryPoolDebugging) not worth it
	return m_debugLiteralTagString; 
}
#endif

#ifdef MEMORYPOOL_DEBUG
/**
	accessor
*/
inline Int MemoryPoolSingleBlock::debugGetLogicalSize() 
{ 
	//USE_PERF_TIMER(MemoryPoolDebugging) not worth it
	return m_logicalSize; 
}
#endif

#ifdef MEMORYPOOL_DEBUG
/**
	accessor
*/
inline Int MemoryPoolSingleBlock::debugGetWastedSize() 
{ 
	//USE_PERF_TIMER(MemoryPoolDebugging) not worth it
	return m_wastedSize; 
}
#endif

#ifdef MEMORYPOOL_DEBUG
inline void MemoryPoolSingleBlock::debugSetWastedSize(Int w) 
{ 
	//USE_PERF_TIMER(MemoryPoolDebugging) not worth it
	m_wastedSize = w; 
}
#endif

#ifdef MEMORYPOOL_CHECKPOINTING
/**
	accessor
*/
inline BlockCheckpointInfo *MemoryPoolSingleBlock::debugGetCheckpointInfo() 
{ 
	return m_checkpointInfo; 
}
#endif

#ifdef MEMORYPOOL_CHECKPOINTING
/**
	set the checkpoint info for this block.
*/
inline void MemoryPoolSingleBlock::debugSetCheckpointInfo(BlockCheckpointInfo *bi) 
{ 
	DEBUG_ASSERTCRASH(m_checkpointInfo == NULL, ("should be null")); 
	m_checkpointInfo = bi; 
}
#endif

#ifdef MEMORYPOOL_CHECKPOINTING
/**
	sets the checkpointinfo to null, but does NOT free it... the checkpointinfo
	is expected to be freed elsewhere.
*/
inline void MemoryPoolSingleBlock::debugResetCheckpoint() 
{ 
	m_checkpointInfo = NULL; 
}
#endif

// ----------------------------------------------------------------------------
/// accessor
inline MemoryPoolBlob *MemoryPoolBlob::getNextInList() { return m_nextBlob; }
/// accessor
inline Bool MemoryPoolBlob::hasAnyFreeBlocks() { return m_firstFreeBlock != NULL; }
/// accessor
inline MemoryPool *MemoryPoolBlob::getOwningPool() { return m_owningPool; }
/// accessor
inline Int MemoryPoolBlob::getFreeBlockCount() { return getTotalBlockCount() - getUsedBlockCount(); }
/// accessor
inline Int MemoryPoolBlob::getUsedBlockCount() { return m_usedBlocksInBlob; }
/// accessor
inline Int MemoryPoolBlob::getTotalBlockCount() { return m_totalBlocksInBlob; }

//-----------------------------------------------------------------------------
// METHODS for BlockCheckpointInfo
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_CHECKPOINTING
/// return true iff this checkpointinfo should be included in a checkpointreport with the given parameters.
Bool BlockCheckpointInfo::shouldBeInReport(Int flags, Int startCheckpoint, Int endCheckpoint)
{
	Bool allocFlagsOK = false;
	Bool freedFlagsOK = false;

	if (m_allocCheckpoint < startCheckpoint && (flags & _REPORT_CP_ALLOCATED_BEFORE))
		allocFlagsOK = true;
	if (m_allocCheckpoint >= startCheckpoint && m_allocCheckpoint < endCheckpoint && (flags & _REPORT_CP_ALLOCATED_BETWEEN))
		allocFlagsOK = true;

	if (m_freeCheckpoint == -1)
	{
		// block still exists! process this only if we want 'em.
		freedFlagsOK = (flags & _REPORT_CP_FREED_NEVER) ? true : false;
	}
	else
	{
		if (m_freeCheckpoint < startCheckpoint && (flags & _REPORT_CP_FREED_BEFORE))
			freedFlagsOK = true;
		if (m_freeCheckpoint >= startCheckpoint && m_freeCheckpoint < endCheckpoint && (flags & _REPORT_CP_FREED_BETWEEN))
			freedFlagsOK = true;
	}

	// the block must match both the 'alloc' and 'free' criteria to get a report
	return allocFlagsOK && freedFlagsOK;
}
#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_CHECKPOINTING
/// print a checkpointreport for the given checkpointinfo. if checkpointinfo is null, print column headers.
/*static*/ void BlockCheckpointInfo::doBlockCheckpointReport(BlockCheckpointInfo *bi, 
			const char *poolName, Int flags, Int startCheckpoint, Int endCheckpoint )
{
	const char *PREPEND = "BLOCKINFO";	// allows grepping more easily

	if (!bi)
	{
		DEBUG_LOG(("%s,%32s,%6s,%6s,%6s,%s\n",PREPEND,"POOLNAME","BLKSZ","ALLOC","FREED","BLOCKNAME"));
	}
	else
	{
		DEBUG_ASSERTCRASH(startCheckpoint >= 0 && startCheckpoint <= endCheckpoint, ("bad checkpoints"));
		DEBUG_ASSERTCRASH((flags & _REPORT_CP_ALLOCATED_DONTCARE) != 0, ("bad flags: must set at least one alloc flag"));
		DEBUG_ASSERTCRASH((flags & _REPORT_CP_FREED_DONTCARE) != 0, ("bad flags: must set at least one freed flag"));

		if (bi->shouldBeInReport(flags, startCheckpoint, endCheckpoint))
		{
			DEBUG_LOG(("%s,%32s,%6d,%6d,%6d,%s\n",PREPEND,poolName,bi->m_blockSize,bi->m_allocCheckpoint,bi->m_freeCheckpoint,bi->m_debugLiteralTagString));
	#ifdef MEMORYPOOL_STACKTRACE
			if (flags & REPORT_CP_STACKTRACE)
			{
				::doStackDump(bi->m_stacktrace, min(MEMORYPOOL_STACKTRACE_SIZE, theStackTraceDepth ));
			}
	#endif
		}
	}
}
#endif

// ----------------------------------------------------------------------------
#ifdef MEMORYPOOL_CHECKPOINTING
/// free an entire list of checkpointinfos.
/*static*/ void BlockCheckpointInfo::freeList(BlockCheckpointInfo **pHead)
{
	BlockCheckpointInfo *p = *pHead;
	while (p)
	{
		BlockCheckpointInfo *n = p->m_next;
		::sysFree((void *)p);
		p = n;
	}
	*pHead = NULL;
}
#endif

// ----------------------------------------------------------------------------
#ifdef MEMORYPOOL_CHECKPOINTING
/**
	allocate a new checkpointinfo with the given tag/checkpoint/size, add it to the
	linked list, and return the checkpointinfo. (note that this will NOT throw an exception;
	if there is not enough memory to allocate a new checkpointinfo, it will quietly return null.)
*/
/*static*/ BlockCheckpointInfo *BlockCheckpointInfo::addToList(
	BlockCheckpointInfo **pHead,
	const char *debugLiteralTagString,
	Int allocCheckpoint,
	Int blockSize
)
{
	DEBUG_ASSERTCRASH(debugLiteralTagString != FREE_SINGLEBLOCK_TAG_STRING, ("bad tag string"));

	BlockCheckpointInfo *freed = NULL;
	try {
		freed = (BlockCheckpointInfo *)::sysAllocate(sizeof(BlockCheckpointInfo));
	} catch (...) {
		freed = NULL;
	}
	if (freed) 
	{
		DEBUG_ASSERTCRASH(debugLiteralTagString != NULL, ("null tagstrings are not allowed"));
		freed->m_debugLiteralTagString = debugLiteralTagString;
		freed->m_allocCheckpoint = allocCheckpoint;
		freed->m_freeCheckpoint = -1;
		freed->m_blockSize = blockSize;
		freed->m_next = *pHead;
		*pHead = freed;
	}
	return freed;
}
#endif

//-----------------------------------------------------------------------------
// METHODS for MemoryPoolSingleBlock
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
/**
	fill in a block's fields. this is usually done only just after a block is allocated,
	but might also be done at other points in debug mode.
*/
void MemoryPoolSingleBlock::initBlock(Int logicalSize, MemoryPoolBlob *owningBlob, 
			MemoryPoolFactory *owningFactory DECLARE_LITERALSTRING_ARG2)
{
	// Note that while it is OK for owningBlob to be null, it is NEVER ok
	// for owningFactory to be null.
	DEBUG_ASSERTCRASH(owningFactory, ("null factory"));

#ifdef MEMORYPOOL_DEBUG
{
	USE_PERF_TIMER(MemoryPoolDebugging)
	m_magicCookie = SINGLEBLOCK_MAGIC_COOKIE;
	m_debugFlags = 0;
	if (!theMainInitFlag)
		debugIgnoreLeaksForThisBlock();
	DEBUG_ASSERTCRASH(debugLiteralTagString != NULL, ("null tagstrings are not allowed"));
	m_debugLiteralTagString = debugLiteralTagString;
	m_logicalSize = logicalSize;
	m_wastedSize = 0;

#ifdef MEMORYPOOL_SINGLEBLOCK_GETS_STACKTRACE
	if (theStackTraceDepth > 0 && (!TheGlobalData || TheGlobalData->m_checkForLeaks)) 
	{
		memset(m_stacktrace, 0, MEMORYPOOL_STACKTRACE_SIZE_BYTES);
		::FillStackAddresses(m_stacktrace, min(MEMORYPOOL_STACKTRACE_SIZE, theStackTraceDepth), MEMORYPOOL_STACKTRACE_SKIP_SIZE);
	}
	else 
	{
		m_stacktrace[0] = NULL;
	}
#endif
}
#endif MEMORYPOOL_DEBUG
#ifdef MEMORYPOOL_CHECKPOINTING
	m_checkpointInfo = NULL;
#endif
#ifdef MEMORYPOOL_BOUNDINGWALL
	m_wallPattern = theBoundingWallPattern++;
	debugFillInWalls();
#endif

	m_nextBlock = NULL;
#ifdef MPSB_DLINK
	m_prevBlock = NULL;
#endif
	m_owningBlob = owningBlob;	// could be NULL
}

//-----------------------------------------------------------------------------
/**
	given a 'public' ptr to a block (ie, the ptr returned by the MemoryPool::allocateBlock),
	recover the ptr to the MemoryPoolSingleBlock, so we can access the hidden fields.
*/
/* static */ MemoryPoolSingleBlock *MemoryPoolSingleBlock::recoverBlockFromUserData(void* pUserData)
{
	DEBUG_ASSERTCRASH(pUserData, ("null pUserData"));
	if (!pUserData)
		return NULL;
	char* p = ((char*)pUserData) - sizeof(MemoryPoolSingleBlock);
	#ifdef MEMORYPOOL_BOUNDINGWALL
	p -= WALLSIZE;
	#endif
	MemoryPoolSingleBlock *block = (MemoryPoolSingleBlock *)p;
// yes, verify the block in this case for plain debug mode (not intense-verify mode)
#ifdef MEMORYPOOL_DEBUG
	block->debugVerifyBlock();
#endif
	return block;
}

//-----------------------------------------------------------------------------
/**
	allocate and initialize a single block. this should only used by DynamicMemoryAllocator
	when allocating an extraordinarily large block.
*/
/*static*/ MemoryPoolSingleBlock *MemoryPoolSingleBlock::rawAllocateSingleBlock(
	MemoryPoolSingleBlock **pRawListHead, 
	Int logicalSize, 
	MemoryPoolFactory *owningFactory
	DECLARE_LITERALSTRING_ARG2)
{
	MemoryPoolSingleBlock *block = (MemoryPoolSingleBlock *)::sysAllocateDoNotZero(calcRawBlockSize(logicalSize));
	block->initBlock(logicalSize, NULL, owningFactory PASS_LITERALSTRING_ARG2);
	block->setNextRawBlock(*pRawListHead);
	*pRawListHead = block;
	return block;
}

//-----------------------------------------------------------------------------
/**
	remove the block from the list, which is presumed to be a list of raw blocks.
	generally, only the DynamicMemoryAllocator should call this.
*/
void MemoryPoolSingleBlock::removeBlockFromList(MemoryPoolSingleBlock **pHead)
{
	DEBUG_ASSERTCRASH(this->m_owningBlob == NULL, ("this function should only be used on raw blocks"));
#ifdef MPSB_DLINK
	DEBUG_ASSERTCRASH(this->m_nextBlock == NULL || this->m_nextBlock->m_owningBlob == NULL, ("this function should only be used on raw blocks"));
	if (this->m_prevBlock)
	{
		DEBUG_ASSERTCRASH(this->m_prevBlock->m_owningBlob == NULL, ("this function should only be used on raw blocks"));
		DEBUG_ASSERTCRASH(*pHead != this, ("bad linkage"));
		this->m_prevBlock->m_nextBlock = this->m_nextBlock;
	}
	else
	{
		DEBUG_ASSERTCRASH(*pHead == this, ("bad linkage"));
		*pHead = this->m_nextBlock;
	}

	if (this->m_nextBlock)
	{
		DEBUG_ASSERTCRASH(this->m_nextBlock->m_owningBlob == NULL, ("this function should only be used on raw blocks"));
		this->m_nextBlock->m_prevBlock = this->m_prevBlock;
	}
#else
	// this isn't very efficient, and may need upgrading... but to do so
	// would require adding a back link, so I'd rather do some testing
	// first to see if it's really a speed issue in practice. (the only place
	// this is used is when freeing 'raw' blocks allocated via the DMA).
	MemoryPoolSingleBlock *prev = NULL;
	for (MemoryPoolSingleBlock *cur = *pHead; cur; cur = cur->m_nextBlock)
	{
		DEBUG_ASSERTCRASH(cur->m_owningBlob == NULL, ("this function should only be used on raw blocks"));
		if (cur == this)
		{
			if (prev)
			{
				prev->m_nextBlock = this->m_nextBlock;
			}
			else
			{
				*pHead = this->m_nextBlock;
			}
			break;
		}
		prev = cur;
	}
#endif
}

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_DEBUG
Int MemoryPoolSingleBlock::debugSingleBlockReportLeak(const char* owner)
{
	//USE_PERF_TIMER(MemoryPoolDebugging) skip end-of-run reporting stuff

	// if allocated before main... just ignore the leak.
	if (m_debugFlags & IGNORE_LEAKS)
		return 0;

	// it's free, ergo, not leaked.
	if (m_debugLiteralTagString == FREE_SINGLEBLOCK_TAG_STRING)
		return 0;

	if (strcmp(m_debugLiteralTagString, "STR_AsciiString::ensureUniqueBufferOfSize") == 0)
	{
		/** @todo srj -- we leak a bunch of these for some reason (probably due to leaking Win32LocalFile)
			so just ignore 'em for now... figure out later. */
	}
	else if (strstr(m_debugLiteralTagString, "Win32LocalFileSystem.cpp") != NULL)
	{
		/** @todo srj -- we leak a bunch of these for some reason 
			so just ignore 'em for now... figure out later. */
	}
	else
	{
		DEBUG_LOG(("Leaked a block of size %d, tagstring %s, from pool/dma %s\n",m_logicalSize,m_debugLiteralTagString,owner));
	}

	#ifdef MEMORYPOOL_SINGLEBLOCK_GETS_STACKTRACE
	if (!TheGlobalData || TheGlobalData->m_checkForLeaks)
		::doStackDump(m_stacktrace, min(MEMORYPOOL_STACKTRACE_SIZE, theStackTraceDepth));
	#endif

	return 1;
}
#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_DEBUG
/**
	Verify internal consistency of this block.
*/
void MemoryPoolSingleBlock::debugVerifyBlock()
{
	USE_PERF_TIMER(MemoryPoolDebugging)

	DEBUG_ASSERTCRASH(this, ("null this"));
	DEBUG_ASSERTCRASH(m_magicCookie == SINGLEBLOCK_MAGIC_COOKIE, ("wrong cookie"));
	DEBUG_ASSERTCRASH(m_debugLiteralTagString != NULL, ("bad tagstring"));
	/// @todo Put this check back in after the AI memory usage is under control (MSB)
	//DEBUG_ASSERTCRASH(m_logicalSize>0 && m_logicalSize < 0x00ffffff, ("unlikely value for m_logicalSize"));
	DEBUG_ASSERTCRASH(!m_nextBlock || m_nextBlock->m_owningBlob == m_owningBlob, ("owning blob mismatch..."));
#ifdef MPSB_DLINK
	DEBUG_ASSERTCRASH(!m_prevBlock || m_prevBlock->m_owningBlob == m_owningBlob, ("owning blob mismatch..."));
#endif
	debugCheckUnderrun();
	debugCheckOverrun();
}
#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_DEBUG
/**
	Fill block with bogus values and mark other internal fields for debugging purposes.
*/
void MemoryPoolSingleBlock::debugMarkBlockAsFree()
{
	USE_PERF_TIMER(MemoryPoolDebugging)

	::memset32(getUserDataNoDbg(), GARBAGE_FILL_VALUE, m_logicalSize);
	m_debugLiteralTagString = FREE_SINGLEBLOCK_TAG_STRING;
	#ifdef MEMORYPOOL_INTENSE_VERIFY
	debugVerifyBlock();
	#endif
}
#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_DEBUG
/**
	Returns true iff someone overwrote part of the first bounding wall
	(ie, tromped on memory just before the block)
*/
Bool MemoryPoolSingleBlock::debugCheckUnderrun()
{
	USE_PERF_TIMER(MemoryPoolDebugging)

#ifdef MEMORYPOOL_BOUNDINGWALL
	Int *p = (Int*)(((char*)getUserDataNoDbg()) - WALLSIZE);
	for (Int i = 0; i < WALLCOUNT; i++, p++)
	{
		if (*p != m_wallPattern+i)
		{
			DEBUG_CRASH(("memory underrun for block \"%s\" (expected %08x, got %08x)\n",m_debugLiteralTagString,m_wallPattern+i,*p));
			return true;
		}
	}
#endif
	return false;
}
#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_DEBUG
/**
	Returns true iff someone overwrote part of the second bounding wall
	(ie, tromped on memory just after the block)
*/
Bool MemoryPoolSingleBlock::debugCheckOverrun()
{
	USE_PERF_TIMER(MemoryPoolDebugging)

#ifdef MEMORYPOOL_BOUNDINGWALL
	Int *p = (Int*)(((char*)getUserDataNoDbg()) + m_logicalSize);
	for (Int i = 0; i < WALLCOUNT; i++, p++)
	{
		if (*p != m_wallPattern-i)
		{
			DEBUG_CRASH(("memory overrun for block \"%s\" (expected %08x, got %08x)\n",m_debugLiteralTagString,m_wallPattern+i,*p));
			return true;
		}
	}
#endif
	return false;
}
#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_BOUNDINGWALL
/**
	Fill in the proper values for this block's bounding walls.
*/
void MemoryPoolSingleBlock::debugFillInWalls()
{
	Int *p;
	Int i;

	p = (Int*)(((char*)getUserDataNoDbg()) - WALLSIZE);
	for (i = 0; i < WALLCOUNT; i++)
		*p++ = m_wallPattern+i;

	p = (Int*)(((char*)getUserDataNoDbg()) + m_logicalSize);
	for (i = 0; i < WALLCOUNT; i++)
		*p++ = m_wallPattern-i;

	#ifdef MEMORYPOOL_INTENSE_VERIFY
	debugVerifyBlock();
	#endif
}
#endif


//-----------------------------------------------------------------------------
// METHODS for MemoryPoolBlob
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
/**
	fill in safe default values.
*/
MemoryPoolBlob::MemoryPoolBlob() :
	m_owningPool(NULL),
	m_nextBlob(NULL),
	m_prevBlob(NULL),
	m_firstFreeBlock(NULL),
	m_usedBlocksInBlob(0),
	m_totalBlocksInBlob(0),
	m_blockData(NULL)
{
}

//-----------------------------------------------------------------------------
/**
	throw away the blob. free the block data, if any.
*/
MemoryPoolBlob::~MemoryPoolBlob()
{
	::sysFree((void *)m_blockData);
}

//-----------------------------------------------------------------------------
/**
	initialize a Blob; this is called just after the blob is allocated.
	allocate space for the blocks in this blob and initialize all those blocks.
*/
void MemoryPoolBlob::initBlob(MemoryPool *owningPool, Int allocationCount)
{
	DEBUG_ASSERTCRASH(m_blockData == NULL, ("unlikely init call"));

	m_owningPool = owningPool;
	m_totalBlocksInBlob = allocationCount;
	m_usedBlocksInBlob = 0;

	Int rawBlockSize = MemoryPoolSingleBlock::calcRawBlockSize(m_owningPool->getAllocationSize());
	m_blockData = (char *)::sysAllocate(rawBlockSize * m_totalBlocksInBlob);	// throws on failure

	// set up the list of free blocks in the blob (namely, all of 'em)
	MemoryPoolSingleBlock *block = (MemoryPoolSingleBlock *)m_blockData;
	MemoryPoolSingleBlock *next;
	for (Int i = m_totalBlocksInBlob-1; i >= 0; i--)
	{
		next = (MemoryPoolSingleBlock *)(((char *)block) + rawBlockSize);
#ifdef MEMORYPOOL_DEBUG
		block->initBlock(m_owningPool->getAllocationSize(), this, owningPool->getOwningFactory(), FREE_SINGLEBLOCK_TAG_STRING);
#else
		block->initBlock(m_owningPool->getAllocationSize(), this, owningPool->getOwningFactory());
#endif
		block->setNextFreeBlock((i > 0) ? next : NULL);
#ifdef MEMORYPOOL_DEBUG
		block->debugMarkBlockAsFree();
#endif
		block = next;
	}
	m_firstFreeBlock = (MemoryPoolSingleBlock *)m_blockData;

#ifdef MEMORYPOOL_INTENSE_VERIFY
	debugMemoryVerifyBlob();
#endif
}

//-----------------------------------------------------------------------------
/**
	add this blob to a given pool's list-of-blobs
*/
void MemoryPoolBlob::addBlobToList(MemoryPoolBlob **ppHead, MemoryPoolBlob **ppTail)
{
	m_prevBlob = *ppTail;
	m_nextBlob =  NULL;

	if (*ppTail != NULL)
		(*ppTail)->m_nextBlob = this;

	if (*ppHead == NULL)
		*ppHead = this;

	*ppTail = this;
}

//-----------------------------------------------------------------------------
/**
	remove this blob from a given pool's list-of-blobs
*/
void MemoryPoolBlob::removeBlobFromList(MemoryPoolBlob **ppHead, MemoryPoolBlob **ppTail)
{
	if (*ppHead == this)
		*ppHead = this->m_nextBlob;
	else
		this->m_prevBlob->m_nextBlob = this->m_nextBlob;
		
	if (*ppTail == this)
		*ppTail = this->m_prevBlob;
	else
		this->m_nextBlob->m_prevBlob = this->m_prevBlob;
}

//-----------------------------------------------------------------------------
/**
	grab a free block from this blob, mark it as taken, and return it.
	this method assumes that there is at least one free block in the blob!
*/
MemoryPoolSingleBlock *MemoryPoolBlob::allocateSingleBlock(DECLARE_LITERALSTRING_ARG1)
{
	DEBUG_ASSERTCRASH(m_firstFreeBlock, ("no free blocks available in MemoryPoolBlob"));
	
	MemoryPoolSingleBlock *block = m_firstFreeBlock;
	m_firstFreeBlock = block->getNextFreeBlock();
	++m_usedBlocksInBlob;

#ifdef MEMORYPOOL_INTENSE_VERIFY
	block->debugVerifyBlock();
#endif
#ifdef MEMORYPOOL_DEBUG
	// this is debug-only because it only serves to update the debugLiteralTagString.
	block->initBlock(m_owningPool->getAllocationSize(), this, m_owningPool->getOwningFactory(), debugLiteralTagString);
#endif
#ifdef MEMORYPOOL_INTENSE_VERIFY
	debugMemoryVerifyBlob();
#endif

//	don't need to zero this out... the caller will do that, if necessary
//	memset(block->getUserData(), 0, m_owningPool->getAllocationSize());

	return block;
}

//-----------------------------------------------------------------------------
/**
	make this block available for future allocations. it is assumed that the block
	belongs to this blob, and is not already free. 
*/
void MemoryPoolBlob::freeSingleBlock(MemoryPoolSingleBlock *block)
{	
	DEBUG_ASSERTCRASH(block->getOwningBlob() == this, ("block does not belong to this blob"));

	block->setNextFreeBlock(m_firstFreeBlock);
	m_firstFreeBlock = block;
	--m_usedBlocksInBlob;

#ifdef MEMORYPOOL_INTENSE_VERIFY
	block->debugVerifyBlock();
#endif
#ifdef MEMORYPOOL_DEBUG
	block->debugMarkBlockAsFree();
#endif
#ifdef MEMORYPOOL_INTENSE_VERIFY
	debugMemoryVerifyBlob();
#endif
	
}

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_DEBUG
/**
	Perform internal consistency checking on this blob and all its blocks.
*/
void MemoryPoolBlob::debugMemoryVerifyBlob()
{
	USE_PERF_TIMER(MemoryPoolDebugging)

	DEBUG_ASSERTCRASH(m_owningPool != NULL, ("bad owner"));
	DEBUG_ASSERTCRASH(m_usedBlocksInBlob >= 0 && m_usedBlocksInBlob <= m_totalBlocksInBlob, ("unlikely m_usedBlocksInBlob"));
	DEBUG_ASSERTCRASH(m_totalBlocksInBlob > 0, ("unlikely m_totalBlocksInBlob"));

	Int rawBlockSize = MemoryPoolSingleBlock::calcRawBlockSize(m_owningPool->getAllocationSize());
	char *blockData = m_blockData;
	for (Int i = m_totalBlocksInBlob-1; i >= 0; i--, blockData += rawBlockSize)
	{
		MemoryPoolSingleBlock *block = (MemoryPoolSingleBlock *)blockData;
		block->debugVerifyBlock();
	}
}
#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_DEBUG
Int MemoryPoolBlob::debugBlobReportLeaks(const char* owner)
{
	//USE_PERF_TIMER(MemoryPoolDebugging) skip end-of-run reporting stuff

	Int any = 0;
	Int rawBlockSize = MemoryPoolSingleBlock::calcRawBlockSize(m_owningPool->getAllocationSize());
	char *blockData = m_blockData;
	for (Int i = m_totalBlocksInBlob-1; i >= 0; i--, blockData += rawBlockSize)
	{
		MemoryPoolSingleBlock *block = (MemoryPoolSingleBlock *)blockData;
		any += block->debugSingleBlockReportLeak(owner);
	}
	return any;
}
#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_DEBUG
/**
	return true iff this block belongs to this blob.
*/
Bool MemoryPoolBlob::debugIsBlockInBlob(void *pBlockPtr)
{
	USE_PERF_TIMER(MemoryPoolDebugging)

	MemoryPoolSingleBlock *block = MemoryPoolSingleBlock::recoverBlockFromUserData(pBlockPtr);
	Int rawBlockSize = MemoryPoolSingleBlock::calcRawBlockSize(m_owningPool->getAllocationSize());
	char *blockData = m_blockData;
	for (Int i = m_totalBlocksInBlob-1; i >= 0; i--)
	{
		if ((char *)block == blockData)
			return true;
		blockData += rawBlockSize;
	}
	return false;
}
#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_CHECKPOINTING
/**
	set all the checkpointinfos to null for all the blocks in this blob.
	this does NOT free the checkpointinfos; that is presumed to happen elsewhere.
*/
void MemoryPoolBlob::debugResetCheckpoints()
{
	Int rawBlockSize = MemoryPoolSingleBlock::calcRawBlockSize(m_owningPool->getAllocationSize());
	char *blockData = m_blockData;
	for (Int i = m_totalBlocksInBlob-1; i >= 0; i--, blockData += rawBlockSize)
	{
		MemoryPoolSingleBlock *block = (MemoryPoolSingleBlock *)blockData;
		block->debugResetCheckpoint();
	}
}
#endif

//-----------------------------------------------------------------------------
// METHODS for Checkpointable
//-----------------------------------------------------------------------------

#ifdef MEMORYPOOL_CHECKPOINTING
//-----------------------------------------------------------------------------
/**
	init fields of Checkpointable to safe values.
*/
Checkpointable::Checkpointable() : 
	m_firstCheckpointInfo(NULL), 
	m_cpiEverFailed(false)
{
}
#endif

#ifdef MEMORYPOOL_CHECKPOINTING
//-----------------------------------------------------------------------------
/**
	destroy the object. discard any remaining checkpointinfo.
*/
Checkpointable::~Checkpointable()
{
	BlockCheckpointInfo::freeList(&m_firstCheckpointInfo);
	m_firstCheckpointInfo = NULL;
	m_cpiEverFailed = false;
}
#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_CHECKPOINTING
/**
	create a new checkpointinfo and fill it in appropriately. this does NOT
	throw an exception on failure; it quietly returns null if there is not
	enough memory, and sets a flag to indicate our checkpointinfo is not complete.
*/
BlockCheckpointInfo *Checkpointable::debugAddCheckpointInfo(
	const char *debugLiteralTagString,
	Int allocCheckpoint,
	Int blockSize
)
{

	BlockCheckpointInfo *bi = BlockCheckpointInfo::addToList(&m_firstCheckpointInfo, debugLiteralTagString,
				allocCheckpoint, blockSize);

	if (bi)
	{
#ifdef MEMORYPOOL_STACKTRACE
		void **stacktrace = bi->getStacktraceInfo();
		if (theStackTraceDepth > 0 && !TheGlobalData || TheGlobalData->m_checkForLeaks) 
		{
			memset(stacktrace, 0, MEMORYPOOL_STACKTRACE_SIZE_BYTES);
			::FillStackAddresses(stacktrace, min(MEMORYPOOL_STACKTRACE_SIZE, theStackTraceDepth), MEMORYPOOL_STACKTRACE_SKIP_SIZE);
		}
		else 
		{
			stacktrace[0] = NULL;
		}
#endif
	}
	else
	{
		m_cpiEverFailed = true;
	}

	return bi;
}
#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_CHECKPOINTING
/**
	print a report on the checkpointinfos belonging to this pool/dma.
*/
void Checkpointable::debugCheckpointReport( Int flags, Int startCheckpoint, Int endCheckpoint, const char *poolName )
{
	DEBUG_ASSERTCRASH(startCheckpoint >= 0 && startCheckpoint <= endCheckpoint, ("bad checkpoints"));
	DEBUG_ASSERTCRASH((flags & _REPORT_CP_ALLOCATED_DONTCARE) != 0, ("bad flags: must set at least one alloc flag"));
	DEBUG_ASSERTCRASH((flags & _REPORT_CP_FREED_DONTCARE) != 0, ("bad flags: must set at least one freed flag"));

	if (m_cpiEverFailed)
	{
		DEBUG_LOG(("  *** WARNING *** info on freed blocks may be inaccurate or incomplete!\n"));
	}

	for (BlockCheckpointInfo *bi = m_firstCheckpointInfo; bi; bi = bi->getNext())
	{
		BlockCheckpointInfo::doBlockCheckpointReport( bi, poolName, flags, startCheckpoint, endCheckpoint );
	}
}
#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_CHECKPOINTING
/**
	throw away all the checkpointinfos. this frees the memory, but blocks might still
	refer to the discarded infos; you must zero those elsewhere.
*/
void Checkpointable::debugResetCheckpoints()
{
	BlockCheckpointInfo::freeList(&m_firstCheckpointInfo);
}
#endif

//-----------------------------------------------------------------------------
// METHODS for MemoryPool
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
/**
	init to safe values.
*/
MemoryPool::MemoryPool() :
	m_factory(NULL),
	m_nextPoolInFactory(NULL),
	m_poolName(""),
	m_allocationSize(0),
	m_initialAllocationCount(0),
	m_overflowAllocationCount(0),
	m_usedBlocksInPool(0),
	m_totalBlocksInPool(0),
	m_peakUsedBlocksInPool(0),
	m_firstBlob(NULL),
	m_lastBlob(NULL),
	m_firstBlobWithFreeBlocks(NULL)
{
}

//-----------------------------------------------------------------------------
/**
	initialize the memory pool with the given parameters. allocate the initial
	set of blocks.
*/
void MemoryPool::init(MemoryPoolFactory *factory, const char *poolName, Int allocationSize, Int initialAllocationCount, Int overflowAllocationCount)
{
	m_factory = factory;
	m_poolName = poolName;
	m_allocationSize = ::roundUpMemBound(allocationSize);	// round up to four-byte boundary
	m_initialAllocationCount = initialAllocationCount;
	m_overflowAllocationCount = overflowAllocationCount;
	m_usedBlocksInPool = 0;
	m_totalBlocksInPool = 0;
	m_peakUsedBlocksInPool = 0;
	m_firstBlob = NULL;
	m_lastBlob = NULL;
	m_firstBlobWithFreeBlocks = NULL;

	// go ahead and init the initial block here (will throw on failure)
	createBlob(m_initialAllocationCount);
}

//-----------------------------------------------------------------------------
/**
	throw away the pool, and all blocks/blobs associated with it.
*/
MemoryPool::~MemoryPool()
{   
	// toss everything. we could do this slightly more efficiently,
	// but not really worth the extra code to do so.
	while (m_firstBlob) 
	{
		freeBlob(m_firstBlob);
	}
}

//-----------------------------------------------------------------------------
/**
	create a new blob for this pool. if you set up good values for initialAllocationCount,
	this should rarely (if ever) be called (though during development it will be called
	frequently).
*/
MemoryPoolBlob* MemoryPool::createBlob(Int allocationCount)
{
	DEBUG_ASSERTCRASH(allocationCount > 0 && allocationCount%MEM_BOUND_ALIGNMENT==0, ("bad allocationCount (must be >0 and evenly divisible by %d)",MEM_BOUND_ALIGNMENT));

	MemoryPoolBlob* blob = new (::sysAllocate(sizeof(MemoryPoolBlob))) MemoryPoolBlob;	// will throw on failure

	blob->initBlob(this, allocationCount);	// will throw on failure

	blob->addBlobToList(&m_firstBlob, &m_lastBlob);

	DEBUG_ASSERTCRASH(m_firstBlobWithFreeBlocks == NULL, ("DO NOT IGNORE. Please call John McD - x36872 (m_firstBlobWithFreeBlocks != NULL)"));
	m_firstBlobWithFreeBlocks = blob;

	// bookkeeping
	m_totalBlocksInPool += allocationCount;

#ifdef MEMORYPOOL_DEBUG
	m_factory->adjustTotals("", 0, allocationCount*getAllocationSize());
#endif

	return blob;
}
	
//-----------------------------------------------------------------------------
/**
	throw away a given blob, and all its blocks. it's assumed that the blob belongs
	to this pool.
*/
Int MemoryPool::freeBlob(MemoryPoolBlob* blob)
{
	DEBUG_ASSERTCRASH(blob, ("null blob"));
	DEBUG_ASSERTCRASH(blob->getOwningPool() == this, ("blob does not belong to this pool"));

	// save these for later...
	Int totalBlocksInBlob = blob->getTotalBlockCount();
	Int usedBlocksInBlob = blob->getUsedBlockCount();
	DEBUG_ASSERTCRASH(usedBlocksInBlob == 0, ("freeing a nonempty blob (%d)\n",usedBlocksInBlob));

	// this is really just an estimate... will be too small in debug mode.
	Int amtFreed = totalBlocksInBlob * getAllocationSize() + sizeof(MemoryPoolBlob);

	// de-link it from our list
	blob->removeBlobFromList(&m_firstBlob, &m_lastBlob);
	
	// ensure that the 'first free' blob is still a valid blob. 
	// (doesn't need to actually have free blocks, just be a valid blob) 
	if (m_firstBlobWithFreeBlocks == blob)
		m_firstBlobWithFreeBlocks = m_firstBlob;

	// this is evil... since there is no 'placement delete' we must do this the hard way
	// and call the dtor directly. ordinarily this is heinous, but in this case we'll
	// make an exception.
	blob->~MemoryPoolBlob();
	::sysFree((void *)blob);

	// finally... bookkeeping
	m_usedBlocksInPool -= usedBlocksInBlob;
	m_totalBlocksInPool -= totalBlocksInBlob;

#ifdef MEMORYPOOL_DEBUG
	m_factory->adjustTotals("", -usedBlocksInBlob*getAllocationSize(), -totalBlocksInBlob*getAllocationSize());
#endif

	return amtFreed;
}

//-----------------------------------------------------------------------------
/**
	allocate a block from this pool and return it, but don't bother zeroing
	out the block. if unable to allocate, throw ERROR_OUT_OF_MEMORY. this
	function will never return null.
*/
void* MemoryPool::allocateBlockDoNotZeroImplementation(DECLARE_LITERALSTRING_ARG1)
{
	ScopedCriticalSection scopedCriticalSection(TheMemoryPoolCriticalSection);

	if (m_firstBlobWithFreeBlocks != NULL && !m_firstBlobWithFreeBlocks->hasAnyFreeBlocks()) 
	{
		// hmm... the current 'free' blob has nothing available. look and see if there
		// are any other existing blobs with freespace.
		MemoryPoolBlob *blob;
		for (blob = m_firstBlob; blob != NULL; blob = blob->getNextInList()) 
		{
			if (blob->hasAnyFreeBlocks())
			 	break;
		}

		// note that if we walk thru the list without finding anything, this will 
		// reset m_firstBlobWithFreeBlocks to null and fall thru.
	 	m_firstBlobWithFreeBlocks = blob;
	}
	
	// OK, if we are here then we have no blobs with freespace... darn.
	// allocate an overflow block.
	if (m_firstBlobWithFreeBlocks == NULL) 
	{
		if (m_overflowAllocationCount == 0)
		{
			throw ERROR_OUT_OF_MEMORY;	// this pool is not allowed to grow
		}
		else 
		{
			createBlob(m_overflowAllocationCount); // throws on failure
		}
	}
	
	MemoryPoolBlob *blob = m_firstBlobWithFreeBlocks;

	DEBUG_ASSERTCRASH(blob, ("no blob with free blocks available in MemoryPool::allocate"));
		
	MemoryPoolSingleBlock *block = blob->allocateSingleBlock(PASS_LITERALSTRING_ARG1);
	DEBUG_ASSERTCRASH(block, ("should not fail here"));

#ifdef MEMORYPOOL_CHECKPOINTING
	BlockCheckpointInfo *bi = debugAddCheckpointInfo(block->debugGetLiteralTagString(), m_factory->getCurCheckpoint(), getAllocationSize());
	if (bi)
		block->debugSetCheckpointInfo(bi);
#endif

	// bookkeeping
	++m_usedBlocksInPool;
	if (m_peakUsedBlocksInPool < m_usedBlocksInPool)
		m_peakUsedBlocksInPool = m_usedBlocksInPool;

#ifdef MEMORYPOOL_DEBUG
	m_factory->adjustTotals(debugLiteralTagString, 1*getAllocationSize(), 0);
	#ifdef USE_FILLER_VALUE
	{
		USE_PERF_TIMER(MemoryPoolInitFilling)
		::memset32(block->getUserData(), s_initFillerValue, getAllocationSize());
	}
	#endif
#endif

	return block->getUserData();
}

//-----------------------------------------------------------------------------
/**
	allocate a block from this pool and return it, and zero out the contents
	of the block. if unable to allocate, throw ERROR_OUT_OF_MEMORY. this
	function will never return null.
*/
void* MemoryPool::allocateBlockImplementation(DECLARE_LITERALSTRING_ARG1)
{
	void* p = allocateBlockDoNotZeroImplementation(PASS_LITERALSTRING_ARG1);	// throws on failure
	memset(p, 0, getAllocationSize());
	return p;
}

//-----------------------------------------------------------------------------
/**
	free a block allocated by this pool. it's ok to pass null.
*/
void MemoryPool::freeBlock(void* pBlockPtr)
{
	if (!pBlockPtr)
		return;	// my, that was easy

	ScopedCriticalSection scopedCriticalSection(TheMemoryPoolCriticalSection);

	MemoryPoolSingleBlock *block = MemoryPoolSingleBlock::recoverBlockFromUserData(pBlockPtr);
	MemoryPoolBlob *blob = block->getOwningBlob();
#ifdef MEMORYPOOL_DEBUG
	const char* tagString = block->debugGetLiteralTagString();
#endif
	
	DEBUG_ASSERTCRASH(blob && blob->getOwningPool() == this, ("block does not belong to this pool"));

#ifdef MEMORYPOOL_CHECKPOINTING
	BlockCheckpointInfo *bi = block->debugGetCheckpointInfo();
	DEBUG_ASSERTCRASH(bi, ("hmm, no checkpoint info"));
	if (bi)
		bi->debugSetFreepoint(m_factory->getCurCheckpoint());
#endif

	blob->freeSingleBlock(block);
	
	// if we want to free the blobs as they become empty, do that here.
	// normally we don't bother, but just in case this is ever desired, here's how you'd do it...
	//
	// if (blob->m_usedBlocksInBlob == 0) 
	// {
	//	freeBlob(blob);
	//	return;
	//} 
	
	if (!m_firstBlobWithFreeBlocks)
		m_firstBlobWithFreeBlocks = blob;

	// bookkeeping
	--m_usedBlocksInPool;

#ifdef MEMORYPOOL_DEBUG
	m_factory->adjustTotals(tagString, -1*getAllocationSize(), 0);
#endif
}

//-----------------------------------------------------------------------------
Int MemoryPool::countBlobsInPool()
{
	Int blobs = 0;
	for (MemoryPoolBlob* blob = m_firstBlob; blob;) 
	{
		++blobs;
		blob = blob->getNextInList();
	}
	return blobs;
}

//-----------------------------------------------------------------------------
/**
	if the pool has any blobs that are completely unused, they are released back to the
	operating system. this will rarely, if ever, be called, but may be useful
	in odd situations.
*/
Int MemoryPool::releaseEmpties()
{
	ScopedCriticalSection scopedCriticalSection(TheMemoryPoolCriticalSection);

	Int released = 0;

	for (MemoryPoolBlob* blob = m_firstBlob; blob;) 
	{
		MemoryPoolBlob* pNext = blob->getNextInList();
		if (blob->getUsedBlockCount() == 0) 
			released += freeBlob(blob);
		blob = pNext;
	}
	return released;
}


//-----------------------------------------------------------------------------
/**
	throw away everything in the pool, but keep the pool itself valid.
*/
void MemoryPool::reset()
{
	ScopedCriticalSection scopedCriticalSection(TheMemoryPoolCriticalSection);

	// toss everything. we could do this slightly more efficiently,
	// but not really worth the extra code to do so.
	while (m_firstBlob) 
	{
		freeBlob(m_firstBlob);
	}
	m_firstBlob = NULL;
	m_lastBlob = NULL;
	m_firstBlobWithFreeBlocks = NULL;

	init(m_factory, m_poolName, m_allocationSize, m_initialAllocationCount, m_overflowAllocationCount);	// will throw on failure

}

//-----------------------------------------------------------------------------
/**
	add this pool to the factory's list-of-pools.
*/
void MemoryPool::addToList(MemoryPool **pHead)
{
	this->m_nextPoolInFactory = *pHead;
	*pHead = this;
}

//-----------------------------------------------------------------------------
/**
	remove this pool from the factory's list-of-pools.
*/
void MemoryPool::removeFromList(MemoryPool **pHead)
{
	// this isn't very efficient, but then, we rarely remove pools...
	// usually only at shutdown. so don't bother optimizing.
	MemoryPool *prev = NULL;
	for (MemoryPool *cur = *pHead; cur; cur = cur->m_nextPoolInFactory)
	{
		if (cur == this)
		{
			if (prev)
			{
				prev->m_nextPoolInFactory = this->m_nextPoolInFactory;
			}
			else
			{
				*pHead = this->m_nextPoolInFactory;
			}
			break;
		}
		prev = cur;
	}
}

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_DEBUG
/**
	print a report about per-pool allocation statistics to the debug log.
	if the pool is null, print column headers.
*/
/*static*/ void MemoryPool::debugPoolInfoReport( MemoryPool *pool, FILE *fp )
{
	//USE_PERF_TIMER(MemoryPoolDebugging) skip end-of-run reporting stuff

	const char *PREPEND = "POOLINFO";	// allows grepping more easily

	if (!pool)
	{
		DEBUG_LOG(("%s,%32s,%6s,%6s,%6s,%6s,%6s,%6s\n",PREPEND,"POOLNAME","BLKSZ","INIT","OVRFL","USED","TOTAL","PEAK"));
		if( fp )
			fprintf( fp, "%s,%32s,%6s,%6s,%6s,%6s,%6s,%6s\n",PREPEND,"POOLNAME","BLKSZ","INIT","OVRFL","USED","TOTAL","PEAK" );
	}
	else
	{
		DEBUG_LOG(("%s,%32s,%6d,%6d,%6d,%6d,%6d,%6d\n",PREPEND,
			pool->m_poolName,pool->m_allocationSize,pool->m_initialAllocationCount,pool->m_overflowAllocationCount,
			pool->m_usedBlocksInPool,pool->m_totalBlocksInPool,pool->m_peakUsedBlocksInPool));
		if( fp )
		{
			fprintf( fp, "%s,%32s,%6d,%6d,%6d,%6d,%6d,%6d\n",PREPEND,
				pool->m_poolName,pool->m_allocationSize,pool->m_initialAllocationCount,pool->m_overflowAllocationCount,
				pool->m_usedBlocksInPool,pool->m_totalBlocksInPool,pool->m_peakUsedBlocksInPool );
		}
	}
}
#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_DEBUG
Int MemoryPool::debugPoolReportLeaks( const char* owner )
{
	//USE_PERF_TIMER(MemoryPoolDebugging) skip end-of-run reporting stuff

	Int any = 0;
	for (MemoryPoolBlob* blob = m_firstBlob; blob; blob = blob->getNextInList()) 
	{
		any += blob->debugBlobReportLeaks(owner);
	}
	return any;
}
#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_DEBUG
/**
	perform internal consistency checking on the memory pool.
*/
void MemoryPool::debugMemoryVerifyPool()
{
	USE_PERF_TIMER(MemoryPoolDebugging)

	Int used = 0;
	Int total = 0;
	for (MemoryPoolBlob* blob = m_firstBlob; blob; blob = blob->getNextInList()) 
	{
		blob->debugMemoryVerifyBlob();
		used += blob->getUsedBlockCount();
		total += blob->getTotalBlockCount();
	}
	DEBUG_ASSERTCRASH(m_usedBlocksInPool == used, ("used mismatch %d %d",m_usedBlocksInPool,used));
	DEBUG_ASSERTCRASH(m_totalBlocksInPool == total, ("total mismatch %d %d",m_totalBlocksInPool,total));
}
#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_DEBUG
/**
	return true iff the block is a valid block in this pool.
*/
Bool MemoryPool::debugIsBlockInPool(void *pBlockPtr)
{
	USE_PERF_TIMER(MemoryPoolDebugging)

	if (!pBlockPtr)
		return false;

#ifdef MEMORYPOOL_INTENSE_VERIFY
	debugMemoryVerifyPool();
#endif

	Bool check1 = false, check2 = false;

	MemoryPoolSingleBlock *block = MemoryPoolSingleBlock::recoverBlockFromUserData(pBlockPtr);
	MemoryPoolBlob *ownerBlob = block->getOwningBlob();
	for (MemoryPoolBlob* blob = m_firstBlob; blob; blob = blob->getNextInList()) 
	{
		if (blob->debugIsBlockInBlob(pBlockPtr))
			check1 = true;
	
		if (blob == ownerBlob)
			check2 = true;
	}

	DEBUG_ASSERTCRASH(check1 == check2, ("mismatch checks in debugIsBlockInPool"));

	return check1 && check2;
}
#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_DEBUG
/**
	return the tagstring for the block. this will never return null; if
	the block is null or invalid, "FREE_SINGLEBLOCK_TAG_STRING" will be returned.
	it is assumed that the block was allocated by this pool.
*/
const char *MemoryPool::debugGetBlockTagString(void *pBlockPtr)
{
	USE_PERF_TIMER(MemoryPoolDebugging)

	if (!pBlockPtr)
		return FREE_SINGLEBLOCK_TAG_STRING;

#ifdef MEMORYPOOL_INTENSE_VERIFY
	debugMemoryVerifyPool();
#endif
	if (!debugIsBlockInPool(pBlockPtr)) 
	{
		DEBUG_CRASH(("block is not in this pool"));
		return FREE_SINGLEBLOCK_TAG_STRING;
	}
	MemoryPoolSingleBlock *block = MemoryPoolSingleBlock::recoverBlockFromUserData(pBlockPtr);
	return block->debugGetLiteralTagString();
}
#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_CHECKPOINTING
/**
	free all checkpointinfo for this pool, and reset all ptrs to checkpointinfo.
*/
void MemoryPool::debugResetCheckpoints()
{
	Checkpointable::debugResetCheckpoints();
	for (MemoryPoolBlob* blob = m_firstBlob; blob; blob = blob->getNextInList()) 
	{
		blob->debugResetCheckpoints();
	}
}
#endif

//-----------------------------------------------------------------------------
// METHODS for DynamicMemoryAllocator
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
/**
	init the DMA to safe values.
*/
DynamicMemoryAllocator::DynamicMemoryAllocator() :
	m_factory(NULL),
	m_nextDmaInFactory(NULL),
	m_numPools(0),
	m_usedBlocksInDma(0),
	m_rawBlocks(NULL)
{
	for (Int i = 0; i < MAX_DYNAMICMEMORYALLOCATOR_SUBPOOLS; i++)
		m_pools[i] = 0;
}

//-----------------------------------------------------------------------------
/**
	Initialize the dma and its subpools.
*/
void DynamicMemoryAllocator::init(MemoryPoolFactory *factory, Int numSubPools, const PoolInitRec pParms[])
{
	const PoolInitRec defaultDMA[7] = 
	{
		{ "dmaPool_16", 16, 64, 64 },
		{ "dmaPool_32", 32, 64, 64 },
		{ "dmaPool_64", 64, 64, 64 },
		{ "dmaPool_128", 128, 64, 64 },
		{ "dmaPool_256", 256, 64, 64 },
		{ "dmaPool_512", 512, 64, 64 },
		{ "dmaPool_1024", 1024, 64, 64 }
	};

	if (numSubPools == 0 || pParms == NULL) 
	{
		// use the defaults...
		numSubPools = 7;
		pParms = defaultDMA;
	}


	m_factory = factory;
	m_numPools = numSubPools;
	if (m_numPools > MAX_DYNAMICMEMORYALLOCATOR_SUBPOOLS)
		m_numPools = MAX_DYNAMICMEMORYALLOCATOR_SUBPOOLS;
	m_usedBlocksInDma = 0;
	for (Int i = 0; i < m_numPools; i++)
	{
		DEBUG_ASSERTCRASH(i == 0 || pParms[i].allocationSize > pParms[i-1].allocationSize, ("alloc size must increase monotonically for DMA"));
		m_pools[i] = m_factory->createMemoryPool(&pParms[i]);
	}
}

//-----------------------------------------------------------------------------
/**
	destroy the dma and its subpools.
*/
DynamicMemoryAllocator::~DynamicMemoryAllocator()
{
	DEBUG_ASSERTCRASH(m_usedBlocksInDma == 0, ("destroying a nonempty dma"));

	/// @todo this may cause double-destruction of the subpools -- test & fix
	for (Int i = 0; i < m_numPools; i++) 
	{
		m_factory->destroyMemoryPool(m_pools[i]);
		m_pools[i] = NULL;
	}

	while (m_rawBlocks)
	{
		freeBytes(m_rawBlocks->getUserData());
	}
}

//-----------------------------------------------------------------------------
/**
	find the best-fitting subpool in this dma for a given allocation size.
	if no subpool can satisfy the size, return null.
*/
MemoryPool *DynamicMemoryAllocator::findPoolForSize(Int allocSize)
{
	for (Int i = 0; i < m_numPools; i++)
	{
		DEBUG_ASSERTCRASH(m_pools[i], ("null pool"));
		if (allocSize <= m_pools[i]->getAllocationSize())
			return m_pools[i];
	}
	return NULL;
}

//-----------------------------------------------------------------------------
/**
	add this DMA to the factory's list of dmas.
*/
void DynamicMemoryAllocator::addToList(DynamicMemoryAllocator **pHead)
{
	this->m_nextDmaInFactory = *pHead;
	*pHead = this;
}

//-----------------------------------------------------------------------------
/**
	remove this DMA from the factory's list of dmas.
*/
void DynamicMemoryAllocator::removeFromList(DynamicMemoryAllocator **pHead)
{
	// this isn't very efficient, but then, we rarely remove these...
	// usually only at shutdown. so don't bother optimizing.
	DynamicMemoryAllocator *prev = NULL;
	for (DynamicMemoryAllocator *cur = *pHead; cur; cur = cur->m_nextDmaInFactory)
	{
		if (cur == this)
		{
			if (prev)
			{
				prev->m_nextDmaInFactory = this->m_nextDmaInFactory;
			}
			else
			{
				*pHead = this->m_nextDmaInFactory;
			}
			break;
		}
		prev = cur;
	}
}

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_DEBUG
void DynamicMemoryAllocator::debugIgnoreLeaksForThisBlock(void* pBlockPtr)
{
	USE_PERF_TIMER(MemoryPoolDebugging)

	if (!pBlockPtr)
		return;

#ifdef MEMORYPOOL_CHECK_BLOCK_OWNERSHIP
	DEBUG_ASSERTCRASH(debugIsBlockInDma(pBlockPtr), ("block is not in this dma"));
#endif

	MemoryPoolSingleBlock *block = MemoryPoolSingleBlock::recoverBlockFromUserData(pBlockPtr);
	if (block->getOwningBlob()) 
	{
#ifdef MEMORYPOOL_DEBUG
		DEBUG_ASSERTCRASH(findPoolForSize(block->debugGetLogicalSize()) == block->getOwningBlob()->getOwningPool(), ("pool mismatch"));
#endif
		block->debugIgnoreLeaksForThisBlock();
	}
	else
	{
		DEBUG_CRASH(("cannot currently ignore leaks for raw blocks (allocation too large)\n"));
	}
}
#endif

//-----------------------------------------------------------------------------
/**
	allocate a chunk-o-bytes from this DMA and return it, but don't bother zeroing
	out the block. if unable to allocate, throw ERROR_OUT_OF_MEMORY. this
	function will never return null.
*/
void *DynamicMemoryAllocator::allocateBytesDoNotZeroImplementation(Int numBytes DECLARE_LITERALSTRING_ARG2)
{
	ScopedCriticalSection scopedCriticalSection(TheDmaCriticalSection);

	void *result = NULL;

#ifdef MEMORYPOOL_DEBUG
	DEBUG_ASSERTCRASH(debugLiteralTagString != NULL, ("bad tagstring"));
	Int waste = 0;
#endif

	MemoryPool *pool = findPoolForSize(numBytes);
	if (pool != NULL) 
	{
		result = pool->allocateBlockDoNotZeroImplementation(PASS_LITERALSTRING_ARG1);
#ifdef MEMORYPOOL_DEBUG
	{
		USE_PERF_TIMER(MemoryPoolDebugging)
		waste = pool->getAllocationSize() - numBytes;
		MemoryPoolSingleBlock *wblock = MemoryPoolSingleBlock::recoverBlockFromUserData(result);
		wblock->debugSetWastedSize(waste);
#ifdef INTENSE_DMA_BOOKKEEPING
		if (doingIntenseDMA == 0)
#endif
		{
			theWastedDMA += (waste);
			if (thePeakWastedDMA < theWastedDMA)
				thePeakWastedDMA = theWastedDMA;
		}
	}
#endif MEMORYPOOL_DEBUG
	}
	else
	{
		// too big for our pools -- just go right to the metal.
		MemoryPoolSingleBlock *block = MemoryPoolSingleBlock::rawAllocateSingleBlock(&m_rawBlocks, numBytes, m_factory PASS_LITERALSTRING_ARG2);

#ifdef MEMORYPOOL_CHECKPOINTING
		BlockCheckpointInfo *bi = debugAddCheckpointInfo(block->debugGetLiteralTagString(), m_factory->getCurCheckpoint(), numBytes);
		if (bi)
			block->debugSetCheckpointInfo(bi);
#endif

		result = block->getUserData();

#ifdef MEMORYPOOL_DEBUG
		m_factory->adjustTotals(debugLiteralTagString, numBytes, numBytes);
		theTotalLargeBlocks += numBytes;
		if (thePeakLargeBlocks < theTotalLargeBlocks)
			thePeakLargeBlocks = theTotalLargeBlocks;
#endif
	}

#ifdef MEMORYPOOL_DEBUG
{
	USE_PERF_TIMER(MemoryPoolDebugging)
	theTotalDMA += numBytes;
	if (thePeakDMA < theTotalDMA)
		thePeakDMA = theTotalDMA;
#ifdef INTENSE_DMA_BOOKKEEPING
	if (isMemoryManagerOfficiallyInited() && doingIntenseDMA == 0)
	{
		++doingIntenseDMA;
		UsedNPeak& up = TheUsedNPeakMap[debugLiteralTagString];
		up.used += numBytes;
		if (up.peak < up.used)
			up.peak = up.used;
		up.waste += waste;
		if (up.peakwaste < up.waste)
			up.peakwaste = up.waste;
		--doingIntenseDMA;
	}
#endif
}
#endif MEMORYPOOL_DEBUG

	++m_usedBlocksInDma;
	DEBUG_ASSERTCRASH(m_usedBlocksInDma >= 0, ("negative count for m_usedBlocksInDma"));
#ifdef MEMORYPOOL_DEBUG
	#ifdef USE_FILLER_VALUE
	{
		USE_PERF_TIMER(MemoryPoolInitFilling)
		::memset32(result, s_initFillerValue, numBytes);
	}
	#endif
#endif
	return result;
}

//-----------------------------------------------------------------------------
/**
	allocate a chunk-o-bytes from this DMA and return it, and zero out the contents first.
	if unable to allocate, throw ERROR_OUT_OF_MEMORY. 
	this function will never return null.
*/
void *DynamicMemoryAllocator::allocateBytesImplementation(Int numBytes DECLARE_LITERALSTRING_ARG2)
{
	void* p = allocateBytesDoNotZeroImplementation(numBytes PASS_LITERALSTRING_ARG2);	// throws on failure
	memset(p, 0, numBytes);
	return p;
}

//-----------------------------------------------------------------------------
/**
	free a chunk-o-bytes allocated by this dma. it's ok to pass null.
*/
void DynamicMemoryAllocator::freeBytes(void* pBlockPtr)
{
	if (!pBlockPtr)
		return;

	ScopedCriticalSection scopedCriticalSection(TheDmaCriticalSection);

#ifdef MEMORYPOOL_CHECK_BLOCK_OWNERSHIP
	DEBUG_ASSERTCRASH(debugIsBlockInDma(pBlockPtr), ("block is not in this dma"));
#endif

	MemoryPoolSingleBlock *block = MemoryPoolSingleBlock::recoverBlockFromUserData(pBlockPtr);
#ifdef MEMORYPOOL_DEBUG
	Int waste = 0, used = 0;
	{
		USE_PERF_TIMER(MemoryPoolDebugging)
		waste = 0;
		used = block->debugGetLogicalSize();
		theTotalDMA -= used;
		if (thePeakDMA < theTotalDMA)
			thePeakDMA = theTotalDMA;
	#ifdef INTENSE_DMA_BOOKKEEPING
		const char* tagString = block->debugGetLiteralTagString();
	#endif
	}
#endif MEMORYPOOL_DEBUG

	if (block->getOwningBlob()) 
	{
#ifdef MEMORYPOOL_DEBUG
		{
			USE_PERF_TIMER(MemoryPoolDebugging)
			DEBUG_ASSERTCRASH(findPoolForSize(used) == block->getOwningBlob()->getOwningPool(), ("pool mismatch"));
	#ifdef INTENSE_DMA_BOOKKEEPING
			if (doingIntenseDMA == 0)
	#endif
			{
				waste = block->debugGetWastedSize();
				theWastedDMA -= waste;
				if (thePeakWastedDMA < theWastedDMA)
					thePeakWastedDMA = theWastedDMA;
			}
		}
#endif MEMORYPOOL_DEBUG
		block->getOwningBlob()->getOwningPool()->freeBlock(pBlockPtr);
	}
	else
	{
		// was allocated via sysAllocate.
#ifdef MEMORYPOOL_CHECKPOINTING
		BlockCheckpointInfo *bi = block->debugGetCheckpointInfo();
		DEBUG_ASSERTCRASH(bi, ("hmm, no checkpoint info"));
		if (bi)
			bi->debugSetFreepoint(m_factory->getCurCheckpoint());
#endif

#ifdef MEMORYPOOL_DEBUG
		m_factory->adjustTotals(block->debugGetLiteralTagString(), -used, -used);
		theTotalLargeBlocks -= used;
		if (thePeakLargeBlocks < theTotalLargeBlocks)
			thePeakLargeBlocks = theTotalLargeBlocks;
		block->debugMarkBlockAsFree();
#endif

		block->removeBlockFromList(&m_rawBlocks);

		::sysFree((void *)block);

	}
	--m_usedBlocksInDma;
	DEBUG_ASSERTCRASH(m_usedBlocksInDma >= 0, ("negative count for m_usedBlocksInDma"));

#ifdef INTENSE_DMA_BOOKKEEPING
	if (isMemoryManagerOfficiallyInited() && doingIntenseDMA == 0)
	{
		++doingIntenseDMA;
		UsedNPeak& up = TheUsedNPeakMap[tagString];
		up.used -= used;
		if (up.peak < up.used)
			up.peak = up.used;
		up.waste -= waste;
		if (up.peakwaste < up.waste)
			up.peakwaste = up.waste;
		--doingIntenseDMA;
	}
#endif

}

//-----------------------------------------------------------------------------
Int DynamicMemoryAllocator::getActualAllocationSize(Int numBytes)
{
	MemoryPool *pool = findPoolForSize(numBytes);
	return pool ? pool->getAllocationSize() : numBytes;
}

//-----------------------------------------------------------------------------
/**
	throw away everything in the DMA, but keep the DMA itself valid.
*/
void DynamicMemoryAllocator::reset()
{
	for (Int i = 0; i < m_numPools; i++)
	{
		if (m_pools[i])
		{
			m_pools[i]->reset();
		}
	}

	while (m_rawBlocks)
		freeBytes(m_rawBlocks->getUserData());

	m_usedBlocksInDma = 0;
}

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_DEBUG
/**
	return true iff the given pool is a subpool of this dma.
*/
Bool DynamicMemoryAllocator::debugIsPoolInDma(MemoryPool *pool)
{
	USE_PERF_TIMER(MemoryPoolDebugging)

	if (!pool)
		return false;

	for (Int i = 0; i < m_numPools; i++)
	{
		if (m_pools[i] == pool)
			return true;
	}

	return false;
}
#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_DEBUG
/**
	return true iff the block was allocated by this dma
	(either from a subpool or as a raw block).
*/
Bool DynamicMemoryAllocator::debugIsBlockInDma(void *pBlockPtr)
{
	USE_PERF_TIMER(MemoryPoolDebugging)

	if (!pBlockPtr)
		return false;

	MemoryPoolSingleBlock *block = MemoryPoolSingleBlock::recoverBlockFromUserData(pBlockPtr);
	if (block->getOwningBlob()) 
	{
		MemoryPool *pool = block->getOwningBlob()->getOwningPool();
		return pool && pool->debugIsBlockInPool(pBlockPtr) && debugIsPoolInDma(pool);
	}
	else
	{
		for (MemoryPoolSingleBlock *b = m_rawBlocks; b; b = b->getNextRawBlock())
		{
			if (b == block)
				return true;
		}
		return false;
	}
}
#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_DEBUG
/**
	return the tagstring for the block. this will never return null; if
	the block is null or invalid, "FREE_SINGLEBLOCK_TAG_STRING" will be returned.
	it is assumed that the block was allocated by this dma.
*/
const char *DynamicMemoryAllocator::debugGetBlockTagString(void *pBlockPtr)
{
	USE_PERF_TIMER(MemoryPoolDebugging)

	if (!pBlockPtr)
		return FREE_SINGLEBLOCK_TAG_STRING;

	if (!debugIsBlockInDma(pBlockPtr)) 
	{
		DEBUG_CRASH(("block is not in this dma"));
		return FREE_SINGLEBLOCK_TAG_STRING;
	}
	MemoryPoolSingleBlock *block = MemoryPoolSingleBlock::recoverBlockFromUserData(pBlockPtr);
	return block->debugGetLiteralTagString();
}
#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_DEBUG
/**
	perform internal consistency checking on this DMA.
*/
void DynamicMemoryAllocator::debugMemoryVerifyDma()
{
	USE_PERF_TIMER(MemoryPoolDebugging)

	for (MemoryPoolSingleBlock *b = m_rawBlocks; b; b = b->getNextRawBlock())
	{
		b->debugVerifyBlock();
	}
}
#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_CHECKPOINTING
/**
	free all checkpointinfo for this dma, and reset all ptrs to checkpointinfo.
*/
void DynamicMemoryAllocator::debugResetCheckpoints()
{
	Checkpointable::debugResetCheckpoints();
	for (MemoryPoolSingleBlock *b = m_rawBlocks; b; b = b->getNextRawBlock())
	{
		b->debugResetCheckpoint();
	}
}
#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_DEBUG
/**
	calculate the total number of raw blocks allocated by this dma,
	and the total number of bytes (logical) used by those raw blocks.
*/
Int DynamicMemoryAllocator::debugCalcRawBlockBytes(Int *numBlocks)
{
	USE_PERF_TIMER(MemoryPoolDebugging)

	if (numBlocks)
		*numBlocks = 0;
	Int bytes = 0;
	for (MemoryPoolSingleBlock *b = m_rawBlocks; b; b = b->getNextRawBlock())
	{
		if (numBlocks)
			*numBlocks += 1;
		bytes += b->debugGetLogicalSize();
	}
	return bytes;
}
#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_DEBUG
Int DynamicMemoryAllocator::debugDmaReportLeaks()
{
	//USE_PERF_TIMER(MemoryPoolDebugging) skip end-of-run reporting stuff

	Int any = false;
	for (MemoryPoolSingleBlock *b = m_rawBlocks; b; b = b->getNextRawBlock())
	{
		any += b->debugSingleBlockReportLeak("(DMA)");
	}
	return any;
}
#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_DEBUG
/**
	print a report about raw block allocations to the debug log.
*/
void DynamicMemoryAllocator::debugDmaInfoReport( FILE *fp )
{
	//USE_PERF_TIMER(MemoryPoolDebugging) skip end-of-run reporting stuff

	const char *PREPEND = "POOLINFO";	// allows grepping more easily

	Int numBlocks;
	Int bytes = debugCalcRawBlockBytes(&numBlocks);
	DEBUG_LOG(("%s,Total Raw Blocks = %d\n",PREPEND,numBlocks));
	DEBUG_LOG(("%s,Total Raw Block Bytes = %d\n",PREPEND,bytes));
	DEBUG_LOG(("%s,Average Raw Block Size = %d\n",PREPEND,numBlocks?bytes/numBlocks:0));
	DEBUG_LOG(("%s,Raw Blocks:\n",PREPEND));
	if( fp )
	{
		fprintf( fp, "%s,Total Raw Blocks = %d\n",PREPEND,numBlocks );
		fprintf( fp, "%s,Total Raw Block Bytes = %d\n",PREPEND,bytes );
		fprintf( fp, "%s,Average Raw Block Size = %d\n",PREPEND,numBlocks?bytes/numBlocks:0 );
		fprintf( fp, "%s,Raw Blocks:\n",PREPEND );
	}
	for (MemoryPoolSingleBlock *b = m_rawBlocks; b; b = b->getNextRawBlock())
	{
		DEBUG_LOG(("%s,  Blocksize=%d\n",PREPEND,b->debugGetLogicalSize()));
		//if( fp )
		//{
		//	fprintf( fp, "%s,  Blocksize=%d\n",PREPEND,b->debugGetLogicalSize() );
		//}
	}

}
#endif

//-----------------------------------------------------------------------------
// METHODS for MemoryPoolFactory
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
/**
	init the factory to safe values.
*/
MemoryPoolFactory::MemoryPoolFactory() :
	m_firstPoolInFactory(NULL),
	m_firstDmaInFactory(NULL)
#ifdef MEMORYPOOL_CHECKPOINTING
	, m_curCheckpoint(0)
#endif
#ifdef MEMORYPOOL_DEBUG
	, m_usedBytes(0)
	, m_physBytes(0)
	, m_peakUsedBytes(0)
	, m_peakPhysBytes(0)
#endif
{
#ifdef MEMORYPOOL_DEBUG
	for (int i = 0; i < MAX_SPECIAL_USED; ++i)
	{
		m_usedBytesSpecial[i] = 0;
		m_usedBytesSpecialPeak[i] = 0;
		m_physBytesSpecial[i] = 0;
		m_physBytesSpecialPeak[i] = 0;
	}
	#ifdef USE_FILLER_VALUE
	calcFillerValue(GameClientRandomValue(0, MAX_INIT_FILLER_COUNT-1));
	#endif
#endif
}
//-----------------------------------------------------------------------------
/**
	initialize the factory.
*/
void MemoryPoolFactory::init()
{
	// my, that was easy
}

//-----------------------------------------------------------------------------
/**
	destroy the factory, and all its pools and dmas.
*/
MemoryPoolFactory::~MemoryPoolFactory()
{
	while (m_firstPoolInFactory)
	{
		destroyMemoryPool(m_firstPoolInFactory);
	}

	while (m_firstDmaInFactory)
	{
		destroyDynamicMemoryAllocator(m_firstDmaInFactory);
	}
}

//-----------------------------------------------------------------------------
/**
	create a new memory pool.
*/
MemoryPool *MemoryPoolFactory::createMemoryPool(const PoolInitRec *parms)
{
	return createMemoryPool(parms->poolName, parms->allocationSize, parms->initialAllocationCount, parms->overflowAllocationCount);
}

//-----------------------------------------------------------------------------
/**
	create a new memory pool. (alternate argument list)
*/
MemoryPool *MemoryPoolFactory::createMemoryPool(const char *poolName, Int allocationSize, Int initialAllocationCount, Int overflowAllocationCount)
{
	MemoryPool *pool = findMemoryPool(poolName);
	if (pool) 
	{
		DEBUG_ASSERTCRASH(allocationSize == pool->getAllocationSize(), ("pool size mismatch"));
		return pool;
	}

	userMemoryAdjustPoolSize(poolName, initialAllocationCount, overflowAllocationCount);

	if (initialAllocationCount <= 0 || overflowAllocationCount < 0)
	{
		DEBUG_CRASH(("illegal pool size: %d %d\n",initialAllocationCount,overflowAllocationCount));
		throw ERROR_OUT_OF_MEMORY;
	}

	pool = new (::sysAllocate(sizeof(MemoryPool))) MemoryPool;	// will throw on failure
	pool->init(this, poolName, allocationSize, initialAllocationCount, overflowAllocationCount);	// will throw on failure

	pool->addToList(&m_firstPoolInFactory);

	return pool;
}

//-----------------------------------------------------------------------------
/**
	find a memory pool with the given name; return null if no such pool exists,
	return null. note that this function isn't particularly fast (it just does
	a linear search), so you should probably cache the result.
*/
MemoryPool *MemoryPoolFactory::findMemoryPool(const char *poolName)
{
	for (MemoryPool *pool = m_firstPoolInFactory; pool; pool = pool->getNextPoolInList())
	{
		if (!strcmp(poolName, pool->getPoolName())) 
		{
			DEBUG_ASSERTCRASH(poolName == pool->getPoolName(), ("hmm, ptrs should probably match here"));
			return pool;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
/**
	destroy the given memory pool. you normally will never need to call this directly.
*/
void MemoryPoolFactory::destroyMemoryPool(MemoryPool *pMemoryPool)
{
	if (!pMemoryPool)
		return;

	DEBUG_ASSERTCRASH(pMemoryPool->getUsedBlockCount() == 0, ("destroying a nonempty pool"));

	pMemoryPool->removeFromList(&m_firstPoolInFactory);

	// this is evil... since there is no 'placement delete' we must do this the hard way
	// and call the dtor directly. ordinarily this is heinous, but in this case we'll
	// make an exception.
	pMemoryPool->~MemoryPool();
	::sysFree((void *)pMemoryPool);
}

//-----------------------------------------------------------------------------
/**
	create a new dynamicMemoryAllocator. You normally will never need to call this directly.
*/
DynamicMemoryAllocator *MemoryPoolFactory::createDynamicMemoryAllocator(Int numSubPools, const PoolInitRec pParms[])
{
	DynamicMemoryAllocator *dma;

	dma = new (::sysAllocate(sizeof(DynamicMemoryAllocator))) DynamicMemoryAllocator;	// will throw on failure
	dma->init(this, numSubPools, pParms);	// will throw on failure

	dma->addToList(&m_firstDmaInFactory);

	return dma;
}

//-----------------------------------------------------------------------------
/**
	destroy the given dynamicMemoryAllocator. You normally will never need to call this directly.
*/
void MemoryPoolFactory::destroyDynamicMemoryAllocator(DynamicMemoryAllocator *dma)
{
	if (!dma)
		return;

	dma->removeFromList(&m_firstDmaInFactory);

	// this is evil... since there is no 'placement delete' we must do this the hard way
	// and call the dtor directly. ordinarily this is heinous, but in this case we'll
	// make an exception.
	dma->~DynamicMemoryAllocator();
	::sysFree((void *)dma);
}

//-----------------------------------------------------------------------------
/**
	throw away everything in all pools/dmas owned by this factory, but keep the factory
	and pools/dmas themselves valid.
*/
void MemoryPoolFactory::reset()
{
#ifdef MEMORYPOOL_CHECKPOINTING
	debugResetCheckpoints();
#endif

	for (MemoryPool *pool = m_firstPoolInFactory; pool; pool = pool->getNextPoolInList())
	{
		pool->reset();
	}
	for (DynamicMemoryAllocator *dma = m_firstDmaInFactory; dma; dma = dma->getNextDmaInList())
	{
		dma->reset();
	}

#ifdef MEMORYPOOL_DEBUG
	m_usedBytes = 0;
	m_physBytes = 0;
	m_peakUsedBytes = 0;
	m_peakPhysBytes = 0;
	for (int i = 0; i < MAX_SPECIAL_USED; ++i)
	{
		m_usedBytesSpecial[i] = 0;
		m_usedBytesSpecialPeak[i] = 0;
		m_physBytesSpecial[i] = 0;
		m_physBytesSpecialPeak[i] = 0;
	}
	#ifdef USE_FILLER_VALUE
	calcFillerValue(GameClientRandomValue(0, MAX_INIT_FILLER_COUNT-1));
	#endif
#endif
}

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_DEBUG
static const char* s_specialPrefixes[MAX_SPECIAL_USED] =
{
	"Misc",			// the catchall for stuff that doesn't match others
	"W3D_",
	"W3A_",
	"STL_",
	"STR_",
	NULL
};

#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_DEBUG
/**
	perform bookkeeping on memory usage statistics.
*/
void MemoryPoolFactory::adjustTotals(const char* tagString, Int usedDelta, Int physDelta)
{
	USE_PERF_TIMER(MemoryPoolDebugging)

	m_usedBytes += usedDelta;
	m_physBytes += physDelta;
	if (m_peakUsedBytes < m_usedBytes)
		m_peakUsedBytes = m_usedBytes;
	if (m_peakPhysBytes < m_physBytes)
		m_peakPhysBytes = m_physBytes;

	int found = 0;	// if no matches found, goes into slot zero
	for (int i = 1; i < MAX_SPECIAL_USED; ++i)	// start at 1, not zero
	{
		if (s_specialPrefixes[i] == NULL)
			break;

		if (strncmp(tagString, s_specialPrefixes[i], strlen(s_specialPrefixes[i])) == 0)
		{
			found = i;
			break;
		}
	}
	m_usedBytesSpecial[found] += usedDelta;
	m_physBytesSpecial[found] += physDelta;
	if (m_usedBytesSpecialPeak[found] < m_usedBytesSpecial[found])
		m_usedBytesSpecialPeak[found] = m_usedBytesSpecial[found];
	if (m_physBytesSpecialPeak[found] < m_physBytesSpecial[found])
		m_physBytesSpecialPeak[found] = m_physBytesSpecial[found];
}
#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_DEBUG
void MemoryPoolFactory::debugSetInitFillerIndex(Int index)
{
	#ifdef USE_FILLER_VALUE
	if (index < 0 || index >= MAX_INIT_FILLER_COUNT)
		index = GameClientRandomValue(0, MAX_INIT_FILLER_COUNT-1);
	calcFillerValue(index);
	#endif
}
#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_DEBUG
/**
	perform internal consistency checking on the factory, and all of its
	pools and dmas.
*/
void MemoryPoolFactory::debugMemoryVerify()
{
	USE_PERF_TIMER(MemoryPoolDebugging)

	Int used = 0, phys = 0;

	for (MemoryPool *pool = m_firstPoolInFactory; pool; pool = pool->getNextPoolInList())
	{
		pool->debugMemoryVerifyPool();
		used += pool->getUsedBlockCount()*pool->getAllocationSize();
		phys += pool->getTotalBlockCount()*pool->getAllocationSize();
	}

	for (DynamicMemoryAllocator *dma = m_firstDmaInFactory; dma; dma = dma->getNextDmaInList())
	{
		dma->debugMemoryVerifyDma();
		Int tmp = dma->debugCalcRawBlockBytes(NULL);
		used += tmp;
		phys += tmp;
	}

	DEBUG_ASSERTCRASH(used == m_usedBytes, ("used count mismatch"));
	DEBUG_ASSERTCRASH(phys == m_physBytes, ("phys count mismatch"));
}
#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_DEBUG
/**
	return true iff the block was allocated by any of the pools/dmas owned
	by this factory.
*/
Bool MemoryPoolFactory::debugIsBlockInAnyPool(void *pBlock)
{
	USE_PERF_TIMER(MemoryPoolDebugging)

#ifdef MEMORYPOOL_INTENSE_VERIFY
	debugMemoryVerify();
#endif
	for (MemoryPool *pool = m_firstPoolInFactory; pool; pool = pool->getNextPoolInList())
	{
		if (pool->debugIsBlockInPool(pBlock))
			return true;
	}

	for (DynamicMemoryAllocator *dma = m_firstDmaInFactory; dma; dma = dma->getNextDmaInList())
	{
		if (dma->debugIsBlockInDma(pBlock))
			return true;
	}

	return false;
}
#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_DEBUG
/**
	return the tagstring for this block. this will never return null; if
	the block is null or invalid, "FREE_SINGLEBLOCK_TAG_STRING" will be returned.
	it is assumed that the block was allocated by this factory.
*/
const char *MemoryPoolFactory::debugGetBlockTagString(void *pBlockPtr)
{
	USE_PERF_TIMER(MemoryPoolDebugging)

	if (!pBlockPtr)
		return FREE_SINGLEBLOCK_TAG_STRING;

#ifdef MEMORYPOOL_INTENSE_VERIFY
	debugMemoryVerify();
#endif
	if (!debugIsBlockInAnyPool(pBlockPtr)) 
	{
		DEBUG_CRASH(("block is not in this factory"));
		return FREE_SINGLEBLOCK_TAG_STRING;
	}
	MemoryPoolSingleBlock *block = MemoryPoolSingleBlock::recoverBlockFromUserData(pBlockPtr);
	return block->debugGetLiteralTagString();
}
#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_CHECKPOINTING
/**
	set a new checkpoint for all future blocks allocated/freed by this factory's pools/dmas.
*/
Int MemoryPoolFactory::debugSetCheckpoint()
{
	return ++m_curCheckpoint;
}
#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_CHECKPOINTING
/**
	throw away all checkpoint info for all pools/dmas.
*/
void MemoryPoolFactory::debugResetCheckpoints()
{
	for (MemoryPool *pool = m_firstPoolInFactory; pool; pool = pool->getNextPoolInList())
	{
		pool->debugResetCheckpoints();
	}
	for (DynamicMemoryAllocator *dma = m_firstDmaInFactory; dma; dma = dma->getNextDmaInList())
	{
		dma->debugResetCheckpoints();
	}
	m_curCheckpoint = 0;
}
#endif

//-----------------------------------------------------------------------------
void MemoryPoolFactory::memoryPoolUsageReport( const char* filename, FILE *appendToFileInstead )
{
#ifdef MEMORYPOOL_DEBUG
	//USE_PERF_TIMER(MemoryPoolDebugging) skip end-of-run reporting stuff

	FILE* perfStatsFile = NULL;
	Int totalNamedPoolPeak = 0;

	if( !appendToFileInstead )
	{
		char tmp[256];
		strcpy(tmp,filename);
		strcat(tmp,".csv");
		perfStatsFile = fopen(tmp, "w");
	}
	else
	{
		perfStatsFile = appendToFileInstead;
	}

	if (perfStatsFile == NULL)
	{
		DEBUG_CRASH(("could not open/create perf file %s -- is it open in another app?",filename));
		return;
	}

	Int lineIdx = 0;
	MemoryPool *pool = m_firstPoolInFactory;
#ifdef INTENSE_DMA_BOOKKEEPING
	UsedNPeakMap::const_iterator it = TheUsedNPeakMap.begin();
#endif
	for (;;)
	{
		Bool keepGoing = false;
		if (pool)
		{
			if (lineIdx == 0)
			{
				fprintf(perfStatsFile, "%s,%d","Unpooled Large Blocks (>1024 bytes)",thePeakLargeBlocks/1024);
			}
			else
			{
				Int sz = pool->getAllocationSize();
				Int initial = pool->getInitialBlockCount()*sz;
				Int peak = pool->getPeakBlockCount()*sz;
				Int waste = initial - peak;
				if (waste < 0) waste = 0;
				fprintf(perfStatsFile, "%s,%d,%d",pool->getPoolName(),peak/1024,waste/1024);
				totalNamedPoolPeak += peak;
				pool = pool->getNextPoolInList();
			}
			keepGoing = true;
		}
		else
		{
			fprintf(perfStatsFile, ",,");
		}

#ifdef INTENSE_DMA_BOOKKEEPING
		if (it != TheUsedNPeakMap.end())
		{
			Int wastepct = (it->second.peakwaste * 100) / (it->second.peak + it->second.peakwaste);
			fprintf(perfStatsFile, ",,,,%s,%d,%d,%d",it->first,it->second.peak/1024,it->second.peakwaste/1024,wastepct);
			++it;
			keepGoing = true;
		}
		else
		{
			fprintf(perfStatsFile, ",,,,,,,");
		}
#endif
		
		if (lineIdx < MAX_SPECIAL_USED && s_specialPrefixes[lineIdx] != NULL)
		{
			fprintf(perfStatsFile, ",,,%s,%d",s_specialPrefixes[lineIdx],m_usedBytesSpecialPeak[lineIdx]/1024);
			keepGoing = true;
		}

		fprintf(perfStatsFile, "\n");
		++lineIdx;

		if (!keepGoing)
			break;
	}

	fflush(perfStatsFile);

	if( !appendToFileInstead )
	{
		fclose(perfStatsFile);
	}
#endif
}

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_DEBUG
/**
	send a memory reports (based on the flags/checkpoints) to the debug log.
*/
void MemoryPoolFactory::debugMemoryReport(Int flags, Int startCheckpoint, Int endCheckpoint, FILE *fp )
{
	//USE_PERF_TIMER(MemoryPoolDebugging) skip end-of-run reporting stuff

	Int oldFlags = DebugGetFlags();
	DebugSetFlags(oldFlags & ~DEBUG_FLAG_PREPEND_TIME);

#ifdef MEMORYPOOL_CHECKPOINTING
	Bool doBlockReport = (flags & _REPORT_CP_ALLOCATED_DONTCARE) != 0 && (flags & _REPORT_CP_FREED_DONTCARE) != 0;
	DEBUG_ASSERTCRASH(startCheckpoint >= 0 && startCheckpoint <= endCheckpoint && endCheckpoint <= m_curCheckpoint, ("bad checkpoints"));
	DEBUG_ASSERTCRASH(((flags & _REPORT_CP_ALLOCATED_DONTCARE) != 0) == ((flags & _REPORT_CP_FREED_DONTCARE) != 0), ("bad flags: must set at both alloc and free flag"));
#endif

	debugMemoryVerify();

	if (flags & REPORT_FACTORYINFO)
	{
		DEBUG_LOG(("------------------------------------------\n"));
		DEBUG_LOG(("Begin Factory Info Report\n"));
		DEBUG_LOG(("------------------------------------------\n"));
		DEBUG_LOG(("Bytes in use (logical) = %d\n",m_usedBytes));
		DEBUG_LOG(("Bytes in use (physical) = %d\n",m_physBytes));
		DEBUG_LOG(("PEAK Bytes in use (logical) = %d\n",m_peakUsedBytes));
		DEBUG_LOG(("PEAK Bytes in use (physical) = %d\n",m_peakPhysBytes));
		DEBUG_LOG(("------------------------------------------\n"));
		DEBUG_LOG(("End Factory Info Report\n"));
		DEBUG_LOG(("------------------------------------------\n"));
		if( fp )
		{
			fprintf( fp, "------------------------------------------\n" );
			fprintf( fp, "Begin Factory Info Report\n" );
			fprintf( fp, "------------------------------------------\n" );
			fprintf( fp, "Bytes in use (logical) = %d\n",m_usedBytes );
			fprintf( fp, "Bytes in use (physical) = %d\n",m_physBytes );
			fprintf( fp, "PEAK Bytes in use (logical) = %d\n",m_peakUsedBytes );
			fprintf( fp, "PEAK Bytes in use (physical) = %d\n",m_peakPhysBytes );
			fprintf( fp, "------------------------------------------\n" );
			fprintf( fp, "End Factory Info Report\n" );
			fprintf( fp, "------------------------------------------\n" );
		}
	}

	if (flags & REPORT_POOLINFO)
	{
		DEBUG_LOG(("------------------------------------------\n"));
		DEBUG_LOG(("Begin Pool Info Report\n"));
		DEBUG_LOG(("------------------------------------------\n"));
		if( fp )
		{
			fprintf( fp, "------------------------------------------\n" );
			fprintf( fp, "Begin Pool Info Report\n" );
			fprintf( fp, "------------------------------------------\n" );
		}
		MemoryPool::debugPoolInfoReport( NULL, fp );
		for (MemoryPool *pool = m_firstPoolInFactory; pool; pool = pool->getNextPoolInList())
		{
			MemoryPool::debugPoolInfoReport( pool, fp );
		}
		for (DynamicMemoryAllocator *dma = m_firstDmaInFactory; dma; dma = dma->getNextDmaInList())
		{
			dma->debugDmaInfoReport( fp );
		}
		DEBUG_LOG(("------------------------------------------\n"));
		DEBUG_LOG(("End Pool Info Report\n"));
		DEBUG_LOG(("------------------------------------------\n"));
		if( fp )
		{
			fprintf( fp, "------------------------------------------\n" );
			fprintf( fp, "End Pool Info Report\n" );
			fprintf( fp, "------------------------------------------\n" );
		}
	}

	if (flags & REPORT_POOL_OVERFLOW)
	{
		DEBUG_LOG(("------------------------------------------\n"));
		DEBUG_LOG(("Begin Pool Overflow Report\n"));
		DEBUG_LOG(("------------------------------------------\n"));
		for (MemoryPool *pool = m_firstPoolInFactory; pool; pool = pool->getNextPoolInList())
		{
			if (pool->getPeakBlockCount() > pool->getInitialBlockCount())
			{
				DEBUG_LOG(("*** Pool %s overflowed initial allocation of %d (peak allocation was %d)\n",pool->getPoolName(),pool->getInitialBlockCount(),pool->getPeakBlockCount()));
			}
		}
		DEBUG_LOG(("------------------------------------------\n"));
		DEBUG_LOG(("End Pool Overflow Report\n"));
		DEBUG_LOG(("------------------------------------------\n"));
		DEBUG_LOG(("------------------------------------------\n"));
		DEBUG_LOG(("Begin Pool Underflow Report\n"));
		DEBUG_LOG(("------------------------------------------\n"));
		for (pool = m_firstPoolInFactory; pool; pool = pool->getNextPoolInList())
		{
			Int peak = pool->getPeakBlockCount()*pool->getAllocationSize();
			Int initial = pool->getInitialBlockCount()*pool->getAllocationSize();
			if (peak < initial/2 && (initial - peak) > 4096)
			{
				DEBUG_LOG(("*** Pool %s used less than half its initial allocation of %d (peak allocation was %d, wasted %dk)\n",
					pool->getPoolName(),pool->getInitialBlockCount(),pool->getPeakBlockCount(),(initial - peak)/1024));
			}
		}
		DEBUG_LOG(("------------------------------------------\n"));
		DEBUG_LOG(("End Pool Underflow Report\n"));
		DEBUG_LOG(("------------------------------------------\n"));
	}

	if( flags & REPORT_SIMPLE_LEAKS )
	{
		DEBUG_LOG(("------------------------------------------\n"));
		DEBUG_LOG(("Begin Simple Leak Report\n"));
		DEBUG_LOG(("------------------------------------------\n"));
		Int any = 0;
		for (MemoryPool *pool = m_firstPoolInFactory; pool; pool = pool->getNextPoolInList())
		{
			any += pool->debugPoolReportLeaks( pool->getPoolName() );
		}
		for (DynamicMemoryAllocator *dma = m_firstDmaInFactory; dma; dma = dma->getNextDmaInList())
		{
			any += dma->debugDmaReportLeaks();
		}
		DEBUG_ASSERTCRASH(!any, ("There were %d memory leaks. Please fix them.\n",any));
		DEBUG_LOG(("------------------------------------------\n"));
		DEBUG_LOG(("End Simple Leak Report\n"));
		DEBUG_LOG(("------------------------------------------\n"));
	}

#ifdef MEMORYPOOL_CHECKPOINTING
	if (doBlockReport)
	{
		const char* nm = (this == TheMemoryPoolFactory) ? "TheMemoryPoolFactory" : "*** UNKNOWN *** MemoryPoolFactory";

		DEBUG_LOG(("\n"));
		DEBUG_LOG(("------------------------------------------\n"));
		DEBUG_LOG(("Begin Block Report for %s\n", nm));
		DEBUG_LOG(("------------------------------------------\n"));
		char buf[256] = "";
		if (flags & _REPORT_CP_ALLOCATED_BEFORE) strcat(buf, "AllocBefore ");
		if (flags & _REPORT_CP_ALLOCATED_BETWEEN) strcat(buf, "AllocBetween ");
		if (flags & _REPORT_CP_FREED_BEFORE) strcat(buf, "FreedBefore ");
		if (flags & _REPORT_CP_FREED_BETWEEN) strcat(buf, "FreedBetween ");
		if (flags & _REPORT_CP_FREED_NEVER) strcat(buf, "StillExisting ");
		DEBUG_LOG(("Options: Between checkpoints %d and %d, report on (%s)\n",startCheckpoint,endCheckpoint,buf));
		DEBUG_LOG(("------------------------------------------\n"));

		BlockCheckpointInfo::doBlockCheckpointReport( NULL, "", 0, 0, 0 );
		for (MemoryPool *pool = m_firstPoolInFactory; pool; pool = pool->getNextPoolInList())
		{
			pool->debugCheckpointReport(flags, startCheckpoint, endCheckpoint, pool->getPoolName());
		}
		for (DynamicMemoryAllocator *dma = m_firstDmaInFactory; dma; dma = dma->getNextDmaInList())
		{
			dma->debugCheckpointReport(flags, startCheckpoint, endCheckpoint, "(Oversized)");
		}

		DEBUG_LOG(("------------------------------------------\n"));
		DEBUG_LOG(("End Block Report for %s\n", nm));
		DEBUG_LOG(("------------------------------------------\n"));
	}
#endif

	DebugSetFlags(oldFlags);
}
#endif

//-----------------------------------------------------------------------------
// GLOBAL FUNCTIONS
//-----------------------------------------------------------------------------

/*
	This is a trick that is intended to force MSVC to link this file (and thus,
	these definitions of new/delete) ahead of all others. (We do debug checking
	to ensure that's the case)
*/
#if defined(_DEBUG)
	#pragma comment(lib, "GameEngineDebug")
#elif defined(_INTERNAL)
	#pragma comment(lib, "GameEngineInternal")
#else
	#pragma comment(lib, "GameEngine")
#endif

#ifdef MEMORYPOOL_OVERRIDE_MALLOC
	#pragma comment(linker, "/force:multiple")
#endif

static int theLinkTester = 0;

//-----------------------------------------------------------------------------
void* STLSpecialAlloc::allocate(size_t __n) 
{  
	++theLinkTester;
	preMainInitMemoryManager();
	DEBUG_ASSERTCRASH(TheDynamicMemoryAllocator != NULL, ("must init memory manager before calling global operator new"));
	return TheDynamicMemoryAllocator->allocateBytes(__n, "STL_"); 
}

//-----------------------------------------------------------------------------
void STLSpecialAlloc::deallocate(void* __p, size_t) 
{ 
	++theLinkTester;
	preMainInitMemoryManager();
	DEBUG_ASSERTCRASH(TheDynamicMemoryAllocator != NULL, ("must init memory manager before calling global operator new"));
	TheDynamicMemoryAllocator->freeBytes(__p); 
}

//-----------------------------------------------------------------------------
/**
	overload for global operator new; send requests to TheDynamicMemoryAllocator.
*/
void *operator new(size_t size)
{
	++theLinkTester;
	preMainInitMemoryManager();
	DEBUG_ASSERTCRASH(TheDynamicMemoryAllocator != NULL, ("must init memory manager before calling global operator new"));
	return TheDynamicMemoryAllocator->allocateBytes(size, "global operator new");
}

//-----------------------------------------------------------------------------
/**
	overload for global operator new[]; send requests to TheDynamicMemoryAllocator.
*/
void *operator new[](size_t size)
{
	++theLinkTester;
	preMainInitMemoryManager();
	DEBUG_ASSERTCRASH(TheDynamicMemoryAllocator != NULL, ("must init memory manager before calling global operator new"));
	return TheDynamicMemoryAllocator->allocateBytes(size, "global operator new[]");
}
 
//-----------------------------------------------------------------------------
/**
	overload for global operator delete; send requests to TheDynamicMemoryAllocator.
*/
void operator delete(void *p)
{
	++theLinkTester;
	preMainInitMemoryManager();
	DEBUG_ASSERTCRASH(TheDynamicMemoryAllocator != NULL, ("must init memory manager before calling global operator delete"));
	TheDynamicMemoryAllocator->freeBytes(p);
}

//-----------------------------------------------------------------------------
/**
	overload for global operator delete[]; send requests to TheDynamicMemoryAllocator.
*/
void operator delete[](void *p)
{
	++theLinkTester;
	preMainInitMemoryManager();
	DEBUG_ASSERTCRASH(TheDynamicMemoryAllocator != NULL, ("must init memory manager before calling global operator delete"));
	TheDynamicMemoryAllocator->freeBytes(p);
}

//-----------------------------------------------------------------------------
/**
	overload for global operator new (MFC debug version); send requests to TheDynamicMemoryAllocator.
*/
void* operator new(size_t size, const char * fname, int)
{
	++theLinkTester;
	preMainInitMemoryManager();
	DEBUG_ASSERTCRASH(TheDynamicMemoryAllocator != NULL, ("must init memory manager before calling global operator new"));
#ifdef MEMORYPOOL_DEBUG
	return TheDynamicMemoryAllocator->allocateBytesImplementation(size, fname);
#else
	return TheDynamicMemoryAllocator->allocateBytesImplementation(size);
#endif
}

//-----------------------------------------------------------------------------
/**
	overload for global operator delete (MFC debug version); send requests to TheDynamicMemoryAllocator.
*/
void operator delete(void * p, const char *, int)
{
	++theLinkTester;
	preMainInitMemoryManager();
	DEBUG_ASSERTCRASH(TheDynamicMemoryAllocator != NULL, ("must init memory manager before calling global operator delete"));
	TheDynamicMemoryAllocator->freeBytes(p);
}

//-----------------------------------------------------------------------------
/**
	overload for global operator new (MFC debug version); send requests to TheDynamicMemoryAllocator.
*/
void* operator new[](size_t size, const char * fname, int)
{
	++theLinkTester;
	preMainInitMemoryManager();
	DEBUG_ASSERTCRASH(TheDynamicMemoryAllocator != NULL, ("must init memory manager before calling global operator new"));
#ifdef MEMORYPOOL_DEBUG
	return TheDynamicMemoryAllocator->allocateBytesImplementation(size, fname);
#else
	return TheDynamicMemoryAllocator->allocateBytesImplementation(size);
#endif
}

//-----------------------------------------------------------------------------
/**
	overload for global operator delete (MFC debug version); send requests to TheDynamicMemoryAllocator.
*/
void operator delete[](void * p, const char *, int)
{
	++theLinkTester;
	preMainInitMemoryManager();
	DEBUG_ASSERTCRASH(TheDynamicMemoryAllocator != NULL, ("must init memory manager before calling global operator delete"));
	TheDynamicMemoryAllocator->freeBytes(p);
}

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_OVERRIDE_MALLOC
void *calloc(size_t a, size_t b)
{
	++theLinkTester;
	preMainInitMemoryManager();
	DEBUG_ASSERTCRASH(TheDynamicMemoryAllocator != NULL, ("must init memory manager"));
	return TheDynamicMemoryAllocator->allocateBytes(a * b, "calloc");
}
#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_OVERRIDE_MALLOC
void  free(void * p)
{
	++theLinkTester;
	preMainInitMemoryManager();
	DEBUG_ASSERTCRASH(TheDynamicMemoryAllocator != NULL, ("must init memory manager"));
	TheDynamicMemoryAllocator->freeBytes(p);
}
#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_OVERRIDE_MALLOC
void *malloc(size_t a)
{
	++theLinkTester;
	preMainInitMemoryManager();
	DEBUG_ASSERTCRASH(TheDynamicMemoryAllocator != NULL, ("must init memory manager"));
	return TheDynamicMemoryAllocator->allocateBytesDoNotZero(a, "malloc");
}
#endif

//-----------------------------------------------------------------------------
#ifdef MEMORYPOOL_OVERRIDE_MALLOC
void *realloc(void *p, size_t s)
{
	DEBUG_CRASH(("realloc is evil. do not call it."));
	throw ERROR_OUT_OF_MEMORY;
}
#endif

//-----------------------------------------------------------------------------
/**
	Initialize the memory manager, and create TheMemoryPoolFactory and TheDynamicMemoryAllocator.
*/
void initMemoryManager()
{
	if (TheMemoryPoolFactory == NULL) 
	{
		Int numSubPools;
		const PoolInitRec *pParms;
		userMemoryManagerGetDmaParms(&numSubPools, &pParms);
		TheMemoryPoolFactory = new (::sysAllocate(sizeof(MemoryPoolFactory))) MemoryPoolFactory;	// will throw on failure
		TheMemoryPoolFactory->init();	// will throw on failure
		TheDynamicMemoryAllocator = TheMemoryPoolFactory->createDynamicMemoryAllocator(numSubPools, pParms);	// will throw on failure
		userMemoryManagerInitPools();
		thePreMainInitFlag = false;
	}
	else
	{
		if (thePreMainInitFlag)
		{
			// quietly ignore the call
		}
		else 
		{
			DEBUG_CRASH(("memory manager is already inited"));
		}
	}

	char* linktest;
	
	theLinkTester = 0; 

	linktest = new char;
	delete linktest;

	linktest = new char[8];
	delete [] linktest;

	linktest = new char[1];
	delete linktest;

#ifdef MEMORYPOOL_OVERRIDE_MALLOC
	linktest = (char*)malloc(1);
	free(linktest);

	linktest = (char*)calloc(1,1);
	free(linktest);
#endif

#ifdef MEMORYPOOL_OVERRIDE_MALLOC
	if (theLinkTester != 10)
#else
	if (theLinkTester != 6)
#endif
	{
		DEBUG_CRASH(("Wrong operator new/delete linked in! Fix this...\n"));
		exit(-1);
	}

	theMainInitFlag = true;

}

//-----------------------------------------------------------------------------
Bool isMemoryManagerOfficiallyInited()
{
	return theMainInitFlag;
}

//-----------------------------------------------------------------------------
/**
	Initialize the memory manager, and create TheMemoryPoolFactory and TheDynamicMemoryAllocator.
	This is only called if memory is allocated prior to the normal call to initMemoryManager
	(generally via a static C++ ctor).
*/
static void preMainInitMemoryManager()
{
	if (TheMemoryPoolFactory == NULL)
	{
		DEBUG_INIT(DEBUG_FLAGS_DEFAULT);
		DEBUG_LOG(("*** Initing Memory Manager prior to main!\n"));

		Int numSubPools;
		const PoolInitRec *pParms;
		userMemoryManagerGetDmaParms(&numSubPools, &pParms);
		TheMemoryPoolFactory = new (::sysAllocate(sizeof(MemoryPoolFactory))) MemoryPoolFactory;	// will throw on failure
		TheMemoryPoolFactory->init();	// will throw on failure

		TheDynamicMemoryAllocator = TheMemoryPoolFactory->createDynamicMemoryAllocator(numSubPools, pParms);	// will throw on failure
		userMemoryManagerInitPools();
		thePreMainInitFlag = true;
	}
}

//-----------------------------------------------------------------------------
/**
	shutdown the memory manager and discard all memory. Note: if preMainInitMemoryManager()
	was called prior to initMemoryManager(), this call will do nothing.
*/
void shutdownMemoryManager()
{
	if (thePreMainInitFlag)
	{
	#ifdef MEMORYPOOL_DEBUG
		DEBUG_LOG(("*** Memory Manager was inited prior to main -- skipping shutdown!\n"));
	#endif
	}
	else
	{
		if (TheDynamicMemoryAllocator)
		{
			DEBUG_ASSERTCRASH(TheMemoryPoolFactory, ("hmm, no factory"));
			if (TheMemoryPoolFactory) 
				TheMemoryPoolFactory->destroyDynamicMemoryAllocator(TheDynamicMemoryAllocator);
			TheDynamicMemoryAllocator = NULL;
		}

		if (TheMemoryPoolFactory) 
		{
			// this is evil... since there is no 'placement delete' we must do this the hard way
			// and call the dtor directly. ordinarily this is heinous, but in this case we'll
			// make an exception.
			TheMemoryPoolFactory->~MemoryPoolFactory();
			::sysFree((void *)TheMemoryPoolFactory);
			TheMemoryPoolFactory = NULL;
		}

	#ifdef MEMORYPOOL_DEBUG
		DEBUG_LOG(("Peak system allocation was %d bytes\n",thePeakSystemAllocationInBytes));
		DEBUG_LOG(("Wasted DMA space (peak) was %d bytes\n",thePeakWastedDMA));
		DEBUG_ASSERTCRASH(theTotalSystemAllocationInBytes == 0, ("Leaked a total of %d raw bytes\n", theTotalSystemAllocationInBytes));
	#endif
	}

	theMainInitFlag = false;
}

//-----------------------------------------------------------------------------
void* createW3DMemPool(const char *poolName, int allocationSize)
{
	++theLinkTester;
	preMainInitMemoryManager();
	MemoryPool* pool = TheMemoryPoolFactory->createMemoryPool(poolName, allocationSize, 0, 0);
	DEBUG_ASSERTCRASH(pool && pool->getAllocationSize() == allocationSize, ("bad w3d pool"));
	return pool;
}

//-----------------------------------------------------------------------------
void* allocateFromW3DMemPool(void* pool, int allocationSize)
{
	DEBUG_ASSERTCRASH(pool, ("pool is null\n"));
	DEBUG_ASSERTCRASH(pool && ((MemoryPool*)pool)->getAllocationSize() == allocationSize, ("bad w3d pool size %s",((MemoryPool*)pool)->getPoolName()));
	return ((MemoryPool*)pool)->allocateBlock("allocateFromW3DMemPool");
}

//-----------------------------------------------------------------------------
void* allocateFromW3DMemPool(void* pool, int allocationSize, const char* msg, int unused)
{
	DEBUG_ASSERTCRASH(pool, ("pool is null\n"));
	DEBUG_ASSERTCRASH(pool && ((MemoryPool*)pool)->getAllocationSize() == allocationSize, ("bad w3d pool size %s",((MemoryPool*)pool)->getPoolName()));
	return ((MemoryPool*)pool)->allocateBlock(msg);
}

//-----------------------------------------------------------------------------
void freeFromW3DMemPool(void* pool, void* p)
{
	DEBUG_ASSERTCRASH(pool, ("pool is null\n"));
	((MemoryPool*)pool)->freeBlock(p);
}
