use core::fmt;

use crate::WasmFeatures;
use crate::prelude::*;

/// A binary reader for WebAssembly modules.
#[derive(Debug, Clone)]
pub struct Error {
    // Wrap the actual error data in a `Box` so that the error is just one
    // word. This means that we can continue returning small `Result`s in
    // registers.
    pub(crate) inner: Box<ErrorInner>,
}

#[derive(Debug, Clone)]
pub(crate) struct ErrorInner {
    message: String,
    kind: ErrorKind,
    offset: usize,
    needed_hint: Option<usize>,
}

#[derive(Debug, Clone, Copy)]
pub(crate) enum ErrorKind {
    Uncategorized,
    InvalidHeapType,
    WasmFeature(WasmFeatures),
}

/// The result for `BinaryReader` operations.
pub type Result<T, E = Error> = core::result::Result<T, E>;

impl core::error::Error for Error {}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(
            f,
            "{} (at offset 0x{:x})",
            self.inner.message, self.inner.offset
        )
    }
}

impl Error {
    #[cold]
    pub(crate) fn _new(kind: ErrorKind, message: String, offset: usize) -> Self {
        Error {
            inner: Box::new(ErrorInner {
                kind,
                message,
                offset,
                needed_hint: None,
            }),
        }
    }

    #[cold]
    pub(crate) fn new(message: impl Into<String>, offset: usize) -> Self {
        Self::_new(ErrorKind::Uncategorized, message.into(), offset)
    }

    #[cold]
    pub(crate) fn invalid_heap_type(msg: &'static str, offset: usize) -> Self {
        Self::_new(ErrorKind::InvalidHeapType, msg.into(), offset)
    }

    #[cold]
    pub(crate) fn wasm_feature(
        feature: crate::WasmFeatures,
        msg: impl fmt::Display,
        offset: usize,
    ) -> Self {
        Self::_new(ErrorKind::WasmFeature(feature), msg.to_string(), offset)
    }

    #[cold]
    pub(crate) fn fmt(args: fmt::Arguments<'_>, offset: usize) -> Self {
        Error::new(args.to_string(), offset)
    }

    #[cold]
    pub(crate) fn eof(offset: usize, needed_hint: usize) -> Self {
        let mut err = Error::new("unexpected end-of-file", offset);
        err.inner.needed_hint = Some(needed_hint);
        err
    }

    pub(crate) fn kind(&self) -> ErrorKind {
        self.inner.kind
    }

    /// Get this error's message.
    pub fn message(&self) -> &str {
        &self.inner.message
    }

    /// Get the offset within the Wasm binary where the error occurred.
    pub fn offset(&self) -> usize {
        self.inner.offset
    }

    /// Returns `Some` if this error is due to a disabled Wasm feature.
    ///
    /// If the `features` crate feature is enabled, a returned [`WasmFeatures`]
    /// will contain a feature that could be enabled to prevent this error.
    ///
    /// **Note:** This is not comprehensive; `None` may be returned in some
    /// cases where enabling a feature could avoid the error.
    pub fn missing_wasm_feature(&self) -> Option<WasmFeatures> {
        match self.inner.kind {
            ErrorKind::WasmFeature(features) => Some(features),
            _ => None,
        }
    }

    #[cfg(all(feature = "validate", feature = "component-model"))]
    pub(crate) fn add_context(&mut self, context: String) {
        self.inner.message = format!("{context}\n{}", self.inner.message);
    }

    pub(crate) fn set_message(&mut self, message: impl Into<String>) {
        self.inner.message = message.into();
    }

    pub(crate) fn needed_hint(&self) -> Option<usize> {
        self.inner.needed_hint
    }

    pub(crate) fn without_needed_hint(mut self) -> Self {
        self.inner.needed_hint = None;
        self
    }
}
