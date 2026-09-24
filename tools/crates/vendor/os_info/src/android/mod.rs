use log::trace;

use crate::{Bitness, Info, Type, Version};

use android_system_properties::AndroidSystemProperties;

pub fn current_platform() -> Info {
    trace!("android::current_platform is called");

    let bitness = match std::env::consts::ARCH {
        "x86" | "arm" => Bitness::X32,
        "x86_64" | "aarch64" => Bitness::X64,
        _ => Bitness::Unknown,
    };

    let info = Info {
        os_type: Type::Android,
        version: version(),
        bitness,
        ..Default::default()
    };
    trace!("Returning {:?}", info);
    info
}

fn version() -> Version {
    let android_system_properties = AndroidSystemProperties::new();

    match android_system_properties.get("ro.build.version.release") {
        Some(v) => Version::from_string(v),
        None => Version::Unknown,
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use pretty_assertions::assert_eq;

    #[test]
    fn os_type() {
        let version = current_platform();
        assert_eq!(Type::Android, version.os_type());
    }
}
