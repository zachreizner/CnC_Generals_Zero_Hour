use std::any::Any;
use std::collections::HashMap;
use std::sync::OnceLock;
use std::sync::atomic::AtomicUsize;
use std::sync::atomic::Ordering::SeqCst;

use parking_lot::{Mutex, MutexGuard};

pub trait HandleImpl: Any + Send + Sync {}

pub struct HandleObject {
    inner: Box<dyn HandleImpl>,
}

static NEXT_HANDLE: AtomicUsize = AtomicUsize::new(1);

type HandlesMap = HashMap<usize, HandleObject>;
fn global_handles() -> MutexGuard<'static, HandlesMap> {
    static HANDLES: OnceLock<Mutex<HandlesMap>> = OnceLock::new();
    HANDLES.get_or_init(Default::default).lock()
}

pub fn open<T: HandleImpl>(h: T) -> usize {
    let handle = NEXT_HANDLE.fetch_add(1, SeqCst);
    global_handles().insert(handle, HandleObject { inner: Box::new(h) });
    handle
}

pub fn close(handle: usize) {
    global_handles().remove(&handle);
}
