use std::ptr::null_mut;

use autocxx::prelude::*;
use autocxx::subclass::*;
use ffi::*;

include_cpp! {
    #include "PreRTS.h"
    #include "Common/crc.h"
    #include "Common/GameEngine.h"
    #include "Common/LocalFileSystem.h"
    safety!(unsafe)
    generate!("CRC")
    generate!("GameMain")
    subclass!("GameEngine", GeneralsGameEngine)
    subclass!("LocalFileSystem", GeneralsLocalFileSystem)
}

#[subclass]
#[derive(Default)]
pub struct GeneralsGameEngine;

impl GameEngine_methods for GeneralsGameEngine {
    fn createLocalFileSystem(&mut self) -> *mut LocalFileSystem {
        todo!()
    }

    fn createArchiveFileSystem(&mut self) -> *mut ArchiveFileSystem {
        todo!()
    }

    fn createGameLogic(&mut self) -> *mut GameLogic {
        todo!()
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
}

#[subclass]
pub struct GeneralsLocalFileSystem;

impl LocalFileSystem_methods for GeneralsLocalFileSystem {
    fn init(&mut self) {
        todo!()
    }

    fn reset(&mut self) {
        todo!()
    }

    fn update(&mut self) {
        todo!()
    }

    unsafe fn openFile(
        &mut self,
        filename: *const ::std::os::raw::c_char,
        access: autocxx::c_int,
    ) -> *mut File {
        todo!()
    }

    unsafe fn doesFileExist(&self, filename: *const ::std::os::raw::c_char) -> bool {
        todo!()
    }

    fn getFileListInDirectory(
        &self,
        currentDirectory: &AsciiString,
        originalDirectory: &AsciiString,
        searchName: &AsciiString,
        filenameList: ::core::pin::Pin<&mut FilenameList>,
        searchSubdirectories: bool,
    ) {
        todo!()
    }

    unsafe fn getFileInfo(&self, filename: &AsciiString, fileInfo: *mut FileInfo) -> bool {
        todo!()
    }

    fn createDirectory(&mut self, directory: cxx::UniquePtr<AsciiString>) -> bool {
        todo!()
    }
}

#[no_mangle]
pub fn CreateGameEngine() -> cxx::UniquePtr<GameEngine> {
    GeneralsGameEngine::as_GameEngine_unique_ptr(GeneralsGameEngine::default_cpp_owned())
}

#[no_mangle]
pub fn timeGetTime() -> u32 {
    todo!()
}

fn main() {
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
        GameMain(autocxx::c_int(0), null_mut());
    }
}
