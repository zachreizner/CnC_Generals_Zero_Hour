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

// FILE: Image.cpp ////////////////////////////////////////////////////////////////////////////////
// Created:   Colin Day, June 2001
// Desc:      High level representation of images, this is currently being
//						written so we have a way to refer to images in the windows
//						GUI, this system should be replaced with something that can
//						handle real image management or written to accomodate 
//						all parts of the engine that need images.
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#define DEFINE_IMAGE_STATUS_NAMES
#include "Lib/BaseType.h"
#include "Common/Debug.h"
#include "Common/INI.h"
#include "Common/GlobalData.h"
#include "GameClient/Image.h"


// PRIVATE DATA ///////////////////////////////////////////////////////////////////////////////////
const FieldParse Image::m_imageFieldParseTable[] = 
{

	{ "Texture",				INI::parseAsciiString,							NULL, 		offsetof( Image, m_filename ) },
	{ "TextureWidth",		INI::parseInt,											NULL, 		offsetof( Image, m_textureSize.x ) },
	{ "TextureHeight",	INI::parseInt,											NULL, 		offsetof( Image, m_textureSize.y ) },
	{ "Coords",					Image::parseImageCoords,						NULL, 		offsetof( Image, m_UVCoords ) },
	{ "Status",					Image::parseImageStatus,						NULL, 		offsetof( Image, m_status ) },

	{ NULL,							NULL,																NULL, 		0 }

};

// PRIVATE FUNCTIONS //////////////////////////////////////////////////////////////////////////////
//-------------------------------------------------------------------------------------------------
/** Parse an image coordinates in the form of
	*
	* COORDS = Left:AAA Top:BBB Right:CCC Bottom:DDD */
//-------------------------------------------------------------------------------------------------
void Image::parseImageCoords( INI* ini, void *instance, void *store, const void* /*userData*/ )
{
	Int left = INI::scanInt(ini->getNextSubToken("Left"));
	Int top = INI::scanInt(ini->getNextSubToken("Top"));
	Int right = INI::scanInt(ini->getNextSubToken("Right"));
	Int bottom = INI::scanInt(ini->getNextSubToken("Bottom"));

	// get the image we're storing in
	Image *theImage = (Image *)instance;

	//
	// store the UV coords based on what we've read in and the texture size
	// defined for this image
	//
	Region2D uvCoords;

	uvCoords.lo.x = (Real)left;
	uvCoords.lo.y = (Real)top;
	uvCoords.hi.x = (Real)right;
	uvCoords.hi.y = (Real)bottom;
	
	// adjust the coords by texture size
	const ICoord2D *textureSize = theImage->getTextureSize();
	if( textureSize->x )
	{
		uvCoords.lo.x /= (Real)textureSize->x;
		uvCoords.hi.x /= (Real)textureSize->x;
	}  // end if
	if( textureSize->y )
	{
		uvCoords.lo.y /= (Real)textureSize->y;
		uvCoords.hi.y /= (Real)textureSize->y;
	}  // end if

	// store the uv coords
	theImage->setUV( &uvCoords );

	// compute the image size based on the coords we read and store
	ICoord2D imageSize;
	imageSize.x = right - left;
	imageSize.y = bottom - top;
	theImage->setImageSize( &imageSize );

}  // end parseImageCoord

//-------------------------------------------------------------------------------------------------
/** Parse the image status line */
//-------------------------------------------------------------------------------------------------
void Image::parseImageStatus( INI* ini, void *instance, void *store, const void* /*userData*/)
{	
	// use existing INI parsing for the bit strings
	INI::parseBitString32(ini, instance, store, imageStatusNames);

	//
	// if we are rotated 90 degrees clockwise we need to swap our width and height as
	// they were computed from the page location rect, which was for the rotated image
	// (see ImagePacker tool for more details)
	//
	UnsignedInt *theStatusBits = (UnsignedInt *)store;
	if( BitTest( *theStatusBits, IMAGE_STATUS_ROTATED_90_CLOCKWISE ) )
	{
		Image *theImage = (Image *)instance;
		ICoord2D imageSize;

		imageSize.x = theImage->getImageHeight();  // note it's height not width
		imageSize.y = theImage->getImageWidth();   // note it's width not height
		theImage->setImageSize( &imageSize );

	}  // end if

}  // end parseImageStatus

// PUBLIC DATA ////////////////////////////////////////////////////////////////////////////////////
ImageCollection *TheMappedImageCollection = NULL;  ///< mapped images

// PUBLIC FUNCTIONS////////////////////////////////////////////////////////////////////////////////
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Image::Image( void )
{

	m_name.clear();
	m_filename.clear();
	m_textureSize.x = 0;
	m_textureSize.y = 0;
	m_UVCoords.lo.x = 0.0f;
	m_UVCoords.lo.y = 0.0f;
	m_UVCoords.hi.x = 1.0f;
	m_UVCoords.hi.y = 1.0f;
	m_imageSize.x = 0;
	m_imageSize.y = 0;
	m_rawTextureData = NULL;
	m_status = IMAGE_STATUS_NONE;
	m_next = NULL;

}  // end Image

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Image::~Image( void )
{

}  // end ~Image

//-------------------------------------------------------------------------------------------------
/** Set a status bit into the existing status, return the previous status
	* bit collection from before the set */
//-------------------------------------------------------------------------------------------------
UnsignedInt Image::setStatus( UnsignedInt bit )
{
	UnsignedInt prevStatus = m_status;

	BitSet( m_status, bit );
	return prevStatus;

}  // end setStatus

//-------------------------------------------------------------------------------------------------
/** Clear a status bit from the existing status, return the previous
	* status bit collection from before the clear */
//-------------------------------------------------------------------------------------------------
UnsignedInt Image::clearStatus( UnsignedInt bit )
{
	UnsignedInt prevStatus = m_status;

	BitClear( m_status, bit );
	return prevStatus;

}  // end clearStatus

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ImageCollection::ImageCollection( void )
{

	m_imageList = NULL;

}  // end ImageCollection

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ImageCollection::~ImageCollection( void )
{
	Image *image, *next;

	// delete the images
	image = m_imageList;
	while( image )
	{

		next = image->m_next;
		image->deleteInstance();
		image = next;

	}  // end while
	m_imageList = NULL;

}  // end ~ImageCollection

//-------------------------------------------------------------------------------------------------
/** Return the next image in the collection */
//-------------------------------------------------------------------------------------------------
Image *ImageCollection::nextImage( Image *image )
{

	if( image )
		return image->m_next;

	return NULL;

}  // end nextImage

//-------------------------------------------------------------------------------------------------
/** Allocate a new image, tie to the image list and return it */
//-------------------------------------------------------------------------------------------------
Image *ImageCollection::newImage( void )
{
	Image *image = newInstance(Image);

	// attach to collection list
	image->m_next = m_imageList;
	m_imageList = image;

	return image;

}  // end newImage

//-------------------------------------------------------------------------------------------------
/** Find an image given the image name */
//-------------------------------------------------------------------------------------------------
const Image *ImageCollection::findImageByName( const AsciiString& name )
{
	Image *image;

	/** @todo this needs to be more intelligent if this image collection 
	becomes a real system we use a lot */

	// search the images
	image = m_imageList;
	while( image )
	{

		//
		// want to do a case insensitive compare here cause image INI files are
		// autogenerated from filenames using the image packer tool
		//
		if( image->getName().compareNoCase( name.str() ) == 0 )
			return image;
		image = image->m_next;

	}  // end while

	// not found
	return NULL;

}  // end findImageByName

//-------------------------------------------------------------------------------------------------
/** Find image given image filename */
//-------------------------------------------------------------------------------------------------
const Image *ImageCollection::findImageByFilename( const AsciiString& filename )
{
	Image *image;

	/** @todo this needs to be more intelligent if this image collection 
	becomes a real system we use a lot */

	// search the images
	image = m_imageList;
	while( image )
	{

		if( image->getFilename() == filename )
			return image;
		image = image->m_next;

	}  // end while

	// not found
	return NULL;

}  // end findImageByFilename

//-------------------------------------------------------------------------------------------------
/** Load this image collection with all the images specified in the INI files
	* for the proper texture size directory */
//-------------------------------------------------------------------------------------------------
void ImageCollection::load( Int textureSize )
{
	char buffer[ _MAX_PATH ];
	INI ini;
	// first load in the user created mapped image files if we have them.
	// WIN32_FIND_DATA findData;
	// AsciiString userDataPath;	
	// if(TheGlobalData)
	// {
	// 	userDataPath.format("%sINI\\MappedImages\\*.ini",TheGlobalData->getPath_UserData().str());
	// 	if(FindFirstFile(userDataPath.str(), &findData) !=INVALID_HANDLE_VALUE)
	// 	{
	// 		userDataPath.format("%sINI\\MappedImages",TheGlobalData->getPath_UserData().str());
	// 		ini.loadDirectory(userDataPath, TRUE, INI_LOAD_OVERWRITE, NULL );
	// 	}
	// }

	// construct path to the mapped images folder of the correct texture size
	sprintf( buffer, "Data\\INI\\MappedImages\\TextureSize_%d", textureSize );

	// load all the ine files in that directory

	ini.loadDirectory( AsciiString( buffer ), TRUE, INI_LOAD_OVERWRITE, NULL );

	ini.loadDirectory("Data\\INI\\MappedImages\\HandCreated", TRUE, INI_LOAD_OVERWRITE, NULL );


}  // end load
