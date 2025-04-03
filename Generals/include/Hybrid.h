#ifndef __HYBRID_H__
#define __HYBRID_H__

#include "Common/FileSystem.h"
#include "Common/GameAudio.h"

void InsertFilenameList(FilenameList& filenameList, const std::string& filename);
AudioManager* CreateAudioManager();

#endif
