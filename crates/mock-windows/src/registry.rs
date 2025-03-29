use crate::handle::HandleImpl;

pub struct Registry {}

impl Registry {}

pub struct RegistryKey {
    sub_key: String,
}

impl HandleImpl for RegistryKey {}

impl RegistryKey {
    pub fn open(sub_key: &str) -> Self {
        RegistryKey {
            sub_key: sub_key.to_owned(),
        }
    }
}
