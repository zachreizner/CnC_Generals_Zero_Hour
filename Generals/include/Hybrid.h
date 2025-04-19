#ifndef __HYBRID_H__
#define __HYBRID_H__

#include "Common/FileSystem.h"
#include "Common/FunctionLexicon.h"
#include "Common/GameAudio.h"

class ParticleSystemManager;

void InsertFilenameList(FilenameList& filenameList, const std::string& filename);
AudioManager* CreateAudioManager();
FunctionLexicon* CreateFunctionLexicon();
ParticleSystemManager* CreateParticleSystemManager();

#endif
