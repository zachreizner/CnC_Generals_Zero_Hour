use std::ffi::CStr;
use std::pin::Pin;
use std::ptr::null;
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
pub unsafe extern "C" fn log_crash_buffer(buf: *const ::std::os::raw::c_char) {
    let log_message = unsafe { CStr::from_ptr(buf).to_str().unwrap().trim() };
    log::error!(target: "Generals", "{}", log_message);
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn emit_mini_log(
    fname: *const ::std::os::raw::c_char,
    buf: *const ::std::os::raw::c_char,
) {
    let filename = unsafe { CStr::from_ptr(fname).to_str().unwrap() };
    let log_message = unsafe { CStr::from_ptr(buf).to_str().unwrap() };
    log::info!(target: filename, "{log_message}")
}

include_cpp! {
    #include "PreRTS.h"
    #include "Common/crc.h"
    #include "Common/GameEngine.h"
    #include "Common/LocalFileSystem.h"
    #include "Common/ArchiveFileSystem.h"
    // #include "GameLogic/GameLogic.h"
    // #include "GameClient/GameClient.h"
    // #include "Common/ModuleFactory.h"
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
    generate!("CRC")
    generate!("GameMain")
    generate!("initMemoryManager")
    generate!("File")
    generate!("AsciiString")
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
    pub fn CreateHybridFile() -> *mut File;
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
        todo!()
    }

    fn createThingFactory(&mut self) -> *mut ThingFactory {
        todo!()
    }

    fn createFunctionLexicon(&mut self) -> *mut FunctionLexicon {
        todo!()
    }

    fn createRadar(&mut self) -> *mut Radar {
        todo!()
    }

    fn createParticleSystemManager(&mut self) -> *mut ParticleSystemManager {
        todo!()
    }

    fn createAudioManager(&mut self) -> *mut AudioManager {
        todo!()
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

    // fn createAudioManager(&mut self) -> *mut AudioManager {
    //     GeneralsAudioManager::as_AudioManager_unique_ptr(
    //         GeneralsAudioManager::default_cpp_owned(),
    //     )
    //     .into_raw()
    // }
}

#[subclass]
#[derive(Default)]
pub struct HybridLocalFileSystem;

impl LocalFileSystem_methods for HybridLocalFileSystem {
    fn init(&mut self) {
        log::debug!("LocalFileSystem init");
    }

    fn reset(&mut self) {
        log::debug!("LocalFileSystem reset");
    }

    fn update(&mut self) {
        log::debug!("LocalFileSystem update");
    }

    unsafe fn openFile(
        &mut self,
        filename: *const ::std::os::raw::c_char,
        access: autocxx::c_int,
    ) -> *mut File {
        let filename = unsafe { CStr::from_ptr(filename) };
        log::debug!("openFile filename={:?}", filename);
        CreateHybridFile()
    }

    unsafe fn doesFileExist(&self, filename: *const ::std::os::raw::c_char) -> bool {
        todo!()
    }

    fn getFileListInDirectory(
        &self,
        currentDirectory: &AsciiString,
        originalDirectory: &AsciiString,
        searchName: &AsciiString,
        filenameList: Pin<&mut FilenameList>,
        searchSubdirectories: bool,
    ) {
        let current = unsafe { CStr::from_ptr(currentDirectory.str()) };
        let original = unsafe { CStr::from_ptr(originalDirectory.str()) };
        let name = unsafe { CStr::from_ptr(searchName.str()) };
        log::debug!("LocalFileSystem::getFileListInDirectory current={current:?} original={original:?} name={name:?}");
        panic!();
    }

    unsafe fn getFileInfo(&self, filename: &AsciiString, fileInfo: *mut FileInfo) -> bool {
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
//         filename: *const ::std::os::raw::c_char,
//     ) -> *mut ArchiveFile {
//         todo!()
//     }

//     unsafe fn closeArchiveFile(&mut self, filename: *const ::std::os::raw::c_char) {
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
//     fn init(&mut self) { log::debug!("AudioManager init"); }
//     fn update(&mut self) { log::debug!("AudioManager update"); }
//     fn reset(&mut self) { log::debug!("AudioManager reset"); }
// }

#[no_mangle]
pub fn CreateGameEngine() -> cxx::UniquePtr<GameEngine> {
    GeneralsGameEngine::as_GameEngine_unique_ptr(GeneralsGameEngine::default_cpp_owned())
}

#[repr(transparent)]
struct GlobalCstr(*const std::ffi::c_char);
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
