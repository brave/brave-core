use crate::Timestamp;

impl From<js_sys::Date> for Timestamp {
    /// # Panics
    ///
    /// This may panic if the timestamp can not be represented.
    fn from(js_date: js_sys::Date) -> Self {
        // get_time() returns milliseconds
        let timestamp_millis = js_date.get_time() as i64;
        Self::from_milliseconds(timestamp_millis)
            .expect("invalid timestamp: Timestamp cannot fit in range")
    }
}

impl From<Timestamp> for js_sys::Date {
    fn from(datetime: Timestamp) -> Self {
        // new Date() takes milliseconds
        let timestamp = datetime.as_milliseconds() as f64;
        Self::new(&timestamp.into())
    }
}
