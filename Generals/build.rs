use miette::IntoDiagnostic;
use std::path::PathBuf;

fn concat_globs(globs: &[&str], excludes: &[&str]) -> miette::Result<Vec<PathBuf>> {
    let mut files = Vec::new();
    for pattern in globs {
        for p in glob::glob(pattern).into_diagnostic()? {
            let p = p.into_diagnostic()?;
            if excludes.iter().all(|e| !p.ends_with(e)) {
                files.push(p);
            }
        }
    }
    Ok(files)
}

fn main() -> miette::Result<()> {
    let incs: Vec<_> = [
        "include",
        "Code/GameEngine/Include",
        "Code/GameEngine/Include/Precompiled",
        "Code/Libraries/Include",
        "Code/Libraries/Source/WWVegas",
        "Code/Libraries/Source/WWVegas/WWLib",
        "Code/Libraries/Source/Compression/",
    ]
    .iter()
    .map(PathBuf::from)
    .collect();

    let files = concat_globs(
        &[
            "Code/GameEngine/Source/Common/Audio/*.cpp",
            "Code/GameEngine/Source/Common/INI/*.cpp",
            "Code/GameEngine/Source/Common/RTS/*.cpp",
            "Code/GameEngine/Source/Common/Thing/*.cpp",
            "Code/GameEngine/Source/Common/System/*.cpp",
            "Code/GameEngine/Source/Common/*.cpp",
        ],
        &[
            "LocalFile.cpp",
            "registry.cpp",
            "Debug.cpp",
            "simpleplayer.cpp",
            "urllaunch.cpp",
            "GameSpeech.cpp",
        ],
    )?;

    eprintln!("C++ files to build");
    for f in files.iter() {
        eprintln!("  {}", f.display());
    }

    // This assumes all your C++ bindings are in main.rs
    let mut b = autocxx_build::Builder::new("src/main.rs", &incs).build()?;
    b.flag_if_supported("-std=c++14")
        .extra_warnings(false)
        .flag_if_supported("-Wno-deprecated-copy")
        .flag_if_supported("-Wno-deprecated")
        .flag_if_supported("-Wno-endif-labels")
        .flag_if_supported("-Wno-enum-compare")
        .flag_if_supported("-Wno-ignored-qualifiers")
        .flag_if_supported("-Wno-implicit-fallthrough")
        .flag_if_supported("-Wno-invalid-offsetof")
        .flag_if_supported("-Wno-logical-not-parentheses")
        .flag_if_supported("-Wno-logical-not-parentheses")
        .flag_if_supported("-Wno-nonnull-compare")
        .flag_if_supported("-Wno-parentheses")
        .flag_if_supported("-Wno-reorder")
        .flag_if_supported("-Wno-sign-compare")
        .flag_if_supported("-Wno-switch")
        .flag_if_supported("-Wno-unknown-pragmas")
        .flag_if_supported("-Wno-unused-but-set-variable")
        .flag_if_supported("-Wno-unused-parameter")
        .flag_if_supported("-Wno-unused-value")
        .flag_if_supported("-Wno-unused-variable")
        .flag_if_supported("-Wno-write-strings")
        .flag_if_supported("-Wno-type-limits")
        .flag_if_supported("-Wno-unused-function")
        .flag_if_supported("-Wno-conversion-null")
        .flag_if_supported("-Wno-narrowing") // This one affects Trig.cpp
        .files(files)
        .compile("autocxx-demo"); // arbitrary library name, pick anything

    println!("cargo:rerun-if-changed=src/main.rs");

    Ok(())
}
