use std::ffi::CStr;
use std::io::{Read, Seek, SeekFrom};
use std::os::raw::{c_char, c_int, c_void};
use std::path::PathBuf;
use std::pin::Pin;
use std::ptr::null_mut;

use autocxx::prelude::*;
use autocxx::subclass::*;
use ffi::*;
use mock_windows::HWND;

extern crate mock_windows;
// opaque!("GameMessage")
// block!("ObjectPtrHash")
// block!("ObjectPtrIter")
// block!("BuildableMap")
// block!("ControlBarOverrideMap")
// block!("std::unordered_map")

#[unsafe(no_mangle)]
pub unsafe extern "C" fn log_backtrace() {
    log::debug!(target: "", "\n{}", std::backtrace::Backtrace::force_capture());
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn log_crash_buffer(buf: *const c_char) {
    let log_message = unsafe { CStr::from_ptr(buf).to_str().unwrap().trim() };
    log::error!(target: "Generals", "{}", log_message);
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn emit_mini_log(fname: *const c_char, buf: *const c_char) {
    let filename = unsafe { CStr::from_ptr(fname).to_str().unwrap() };
    let log_message = unsafe { CStr::from_ptr(buf).to_str().unwrap() };
    log::info!(target: filename, "{log_message}")
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn HybridFileRead(
    handle: *mut c_void,
    buffer: *mut c_void,
    bytes: c_int,
) -> c_int {
    let file = unsafe { &mut *handle.cast::<std::fs::File>() };
    let buffer = std::slice::from_raw_parts_mut(buffer.cast(), bytes as _);
    file.read(buffer).unwrap() as _
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn HybridFileSeek(
    handle: *mut c_void,
    new_pos: c_int,
    seek_mode: c_int,
) -> c_int {
    let file = unsafe { &mut *handle.cast::<std::fs::File>() };
    let pos = match seek_mode {
        0 => SeekFrom::Start(new_pos as _),
        1 => SeekFrom::Current(new_pos as _),
        2 => SeekFrom::End(new_pos as _),
        _ => panic!("unknown seek_mode {seek_mode}"),
    };
    file.seek(pos).unwrap() as _
}

include_cpp! {
    #include "PreRTS.h"
    #include "Hybrid.h"
    #include "Common/crc.h"
    #include "Common/GameEngine.h"
    #include "Common/LocalFileSystem.h"
    #include "Common/ArchiveFileSystem.h"
    // #include "GameLogic/GameLogic.h"
    // #include "GameClient/GameClient.h"
    #include "Common/ModuleFactory.h"
    // #include "Common/ThingFactory.h"
    #include "Common/FunctionLexicon.h"
    #include "Common/Radar.h"
    // #include "GameClient/ParticleSys.h"
    // #include "Common/GameAudio.h"
    // block!("Display")
    // block!("InGameUI")
    // block!("GameWindowManager")
    // block!("FontLibrary")
    // block!("DisplayStringManager")
    safety!(unsafe)
    opaque!("AudioManager")
    opaque!("ParticleSystemManager")
    generate!("CRC")
    generate!("InsertFilenameList")
    generate!("CreateAudioManager")
    generate!("CreateFunctionLexicon")
    generate!("CreateParticleSystemManager")
    generate!("GameMain")
    generate!("initMemoryManager")
    generate!("File")
    generate!("FilenameList")
    generate_pod!("FileInfo")
    generate!("AsciiString")
    generate!("ModuleFactory")
    subclass!("GameEngine", GeneralsGameEngine)
    subclass!("LocalFileSystem", HybridLocalFileSystem)
    // subclass!("ArchiveFileSystem", GeneralsArchiveFileSystem)
    // subclass!("GameLogic", GeneralsGameLogic)
    // subclass!("GameClient", GeneralsGameClient)
    // subclass!("ModuleFactory", GeneralsModuleFactory)
    // subclass!("ThingFactory", GeneralsThingFactory)
    subclass!("FunctionLexicon", GeneralsFunctionLexicon)
    subclass!("Radar", GeneralsRadar)
    // subclass!("ParticleSystemManager", GeneralsParticleSystemManager)
    // subclass!("AudioManager", GeneralsAudioManager)
}

unsafe extern "C" {
    pub fn CreateHybridFile(handle: *mut c_void) -> *mut File;
    pub fn CreateHybridArchiveFileSystem() -> *mut ArchiveFileSystem;
    pub fn CreateHybridGameLogic() -> *mut GameLogic;
}

#[subclass]
#[derive(Default)]
pub struct GeneralsGameEngine;

impl GameEngine_methods for GeneralsGameEngine {
    fn createLocalFileSystem(&mut self) -> *mut LocalFileSystem {
        // unsafe { CreateHybridLocalFileSystem() }
        HybridLocalFileSystem::as_LocalFileSystem_unique_ptr(
            HybridLocalFileSystem::default_cpp_owned(),
        )
        .into_raw()
    }

    fn createArchiveFileSystem(&mut self) -> *mut ArchiveFileSystem {
        unsafe { CreateHybridArchiveFileSystem() }
        // log::debug!("creating ArchiveFileSysten");
        // GeneralsArchiveFileSystem::as_ArchiveFileSystem_unique_ptr(
        //     GeneralsArchiveFileSystem::default_cpp_owned(),
        // )
        // .into_raw()
    }

    fn createGameLogic(&mut self) -> *mut GameLogic {
        // GeneralsGameLogic::as_GameLogic_unique_ptr(
        //     GeneralsGameLogic::default_cpp_owned(),
        // )
        // .into_raw()
        unsafe { CreateHybridGameLogic() }
    }

    fn createGameClient(&mut self) -> *mut GameClient {
        todo!()
    }

    fn createModuleFactory(&mut self) -> *mut ModuleFactory {
        ModuleFactory::new().within_unique_ptr().into_raw()
    }

    fn createThingFactory(&mut self) -> *mut ThingFactory {
        todo!()
    }

    fn createFunctionLexicon(&mut self) -> *mut FunctionLexicon {
        CreateFunctionLexicon()
    }

    fn createRadar(&mut self) -> *mut Radar {
        todo!()
    }

    fn createParticleSystemManager(&mut self) -> *mut ParticleSystemManager {
        CreateParticleSystemManager()
    }

    // fn createGameClient(&mut self) -> *mut GameClient {
    //     GeneralsGameClient::as_GameClient_unique_ptr(
    //         GeneralsGameClient::default_cpp_owned(),
    //     )
    //     .into_raw()
    // }

    // fn createModuleFactory(&mut self) -> *mut ModuleFactory {
    //     GeneralsModuleFactory::as_ModuleFactory_unique_ptr(
    //         GeneralsModuleFactory::default_cpp_owned(),
    //     )
    //     .into_raw()
    // }

    // fn createThingFactory(&mut self) -> *mut ThingFactory {
    //     GeneralsThingFactory::as_ThingFactory_unique_ptr(
    //         GeneralsThingFactory::default_cpp_owned(),
    //     )
    //     .into_raw()
    // }

    // fn createFunctionLexicon(&mut self) -> *mut FunctionLexicon {
    //     GeneralsFunctionLexicon::as_FunctionLexicon_unique_ptr(
    //         GeneralsFunctionLexicon::default_cpp_owned(),
    //     )
    //     .into_raw()
    // }

    // fn createRadar(&mut self) -> *mut Radar {
    //     GeneralsRadar::as_Radar_unique_ptr(
    //         GeneralsRadar::default_cpp_owned(),
    //     )
    //     .into_raw()
    // }

    // fn createParticleSystemManager(&mut self) -> *mut ParticleSystemManager {
    //     GeneralsParticleSystemManager::as_ParticleSystemManager_unique_ptr(
    //         GeneralsParticleSystemManager::default_cpp_owned(),
    //     )
    //     .into_raw()
    // }

    fn createAudioManager(&mut self) -> *mut AudioManager {
        CreateAudioManager()
        // GeneralsAudioManager::as_AudioManager_unique_ptr(
        //     GeneralsAudioManager::default_cpp_owned(),
        // )
        // .into_raw()
    }
}

#[subclass]
#[derive(Default)]
pub struct HybridLocalFileSystem {
    root_path: PathBuf,
}

impl LocalFileSystem_methods for HybridLocalFileSystem {
    fn init(&mut self) {
        log::debug!("LocalFileSystem init");

        let generals_root = std::env::var_os("GENERALS_ROOT").expect("GENERALS_ROOT must be set");
        log::info!("root path={:?}", generals_root);
        self.root_path = PathBuf::from(generals_root);

        if !matches!(
            std::fs::exists(self.root_path.join("Data/Scripts/SkirmishScripts.scb")),
            Ok(true)
        ) {
            log::warn!("sanity check for root path (GENERALS_ROOT) failed");
        }
    }

    fn reset(&mut self) {
        log::debug!("LocalFileSystem reset");
    }

    fn update(&mut self) {
        log::debug!("LocalFileSystem update");
    }

    unsafe fn openFile(&mut self, filename: *const c_char, access: autocxx::c_int) -> *mut File {
        let filename = unsafe { CStr::from_ptr(filename).to_str().unwrap() };
        log::debug!("openFile filename={}", filename);
        match std::fs::File::open(self.root_path.join(filename)) {
            Ok(f) => CreateHybridFile(Box::into_raw(Box::new(f)).cast()),
            Err(e) => {
                log::warn!("failed to open file {filename:?}: {e}");
                null_mut()
            }
        }
    }

    unsafe fn doesFileExist(&self, filename: *const c_char) -> bool {
        let filename = unsafe { CStr::from_ptr(filename).to_str().unwrap() };
        log::debug!("doesFileExist filename={}", filename);
        self.root_path.join(filename).exists()
    }

    fn getFileListInDirectory(
        &self,
        currentDirectory: &AsciiString,
        originalDirectory: &AsciiString,
        searchName: &AsciiString,
        mut filenameList: Pin<&mut FilenameList>,
        searchSubdirectories: bool,
    ) {
        let current = unsafe { CStr::from_ptr(currentDirectory.str()) };
        let original = unsafe { CStr::from_ptr(originalDirectory.str()) };
        let name = unsafe { CStr::from_ptr(searchName.str()).to_str().unwrap() };
        log::debug!("LocalFileSystem::getFileListInDirectory current={current:?} original={original:?} name={name:?}");
        assert!(current.is_empty());
        assert!(original.is_empty());

        let pattern = if searchSubdirectories {
            self.root_path.join("**").join(name)
        } else {
            self.root_path.join(name)
        };

        log::debug!("globbing for pattern {pattern:?}");

        for path in glob::glob(&pattern.into_os_string().into_string().unwrap()).unwrap() {
            let Ok(path) = path else {
                continue;
            };
            log::debug!("glob found file {path:?}");
            cxx::let_cxx_string!(s = path.as_os_str().as_encoded_bytes());
            InsertFilenameList(filenameList.as_mut(), &s);
        }
    }

    unsafe fn getFileInfo(&self, filename: &AsciiString, fileInfo: *mut FileInfo) -> bool {
        // fileInfo.write(FileInfo::);
        let file_info = unsafe { &mut *fileInfo };
        todo!()
    }

    fn createDirectory(&mut self, directory: cxx::UniquePtr<AsciiString>) -> bool {
        todo!()
    }
}

// #[subclass]
// #[derive(Default)]
// pub struct GeneralsArchiveFileSystem;

// impl ArchiveFileSystem_methods for GeneralsArchiveFileSystem {
//     fn init(&mut self) {
//         log::debug!("ArchiveFileSystem init");
//         self.loadBigFilesFromDirectory("", "*.big", false);
//     }

//     fn update(&mut self) {
//         log::debug!("ArchiveFileSystem update");
//     }

//     fn reset(&mut self) {
//         log::debug!("ArchiveFileSystem reset");
//     }

//     fn postProcessLoad(&mut self) {
//         todo!()
//     }

//     unsafe fn openArchiveFile(
//         &mut self,
//         filename: *const c_char,
//     ) -> *mut ArchiveFile {
//         todo!()
//     }

//     unsafe fn closeArchiveFile(&mut self, filename: *const c_char) {
//         todo!()
//     }

//     fn closeAllArchiveFiles(&mut self) {
//         todo!()
//     }

//     fn closeAllFiles(&mut self) {
//         todo!()
//     }

//     fn loadBigFilesFromDirectory(
//         &mut self,
//         dir: cxx::UniquePtr<AsciiString>,
//         fileMask: cxx::UniquePtr<AsciiString>,
//         overwrite: bool,
//     ) -> bool {
//         todo!()
//     }
// }

// #[subclass]
// #[derive(Default)]
// pub struct GeneralsGameLogic;

// impl GameLogic_methods for GeneralsGameLogic {
//     fn init(&mut self) { log::debug!("GameLogic init"); }
//     fn update(&mut self) { log::debug!("GameLogic update"); }
//     fn reset(&mut self) { log::debug!("GameLogic reset"); }
// }

// #[subclass]
// #[derive(Default)]
// pub struct GeneralsGameClient;

// impl GameClient_methods for GeneralsGameClient {
//     fn init(&mut self) { log::debug!("GameClient init"); }
//     fn update(&mut self) { log::debug!("GameClient update"); }
//     fn reset(&mut self) { log::debug!("GameClient reset"); }
// }

// #[subclass]
// #[derive(Default)]
// pub struct GeneralsModuleFactory;

// impl ModuleFactory_methods for GeneralsModuleFactory {
//     fn init(&mut self) { log::debug!("ModuleFactory init"); }
//     fn update(&mut self) { log::debug!("ModuleFactory update"); }
//     fn reset(&mut self) { log::debug!("ModuleFactory reset"); }
// }

// #[subclass]
// #[derive(Default)]
// pub struct GeneralsThingFactory;

// impl ThingFactory_methods for GeneralsThingFactory {
//     fn init(&mut self) { log::debug!("ThingFactory init"); }
//     fn update(&mut self) { log::debug!("ThingFactory update"); }
//     fn reset(&mut self) { log::debug!("ThingFactory reset"); }
// }

#[subclass]
#[derive(Default)]
pub struct GeneralsFunctionLexicon;

impl FunctionLexicon_methods for GeneralsFunctionLexicon {
    fn init(&mut self) {
        log::debug!("FunctionLexicon init");
    }
    fn update(&mut self) {
        log::debug!("FunctionLexicon update");
    }
    fn reset(&mut self) {
        log::debug!("FunctionLexicon reset");
    }
}

#[subclass]
#[derive(Default)]
pub struct GeneralsRadar;

impl Radar_methods for GeneralsRadar {
    fn init(&mut self) {
        log::debug!("Radar init");
    }
    fn update(&mut self) {
        log::debug!("Radar update");
    }
    fn reset(&mut self) {
        log::debug!("Radar reset");
    }

    fn draw(
        &mut self,
        pixelX: autocxx::c_int,
        pixelY: autocxx::c_int,
        width: autocxx::c_int,
        height: autocxx::c_int,
    ) {
        todo!()
    }

    fn clearShroud(&mut self) {
        todo!()
    }

    fn setShroudLevel(&mut self, x: autocxx::c_int, y: autocxx::c_int, setting: CellShroudStatus) {
        todo!()
    }
}

// #[subclass]
// #[derive(Default)]
// pub struct GeneralsParticleSystemManager;

// impl ParticleSystemManager_methods for GeneralsParticleSystemManager {
//     fn init(&mut self) { log::debug!("ParticleSystemManager init"); }
//     fn update(&mut self) { log::debug!("ParticleSystemManager update"); }
//     fn reset(&mut self) { log::debug!("ParticleSystemManager reset"); }
// }

// #[subclass]
// #[derive(Default)]
// pub struct GeneralsAudioManager;

// impl AudioManager_methods for GeneralsAudioManager {
//     fn init(&mut self) {
//         log::debug!("AudioManager init");
//     }
//     fn update(&mut self) {
//         log::debug!("AudioManager update");
//     }
//     fn reset(&mut self) {
//         log::debug!("AudioManager reset");
//     }

//     unsafe fn audioDebugDisplay(
//         &mut self,
//         dd: *mut DebugDisplayInterface,
//         userData: *mut autocxx::c_void,
//         fp: *mut _IO_FILE,
//     ) {
//         todo!()
//     }

//     fn stopAudio(&mut self, which: AudioAffect) {
//         todo!()
//     }

//     fn pauseAudio(&mut self, which: AudioAffect) {
//         todo!()
//     }

//     fn resumeAudio(&mut self, which: AudioAffect) {
//         todo!()
//     }

//     fn pauseAmbient(&mut self, shouldPause: bool) {
//         todo!()
//     }

//     unsafe fn stopAllAmbientsBy(&mut self, obj: *mut Object) {
//         todo!()
//     }

//     unsafe fn stopAllAmbientsBy1(&mut self, draw: *mut Drawable) {
//         todo!()
//     }

//     fn killAudioEventImmediately(&mut self, audioEvent: autocxx::c_uint) {
//         todo!()
//     }

//     fn nextMusicTrack(&mut self) {
//         todo!()
//     }

//     fn prevMusicTrack(&mut self) {
//         todo!()
//     }

//     fn isMusicPlaying(&self) -> bool {
//         todo!()
//     }

//     fn hasMusicTrackCompleted(
//         &self,
//         trackName: &AsciiString,
//         numberOfTimes: autocxx::c_int,
//     ) -> bool {
//         todo!()
//     }

//     fn getMusicTrackName(&self) -> cxx::UniquePtr<AsciiString> {
//         todo!()
//     }

//     fn openDevice(&mut self) {
//         todo!()
//     }

//     fn closeDevice(&mut self) {
//         todo!()
//     }

//     fn getDevice(&mut self) -> *mut autocxx::c_void {
//         todo!()
//     }

//     fn notifyOfAudioCompletion(&mut self, audioCompleted: autocxx::c_uint, flags: autocxx::c_uint) {
//         todo!()
//     }

//     fn getProviderCount(&self) -> autocxx::c_uint {
//         todo!()
//     }

//     fn getProviderName(&self, providerNum: autocxx::c_uint) -> cxx::UniquePtr<AsciiString> {
//         todo!()
//     }

//     fn getProviderIndex(&self, providerName: cxx::UniquePtr<AsciiString>) -> autocxx::c_uint {
//         todo!()
//     }

//     fn selectProvider(&mut self, providerNdx: autocxx::c_uint) {
//         todo!()
//     }

//     fn unselectProvider(&mut self) {
//         todo!()
//     }

//     fn getSelectedProvider(&self) -> autocxx::c_uint {
//         todo!()
//     }

//     fn setSpeakerType(&mut self, speakerType: autocxx::c_uint) {
//         todo!()
//     }

//     fn getSpeakerType(&mut self) -> autocxx::c_uint {
//         todo!()
//     }

//     fn getNum2DSamples(&self) -> autocxx::c_uint {
//         todo!()
//     }

//     fn getNum3DSamples(&self) -> autocxx::c_uint {
//         todo!()
//     }

//     fn getNumStreams(&self) -> autocxx::c_uint {
//         todo!()
//     }

//     unsafe fn doesViolateLimit(&self, event: *mut AudioEventRTS) -> bool {
//         todo!()
//     }

//     unsafe fn isPlayingLowerPriority(&self, event: *mut AudioEventRTS) -> bool {
//         todo!()
//     }

//     unsafe fn isPlayingAlready(&self, event: *mut AudioEventRTS) -> bool {
//         todo!()
//     }

//     fn isObjectPlayingVoice(&self, objID: autocxx::c_uint) -> bool {
//         todo!()
//     }

//     fn adjustVolumeOfPlayingAudio(
//         &mut self,
//         eventName: cxx::UniquePtr<AsciiString>,
//         newVolume: f32,
//     ) {
//         todo!()
//     }

//     fn removePlayingAudio(&mut self, eventName: cxx::UniquePtr<AsciiString>) {
//         todo!()
//     }

//     fn removeAllDisabledAudio(&mut self) {
//         todo!()
//     }

//     fn getHandleForBink(&mut self) -> *mut autocxx::c_void {
//         todo!()
//     }

//     fn releaseHandleForBink(&mut self) {
//         todo!()
//     }

//     unsafe fn friend_forcePlayAudioEventRTS(&mut self, eventToPlay: *const AudioEventRTS) {
//         todo!()
//     }

//     fn setPreferredProvider(&mut self, providerNdx: cxx::UniquePtr<AsciiString>) {
//         todo!()
//     }

//     fn setPreferredSpeaker(&mut self, speakerType: cxx::UniquePtr<AsciiString>) {
//         todo!()
//     }

//     fn getFileLengthMS(&self, strToLoad: cxx::UniquePtr<AsciiString>) -> f32 {
//         todo!()
//     }

//     unsafe fn closeAnySamplesUsingFile(&mut self, fileToClose: *const autocxx::c_void) {
//         todo!()
//     }

//     fn setDeviceListenerPosition(&mut self) {
//         todo!()
//     }
// }

#[no_mangle]
pub fn CreateGameEngine() -> cxx::UniquePtr<GameEngine> {
    GeneralsGameEngine::as_GameEngine_unique_ptr(GeneralsGameEngine::default_cpp_owned())
}

#[repr(transparent)]
struct GlobalCstr(*const c_char);
unsafe impl Sync for GlobalCstr {}

#[no_mangle]
static g_strFile: GlobalCstr = GlobalCstr(c"data/Generals.str".as_ptr());

#[no_mangle]
static g_csfFile: GlobalCstr = GlobalCstr(c"data/%s/Generals.csf".as_ptr());

#[no_mangle]
static mut ApplicationHWnd: HWND = null_mut();

fn main() {
    env_logger::Builder::from_env(env_logger::Env::default().default_filter_or("debug"))
        .format_timestamp(None)
        .init();
    log::info!("starting up");

    println!("Hello, world!");
    let s = "\x01\x01";

    moveit! {
        let mut crc = CRC::new();
    }
    let res = unsafe {
        crc.as_mut()
            .computeCRC(s.as_ptr().cast(), autocxx::c_int(s.len() as _));
        crc.as_mut().get()
    };

    eprintln!("crc = {res:?}");

    unsafe {
        initMemoryManager();
        GameMain(autocxx::c_int(0), null_mut());
    }
}
