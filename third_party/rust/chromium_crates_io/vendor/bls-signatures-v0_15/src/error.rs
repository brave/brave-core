use thiserror::Error;

#[derive(Debug, Error)]
pub enum Error {
    #[error("Size mismatch")]
    SizeMismatch,
    #[error("Io error: {0}")]
    Io(#[from] std::io::Error),
    #[error("Group decode error")]
    GroupDecode,
    #[error("Curve decode error")]
    CurveDecode,
    #[error("Prime field decode error")]
    FieldDecode,
    #[error("Invalid Private Key")]
    InvalidPrivateKey,
    #[error("Zero sized input")]
    ZeroSizedInput,
}
