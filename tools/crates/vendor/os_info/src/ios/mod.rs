use log::trace;

use objc2::{msg_send, rc::Retained, ClassType};
use objc2_foundation::NSString;
use objc2_ui_kit::UIDevice;

use crate::{Bitness, Info, Type, Version};

pub fn current_platform() -> Info {
    trace!("ios::current_platform is called");

    let bitness = match std::env::consts::ARCH {
        "x86" | "arm" => Bitness::X32,
        "x86_64" | "aarch64" => Bitness::X64,
        _ => Bitness::Unknown,
    };

    let info = Info {
        os_type: Type::Ios,
        version: version(),
        bitness,
        ..Default::default()
    };
    trace!("Returning {:?}", info);
    info
}

fn version() -> Version {
    match system_version().map(|ns| ns.to_string()) {
        Some(v) => Version::from_string(v),
        None => Version::Unknown,
    }
}

#[allow(unsafe_code)]
fn system_version() -> Option<Retained<NSString>> {
    let device: Retained<UIDevice> = unsafe { msg_send![UIDevice::class(), currentDevice] };
    let ver: Retained<NSString> = unsafe { msg_send![&device, systemVersion] };
    Some(ver)
}

#[cfg(test)]
mod tests {
    use super::*;
    use pretty_assertions::assert_eq;

    #[test]
    fn os_type() {
        let version = current_platform();
        assert_eq!(Type::Ios, version.os_type());
    }
}
