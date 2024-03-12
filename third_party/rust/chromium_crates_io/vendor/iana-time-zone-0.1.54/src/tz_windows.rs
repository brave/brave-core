use windows::Globalization::Calendar;

impl From<windows::core::Error> for crate::GetTimezoneError {
    fn from(orig: windows::core::Error) -> Self {
        crate::GetTimezoneError::IoError(std::io::Error::new(std::io::ErrorKind::Other, orig))
    }
}

pub(crate) fn get_timezone_inner() -> Result<String, crate::GetTimezoneError> {
    let cal = Calendar::new()?;
    let tz_hstring = cal.GetTimeZone()?;
    Ok(tz_hstring.to_string())
}
