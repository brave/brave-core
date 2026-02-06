// temperature.rs

use byteorder::ByteOrder;
use ctl_error::SysctlError;
use ctl_info::CtlInfo;
use ctl_type::CtlType;
use ctl_value::CtlValue;

use std::f32;

/// A custom type for temperature sysctls.
///
/// # Example
/// ```
/// # use sysctl::Sysctl;
/// if let Ok(ctl) = sysctl::Ctl::new("dev.cpu.0.temperature") {
///     let val = ctl.value().unwrap();
///     let temp = val.as_temperature().unwrap();
///     println!("Temperature: {:.2}K, {:.2}F, {:.2}C",
///               temp.kelvin(),
///               temp.fahrenheit(),
///               temp.celsius());
/// }
/// ```
/// Not available on MacOS
#[derive(Debug, Copy, Clone, PartialEq, PartialOrd)]
pub struct Temperature {
    value: f32, // Kelvin
}
impl Temperature {
    pub fn kelvin(&self) -> f32 {
        self.value
    }
    pub fn celsius(&self) -> f32 {
        self.value - 273.15
    }
    pub fn fahrenheit(&self) -> f32 {
        1.8 * self.celsius() + 32.0
    }
}

pub fn temperature(info: &CtlInfo, val: &[u8]) -> Result<CtlValue, SysctlError> {
    let prec: u32 = {
        match info.fmt.len() {
            l if l > 2 => match info.fmt[2..3].parse::<u32>() {
                Ok(x) if x <= 9 => x,
                _ => 1,
            },
            _ => 1,
        }
    };

    let base = 10u32.pow(prec) as f32;

    let make_temp = move |f: f32| -> Result<CtlValue, SysctlError> {
        Ok(CtlValue::Temperature(Temperature { value: f / base }))
    };

    match info.ctl_type {
        CtlType::Int => make_temp(byteorder::LittleEndian::read_i32(&val) as f32),
        CtlType::S64 => make_temp(byteorder::LittleEndian::read_i64(&val) as f32),
        CtlType::Uint => make_temp(byteorder::LittleEndian::read_u32(&val) as f32),
        CtlType::Long => make_temp(byteorder::LittleEndian::read_i64(&val) as f32),
        CtlType::Ulong => make_temp(byteorder::LittleEndian::read_u64(&val) as f32),
        CtlType::U64 => make_temp(byteorder::LittleEndian::read_u64(&val) as f32),
        CtlType::U8 => make_temp(val[0] as u8 as f32),
        CtlType::U16 => make_temp(byteorder::LittleEndian::read_u16(&val) as f32),
        CtlType::S8 => make_temp(val[0] as i8 as f32),
        CtlType::S16 => make_temp(byteorder::LittleEndian::read_i16(&val) as f32),
        CtlType::S32 => make_temp(byteorder::LittleEndian::read_i32(&val) as f32),
        CtlType::U32 => make_temp(byteorder::LittleEndian::read_u32(&val) as f32),
        _ => Err(SysctlError::UnknownType),
    }
}

#[cfg(all(test, target_os = "freebsd"))]
mod tests_freebsd {
    use byteorder::WriteBytesExt;

    #[test]
    fn ctl_temperature_ik() {
        let info = crate::CtlInfo {
            ctl_type: crate::CtlType::Int,
            fmt: "IK".into(),
            flags: 0,
        };
        let mut val = vec![];
        // Default value (IK) in deciKelvin integer
        val.write_i32::<byteorder::LittleEndian>(3330)
            .expect("Error parsing value to byte array");

        let t = super::temperature(&info, &val).unwrap();
        let tt = t.as_temperature().unwrap();
        assert!(tt.kelvin() - 333.0 < 0.1);
        assert!(tt.celsius() - 59.85 < 0.1);
        assert!(tt.fahrenheit() - 139.73 < 0.1);
    }

    #[test]
    fn ctl_temperature_ik3() {
        let info = crate::CtlInfo {
            ctl_type: crate::CtlType::Int,
            fmt: "IK3".into(),
            flags: 0,
        };
        let mut val = vec![];
        // Set value in milliKelvin
        val.write_i32::<byteorder::LittleEndian>(333000)
            .expect("Error parsing value to byte array");

        let t = super::temperature(&info, &val).unwrap();
        let tt = t.as_temperature().unwrap();
        assert!(tt.kelvin() - 333.0 < 0.1);
    }
}
