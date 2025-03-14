use std::path::PathBuf;

// const CODE_BASE_PATH: &str = "Code";

fn main() -> miette::Result<()> {
    let incs: Vec<_> = [
        "Code/GameEngine/Include",
        "Code/GameEngine/Include/Precompiled",
        "Code/Libraries/Include",
        "Code/Libraries/Source/WWVegas",
        "Code/Libraries/Source/WWVegas/WWLib",
    ]
    .iter()
    .map(PathBuf::from)
    .collect();

    // This assumes all your C++ bindings are in main.rs
    let mut b = autocxx_build::Builder::new("src/main.rs", &incs).build()?;
    b.flag_if_supported("-std=c++14")
        .flag_if_supported("-Wno-everything")
        .file("Code/GameEngine/Source/Common/crc.cpp")
        .file("Code/GameEngine/Source/Common/MiniLog.cpp")
        .file("Code/GameEngine/Source/Common/Language.cpp")
        .file("Code/GameEngine/Source/Common/BitFlags.cpp")
        .file("Code/GameEngine/Source/Common/DiscreteCircle.cpp")
        .file("Code/GameEngine/Source/Common/StateMachine.cpp")
        .compile("autocxx-demo"); // arbitrary library name, pick anything

    println!("cargo:rerun-if-changed=src/main.rs");

    Ok(())
}
