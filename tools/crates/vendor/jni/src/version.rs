/// JNI Version
#[derive(Debug, Copy, Clone, PartialEq, PartialOrd, Ord, Eq, Hash)]
#[repr(transparent)]
pub struct JNIVersion {
    ver: u32,
}

impl JNIVersion {
    /// JNI Version 1.1
    pub const V1_1: Self = JNIVersion {
        ver: jni_sys::JNI_VERSION_1_1 as u32,
    };
    /// JNI Version 1.2
    pub const V1_2: Self = JNIVersion {
        ver: jni_sys::JNI_VERSION_1_2 as u32,
    };
    /// JNI Version 1.4
    pub const V1_4: Self = JNIVersion {
        ver: jni_sys::JNI_VERSION_1_4 as u32,
    };
    /// JNI Version 1.6
    pub const V1_6: Self = JNIVersion {
        ver: jni_sys::JNI_VERSION_1_6 as u32,
    };
    /// JNI Version 1.8
    pub const V1_8: Self = JNIVersion {
        ver: jni_sys::JNI_VERSION_1_8 as u32,
    };
    /// JNI Version 9.0
    pub const V9: Self = JNIVersion {
        ver: jni_sys::JNI_VERSION_9 as u32,
    };
    /// JNI Version 10.0
    pub const V10: Self = JNIVersion {
        ver: jni_sys::JNI_VERSION_10 as u32,
    };
    /// JNI Version 19.0
    pub const V19: Self = JNIVersion {
        ver: jni_sys::JNI_VERSION_19 as u32,
    };
    /// JNI Version 20.0
    pub const V20: Self = JNIVersion {
        ver: jni_sys::JNI_VERSION_20 as u32,
    };
    /// JNI Version 21.0
    pub const V21: Self = JNIVersion {
        ver: jni_sys::JNI_VERSION_21 as u32,
    };

    /// Return a version from a raw version constant like [`jni_sys::JNI_VERSION_1_2`]
    pub fn new(ver: jni_sys::jint) -> Self {
        Self::from(ver)
    }

    /// Get the major component of the version number
    pub fn major(&self) -> u16 {
        ((self.ver & 0x00ff0000) >> 16) as u16
    }

    /// Get the minor component of the version number
    pub fn minor(&self) -> u16 {
        (self.ver & 0xff) as u16
    }
}

impl From<jni_sys::jint> for JNIVersion {
    fn from(value: jni_sys::jint) -> Self {
        Self { ver: value as u32 }
    }
}

impl From<JNIVersion> for jni_sys::jint {
    fn from(val: JNIVersion) -> Self {
        val.ver as i32
    }
}

#[test]
fn jni_version_major_minor() {
    macro_rules! check_major_minor {
        ($major:expr, $minor:expr, $jni_ver:tt, $jni_sys_ver:tt) => {
            let v = JNIVersion::$jni_ver;
            assert_eq!(v.major(), $major);
            assert_eq!(v.minor(), $minor);
            let v = JNIVersion::new(jni_sys::$jni_sys_ver);
            assert_eq!(v.major(), $major);
            assert_eq!(v.minor(), $minor);
        };
    }

    check_major_minor!(1, 1, V1_1, JNI_VERSION_1_1);
    check_major_minor!(1, 2, V1_2, JNI_VERSION_1_2);
    check_major_minor!(1, 4, V1_4, JNI_VERSION_1_4);
    check_major_minor!(1, 6, V1_6, JNI_VERSION_1_6);
    check_major_minor!(1, 8, V1_8, JNI_VERSION_1_8);
    check_major_minor!(9, 0, V9, JNI_VERSION_9);
    check_major_minor!(10, 0, V10, JNI_VERSION_10);
    check_major_minor!(19, 0, V19, JNI_VERSION_19);
    check_major_minor!(20, 0, V20, JNI_VERSION_20);
    check_major_minor!(21, 0, V21, JNI_VERSION_21);
}
