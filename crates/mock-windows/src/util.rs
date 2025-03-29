use std::ffi::c_char;

pub unsafe fn cstr_copy(src: &str, dst: *mut c_char, dst_len: usize) -> usize {
    let max_copy = src.len().min((dst_len - 1) as _);
    unsafe {
        dst.copy_from(src.as_ptr().cast(), max_copy);
        dst.add(max_copy).write(0);
    }
    max_copy
}
