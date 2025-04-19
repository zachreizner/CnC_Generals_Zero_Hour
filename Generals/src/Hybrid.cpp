#include "PreRTS.h"

#include <iostream>
#include <arpa/inet.h>

#include "Common/AudioAffect.h"
#include "Common/ArchiveFile.h"
#include "Common/ArchiveFileSystem.h"
#include "Common/File.h"
#include "Common/GameAudio.h"
#include "Common/LocalFileSystem.h"
#include "Common/RAMFile.h"
#include "Common/StreamingArchiveFile.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/GhostObject.h"
#include "GameLogic/TerrainLogic.h"
#include "GameClient/ParticleSys.h"

#include "Hybrid.h"

extern "C"
{
    Int HybridFileRead(void *handle, void *buffer, Int bytes);
    Int HybridFileSeek(void *handle, Int new_pos, int mode);
}

[[noreturn]] void todo(const char *f, int line)
{
    DEBUG_LOG(("TODO on %s:%d", f, line));

    std::quick_exit(1);
}

#define TODO todo(__FILE__, __LINE__);

void addIcon(const Coord3D *pos, Real width, Int numFramesDuration, RGBColor color)
{
}

void InsertFilenameList(FilenameList &filenameList, const std::string &filename)
{
    filenameList.insert(filename.c_str());
}

class HybridFile : public File
{
    MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE(HybridFile, "HybridFile")
private:
    void *m_handle;

public:
    HybridFile(void *handle);

    virtual Bool open(const Char *filename, Int access = 0) override;
    virtual Int read(void *buffer, Int bytes) override;
    virtual Int write(const void *buffer, Int bytes) override;
    virtual Int seek(Int new_pos, seekMode mode = CURRENT) override;
    virtual void nextLine(Char *buf = NULL, Int bufSize = 0) override;
    virtual Bool scanInt(Int &newInt) override;
    virtual Bool scanReal(Real &newReal) override;
    virtual Bool scanString(AsciiString &newString) override;
    virtual char *readEntireAndClose() override;
    virtual File *convertToRAMFile() override;
};

HybridFile::HybridFile(void *handle) : File(), m_handle(handle)
{
}

HybridFile::~HybridFile() {}

Bool HybridFile::open(const Char *filename, Int access)
{
    if (!File::open(filename, access))
    {
        return FALSE;
    }

    TODO;

    return FALSE;
}

Int HybridFile::read(void *buffer, Int bytes)
{
    return HybridFileRead(m_handle, buffer, bytes);
}

Int HybridFile::write(const void *buffer, Int bytes) { TODO; }
Int HybridFile::seek(Int new_pos, seekMode mode)
{
    return HybridFileSeek(m_handle, new_pos, (int)mode);
}
void HybridFile::nextLine(Char *buf, Int bufSize) { TODO; }
Bool HybridFile::scanInt(Int &newInt) { TODO; }
Bool HybridFile::scanReal(Real &newReal) { TODO; }
Bool HybridFile::scanString(AsciiString &newString) { TODO; }
char *HybridFile::readEntireAndClose() { TODO; }
File *HybridFile::convertToRAMFile() { TODO }

class HybridArchiveFile : public ArchiveFile
{
public:
    HybridArchiveFile();
    virtual ~HybridArchiveFile();

    virtual Bool getFileInfo(const AsciiString &filename, FileInfo *fileInfo) const; ///< fill in the fileInfo struct with info about the requested file.
    virtual File *openFile(const Char *filename, Int access = 0);                    ///< Open the specified file within the BIG file
    virtual void closeAllFiles(void);                                                ///< Close all file opened in this BIG file
    virtual AsciiString getName(void);                                               ///< Returns the name of the BIG file
    virtual AsciiString getPath(void);                                               ///< Returns full path and name of BIG file
    virtual void setSearchPriority(Int new_priority);                                ///< Set this BIG file's search priority
    virtual void close(void);                                                        ///< Close this BIG file

protected:
    AsciiString m_name;
    AsciiString m_path;
};

HybridArchiveFile::HybridArchiveFile()
{
}

HybridArchiveFile::~HybridArchiveFile()
{
}

void HybridArchiveFile::closeAllFiles(void)
{
}

AsciiString HybridArchiveFile::getName(void)
{
    return m_name;
}
AsciiString HybridArchiveFile::getPath(void)
{
    return m_path;
}
void HybridArchiveFile::setSearchPriority(Int new_priority)
{
}

void HybridArchiveFile::close(void)
{
}

Bool HybridArchiveFile::getFileInfo(const AsciiString &filename, FileInfo *fileInfo) const
{
    const ArchivedFileInfo *tempFileInfo = getArchivedFileInfo(filename);

    if (tempFileInfo == NULL)
    {
        return FALSE;
    }

    TheLocalFileSystem->getFileInfo(AsciiString(m_file->getName()), fileInfo);

    // fill in the size info.  Since the size can't be bigger than a JUNK file, the high Int will always be 0.
    fileInfo->sizeHigh = 0;
    fileInfo->sizeLow = tempFileInfo->m_size;

    return TRUE;
}

File *HybridArchiveFile::openFile(const Char *filename, Int access)
{
    const ArchivedFileInfo *fileInfo = getArchivedFileInfo(AsciiString(filename));

    if (fileInfo == NULL)
    {
        return NULL;
    }

    RAMFile *ramFile = NULL;

    if (BitTest(access, File::STREAMING))
        ramFile = newInstance(StreamingArchiveFile);
    else
        ramFile = newInstance(RAMFile);

    ramFile->deleteOnClose();
    if (ramFile->openFromArchive(m_file, fileInfo->m_filename, fileInfo->m_offset, fileInfo->m_size) == FALSE)
    {
        ramFile->close();
        ramFile = NULL;
        return NULL;
    }

    if ((access & File::WRITE) == 0)
    {
        // requesting read only access. Just return the RAM file.
        return ramFile;
    }

    // whoever is opening this file wants write access, so copy the file to the local disk
    // and return that file pointer.

    File *localFile = TheLocalFileSystem->openFile(filename, access);
    if (localFile != NULL)
    {
        ramFile->copyDataToFile(localFile);
    }

    ramFile->close();
    ramFile = NULL;

    return localFile;
}

class HybridArchiveFileSystem : public ArchiveFileSystem
{
public:
    HybridArchiveFileSystem();
    virtual ~HybridArchiveFileSystem();

    virtual void init() override;
    virtual void update() override;
    virtual void reset() override;
    virtual void postProcessLoad() override;
    virtual ArchiveFile *openArchiveFile(const Char *filename) override;
    virtual void closeArchiveFile(const Char *filename) override;
    virtual void closeAllArchiveFiles() override;
    virtual void closeAllFiles() override;
    virtual Bool loadBigFilesFromDirectory(AsciiString dir, AsciiString fileMask, Bool overwrite = FALSE) override;
};

HybridArchiveFileSystem::HybridArchiveFileSystem() : ArchiveFileSystem()
{
}

HybridArchiveFileSystem::~HybridArchiveFileSystem()
{
}

void HybridArchiveFileSystem::init()
{
    DEBUG_ASSERTCRASH(TheLocalFileSystem != NULL, ("TheLocalFileSystem must be initialized before TheArchiveFileSystem."));
    if (TheLocalFileSystem == NULL)
    {
        return;
    }

    loadBigFilesFromDirectory("", "*.big");
}

void HybridArchiveFileSystem::reset()
{
}

void HybridArchiveFileSystem::update()
{
}

void HybridArchiveFileSystem::postProcessLoad()
{
}

ArchiveFile *HybridArchiveFileSystem::openArchiveFile(const Char *filename)
{
    File *fp = TheLocalFileSystem->openFile(filename, File::READ | File::BINARY);
    AsciiString archiveFileName;
    archiveFileName = filename;
    archiveFileName.toLower();
    Int archiveFileSize = 0;
    Int numLittleFiles = 0;

    ArchiveFile *archiveFile = NEW HybridArchiveFile;

    DEBUG_LOG(("HybridArchiveFileSystem::openArchiveFile - opening BIG file \"%s\"\n", filename));

    if (fp == NULL)
    {
        DEBUG_CRASH(("Could not open archive file %s for parsing", filename));
        return NULL;
    }

    AsciiString asciibuf;
    char buffer[_MAX_PATH];
    fp->read(buffer, 4); // read the "BIG" at the beginning of the file.
    buffer[4] = 0;
    if (strcmp(buffer, "BIGF") != 0)
    {
        DEBUG_CRASH(("Error reading BIG file identifier in file %s", filename));
        fp->close();
        fp = NULL;
        return NULL;
    }

    // read in the file size.
    fp->read(&archiveFileSize, 4);

    DEBUG_LOG(("HybridArchiveFileSystem::openArchiveFile - size of archive file is %d bytes\n", archiveFileSize));

    fp->read(&numLittleFiles, 4);
    numLittleFiles = ntohl(numLittleFiles);

    DEBUG_LOG(("HybridArchiveFileSystem::openArchiveFile - %d are contained in archive\n", numLittleFiles));

    // seek to the beginning of the directory listing.
    fp->seek(0x10, File::START);
    // read in each directory listing.
    ArchivedFileInfo *fileInfo = NEW ArchivedFileInfo;

    for (Int i = 0; i < numLittleFiles; ++i)
    {
        Int filesize = 0;
        Int fileOffset = 0;
        fp->read(&fileOffset, 4);
        fp->read(&filesize, 4);

        filesize = ntohl(filesize);
        fileOffset = ntohl(fileOffset);

        fileInfo->m_archiveFilename = archiveFileName;
        fileInfo->m_offset = fileOffset;
        fileInfo->m_size = filesize;

        // read in the path name of the file.
        Int pathIndex = -1;
        do
        {
            ++pathIndex;
            fp->read(buffer + pathIndex, 1);
        } while (buffer[pathIndex] != 0);

        Int filenameIndex = pathIndex;
        while ((buffer[filenameIndex] != '\\') && (buffer[filenameIndex] != '/') && (filenameIndex >= 0))
        {
            --filenameIndex;
        }

        fileInfo->m_filename = (char *)(buffer + filenameIndex + 1);
        fileInfo->m_filename.toLower();
        buffer[filenameIndex + 1] = 0;

        AsciiString path;
        path = buffer;

        AsciiString debugpath;
        debugpath = path;
        debugpath.concat(fileInfo->m_filename);
        DEBUG_LOG(("HybridArchiveFileSystem::openArchiveFile - adding file %s to archive file %s, file number %d\n", debugpath.str(), fileInfo->m_archiveFilename.str(), i));

        archiveFile->addFile(path, fileInfo);
    }

    archiveFile->attachFile(fp);

    delete fileInfo;
    fileInfo = NULL;

    // leave fp open as the archive file will be using it.

    return archiveFile;
}

void HybridArchiveFileSystem::closeArchiveFile(const Char *filename)
{
    // Need to close the specified big file
    ArchiveFileMap::iterator it = m_archiveFileMap.find(filename);
    if (it == m_archiveFileMap.end())
    {
        return;
    }

    if (strcasecmp(filename, MUSIC_BIG) == 0)
    {
        // Stop the current audio
        TheAudio->stopAudio(AudioAffect_Music);

        // No need to turn off other audio, as the lookups will just fail.
    }
    DEBUG_ASSERTCRASH(strcasecmp(filename, MUSIC_BIG) == 0, ("Attempting to close Archive file '%s', need to add code to handle its shutdown correctly.", filename));

    // may need to do some other processing here first.

    delete (it->second);
    m_archiveFileMap.erase(it);
}

void HybridArchiveFileSystem::closeAllArchiveFiles()
{
}

void HybridArchiveFileSystem::closeAllFiles()
{
}

Bool HybridArchiveFileSystem::loadBigFilesFromDirectory(AsciiString dir, AsciiString fileMask, Bool overwrite)
{
    FilenameList filenameList;
    TheLocalFileSystem->getFileListInDirectory(dir, AsciiString(""), fileMask, filenameList, TRUE);

    Bool actuallyAdded = FALSE;
    FilenameListIter it = filenameList.begin();
    while (it != filenameList.end())
    {
        ArchiveFile *archiveFile = openArchiveFile((*it).str());

        if (archiveFile != NULL)
        {
            DEBUG_LOG(("HybridArchiveFileSystem::loadBigFilesFromDirectory - loading %s into the directory tree.\n", (*it).str()));
            loadIntoDirectoryTree(archiveFile, *it, overwrite);
            m_archiveFileMap[(*it)] = archiveFile;
            DEBUG_LOG(("HybridArchiveFileSystem::loadBigFilesFromDirectory - %s inserted into the archive file map.\n", (*it).str()));
            actuallyAdded = TRUE;
        }

        it++;
    }

    return actuallyAdded;
}

class HybridGameLogic : public GameLogic
{
protected:
    virtual TerrainLogic *createTerrainLogic()
    {
        return new TerrainLogic;
    };
    virtual GhostObjectManager *createGhostObjectManager()
    {
        return new GhostObjectManager;
    }
};

extern "C"
{
    File *CreateHybridFile(void *handle)
    {
        return newInstance(HybridFile)(handle);
    }

    ArchiveFileSystem *CreateHybridArchiveFileSystem()
    {
        return new HybridArchiveFileSystem;
    }

    GameLogic *CreateHybridGameLogic()
    {
        return new HybridGameLogic;
    }
}

class HybridAudioManager : public AudioManager
{
    virtual void audioDebugDisplay(DebugDisplayInterface *dd, void *userData, FILE *fp = NULL);
    virtual void stopAudio(AudioAffect which);
    virtual void pauseAudio(AudioAffect which);
    virtual void resumeAudio(AudioAffect which);
    virtual void pauseAmbient(Bool shouldPause);
    virtual void stopAllAmbientsBy(Object *obj);
    virtual void stopAllAmbientsBy(Drawable *draw);
    virtual void killAudioEventImmediately(AudioHandle audioEvent);
    virtual void nextMusicTrack(void);
    virtual void prevMusicTrack(void);
    virtual Bool isMusicPlaying(void) const;
    virtual Bool hasMusicTrackCompleted(const AsciiString &trackName, Int numberOfTimes) const;
    virtual AsciiString getMusicTrackName(void) const;
    virtual void openDevice(void);
    virtual void closeDevice(void);
    virtual void *getDevice(void);
    virtual void notifyOfAudioCompletion(UnsignedInt audioCompleted, UnsignedInt flags);
    virtual UnsignedInt getProviderCount(void) const;
    virtual AsciiString getProviderName(UnsignedInt providerNum) const;
    virtual UnsignedInt getProviderIndex(AsciiString providerName) const;
    virtual void selectProvider(UnsignedInt providerNdx);
    virtual void unselectProvider(void);
    virtual UnsignedInt getSelectedProvider(void) const;
    virtual void setSpeakerType(UnsignedInt speakerType);
    virtual UnsignedInt getSpeakerType(void);
    virtual UnsignedInt getNum2DSamples(void) const;
    virtual UnsignedInt getNum3DSamples(void) const;
    virtual UnsignedInt getNumStreams(void) const;
    virtual Bool doesViolateLimit(AudioEventRTS *event) const;
    virtual Bool isPlayingLowerPriority(AudioEventRTS *event) const;
    virtual Bool isPlayingAlready(AudioEventRTS *event) const;
    virtual Bool isObjectPlayingVoice(UnsignedInt objID) const;
    virtual void adjustVolumeOfPlayingAudio(AsciiString eventName, Real newVolume);
    virtual void removePlayingAudio(AsciiString eventName);
    virtual void removeAllDisabledAudio();
    virtual void *getHandleForBink(void);
    virtual void releaseHandleForBink(void);
    virtual void friend_forcePlayAudioEventRTS(const AudioEventRTS *eventToPlay);
    virtual void setPreferredProvider(AsciiString providerNdx);
    virtual void setPreferredSpeaker(AsciiString speakerType);
    virtual Real getFileLengthMS(AsciiString strToLoad) const;
    virtual void closeAnySamplesUsingFile(const void *fileToClose);
    virtual void setDeviceListenerPosition(void);
};

void HybridAudioManager::audioDebugDisplay(DebugDisplayInterface *dd, void *userData, FILE *fp) { TODO; }
void HybridAudioManager::stopAudio(AudioAffect which) { TODO; }
void HybridAudioManager::pauseAudio(AudioAffect which) { TODO; }
void HybridAudioManager::resumeAudio(AudioAffect which) { TODO; }
void HybridAudioManager::pauseAmbient(Bool shouldPause) { TODO; }
void HybridAudioManager::stopAllAmbientsBy(Object *obj) { TODO; }
void HybridAudioManager::stopAllAmbientsBy(Drawable *draw) { TODO; }
void HybridAudioManager::killAudioEventImmediately(AudioHandle audioEvent) { TODO; }
void HybridAudioManager::nextMusicTrack(void) { TODO; }
void HybridAudioManager::prevMusicTrack(void) { TODO; }
Bool HybridAudioManager::isMusicPlaying(void) const { TODO; }
Bool HybridAudioManager::hasMusicTrackCompleted(const AsciiString &trackName, Int numberOfTimes) const { TODO; }
AsciiString HybridAudioManager::getMusicTrackName(void) const { TODO; }
void HybridAudioManager::openDevice(void) { TODO; }
void HybridAudioManager::closeDevice(void) { TODO; }
void *HybridAudioManager::getDevice(void) { TODO; }
void HybridAudioManager::notifyOfAudioCompletion(UnsignedInt audioCompleted, UnsignedInt flags) { TODO; }
UnsignedInt HybridAudioManager::getProviderCount(void) const { TODO; }
AsciiString HybridAudioManager::getProviderName(UnsignedInt providerNum) const { TODO; }
UnsignedInt HybridAudioManager::getProviderIndex(AsciiString providerName) const { TODO; }
void HybridAudioManager::selectProvider(UnsignedInt providerNdx) { TODO; }
void HybridAudioManager::unselectProvider(void) { TODO; }
UnsignedInt HybridAudioManager::getSelectedProvider(void) const { TODO; }
void HybridAudioManager::setSpeakerType(UnsignedInt speakerType) { TODO; }
UnsignedInt HybridAudioManager::getSpeakerType(void) { TODO; }
UnsignedInt HybridAudioManager::getNum2DSamples(void) const { TODO; }
UnsignedInt HybridAudioManager::getNum3DSamples(void) const { TODO; }
UnsignedInt HybridAudioManager::getNumStreams(void) const { TODO; }
Bool HybridAudioManager::doesViolateLimit(AudioEventRTS *event) const { TODO; }
Bool HybridAudioManager::isPlayingLowerPriority(AudioEventRTS *event) const { TODO; }
Bool HybridAudioManager::isPlayingAlready(AudioEventRTS *event) const { TODO; }
Bool HybridAudioManager::isObjectPlayingVoice(UnsignedInt objID) const { TODO; }
void HybridAudioManager::adjustVolumeOfPlayingAudio(AsciiString eventName, Real newVolume) { TODO; }
void HybridAudioManager::removePlayingAudio(AsciiString eventName) { TODO; }
void HybridAudioManager::removeAllDisabledAudio() { TODO; }
void *HybridAudioManager::getHandleForBink(void) { TODO; }
void HybridAudioManager::releaseHandleForBink(void) { TODO; }
void HybridAudioManager::friend_forcePlayAudioEventRTS(const AudioEventRTS *eventToPlay) { TODO; }
void HybridAudioManager::setPreferredProvider(AsciiString providerNdx)
{
    DEBUG_LOG(("HybridAudioManager setPreferredProvider \"%s\"", providerNdx.str()));
}
void HybridAudioManager::setPreferredSpeaker(AsciiString speakerType)
{
    DEBUG_LOG(("HybridAudioManager setPreferredSpeaker \"%s\"", speakerType.str()));
}
Real HybridAudioManager::getFileLengthMS(AsciiString strToLoad) const { TODO; }
void HybridAudioManager::closeAnySamplesUsingFile(const void *fileToClose) { TODO; }
void HybridAudioManager::setDeviceListenerPosition(void) { TODO; }

AudioManager *CreateAudioManager()
{
    return new HybridAudioManager;
}

class HybridFunctionLexicon : public FunctionLexicon
{
public:
    HybridFunctionLexicon(void);
    virtual ~HybridFunctionLexicon(void);
};

HybridFunctionLexicon::HybridFunctionLexicon()
{
}

HybridFunctionLexicon::~HybridFunctionLexicon()
{
}

FunctionLexicon *CreateFunctionLexicon()
{
    return new HybridFunctionLexicon;
}

class HybridParticleSystemManager : public ParticleSystemManager
{
public:
    HybridParticleSystemManager();
    ~HybridParticleSystemManager();
    virtual void doParticles(RenderInfoClass &rinfo);
    virtual void queueParticleRender();
    virtual Int getOnScreenParticleCount() { return m_onScreenParticleCount; }
};

HybridParticleSystemManager::HybridParticleSystemManager()
{
}
HybridParticleSystemManager::~HybridParticleSystemManager() {}

void HybridParticleSystemManager::queueParticleRender()
{
}

void HybridParticleSystemManager::doParticles(RenderInfoClass &rinfo)
{
}

ParticleSystemManager *CreateParticleSystemManager()
{
    return new HybridParticleSystemManager;
}

