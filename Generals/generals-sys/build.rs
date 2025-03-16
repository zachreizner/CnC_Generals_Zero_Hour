fn main() {
    let dst = cmake::Config::new("../")
        .no_default_flags(true)
        .generator("Ninja")
        .build_target("generals_lib")
        .build();

    println!("cargo:rustc-link-search=native={}/build", dst.display());
    println!("cargo:rustc-link-lib=static=generals_lib");
}
