use log::trace;

use crate::{
    architecture, bitness,
    uname::{uname, UnameField},
    Info, Type, Version,
};

pub fn current_platform() -> Info {
    trace!("hurd::current_platform is called");

    let version = uname(UnameField::Release)
        .map(Version::from_string)
        .unwrap_or_else(|| Version::Unknown);

    let info = Info {
        os_type: Type::Hurd,
        version,
        bitness: bitness::get(),
        architecture: architecture::get(),
        ..Default::default()
    };

    trace!("Returning {:?}", info);
    info
}

#[cfg(test)]
mod tests {
    use super::*;
    use pretty_assertions::assert_eq;

    #[test]
    fn os_type() {
        let version = current_platform();
        assert_eq!(Type::Hurd, version.os_type());
    }
}
