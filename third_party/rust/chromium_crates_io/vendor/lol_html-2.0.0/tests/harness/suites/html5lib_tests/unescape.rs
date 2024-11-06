use serde_json::de::from_str as parse_json;
use serde_json::error::Error;

pub trait Unescape {
    fn unescape(&mut self) -> Result<(), Error>;
}

impl Unescape for String {
    // dummy but does the job
    fn unescape(&mut self) -> Result<(), Error> {
        *self = parse_json(&format!(r#""{}""#, self))?;
        Ok(())
    }
}

impl<T: Unescape> Unescape for Option<T> {
    fn unescape(&mut self) -> Result<(), Error> {
        if let Some(ref mut inner) = *self {
            inner.unescape()?;
        }
        Ok(())
    }
}
