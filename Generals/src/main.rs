use autocxx::prelude::*;
use ffi::CRC; // use all the main autocxx functions

include_cpp! {
    #include "PreRTS.h"
    #include "Common/crc.h" // your header file name
    safety!(unsafe) // see details of unsafety policies described in the 'safety' section of the book
    generate!("CRC") // add this line for each function or type you wish to generate
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
}
