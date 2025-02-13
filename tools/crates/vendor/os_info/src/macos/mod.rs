use std::process::Command;

use log::{trace, warn};

use crate::{architecture, bitness, matcher::Matcher, Info, Type, Version};

pub fn current_platform() -> Info {
    trace!("macos::current_platform is called");

    let info = Info {
        os_type: Type::Macos,
        version: version(),
        bitness: bitness::get(),
        architecture: architecture::get(),
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

fn product_version() -> Option<String> {
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
