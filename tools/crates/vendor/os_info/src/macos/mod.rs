use std::process::Command;

use log::{trace, warn};

use crate::{architecture, bitness, matcher::Matcher, Info, Type, Version};

pub fn current_platform() -> Info {
    trace!("macos::current_platform is called");

    let architecture = architecture::get();
    let bits = architecture
        .as_deref()
        .map(|arch| match arch {
            "arm64" | "x86_64" => bitness::Bitness::X64,
            "i386" => bitness::Bitness::X32,
            _ => bitness::get(),
        })
        .unwrap_or_else(bitness::get);

    let info = Info {
        os_type: Type::Macos,
        version: version(),
        bitness: bits,
        architecture,
        ..Default::default()
    };
    trace!("Returning {:?}", info);
    info
}

fn version() -> Version {
    match product_version() {
        None => Version::Unknown,
        Some(val) => Version::from_string(val),
    }
}

#[allow(unsafe_code)]
fn product_version_from_file() -> Option<String> {
    use objc2_foundation::NSData;
    use objc2_foundation::NSDictionary;
    use objc2_foundation::NSPropertyListFormat;
    use objc2_foundation::NSPropertyListMutabilityOptions;
    use objc2_foundation::NSPropertyListSerialization;
    use objc2_foundation::{ns_string, NSString};

    let buffer = std::fs::read("/System/Library/CoreServices/SystemVersion.plist");
    if let Err(ref e) = buffer {
        warn!("Failed to read SystemVersion.plist: {e:?}");
    }
    let buffer = buffer.ok()?;
    let data = NSData::with_bytes(&buffer);

    let mut format = NSPropertyListFormat::OpenStepFormat;
    // SAFETY: Necessary to call native API, exit-code is checked.
    let result = unsafe {
        NSPropertyListSerialization::propertyListWithData_options_format_error(
            &data,
            NSPropertyListMutabilityOptions(0),
            &mut format,
        )
    };
    if let Err(ref e) = result {
        warn!("Failed to parse SystemVersion.plist: {e:?}");
        return None;
    }
    trace!("Parsed SystemVersion.plist with format: {format:?}");

    let obj = result.ok()?;
    let dict = obj.downcast_ref::<NSDictionary>();
    if dict.is_none() {
        warn!("Failed to downcast to NSDictionary");
        return None;
    }

    let product_version = dict?.objectForKey(ns_string!("ProductVersion"));
    if product_version.is_none() {
        warn!("Failed to get ProductVersion from NSDictionary");
        return None;
    }
    let product_version = product_version?
        .downcast_ref::<NSString>()
        .map(NSString::to_string);
    if product_version.is_none() {
        warn!("Failed to downcast ProductVersion to NSString");
        return None;
    }
    product_version
}

fn product_version() -> Option<String> {
    if let Some(version) = product_version_from_file() {
        trace!("ProductVersion from SystemVersion.plist: {version:?}");
        return Some(version);
    }

    match Command::new("sw_vers").output() {
        Ok(val) => {
            let output = String::from_utf8_lossy(&val.stdout);
            trace!("sw_vers command returned {:?}", output);
            parse(&output)
        }
        Err(e) => {
            warn!("sw_vers command failed with {:?}", e);
            None
        }
    }
}

fn parse(sw_vers_output: &str) -> Option<String> {
    Matcher::PrefixedVersion {
        prefix: "ProductVersion:",
    }
    .find(sw_vers_output)
}

#[cfg(test)]
mod tests {
    use super::*;
    use pretty_assertions::{assert_eq, assert_ne};

    #[test]
    fn os_type() {
        let version = current_platform();
        assert_eq!(Type::Macos, version.os_type());
    }

    #[test]
    fn os_version() {
        let version = version();
        assert_ne!(Version::Unknown, version);
    }

    #[test]
    fn string_product_version() {
        let version = product_version();
        assert!(version.is_some());
    }

    #[test]
    fn parse_version() {
        let parse_output = parse(sw_vers_output());
        assert_eq!(parse_output, Some("10.10.5".to_string()));
    }

    fn sw_vers_output() -> &'static str {
        "ProductName:	Mac OS X\n\
         ProductVersion:	10.10.5\n\
         BuildVersion:	14F27"
    }

    #[test]
    fn parse_beta_version() {
        let parse_output = parse(sw_vers_output_beta());
        assert_eq!(parse_output, Some("10.15".to_string()));
    }

    fn sw_vers_output_beta() -> &'static str {
        "ProductName:	Mac OS X\n\
         ProductVersion:	10.15\n\
         BuildVersion:	19A546d"
    }

    #[test]
    fn parse_double_digit_patch_version() {
        let parse_output = parse(sw_vers_output_double_digit_patch_version());
        assert_eq!(parse_output, Some("10.15.21".to_string()));
    }

    fn sw_vers_output_double_digit_patch_version() -> &'static str {
        "ProductName:	Mac OS X\n\
         ProductVersion:	10.15.21\n\
         BuildVersion:	ABCD123"
    }
}
