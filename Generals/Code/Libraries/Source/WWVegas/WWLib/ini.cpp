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

/*********************************************************************************************** 
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               *** 
 *********************************************************************************************** 
 *                                                                                             * 
 *                 Project Name : Command & Conquer                                            * 
 *                                                                                             * 
 *                     $Archive:: /Commando/Code/wwlib/ini.cpp                                $* 
 *                                                                                             * 
 *                      $Author:: Byon_g                                                      $*
 *                                                                                             * 
 *                     $Modtime:: 7/23/01 6:20p                                               $*
 *                                                                                             * 
 *                    $Revision:: 17                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------* 
 * Functions:                                                                                  * 
 *   INIClass::Clear -- Clears out a section (or all sections) of the INI data.                *
 *   INIClass::Entry_Count -- Fetches the number of entries in a specified section.            *
 *   INIClass::Find_Entry -- Find specified entry within section.                              *
 *   INIClass::Find_Section -- Find the specified section within the INI data.                 *
 *   INIClass::Get_Bool -- Fetch a boolean value for the section and entry specified.          *
 *   INIClass::Get_Entry -- Get the entry identifier name given ordinal number and section name*
 *   INIClass::Get_Float -- Fetch a floating point number from the database.                   * 
 *   INIClass::Get_Hex -- Fetches integer [hex format] from the section and entry specified.   *
 *   INIClass::Get_Int -- Fetch an integer entry from the specified section.                   *
 *   INIClass::Get_PKey -- Fetch a key from the ini database.                                  *
 *   INIClass::Get_String -- Fetch the value of a particular entry in a specified section.     *
 *   INIClass::Get_TextBlock -- Fetch a block of normal text.                                  *
 *   INIClass::Get_UUBlock -- Fetch an encoded block from the section specified.               *
 *   INIClass::INISection::Find_Entry -- Finds a specified entry and returns pointer to it.    *
 *   INIClass::Load -- Load INI data from the file specified.                                  *
 *   INIClass::Load -- Load the INI data from the data stream (straw).                         *
 *   INIClass::Put_Bool -- Store a boolean value into the INI database.                        *
 *   INIClass::Put_Float -- Store a floating point number to the database.                     * 
 *   INIClass::Put_Hex -- Store an integer into the INI database, but use a hex format.        *
 *   INIClass::Put_Int -- Stores a signed integer into the INI data base.                      *
 *   INIClass::Put_PKey -- Stores the key to the INI database.                                 *
 *   INIClass::Put_String -- Output a string to the section and entry specified.               *
 *   INIClass::Put_TextBlock -- Stores a block of text into an INI section.                    *
 *   INIClass::Put_UUBlock -- Store a binary encoded data block into the INI database.         *
 *   INIClass::Save -- Save the ini data to the file specified.                                *
 *   INIClass::Save -- Saves the INI data to a pipe stream.                                    *
 *   INIClass::Section_Count -- Counts the number of sections in the INI data.                 *
 *   INIClass::Strip_Comments -- Strips comments of the specified text line.                   *
 *   INIClass::~INIClass -- Destructor for INI handler.                                        *
 *   INIClass::Put_Rect -- Store a rectangle  into the INI database.                           *
 *   INIClass::Get_Rect -- Retrieve a rectangle data from the database.                        *
 *   INIClass::Put_Point -- Store a point value to the database.                               *
 *   INIClass::Get_Point -- Fetch a point value from the INI database.                         *
 *   INIClass::Put_Point -- Stores a 3D point to the database.                                 *
 *   INIClass::Get_Point -- Fetch a 3D point from the database.                                *
 *   INIClass::Get_Point -- Fetch a 2D point from the INI database.                            *
 *   INIClass::CRC - returns a (hopefully) unique 32-bit value for a string                    *
 *    -- Displays debug information when a duplicate entry is found in an INI file             *
 *   INIClass::Enumerate_Entries -- Count how many entries begin with a certain prefix followed by a range * 
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include	"always.h"
#include	"b64pipe.h"
#include	"b64straw.h"
#include	"cstraw.h"
#include	"ini.h"
#include	"readline.h"
#include	"trim.h"
#include	"win.h"
#include	"xpipe.h"
#include	"xstraw.h"
#include	<stdio.h>
#ifdef _UNIX
#include <ctype.h>
#endif
#include "rawfile.h"
#include "ffactory.h"

// recently transferred from ini.h 
#include "inisup.h"
#include	"trect.h"
#include	"wwfile.h"
#include	"pk.h"
#include	"pipe.h"
#include	"wwstring.h"

#if defined(__WATCOMC__)
// Disable the "temporary object used to initialize a non-constant reference" warning.
#pragma warning 665 9
#endif


INIEntry::~INIEntry(void) 
{
	free(Entry);
	Entry = NULL;
	free(Value);
	Value = NULL;
}

INISection::~INISection(void) 
{
	free(Section);
	Section = 0;
	EntryList.Delete();
}

bool INIClass::Is_Loaded(void) const 
{
	return(!SectionList->Is_Empty());
}

void INIClass::Initialize(void)
{
	SectionList = W3DNEW List<INISection *> ();
	SectionIndex = W3DNEW IndexClass<int, INISection *> ();
}

void INIClass::Shutdown(void)
{
	delete SectionList;
	delete SectionIndex;
}	

/***********************************************************************************************
 * INIClass::INIClass -- Constructor for INI handler.                                          *
 *                                                                                             *
 *                                                                                             *
 * INPUT:   FileClass object                                                                   *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *=============================================================================================*/
INIClass::INIClass(void)
{
	Initialize();
}

/***********************************************************************************************
 * INIClass::INIClass -- Constructor for INI handler.                                          *
 *                                                                                             *
 *                                                                                             *
 * INPUT:   FileClass object                                                                   *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *=============================================================================================*/

INIClass::INIClass(FileClass & file) 
{
	Initialize();
	Load(file);
}



/***********************************************************************************************
 * INIClass::INIClass -- Constructor for INI handler.                                          *
 *                                                                                             *
 *                                                                                             *
 * INPUT:   filename string                                                                    *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *=============================================================================================*/

INIClass::INIClass(const char *filename) 
{
	Initialize();
	FileClass *file=_TheFileFactory->Get_File(filename);
	if ( file ) {
		Load(*file);
		_TheFileFactory->Return_File(file);
	}
}



/***********************************************************************************************
 * INIClass::~INIClass -- Destructor for INI handler.                                          *
 *                                                                                             *
 *    This is the destructor for the INI class. It handles deleting all of the allocations     *
 *    it might have done.                                                                      *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/02/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
INIClass::~INIClass(void)
{
	Clear();
	Shutdown();
}


/***********************************************************************************************
 * INIClass::Clear -- Clears out a section (or all sections) of the INI data.                  *
 *                                                                                             *
 *    This routine is used to clear out the section specified. If no section is specified,     *
 *    then the entire INI data is cleared out. Optionally, this routine can be used to clear   *
 *    out just an individual entry in the specified section.                                   *
 *                                                                                             *
 * INPUT:   section  -- Pointer to the section to clear out [pass NULL to clear all].          *
 *                                                                                             *
 *          entry    -- Pointer to optional entry specifier. If this parameter is specified,   *
 *                      then only this specific entry (if found) will be cleared. Otherwise,   *
 *                      the entire section specified will be cleared.                          *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/02/1996 JLB : Created.                                                                 *
 *   08/21/1996 JLB : Optionally clears section too.                                           *
 *   11/02/1996 JLB : Updates the index list.                                                  *
 *=============================================================================================*/
bool INIClass::Clear(char const * section, char const * entry)
{
	if (section == NULL) {
		SectionList->Delete();
		SectionIndex->Clear();
	} else {
		INISection * secptr = Find_Section(section);
		if (secptr != NULL) {
			if (entry != NULL) {
				INIEntry * entptr = secptr->Find_Entry(entry);
				if (entptr != NULL) {
					/*
					**	Remove the entry from the entry index list.
					*/
					secptr->EntryIndex.Remove_Index(entptr->Index_ID());

					delete entptr;
				}
			} else {
				/*
				**	Remove this section index from the section index list.
				*/
				SectionIndex->Remove_Index(secptr->Index_ID());

				delete secptr;
			}
		}
	}
	return(true);
}


/***********************************************************************************************
 * INIClass::Load -- Load INI data from the file specified.                                    *
 *                                                                                             *
 *    Use this routine to load the INI class with the data from the specified file.            *
 *                                                                                             *
 * INPUT:   file  -- Reference to the file that will be used to fill up this INI manager.      *
 *                                                                                             *
 * OUTPUT:  bool; Was the file loaded successfully?                                            *
 *                                                                                             *
 * WARNINGS:   This routine allocates memory.                                                  *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/02/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
int INIClass::Load(FileClass & file)
{
	FileStraw fs(file);
	return(Load(fs));
}



/***********************************************************************************************
 * INIClass::Load -- Load INI data from the file specified.                                    *
 *                                                                                             *
 *    Use this routine to load the INI class with the data from the specified file.            *
 *                                                                                             *
 * INPUT:   filename  -- Path for the file to open, factory - thing to make file               *
 *                                                                                             *
 * OUTPUT:  bool; Was the file loaded successfully?                                            *
 *                                                                                             *
 * WARNINGS:   This routine allocates memory.                                                  *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   08/01/2000 NAK : Created.                                                                 *
 *=============================================================================================*/
int INIClass::Load(const char *filename)
{
	file_auto_ptr file(_TheFileFactory, filename);
	int retval=Load(*file);

	return(retval);
}



/***********************************************************************************************
 * INIClass::Load -- Load the INI data from the data stream (straw).                           *
 *                                                                                             *
 *    This will fetch data from the straw and build an INI database from it.                   *
 *                                                                                             *
 * INPUT:   straw -- The straw that the data will be provided from.                            *
 *                                                                                             *
 * OUTPUT:  bool; Was the database loaded ok?                                                  *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/10/1996 JLB : Created.                                                                 *
 *   09/29/1997 JLB : Handles the merging case.                                                *
 *   12/09/1997 EHC : Detects duplicate entry CRCs and fails in that case                      *
 *   03/22/2001 AJA : Treat "foobar=" as a valid entry with value " ".                         *
 *=============================================================================================*/
int INIClass::Load(Straw & ffile)
{
	bool end_of_file = false;
	char buffer[MAX_LINE_LENGTH];

	/*
	**	Determine if the INI database has preexisting entries. If it does,
	**	then the slower merging method of loading is required.
	*/
	bool merge = false;
	if (Section_Count() > 0) {
		merge = true;
	}

	CacheStraw file;
	file.Get_From(ffile);

	/*
	**	Prescan until the first section is found.
	*/
	while (!end_of_file) {
		Read_Line(file, buffer, sizeof(buffer), end_of_file);
		if (end_of_file) return(false);
		if (buffer[0] == '[' && strchr(buffer, ']') != NULL) break;
	}

	if (merge) {

		/*
		**	Process a section. The buffer is prefilled with the section name line.
		*/
		while (!end_of_file) {

			/*
			**	Fetch the section name. Preserve it while the section's entries are
			**	being parsed.
			*/
			buffer[0] = ' ';
			char * ptr = strchr(buffer, ']');
			if (ptr != NULL) *ptr = '\0';
			strtrim(buffer);
			char section[64];
			strcpy(section, buffer);

			/*
			**	Read in the entries of this section.
			*/
			while (!end_of_file) {

				/*
				**	If this line is the start of another section, then bail out
				**	of the entry loop and let the outer section loop take
				**	care of it.
				*/
				int len = Read_Line(file, buffer, sizeof(buffer), end_of_file);
				if (buffer[0] == '[' && strchr(buffer, ']') != NULL) break;

				/*
				**	Determine if this line is a comment or blank line. Throw it out if it is.
				*/
				Strip_Comments(buffer);
				if (len == 0 || buffer[0] == ';' || buffer[0] == '=') continue;

				/*
				**	The line isn't an obvious comment. Make sure that there is the "=" character
				**	at an appropriate spot.
				*/
				char * divider = strchr(buffer, '=');
				if (!divider) continue;

				/*
				**	Split the line into entry and value sections. Be sure to catch the
				**	"=foobar" and "foobar=" cases. "=foobar" lines are ignored, while
				** "foobar=" lines are stores has having " " as their value.
				*/
				*divider++ = '\0';
				strtrim(buffer);
				if (!strlen(buffer)) continue;

				strtrim(divider);
				if (!strlen(divider))
					divider = " ";

				if (Put_String(section, buffer, divider) == false) {
					return(false);
				}
			}
		}

	} else {
		/*
		**	Process a section. The buffer is prefilled with the section name line.
		*/
		while (!end_of_file) {

			buffer[0] = ' ';
			char * ptr = strchr(buffer, ']');
			if (ptr != NULL) *ptr = '\0';
			strtrim(buffer);
			INISection * secptr = W3DNEW INISection(strdup(buffer));
			if (secptr == NULL) {
				Clear();
				return(false);
			}

			/*
			**	Read in the entries of this section.
			*/
			while (!end_of_file) {

				/*
				**	If this line is the start of another section, then bail out
				**	of the entry loop and let the outer section loop take
				**	care of it.
				*/
				int len = Read_Line(file, buffer, sizeof(buffer), end_of_file);
				if (buffer[0] == '[' && strchr(buffer, ']') != NULL) break;

				/*
				**	Determine if this line is a comment or blank line. Throw it out if it is.
				*/
				Strip_Comments(buffer);
				if (len == 0 || buffer[0] == ';' || buffer[0] == '=') continue;

				/*
				**	The line isn't an obvious comment. Make sure that there is the "=" character
				**	at an appropriate spot.
				*/
				char * divider = strchr(buffer, '=');
				if (!divider) continue;

				/*
				**	Split the line into entry and value sections. Be sure to catch the
				**	"=foobar" and "foobar=" cases. "=foobar" lines are ignored, while
				** "foobar=" lines are stores has having " " as their value.
				*/
				*divider++ = '\0';
				strtrim(buffer);
				if (!strlen(buffer)) continue;

				strtrim(divider);
				if (!strlen(divider))
					divider = " ";


				INIEntry * entryptr = W3DNEW INIEntry(strdup(buffer), strdup(divider));
				if (entryptr == NULL) {
					delete secptr;
					Clear();
					return(false);
				}

				// 12/09/97 EHC - check to see if an entry with this ID already exists
				if (secptr->EntryIndex.Is_Present(entryptr->Index_ID())) {
					WWDEBUG_SAY(( "Duplicate Entry CRC Section \"%s\" Entry \"%s\"\n", secptr->Section, buffer ));
					DuplicateCRCError("INIClass::Load", buffer);
					return false;
				}

				secptr->EntryIndex.Add_Index(entryptr->Index_ID(), entryptr);
				secptr->EntryList.Add_Tail(entryptr);
			}

			/*
			**	All the entries for this section have been parsed. If this section is blank, then
			**	don't bother storing it.
			*/
			if (secptr->EntryList.Is_Empty()) {
				delete secptr;
			} else {
				SectionIndex->Add_Index(secptr->Index_ID(), secptr);
				SectionList->Add_Tail(secptr);
			}
		}
	}
	return(true);
}


/***********************************************************************************************
 * INIClass::Save -- Save the ini data to the file specified.                                  *
 *                                                                                             *
 *    Use this routine to save the ini data to the file specified. All existing data in the    *
 *    file, if it was present, is replaced.                                                    *
 *                                                                                             *
 * INPUT:   file  -- Reference to the file to write the INI data to.                           *
 *                                                                                             *
 * OUTPUT:  bool; Was the data written to the file?                                            *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/02/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
int INIClass::Save(FileClass & file) const
{
	FilePipe fp(file);
	return(Save(fp));
}


/***********************************************************************************************
 * INIClass::Save -- Save the ini data to the file specified.                                  *
 *                                                                                             *
 *    Use this routine to save the ini data to the file specified. All existing data in the    *
 *    file, if it was present, is replaced.                                                    *
 *                                                                                             *
 * INPUT:   filename  -- Filename to save to.                                                  *
 *                                                                                             *
 * OUTPUT:  bool; Was the data written to the file?                                            *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   01/22/2001 NAK : Created.                                                                 *
 *=============================================================================================*/
int INIClass::Save(const char *filename) const
{
	FileClass *file=_TheWritingFileFactory->Get_File(filename);
	int retval=0;
	if ( file ) {
		retval=Save(*file);
		_TheWritingFileFactory->Return_File(file);
	}
	file=NULL;

	return(retval);
}



/***********************************************************************************************
 * INIClass::Save -- Saves the INI data to a pipe stream.                                      *
 *                                                                                             *
 *    This routine will output the data of the INI file to a pipe stream.                      *
 *                                                                                             *
 * INPUT:   pipe  -- Reference to the pipe stream to pump the INI image to.                    *
 *                                                                                             *
 * OUTPUT:  Returns with the number of bytes output to the pipe.                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/02/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
int INIClass::Save(Pipe & pipe) const
{
	int total = 0;

	#ifdef _UNIX
		const char *EOL="\n";
	#else
		const char *EOL="\r\n";
	#endif

	INISection * secptr = SectionList->First();
	while (secptr && secptr->Is_Valid()) {

		/*
		**	Output the section identifier.
		*/
		total += pipe.Put("[", 1);
		total += pipe.Put(secptr->Section, strlen(secptr->Section));
		total += pipe.Put("]", 1);
		total += pipe.Put(EOL, strlen(EOL));

		/*
		**	Output all the entries and values in this section.
		*/
		INIEntry * entryptr = secptr->EntryList.First();
		while (entryptr && entryptr->Is_Valid()) {
			total += pipe.Put(entryptr->Entry, strlen(entryptr->Entry));
			total += pipe.Put("=", 1);
			total += pipe.Put(entryptr->Value, strlen(entryptr->Value));
			total += pipe.Put(EOL, strlen(EOL));

			entryptr = entryptr->Next();
		}

		/*
		**	After the last entry in this section, output an extra
		**	blank line for readability purposes.
		*/
		total += pipe.Put(EOL, strlen(EOL));

		secptr = secptr->Next();
	}
	total += pipe.End();

	return(total);
}


/***********************************************************************************************
 * INIClass::Find_Section -- Find the specified section within the INI data.                   *
 *                                                                                             *
 *    This routine will scan through the INI data looking for the section specified. If the    *
 *    section could be found, then a pointer to the section control data is returned.          *
 *                                                                                             *
 * INPUT:   section  -- The name of the section to search for. Don't enclose the name in       *
 *                      brackets. Case is NOT sensitive in the search.                         *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the INI section control structure if the section was     *
 *          found. Otherwise, NULL is returned.                                                *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/02/1996 JLB : Created.                                                                 *
 *   11/02/1996 JLB : Uses index manager.                                                      *
 *   12/08/1996 EHC : Uses member CRC function                                                 *
 *=============================================================================================*/
INISection * INIClass::Find_Section(char const * section) const
{
	if (section != NULL) {
//		long crc = CRCEngine()(section, strlen(section));
		long crc = CRC(section);

		if (SectionIndex->Is_Present(crc)) {
			return((*SectionIndex)[crc]);
		}
	}
	return(NULL);
}


/***********************************************************************************************
 * INIClass::Section_Count -- Counts the number of sections in the INI data.                   *
 *                                                                                             *
 *    This routine will scan through all the sections in the INI data and return a count       *
 *    of the number it found.                                                                  *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  Returns with the number of sections recorded in the INI data.                      *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/02/1996 JLB : Created.                                                                 *
 *   11/02/1996 JLB : Uses index manager.                                                      *
 *=============================================================================================*/
int INIClass::Section_Count(void) const
{
	return(SectionIndex->Count());
}


/***********************************************************************************************
 * INIClass::Entry_Count -- Fetches the number of entries in a specified section.              *
 *                                                                                             *
 *    This routine will examine the section specified and return with the number of entries    *
 *    associated with it.                                                                      *
 *                                                                                             *
 * INPUT:   section  -- Pointer to the section that will be examined.                          *
 *                                                                                             *
 * OUTPUT:  Returns with the number entries in the specified section.                          *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/02/1996 JLB : Created.                                                                 *
 *   11/02/1996 JLB : Uses index manager.                                                      *
 *=============================================================================================*/
int INIClass::Entry_Count(char const * section) const
{
	INISection * secptr = Find_Section(section);
	if (secptr != NULL) {
		return(secptr->EntryIndex.Count());
	}
	return(0);
}


/***********************************************************************************************
 * INIClass::Find_Entry -- Find specified entry within section.                                *
 *                                                                                             *
 *    This support routine will find the specified entry in the specified section. If found,   *
 *    a pointer to the entry control structure will be returned.                               *
 *                                                                                             *
 * INPUT:   section  -- Pointer to the section name to search under.                           *
 *                                                                                             *
 *          entry    -- Pointer to the entry name to search for.                               *
 *                                                                                             *
 * OUTPUT:  If the entry was found, then a pointer to the entry control structure will be      *
 *          returned. Otherwise, NULL will be returned.                                        *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/02/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
INIEntry * INIClass::Find_Entry(char const * section, char const * entry) const
{
	INISection * secptr = Find_Section(section);
	if (secptr != NULL) {
		return(secptr->Find_Entry(entry));
	}
	return(NULL);
}


/***********************************************************************************************
 * INIClass::Get_Entry -- Get the entry identifier name given ordinal number and section name. *
 *                                                                                             *
 *    This will return the identifier name for the entry under the section specified. The      *
 *    ordinal number specified is used to determine which entry to retrieve. The entry         *
 *    identifier is the text that appears to the left of the "=" character.                    *
 *                                                                                             *
 * INPUT:   section  -- The section to use.                                                    *
 *                                                                                             *
 *          index    -- The ordinal number to use when fetching an entry name.                 *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the entry name.                                          *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/02/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
char const * INIClass::Get_Entry(char const * section, int index) const
{
	INISection * secptr = Find_Section(section);

	if (secptr != NULL && index < secptr->EntryIndex.Count()) {
		INIEntry * entryptr = secptr->EntryList.First();

		while (entryptr != NULL && entryptr->Is_Valid()) {
			if (index == 0) return(entryptr->Entry);
			index--;
			entryptr = entryptr->Next();
		}
	}
	return(NULL);
}



/***********************************************************************************************
 * Enumerate_Entries -- Count how many entries begin with a certain prefix followed by a range * 
 *                      of numbers.                                                            *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   9/29/99    EHC : Created.                                                                 *
 *=============================================================================================*/
unsigned INIClass::Enumerate_Entries(const char *Section, const char * Entry_Prefix, unsigned StartNumber, unsigned EndNumber)
{
	unsigned count = StartNumber;
	bool present = false;
	char entry[256];
	do 
	{
		sprintf(entry, "%s%d", Entry_Prefix, count);
		present = Is_Present(Section, entry);
		if(present)
			count++;
	} while(present && (count < EndNumber));

	return (count - StartNumber);
}



/***********************************************************************************************
 * INIClass::Put_UUBlock -- Store a binary encoded data block into the INI database.           *
 *                                                                                             *
 *    Use this routine to store an arbitrary length binary block of data into the INI database.*
 *    This routine will covert the data into displayable form and then break it into lines     *
 *    that are stored in sequence to the section. A section used to store data in this         *
 *    fashion can not be used for any other entries.                                           *
 *                                                                                             *
 * INPUT:   section  -- The section identifier to place the data into.                         *
 *                                                                                             *
 *          block    -- Pointer to the block of binary data to store.                          *
 *                                                                                             *
 *          len      -- The length of the binary data.                                         *
 *                                                                                             *
 * OUTPUT:  bool; Was the data stored to the database?                                         *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/03/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool INIClass::Put_UUBlock(char const * section, void const * block, int len)
{
	if (section == NULL || block == NULL || len < 1) return(false);

	Clear(section);

	BufferStraw straw(block, len);
	Base64Straw bstraw(Base64Straw::ENCODE);
	bstraw.Get_From(straw);

	int counter = 1;

	for (;;) {
		char buffer[71];
		char sbuffer[32];

		int length = bstraw.Get(buffer, sizeof(buffer)-1);
		buffer[length] = '\0';
		if (length == 0) break;

		sprintf(sbuffer, "%d", counter);
		Put_String(section, sbuffer, buffer);
		counter++;
	}
	return(true);
}


/***********************************************************************************************
 * INIClass::Get_UUBlock -- Fetch an encoded block from the section specified.                 *
 *                                                                                             *
 *    This routine will take all the entries in the specified section and decompose them into  *
 *    a binary block of data that will be stored into the buffer specified. By using this      *
 *    routine [and the Put_UUBLock counterpart], arbitrary blocks of binary data may be        *
 *    stored in the INI file. A section processed by this routine can contain no other         *
 *    entries than those put there by a previous call to Put_UUBlock.                          *
 *                                                                                             *
 * INPUT:   section  -- The section name to process.                                           *
 *                                                                                             *
 *          block    -- Pointer to the buffer that will hold the retrieved data.               *
 *                                                                                             *
 *          len      -- The length of the buffer. The retrieved data will not fill past this   *
 *                      limit.                                                                 *
 *                                                                                             *
 * OUTPUT:  Returns with the number of bytes decoded into the buffer specified.                *
 *                                                                                             *
 * WARNINGS:   If the number of bytes retrieved exactly matches the length of the buffer       *
 *             specified, then you might have a condition of buffer "overflow".                *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/02/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
int INIClass::Get_UUBlock(char const * section, void * block, int len) const
{
	if (section == NULL) return(0);

	Base64Pipe b64pipe(Base64Pipe::DECODE);
	BufferPipe bpipe(block, len);

	b64pipe.Put_To(&bpipe);

	int total = 0;
	int counter = Entry_Count(section);
	for (int index = 0; index < counter; index++) {
		char buffer[128];

		int length = Get_String(section, Get_Entry(section, index), "=", buffer, sizeof(buffer));
		int outcount = b64pipe.Put(buffer, length);
		total += outcount;
	}
	total += b64pipe.End();
	return(total);
}


/***********************************************************************************************
 * INIClass::Put_TextBlock -- Stores a block of text into an INI section.                      *
 *                                                                                             *
 *    This routine will take an arbitrarily long block of text and store it into the INI       *
 *    database. The text is broken up into lines and each line is then stored as a numbered    *
 *    entry in the specified section. A section used to store text in this way can not be used *
 *    to hold any other entries. The text is presumed to contain space characters scattered    *
 *    throughout it and that one space between words and sentences is natural.                 *
 *                                                                                             *
 * INPUT:   section  -- The section to place the text block into.                              *
 *                                                                                             *
 *          text     -- Pointer to a null terminated text string that holds the block of       *
 *                      text. The length can be arbitrary.                                     *
 *                                                                                             *
 * OUTPUT:  bool; Was the text block placed into the database?                                 *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/03/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool INIClass::Put_TextBlock(char const * section, char const * text)
{
	if (section == NULL) return(false);

	Clear(section);

	int index = 1;
	while (text != NULL && *text != 0) {

		char buffer[128];

		strncpy(buffer, text, 75);
		buffer[75] = '\0';

		char b[32];
		sprintf(b, "%d", index);

		/*
		**	Scan backward looking for a good break position.
		*/
		int count = strlen(buffer);
		if (count > 0) {
			if (count >= 75) {
				while (count) {
					char c = buffer[count];

					if (isspace(c)) break;
					count--;
				}

				if (count == 0) {
					break;
				} else {
					buffer[count] = '\0';
				}
			}

			strtrim(buffer);
			Put_String(section, b, buffer);
			index++;
			text = ((char  *)text) + count;
		} else {
			break;
		}
	}
	return(true);
}


/***********************************************************************************************
 * INIClass::Get_TextBlock -- Fetch a block of normal text.                                    *
 *                                                                                             *
 *    This will take all entries in the specified section and format them into a block of      *
 *    normalized text. That is, text with single spaces between each concatenated line. All    *
 *    entries in the specified section are processed by this routine. Use Put_TextBlock to     *
 *    build the entries in the section.                                                        *
 *                                                                                             *
 * INPUT:   section  -- The section name to process.                                           *
 *                                                                                             *
 *          buffer   -- Pointer to the buffer that will hold the complete text.                *
 *                                                                                             *
 *          len      -- The length of the buffer specified. The text will, at most, fill this  *
 *                      buffer with the last character being forced to null.                   *
 *                                                                                             *
 * OUTPUT:  Returns with the number of characters placed into the buffer. The trailing null    *
 *          is not counted.                                                                    *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/02/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
int INIClass::Get_TextBlock(char const * section, char * buffer, int len) const
{
	if (len <= 0) return(0);

	buffer[0] = '\0';
	if (len <= 1) return(0);

	int elen = Entry_Count(section);
	int total = 0;
	for (int index = 0; index < elen; index++) {

		/*
		**	Add spacers between lines of fetched text.
		*/
		if (index > 0) {
			*buffer++ = ' ';
			len--;
			total++;
		}

		Get_String(section, Get_Entry(section, index), "", buffer, len);

		int partial = strlen(buffer);
		total += partial;
		buffer += partial;
		len -= partial;
		if (len <= 1) break;
	}
	return(total);
}


/***********************************************************************************************
 * INIClass::Put_Int -- Stores a signed integer into the INI data base.                        *
 *                                                                                             *
 *    Use this routine to store an integer value into the section and entry specified.         *
 *                                                                                             *
 * INPUT:   section  -- The identifier for the section that the entry will be placed in.       *
 *                                                                                             *
 *          entry    -- The entry identifier used for the integer number.                      *
 *                                                                                             *
 *          number   -- The integer number to store in the database.                           *
 *                                                                                             *
 *          format   -- The format to store the integer. The format is generally only a        *
 *                      cosmetic affect. The Get_Int operation will interpret the value the    *
 *                      same regardless of what format was used to store the integer.          *
 *                                                                                             *
 *                      0  : plain decimal digit                                               *
 *                      1  : hexadecimal digit (trailing "h")                                  *
 *                      2  : hexadecimal digit (leading "$")                                   *
 *                                                                                             *
 * OUTPUT:  bool; Was the number stored?                                                       *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/03/1996 JLB : Created.                                                                 *
 *   07/10/1996 JLB : Handles multiple integer formats.                                        *
 *=============================================================================================*/
bool INIClass::Put_Int(char const * section, char const * entry, int number, int format)
{
	char buffer[MAX_LINE_LENGTH];

	switch (format) {
		default:
		case 0:
			sprintf(buffer, "%d", number);
			break;

		case 1:
			sprintf(buffer, "%Xh", number);
			break;

		case 2:
			sprintf(buffer, "$%X", number);
			break;
	}
	return(Put_String(section, entry, buffer));
}


/***********************************************************************************************
 * INIClass::Get_Int -- Fetch an integer entry from the specified section.                     *
 *                                                                                             *
 *    This routine will fetch an integer value from the entry and section specified. If no     *
 *    entry could be found, then the default value will be returned instead.                   *
 *                                                                                             *
 * INPUT:   section  -- The section name to search under.                                      *
 *                                                                                             *
 *          entry    -- The entry name to search for.                                          *
 *                                                                                             *
 *          defvalue -- The default value to use if the specified entry could not be found.    *
 *                                                                                             *
 * OUTPUT:  Returns with the integer value specified in the INI database or else returns the   *
 *          default value.                                                                     *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/02/1996 JLB : Created.                                                                 *
 *   07/10/1996 JLB : Handles multiple integer formats.                                        *
 *=============================================================================================*/
int INIClass::Get_Int(char const * section, char const * entry, int defvalue) const
{
	/*
	**	Verify that the parameters are nominally correct.
	*/
	if (section == NULL || entry == NULL) return(defvalue);

	INIEntry * entryptr = Find_Entry(section, entry);
	if (entryptr && entryptr->Value != NULL) {

		if (*entryptr->Value == '$') {
			sscanf(entryptr->Value, "$%x", &defvalue);
		} else {
			if (tolower(entryptr->Value[strlen(entryptr->Value)-1]) == 'h') {
				sscanf(entryptr->Value, "%xh", &defvalue);
			} else {
				defvalue = atoi(entryptr->Value);
			}
		}
	}
	return(defvalue);
}



/***********************************************************************************************
 * INIClass::Put_Rect -- Store a rectangle  into the INI database.                             *
 *                                                                                             *
 *    This routine will store the four values that constitute the specified rectangle into     *
 *    the database under the section and entry specified.                                      *
 *                                                                                             *
 * INPUT:   section  -- Name of the section to place the entry under.                          *
 *                                                                                             *
 *          entry    -- Name of the entry that the rectangle data will be stored to.           *
 *                                                                                             *
 *          value    -- The rectangle value to store.                                          *
 *                                                                                             *
 * OUTPUT:  bool; Was the rectangle data written to the database?                              *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/19/1997 JLB : Created.                                                                 *
 *=============================================================================================*/
bool INIClass::Put_Rect(char const * section, char const * entry, Rect const & value)
{
	char buffer[64];

	sprintf(buffer, "%d,%d,%d,%d", value.X, value.Y, value.Width, value.Height);
	return(Put_String(section, entry, buffer));
}	


/***********************************************************************************************
 * INIClass::Get_Rect -- Retrieve a rectangle data from the database.                          *
 *                                                                                             *
 *    This routine will retrieve the rectangle data from the database at the section and entry *
 *    specified.                                                                               *
 *                                                                                             *
 * INPUT:   section  -- The name of the section that the entry will be scanned for.            *
 *                                                                                             *
 *          entry    -- The entry that the rectangle data will be lifted from.                 *
 *                                                                                             *
 *          defvalue -- The rectangle value to return if the specified section and entry could *
 *                      not be found.                                                          *
 *                                                                                             *
 * OUTPUT:  Returns with the rectangle data from the database or the default value if not      *
 *          found.                                                                             *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/19/1997 JLB : Created.                                                                 *
 *=============================================================================================*/
Rect const INIClass::Get_Rect(char const * section, char const * entry, Rect const & defvalue) const
{
	char buffer[64];

	if (Get_String(section, entry, "0,0,0,0", buffer, sizeof(buffer))) {
		Rect retval = defvalue;
		sscanf(buffer, "%d,%d,%d,%d", &retval.X, &retval.Y, &retval.Width, &retval.Height);
		return(retval);
	}
	return(defvalue);
}	


/***********************************************************************************************
 * INIClass::Put_Hex -- Store an integer into the INI database, but use a hex format.          *
 *                                                                                             *
 *    This routine is similar to the Put_Int routine, but the number is stored as a hexadecimal*
 *    number.                                                                                  *
 *                                                                                             *
 * INPUT:   section  -- The identifier for the section that the entry will be placed in.       *
 *                                                                                             *
 *          entry    -- The entry identifier to tag to the integer number specified.           *
 *                                                                                             *
 *          number   -- The number to assign the the specified entry and placed in the         *
 *                      specified section.                                                     *
 *                                                                                             *
 * OUTPUT:  bool; Was the number placed into the INI database?                                 *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/03/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool INIClass::Put_Hex(char const * section, char const * entry, int number)
{
	char buffer[MAX_LINE_LENGTH];

	sprintf(buffer, "%X", number);
	return(Put_String(section, entry, buffer));
}


/***********************************************************************************************
 * INIClass::Get_Hex -- Fetches integer [hex format] from the section and entry specified.     *
 *                                                                                             *
 *    This routine will search under the section specified, looking for a matching entry. The  *
 *    value is interpreted as a hexadecimal number and then returned. If no entry could be     *
 *    found, then the default value is returned instead.                                       *
 *                                                                                             *
 * INPUT:   section  -- The section identifier to search under.                                *
 *                                                                                             *
 *          entry    -- The entry identifier to search for.                                    *
 *                                                                                             *
 *          defvalue -- The default value to use if the entry could not be located.            *
 *                                                                                             *
 * OUTPUT:  Returns with the integer value from the specified section and entry. If no entry   *
 *          could be found, then the default value will be returned instead.                   *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/02/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
int INIClass::Get_Hex(char const * section, char const * entry, int defvalue) const
{
	/*
	**	Verify that the parameters are nominally correct.
	*/
	if (section == NULL || entry == NULL) return(defvalue);

	INIEntry * entryptr = Find_Entry(section, entry);
	if (entryptr && entryptr->Value != NULL) {
		sscanf(entryptr->Value, "%x", &defvalue);
	}
	return(defvalue);
}


/*********************************************************************************************** 
 * INIClass::Get_Float -- Fetch a floating point number from the database.                     * 
 *                                                                                             * 
 *    This routine will retrieve a floating point number from the database.                    * 
 *                                                                                             * 
 * INPUT:   section  -- The section name to find the entry under.                              * 
 *                                                                                             * 
 *          entry    -- The entry name to fetch the float value from.                          * 
 *                                                                                             * 
 *          defvalue -- Return value to use if the section and entry could not be found.       * 
 *                                                                                             * 
 * OUTPUT:  Returns with the float value from the section and entry specified. If not found,   * 
 *          then the default value is returned.                                                * 
 *                                                                                             * 
 * WARNINGS:   none                                                                            * 
 *                                                                                             * 
 * HISTORY:                                                                                    * 
 *   05/31/1997 JLB : Created.                                                                 * 
 *=============================================================================================*/
float INIClass::Get_Float(char const * section, char const * entry, float defvalue) const
{
	/*
	**	Verify that the parameters are nominally correct.
	*/
	if (section == NULL || entry == NULL) return(defvalue);

	INIEntry * entryptr = Find_Entry(section, entry);
	if (entryptr != NULL && entryptr->Value != NULL) {
		float val;
		sscanf(entryptr->Value, "%f", &val);
		defvalue = val;
		if (strchr(entryptr->Value, '%') != NULL) {
			defvalue /= 100.0f;
		}
	}
	return(defvalue);
}


/*********************************************************************************************** 
 * INIClass::Put_Float -- Store a floating point number to the database.                       * 
 *                                                                                             * 
 *    This routine will store a flaoting point number to the section and entry of the          * 
 *    database.                                                                                * 
 *                                                                                             * 
 * INPUT:   section  -- The section to store the entry under.                                  * 
 *                                                                                             * 
 *          entry    -- The entry to store the floating point number to.                       * 
 *                                                                                             * 
 *          number   -- The floating point number to store.                                    * 
 *                                                                                             * 
 * OUTPUT:  bool; Was the floating point number stored without error?                          * 
 *                                                                                             * 
 * WARNINGS:   none                                                                            * 
 *                                                                                             * 
 * HISTORY:                                                                                    * 
 *   05/31/1997 JLB : Created.                                                                 * 
 *=============================================================================================*/
bool INIClass::Put_Float(char const * section, char const * entry, double number)
{
	char buffer[MAX_LINE_LENGTH];

	sprintf(buffer, "%f", (float)number);
	return(Put_String(section, entry, buffer));
}


/***********************************************************************************************
 * INIClass::Put_String -- Output a string to the section and entry specified.                 *
 *                                                                                             *
 *    This routine will put an arbitrary string to the section and entry specified. Any        *
 *    previous matching entry will be replaced.                                                *
 *                                                                                             *
 * INPUT:   section  -- The section identifier to place the string under.                      *
 *                                                                                             *
 *          entry    -- The entry identifier to identify this string [placed under the section]*
 *                                                                                             *
 *          string   -- Pointer to the string to assign to this entry.                         *
 *                                                                                             *
 * OUTPUT:  bool; Was the entry assigned without error?                                        *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/02/1996 JLB : Created.                                                                 *
 *   11/02/1996 JLB : Uses index handler.                                                      *
 *   12/08/1997 EHC : Debug message for duplicate entries                                      *
 *   03/13/1998 NH  : On duplicate CRC, check if strings identical.                            *
 *=============================================================================================*/
bool INIClass::Put_String(char const * section, char const * entry, char const * string)
{
	if (section == NULL || entry == NULL) return(false);

	INISection * secptr = Find_Section(section);

	if (secptr == NULL) {
		secptr = W3DNEW INISection(strdup(section));
		if (secptr == NULL) return(false);
		SectionList->Add_Tail(secptr);
		SectionIndex->Add_Index(secptr->Index_ID(), secptr);
	}

	/*
	**	Remove the old entry if found and print debug message
	*/
	INIEntry * entryptr = secptr->Find_Entry(entry);
	if (entryptr != NULL) {
      if (strcmp(entryptr->Entry, entry)) {
         DuplicateCRCError("INIClass::Put_String", entry);
      } else {
   		OutputDebugString("INIClass::Put_String - Duplicate Entry \"");
	   	OutputDebugString(entry);
		   OutputDebugString("\"\n");
      }
   	secptr->EntryIndex.Remove_Index(entryptr->Index_ID());
	   delete entryptr;
	}

	/*
	**	Create and add the new entry.
	*/
	if (string != NULL && strlen(string) > 0) {
		entryptr = W3DNEW INIEntry(strdup(entry), strdup(string));

		if (entryptr == NULL) {
			return(false);
		}
		secptr->EntryList.Add_Tail(entryptr);
		secptr->EntryIndex.Add_Index(entryptr->Index_ID(), entryptr);
	}
	return(true);
}


/***********************************************************************************************
 * INIClass::Get_String -- Fetch the value of a particular entry in a specified section.       *
 *                                                                                             *
 *    This will retrieve the entire text to the right of the "=" character. The text is        *
 *    found by finding a matching entry in the section specified. If no matching entry could   *
 *    be found, then the default value will be stored in the output string buffer.             *
 *                                                                                             *
 * INPUT:   section  -- Pointer to the section name to search under.                           *
 *                                                                                             *
 *          entry    -- The entry identifier to search for.                                    *
 *                                                                                             *
 *          defvalue -- If no entry could be found, then this text will be returned.           *
 *                                                                                             *
 *          buffer   -- Output buffer to store the retrieved string into.                      *
 *                                                                                             *
 *          size     -- The size of the output buffer. The maximum string length that could    *
 *                      be retrieved will be one less than this length. This is due to the     *
 *                      forced trailing zero added to the end of the string.                   *
 *                                                                                             *
 * OUTPUT:  Returns with the length of the string retrieved.                                   *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/02/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
int INIClass::Get_String(char const * section, char const * entry, char const * defvalue, char * buffer, int size) const
{
	/*
	**	Verify that the parameters are nominally legal.
	*/
//	if (buffer != NULL && size > 0) {
//		buffer[0] = '\0';
//	}
	if (buffer == NULL || size < 2 || section == NULL || entry == NULL) return(0);

	/*
	**	Fetch the entry string if it is present. If not, then the normal default
	**	value will be used as the entry value.
	*/
	INIEntry * entryptr = Find_Entry(section, entry);
	if (entryptr != NULL && entryptr->Value != NULL) {
		defvalue = entryptr->Value;
	}

	/*
	**	Fill in the buffer with the entry value and return with the length of the string.
	*/
	if (defvalue == NULL) {
		buffer[0] = '\0';
		return(0);
	} else {
		strncpy(buffer, defvalue, size);
		buffer[size-1] = '\0';
		strtrim(buffer);
		return(strlen(buffer));
	}
}


/*
** GetString
*/
StringClass INIClass::Get_String(char const * section, char const * entry, char const * defvalue) const
{
	if (section == NULL || entry == NULL) return StringClass();

	/*
	**	Fetch the entry string if it is present. If not, then the normal default
	**	value will be used as the entry value.
	*/
	INIEntry * entryptr = Find_Entry(section, entry);
	if (entryptr != NULL) { 
		defvalue = entryptr->Value;
	}

	if (defvalue == NULL) return StringClass();
	return StringClass( defvalue );
}



/***********************************************************************************************
 * INIClass::Get_String -- Fetch the value of a particular entry in a specified section.       *
 *                                                                                             *
 *    This will retrieve the entire text to the right of the "=" character. The text is        *
 *    found by finding a matching entry in the section specified. If no matching entry could   *
 *    be found, then the default value will be stored in the output string buffer.             *
 *                                                                                             *
 * INPUT:   section  -- Pointer to the section name to search under.                           *
 *                                                                                             *
 *          entry    -- The entry identifier to search for.                                    *
 *                                                                                             *
 *          defvalue -- If no entry could be found, then this text will be returned.           *
 *                                                                                             *
 *          buffer   -- Output buffer to store the retrieved string into.                      *
 *                                                                                             *
 *          size     -- The size of the output buffer. The maximum string length that could    *
 *                      be retrieved will be one less than this length. This is due to the     *
 *                      forced trailing zero added to the end of the string.                   *
 *                                                                                             *
 * OUTPUT:  Returns with the length of the string retrieved.                                   *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/02/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
char *INIClass::Get_Alloc_String(char const * section, char const * entry, char const * defvalue) const
{
	if (section == NULL || entry == NULL) return(NULL);

	/*
	**	Fetch the entry string if it is present. If not, then the normal default
	**	value will be used as the entry value.
	*/
	INIEntry * entryptr = Find_Entry(section, entry);
	if (entryptr != NULL) { 
		defvalue = entryptr->Value;
	}

	if (defvalue == NULL) return NULL;
	return(strdup(defvalue));
}

int INIClass::Get_List_Index(char const * section, char const * entry, int const defvalue, char *list[])
{
	if (section == NULL || entry == NULL) return(0);

	INIEntry * entryptr = Find_Entry(section, entry);
	if (entryptr == NULL || entryptr->Value == NULL) { 
		return defvalue;
	}

	for (int lp = 0; list[lp]; lp++) {
		if (strcasecmp(entryptr->Value, list[lp]) == 0) {
			return lp;
		}
		assert(lp < 1000);
	}
	return defvalue;
}
int INIClass::Get_Int_Bitfield(char const * section, char const * entry, int defvalue, char *list[])
{
	// if we can't find the entry or the entry is null just return the default value
	INIEntry * entryptr = Find_Entry(section, entry);
	if (entryptr == NULL || entryptr->Value == NULL) { 
		return defvalue;
	}

	// swim through the entry breaking it down into its token pieces and 
	// get the bitfield value for each piece.
	// int count	= 0; (gth) initailized but not referenced...
	int retval	= 0;
	char *str	= strdup(entryptr->Value);

   int lp; 
	for (char *token = strtok(str, "|+"); token; token = strtok(NULL, "|+")) {
		for (lp = 0; list[lp]; lp++) {
			// if this list entry matches our string token then we need
			// to set this bit.
			if (strcasecmp(token, list[lp]) == 0) {
				retval |= (1 << lp);
				break;
			}
		}
		// if we reached the end of the list and found nothing then we need
		// to assert since we have an unidentified value
		if (list[lp] == NULL) assert(lp < 1000);
	}
	free(str);
	return retval;
}

int *	INIClass::Get_Alloc_Int_Array(char const * section, char const * entry, int listend)
{
	int *retval = NULL;

	INIEntry * entryptr = Find_Entry(section, entry);
	if (entryptr == NULL || entryptr->Value == NULL) { 
		retval = W3DNEWARRAY int[1];
		retval[0] = listend;

		return retval;
	}

	// count all the tokens in the string.  Each token should represent an
	// integer number.
	int count = 0;
	char *str = strdup(entryptr->Value);
	char *token;
	for (token = strtok(str, " "); token; token = strtok(NULL, " ")) {
		count++;
	}
	free(str);

	// now that we know how many tokens there are in the string, allocate a int
	// array to hold the tokens and parse out the actual values.
	retval	= W3DNEWARRAY int[count+1];
	count		= 0;
	str		= strdup(entryptr->Value);
	for (token = strtok(str, " "); token; token = strtok(NULL, " ")) {
		retval[count] = atoi(token);
		count++;
	}
	free(str);

	// arrays of integers are terminated with the listend variable passed in
	retval[count] = listend;

	// now that we have the allocated array with the results filled in lets return
	// the results.
	return retval;
}

/***********************************************************************************************
 * INIClass::Put_Bool -- Store a boolean value into the INI database.                          *
 *                                                                                             *
 *    Use this routine to place a boolean value into the INI database. The boolean value will  *
 *    be stored as "yes" or "no".                                                              *
 *                                                                                             *
 * INPUT:   section  -- The section to place the entry and boolean value into.                 *
 *                                                                                             *
 *          entry    -- The entry identifier to tag to the boolean value.                      *
 *                                                                                             *
 *          value    -- The boolean value to place into the database.                          *
 *                                                                                             *
 * OUTPUT:  bool; Was the boolean value placed into the database?                              *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/03/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool INIClass::Put_Bool(char const * section, char const * entry, bool value)
{
	if (value) {
		return(Put_String(section, entry, "yes"));
	} else {
		return(Put_String(section, entry, "no"));
	}
}


/***********************************************************************************************
 * INIClass::Get_Bool -- Fetch a boolean value for the section and entry specified.            *
 *                                                                                             *
 *    This routine will search under the section specified, looking for a matching entry. If   *
 *    one is found, the value is interpreted as a boolean value and then returned. In the case *
 *    of no matching entry, the default value will be returned instead. The boolean value      *
 *    is interpreted using the standard boolean conventions. e.g., "Yes", "Y", "1", "True",    *
 *    "T" are all consider to be a TRUE boolean value.                                         *
 *                                                                                             *
 * INPUT:   section  -- The section to search under.                                           *
 *                                                                                             *
 *          entry    -- The entry to search for.                                               *
 *                                                                                             *
 *          defvalue -- The default value to use if no matching entry could be located.        *
 *                                                                                             *
 * OUTPUT:  Returns with the boolean value of the specified section and entry. If no match     *
 *          then the default boolean value is returned.                                        *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/02/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool INIClass::Get_Bool(char const * section, char const * entry, bool defvalue) const
{
	/*
	**	Verify that the parameters are nominally correct.
	*/
	if (section == NULL || entry == NULL) return(defvalue);

	INIEntry * entryptr = Find_Entry(section, entry);
	if (entryptr && entryptr->Value != NULL) {
		switch (toupper(*entryptr->Value)) {
			case 'Y':
			case 'T':
			case '1':
				return(true);

			case 'N':
			case 'F':
			case '0':
				return(false);
		}
	}
	return(defvalue);
}


/***********************************************************************************************
 * INIClass::Put_Point -- Store a point value to the database.                                 *
 *                                                                                             *
 *    This routine will store the point value to the INI database under the section and entry  *
 *    specified.                                                                               *
 *                                                                                             *
 * INPUT:   section  -- The name of the section to store the entry under.                      *
 *                                                                                             *
 *          entry    -- The entry to store the point data to.                                  *
 *                                                                                             *
 *          value    -- The point value to store.                                              *
 *                                                                                             *
 * OUTPUT:  bool; Was the point value stored to the database?                                  *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/19/1997 JLB : Created.                                                                 *
 *=============================================================================================*/
bool INIClass::Put_Point(char const * section, char const * entry, TPoint2D<int> const & value)
{
	char buffer[54];
	sprintf(buffer, "%d,%d", value.X, value.Y);
	return(Put_String(section, entry, buffer));
}


/***********************************************************************************************
 * INIClass::Get_Point -- Fetch a point value from the INI database.                           *
 *                                                                                             *
 *    This routine will retrieve a point value from the database by looking in the section and *
 *    entry specified.                                                                         *
 *                                                                                             *
 * INPUT:   section  -- The name of the section to search for the entry under.                 *
 *                                                                                             *
 *          entry    -- The entry to search for.                                               *
 *                                                                                             *
 *          defvalue -- The default value to return if the section and entry were not found.   *
 *                                                                                             *
 * OUTPUT:  Returns with the point value retrieved from the database or the default value if   *
 *          the section and entry were not found.                                              *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/19/1997 JLB : Created.                                                                 *
 *=============================================================================================*/
TPoint2D<int> const INIClass::Get_Point(char const * section, char const * entry, TPoint2D<int> const & defvalue) const
{
	char buffer[64];
	if (Get_String(section, entry, "", buffer, sizeof(buffer))) {
		int x,y;
		sscanf(buffer, "%d,%d", &x, &y);
		return(TPoint2D<int>(x, y));
	}
	return(defvalue);
}


/***********************************************************************************************
 * INIClass::Put_Point -- Stores a 3D point to the database.                                   *
 *                                                                                             *
 *    This routine will store the 3D point value to the database under the section and entry   *
 *    specified.                                                                               *
 *                                                                                             *
 * INPUT:   section  -- The name of the section that the entry will be stored under.           *
 *                                                                                             *
 *          entry    -- The name of the entry that the point will be stored to.                *
 *                                                                                             *
 *          value    -- The 3D point value to store.                                           *
 *                                                                                             *
 * OUTPUT:  bool; Was the point stored to the database?                                        *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/19/1997 JLB : Created.                                                                 *
 *=============================================================================================*/
bool INIClass::Put_Point(char const * section, char const * entry, TPoint3D<int> const & value)
{
	char buffer[54];
	sprintf(buffer, "%d,%d,%d", value.X, value.Y, value.Z);
	return(Put_String(section, entry, buffer));
}


/***********************************************************************************************
 * INIClass::Get_Point -- Fetch a 3D point from the database.                                  *
 *                                                                                             *
 *    This routine will retrieve a 3D point from the database from the section and entry       *
 *    specified.                                                                               *
 *                                                                                             *
 * INPUT:   section  -- The name of the section to search for th entry under.                  *
 *                                                                                             *
 *          entry    -- The name of the entry to search for.                                   *
 *                                                                                             *
 *          defvaule -- The default value to return if the section and entry could not be      *
 *                      found.                                                                 *
 *                                                                                             *
 * OUTPUT:  Returns with the 3D point from the database or the default value if the section    *
 *          and entry could not be found.                                                      *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/19/1997 JLB : Created.                                                                 *
 *=============================================================================================*/
TPoint3D<int> const INIClass::Get_Point(char const * section, char const * entry, TPoint3D<int> const & defvalue) const
{
	char buffer[64];
	if (Get_String(section, entry, "", buffer, sizeof(buffer))) {
		int x,y,z;
		sscanf(buffer, "%d,%d,%d", &x, &y, &z);
		return(TPoint3D<int>(x, y, z));
	}
	return(defvalue);
}


/***********************************************************************************************
 * INIClass::Put_Point -- Stores a 3D point to the database.                                   *
 *                                                                                             *
 *    This routine will store the 3D point value to the database under the section and entry   *
 *    specified.                                                                               *
 *                                                                                             *
 * INPUT:   section  -- The name of the section that the entry will be stored under.           *
 *                                                                                             *
 *          entry    -- The name of the entry that the point will be stored to.                *
 *                                                                                             *
 *          value    -- The 3D point value to store.                                           *
 *                                                                                             *
 * OUTPUT:  bool; Was the point stored to the database?                                        *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/19/1997 JLB : Created.                                                                 *
 *=============================================================================================*/
bool INIClass::Put_Point(char const * section, char const * entry, TPoint3D<float> const & value)
{
	char buffer[54];
	sprintf(buffer, "%f,%f,%f", (float)value.X, (float)value.Y, (float)value.Z);
	return(Put_String(section, entry, buffer));
}


/***********************************************************************************************
 * INIClass::Get_Point -- Fetch a 3D point from the database.                                  *
 *                                                                                             *
 *    This routine will retrieve a 3D point from the database from the section and entry       *
 *    specified.                                                                               *
 *                                                                                             *
 * INPUT:   section  -- The name of the section to search for th entry under.                  *
 *                                                                                             *
 *          entry    -- The name of the entry to search for.                                   *
 *                                                                                             *
 *          defvaule -- The default value to return if the section and entry could not be      *
 *                      found.                                                                 *
 *                                                                                             *
 * OUTPUT:  Returns with the 3D point from the database or the default value if the section    *
 *          and entry could not be found.                                                      *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/19/1997 JLB : Created.                                                                 *
 *=============================================================================================*/
TPoint3D<float> const INIClass::Get_Point(char const * section, char const * entry, TPoint3D<float> const & defvalue) const
{
	char buffer[64];
	if (Get_String(section, entry, "", buffer, sizeof(buffer))) {
		float x,y,z;
		sscanf(buffer, "%f,%f,%f", &x, &y, &z);
		return(TPoint3D<float>(x, y, z));
	}
	return(defvalue);
}


/***********************************************************************************************
 * INIClass::Get_Point -- Fetch a point value from the INI database.                           *
 *                                                                                             *
 *    This routine will retrieve a point value from the database by looking in the section and *
 *    entry specified.                                                                         *
 *                                                                                             *
 * INPUT:   section  -- The name of the section to search for the entry under.                 *
 *                                                                                             *
 *          entry    -- The entry to search for.                                               *
 *                                                                                             *
 *          defvalue -- The default value to return if the section and entry were not found.   *
 *                                                                                             *
 * OUTPUT:  Returns with the point value retrieved from the database or the default value if   *
 *          the section and entry were not found.                                              *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/14/1999 NH : Created.                                                                  *
 *=============================================================================================*/
TPoint2D<float> const INIClass::Get_Point(char const * section, char const * entry, TPoint2D<float> const & defvalue) const
{
	char buffer[64];
	if (Get_String(section, entry, "", buffer, sizeof(buffer))) {
		float x,y;
		sscanf(buffer, "%f,%f", &x, &y);
		return(TPoint2D<float>(x, y));
	}
	return(defvalue);
}


/***********************************************************************************************
 * INISection::Find_Entry -- Finds a specified entry and returns pointer to it.					  *
 *                                                                                             *
 *    This routine scans the supplied entry for the section specified. This is used for        *
 *    internal database maintenance.                                                           *
 *                                                                                             *
 * INPUT:   entry -- The entry to scan for.                                                    *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the entry control structure if the entry was found.      *
 *          Otherwise it returns NULL.                                                         *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/03/1996 JLB : Created.                                                                 *
 *   11/02/1996 JLB : Uses index handler.                                                      *
 *   12/08/1997 EHC : Uses member CRC function
 *=============================================================================================*/
INIEntry * INISection::Find_Entry(char const * entry) const
{
	if (entry != NULL) {
//		int crc = CRCEngine()(entry, strlen(entry));
		int crc = CRC::String(entry);
		if (EntryIndex.Is_Present(crc)) {
			return(EntryIndex[crc]);
		}
	}
	return(NULL);
}


/***********************************************************************************************
 * INIClass::Put_PKey -- Stores the key to the INI database.                                   *
 *                                                                                             *
 *    The key stored to the database will have both the exponent and modulus portions saved.   *
 *    Since the fast key only requires the modulus, it is only necessary to save the slow      *
 *    key to the database. However, storing the slow key stores the information necessary to   *
 *    generate the fast and slow keys. Because public key encryption requires one key to be    *
 *    completely secure, only store the fast key in situations where the INI database will     *
 *    be made public.                                                                          *
 *                                                                                             *
 * INPUT:   key   -- The key to store the INI database.                                        *
 *                                                                                             *
 * OUTPUT:  bool; Was the key stored to the database?                                          *
 *                                                                                             *
 * WARNINGS:   Store the fast key for public INI database availability. Store the slow key if  *
 *             the INI database is secure.                                                     *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/08/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool INIClass::Put_PKey(PKey const & key)
{
	char buffer[512];

	int len = key.Encode_Modulus(buffer);
	Put_UUBlock("PublicKey", buffer, len);

	len = key.Encode_Exponent(buffer);
	Put_UUBlock("PrivateKey", buffer, len);
	return(true);
}


/***********************************************************************************************
 * INIClass::Get_PKey -- Fetch a key from the ini database.                                    *
 *                                                                                             *
 *    This routine will fetch the key from the INI database. The key fetched is controlled by  *
 *    the parameter. There are two choices of key -- the fast or slow key.                     *
 *                                                                                             *
 * INPUT:   fast  -- Should the fast key be retrieved? The fast key has the advantage of       *
 *                   requiring only the modulus value.                                         *
 *                                                                                             *
 * OUTPUT:  Returns with the key retrieved.                                                    *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/08/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
PKey INIClass::Get_PKey(bool fast) const
{
	PKey key;
	char buffer[512];

	/*
	**	When retrieving the fast key, the exponent is a known constant. Don't parse the
	**	exponent from the database.
	*/
	if (fast) {
		BigInt exp = PKey::Fast_Exponent();
		exp.DEREncode((unsigned char *)buffer);
		key.Decode_Exponent(buffer);
	} else {
		Get_UUBlock("PrivateKey", buffer, sizeof(buffer));
		key.Decode_Exponent(buffer);
	}

	Get_UUBlock("PublicKey", buffer, sizeof(buffer));
	key.Decode_Modulus(buffer);

	return(key);
}


/***********************************************************************************************
 * INIClass::Strip_Comments -- Strips comments of the specified text line.                     *
 *                                                                                             *
 *    This routine will scan the string (text line) supplied and if any comment portions are   *
 *    found, they will be trimmed off. Leading and trailing blanks are also removed.           *
 *                                                                                             *
 * INPUT:   buffer   -- Pointer to the null terminate string to be processed.                  *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/03/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
void INIClass::Strip_Comments(char * buffer)
{
	if (buffer != NULL) {
		char * comment = strchr(buffer, ';');
		if (comment) {
			*comment = '\0';
			strtrim(buffer);
		}
	}
}


/***********************************************************************************************
 *  INIClass::CRC - returns a (hopefully) unique 32-bit value for a string                     *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:  pointer to null terminated string                                                   *
 *                                                                                             *
 * OUTPUT: integer that is highly likely to be unique for a given INI file.                    *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   12/8/97    EHC : Created.                                                                 *
 *=============================================================================================*/
int INIClass::CRC(const char *string) 
{
	// simply call the CRC class string evaluator.
	return CRC::String(string);
}


/***********************************************************************************************
 *  -- Displays debug information when a duplicate entry is found in an INI file               *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT: message - text description of function with problem                                  *
 *        entry - text buffer for duplicate entry name                                         *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS: will assert(0) and exit program. INI errors are considered fatal at this time.    *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   12/9/97    EHC : Created.                                                                 *
 *=============================================================================================*/
void INIClass::DuplicateCRCError(const char *message, const char *entry) 
{
	OutputDebugString(message);
	OutputDebugString(" - Duplicate Entry CRC \"");
	OutputDebugString(entry);
	OutputDebugString("\"\n");
	assert(0);
//	MessageBox(0, "Duplicate entry CRC in INI file.", message, MB_OK);
}
