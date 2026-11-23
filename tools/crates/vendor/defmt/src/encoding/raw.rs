pub(crate) struct Encoder {
    _private: (),
}

impl Encoder {
    pub(crate) const fn new() -> Self {
        Self { _private: () }
    }

    pub(crate) fn start_frame(&mut self, _write: impl FnMut(&[u8])) {}

    pub(crate) fn end_frame(&mut self, _write: impl FnMut(&[u8])) {}

    pub(crate) fn write(&mut self, data: &[u8], mut write: impl FnMut(&[u8])) {
        write(data)
    }
}
