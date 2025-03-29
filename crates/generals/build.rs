use std::path::PathBuf;

fn main() -> miette::Result<()> {
    let incs: Vec<_> = [
        "../../Generals/include",
        "../../Generals/Code/GameEngine/Include",
        "../../Generals/Code/GameEngine/Include/Precompiled",
        "../../Generals/Code/Libraries/Include",
        "../../Generals/Code/Libraries/Source/WWVegas",
        "../../Generals/Code/Libraries/Source/WWVegas/WWLib",
        "../../Generals/Code/Libraries/Source/Compression/",
    ]
    .iter()
    .map(PathBuf::from)
    .collect();

    // This assumes all your C++ bindings are in main.rs
    let mut b = autocxx_build::Builder::new("src/main.rs", &incs)
        .extra_clang_args(&["-D_DEBUG", "-DDEBUG_CRASHING"])
        .build()?;
    b.flag_if_supported("-std=c++14")
        .extra_warnings(false)
        .define("_DEBUG", None)
        .define("DEBUG_CRASHING", None)
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
        .compile("autocxx-demo"); // arbitrary library name, pick anything

    let dst = cmake::Config::new("../../Generals")
        .no_default_flags(true)
        .profile("Debug")
        .generator("Ninja")
        .define("CMAKE_C_COMPILER", "clang")
        .define("CMAKE_CXX_COMPILER", "clang++")
        .define("CMAKE_C_COMPILER_LAUNCHER", "sccache")
        .define("CMAKE_CXX_COMPILER_LAUNCHER", "sccache")
        .build_target("generals_lib")
        .build();

    println!("cargo:rustc-link-search=native={}/build", dst.display());
    println!("cargo:rustc-link-lib=static=generals_lib");
    println!("cargo:rustc-link-lib=static=z");

    println!("cargo:rerun-if-changed=src/main.rs");
    println!("cargo:rerun-if-changed=../../Generals/src/Hybrid.cpp");
    println!("cargo:rerun-if-changed=../../Generals/CMakeLists.txt");

    Ok(())
}
