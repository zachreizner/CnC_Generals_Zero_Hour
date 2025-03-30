use crate::handle::HandleImpl;

#[allow(dead_code)]
pub struct Registry {}

impl Registry {}

pub struct RegistryKey {
    _sub_key: String,
}

impl HandleImpl for RegistryKey {}

impl RegistryKey {
    pub fn open(sub_key: &str) -> Self {
        RegistryKey {
            _sub_key: sub_key.to_owned(),
        }
    }
}
