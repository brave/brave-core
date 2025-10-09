/// Main library error type.
#[derive(thiserror::Error, Debug)]
pub enum Error {
    /// Integer parse error.
    #[error(transparent)]
    ParseInt(#[from] std::num::ParseIntError),

    /// I/O error.
    #[error(transparent)]
    Io(#[from] std::io::Error),

    #[error(transparent)]
    FromUtf8(#[from] std::string::FromUtf8Error),

    #[error(transparent)]
    BorrowMutError(#[from] std::cell::BorrowMutError),

    #[error(transparent)]
    BorrowError(#[from] std::cell::BorrowError),

    #[error(transparent)]
    LibLoading(#[from] libloading::Error),

    /// Arbitrary errors wrapping.
    #[error(transparent)]
    Wrapped(Box<dyn std::error::Error + Send + Sync>),

    /// Adding path information to an error.
    #[error("path: {path:?} {inner}")]
    WithPath { inner: Box<Self>, path: std::path::PathBuf },

    #[error("{inner}\n{backtrace}")]
    WithBacktrace { inner: Box<Self>, backtrace: Box<std::backtrace::Backtrace> },

    /// User generated error message, typically created via `bail!`.
    #[error("{0}")]
    Msg(String),

    // Box indirection to avoid large variant.
    #[error("{0:?}")]
    MatMulUnexpectedStriding(Box<MatMulUnexpectedStriding>),

    #[error("cannot find tensor {path}")]
    CannotFindTensor { path: String },

    /// SafeTensor error.
    #[error(transparent)]
    SafeTensor(#[from] safetensors::SafeTensorError),

    #[error("unsupported safetensor dtype {0:?}")]
    UnsupportedSafeTensorDtype(safetensors::Dtype),
}

pub type Result<T> = std::result::Result<T, Error>;

impl Error {
    pub fn wrap(err: impl std::error::Error + Send + Sync + 'static) -> Self {
        Self::Wrapped(Box::new(err)).bt()
    }

    pub fn msg(err: impl std::error::Error) -> Self {
        Self::Msg(err.to_string()).bt()
    }

    pub fn debug(err: impl std::fmt::Debug) -> Self {
        Self::Msg(format!("{err:?}")).bt()
    }

    pub fn bt(self) -> Self {
        let backtrace = std::backtrace::Backtrace::capture();
        match backtrace.status() {
            std::backtrace::BacktraceStatus::Disabled
            | std::backtrace::BacktraceStatus::Unsupported => self,
            _ => Self::WithBacktrace { inner: Box::new(self), backtrace: Box::new(backtrace) },
        }
    }

    pub fn with_path<P: AsRef<std::path::Path>>(self, p: P) -> Self {
        Self::WithPath { inner: Box::new(self), path: p.as_ref().to_path_buf() }
    }
}

impl<T> From<std::sync::PoisonError<T>> for Error {
    fn from(_value: std::sync::PoisonError<T>) -> Self {
        Self::Msg("poisoned mutex".into())
    }
}

#[macro_export]
macro_rules! bail {
    ($msg:literal $(,)?) => {
        return Err($crate::Error::Msg(format!($msg).into()).bt())
    };
    ($err:expr $(,)?) => {
        return Err($crate::Error::Msg(format!($err).into()).bt())
    };
    ($fmt:expr, $($arg:tt)*) => {
        return Err($crate::Error::Msg(format!($fmt, $($arg)*).into()).bt())
    };
}

pub fn zip<T, U>(r1: Result<T>, r2: Result<U>) -> Result<(T, U)> {
    match (r1, r2) {
        (Ok(r1), Ok(r2)) => Ok((r1, r2)),
        (Err(e), _) => Err(e),
        (_, Err(e)) => Err(e),
    }
}

#[derive(Debug, Clone)]
pub struct MatMulUnexpectedStriding {
    pub lhs_l: crate::Layout,
    pub rhs_l: crate::Layout,
    pub bmnk: (usize, usize, usize, usize),
    pub msg: &'static str,
}
