use miette::IntoDiagnostic;
use std::path::PathBuf;


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

    let files: Result<Vec<PathBuf>, _> = glob::glob("Code/GameEngine/Source/Common/*.cpp")
        .into_diagnostic()?
        .collect();
    let files = files.into_diagnostic()?;

    eprintln!("C++ files to build");
    for f in files.iter() {
        eprintln!("  {}", f.display());
    }

    // This assumes all your C++ bindings are in main.rs
    let mut b = autocxx_build::Builder::new("src/main.rs", &incs).build()?;
    b.flag_if_supported("-std=c++14")
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
        .files(files)
        .compile("autocxx-demo"); // arbitrary library name, pick anything

    println!("cargo:rerun-if-changed=src/main.rs");

    Ok(())
}
