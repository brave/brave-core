pub const TIME_ZONE_ID_INVALID: u32 = 0xffffffff;

// MSRV(??): Must be marked as public to avoid errors in older Rust compilers.
// Rust 1.71 needs this. But Rust 1.95 does not. I'm not sure where the cross
// over point is, but it'd be nice to tighten this up a bit. In any case, this
// isn't actually re-exported anywhere in Jiff's public API, so it's fine.
// ---AG
#[repr(C)]
pub struct DYNAMIC_TIME_ZONE_INFORMATION {
    bias: i32,
    standard_name: [u16; 32],
    standard_date: SYSTEMTIME,
    standard_bias: i32,
    daylight_name: [u16; 32],
    daylight_date: SYSTEMTIME,
    daylight_bias: i32,
    // This is the only field we actually need.
    pub timezone_key_name: [u16; 128],
    dynamic_daylight_time_disabled: u8,
}

#[repr(C)]
struct SYSTEMTIME {
    year: u16,
    month: u16,
    day_of_week: u16,
    day: u16,
    hour: u16,
    minute: u16,
    seconds: u16,
    milliseconds: u16,
}

/// Retrieves the current time zone and dynamic daylight saving time settings.
///
/// These settings control the translations between Coordinated Universal Time
/// (UTC) and local time..
///
/// See: <https://learn.microsoft.com/en-us/windows/win32/api/timezoneapi/nf-timezoneapi-getdynamictimezoneinformation>
windows_link::link!("kernel32.dll" "system" fn GetDynamicTimeZoneInformation(info: *mut DYNAMIC_TIME_ZONE_INFORMATION) -> u32);
