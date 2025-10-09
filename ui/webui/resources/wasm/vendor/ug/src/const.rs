use crate::DType;
use half::{bf16, f16};

// Wrappers around bf16/f16/f32 that forbid nans and implement eq/hash/ord.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct Bf16(bf16);

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct F16(f16);

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct F32(f32);

impl std::cmp::Eq for Bf16 {}
impl std::cmp::Ord for Bf16 {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        self.0.partial_cmp(&other.0).unwrap()
    }
}
impl std::cmp::PartialOrd for Bf16 {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.cmp(other))
    }
}
impl std::hash::Hash for Bf16 {
    fn hash<H: std::hash::Hasher>(&self, state: &mut H) {
        self.0.to_bits().hash(state)
    }
}
impl std::ops::Deref for Bf16 {
    type Target = bf16;
    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

impl std::cmp::Eq for F16 {}
impl std::cmp::Ord for F16 {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        self.0.partial_cmp(&other.0).unwrap()
    }
}
impl std::cmp::PartialOrd for F16 {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.cmp(other))
    }
}
impl std::hash::Hash for F16 {
    fn hash<H: std::hash::Hasher>(&self, state: &mut H) {
        self.0.to_bits().hash(state)
    }
}
impl std::ops::Deref for F16 {
    type Target = f16;
    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

impl std::cmp::Eq for F32 {}
impl std::cmp::Ord for F32 {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        self.0.partial_cmp(&other.0).unwrap()
    }
}
impl std::cmp::PartialOrd for F32 {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.cmp(other))
    }
}
impl std::hash::Hash for F32 {
    fn hash<H: std::hash::Hasher>(&self, state: &mut H) {
        self.0.to_bits().hash(state)
    }
}
impl std::ops::Deref for F32 {
    type Target = f32;
    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

#[derive(Debug, Clone, Copy, PartialEq, PartialOrd, Ord, Eq, Hash)]
pub enum Const {
    I32(i32),
    I64(i64),
    BF16(Bf16),
    F16(F16),
    F32(F32),
}

impl TryFrom<bf16> for Const {
    type Error = crate::Error;
    fn try_from(value: bf16) -> Result<Self, Self::Error> {
        if value.is_nan() {
            crate::bail!("cannot use nan in const")
        }
        Ok(Self::BF16(Bf16(value)))
    }
}

impl TryFrom<f16> for Const {
    type Error = crate::Error;
    fn try_from(value: f16) -> Result<Self, Self::Error> {
        if value.is_nan() {
            crate::bail!("cannot use nan in const")
        }
        Ok(Self::F16(F16(value)))
    }
}

impl From<i64> for Const {
    fn from(value: i64) -> Self {
        Self::I64(value)
    }
}

impl From<i32> for Const {
    fn from(value: i32) -> Self {
        Self::I32(value)
    }
}

impl TryFrom<f32> for Const {
    type Error = crate::Error;
    fn try_from(value: f32) -> Result<Self, Self::Error> {
        if value.is_nan() {
            crate::bail!("cannot use nan in const")
        }
        Ok(Self::F32(F32(value)))
    }
}

impl Const {
    pub fn dtype(&self) -> DType {
        match self {
            Self::I32(_) => DType::I32,
            Self::I64(_) => DType::I64,
            Self::BF16(_) => DType::BF16,
            Self::F16(_) => DType::F16,
            Self::F32(_) => DType::F32,
        }
    }

    pub fn zero(dtype: DType) -> Self {
        match dtype {
            DType::F16 => Self::F16(F16(f16::ZERO)),
            DType::BF16 => Self::BF16(Bf16(bf16::ZERO)),
            DType::F32 => Self::F32(F32(0f32)),
            DType::I32 => Self::I32(0i32),
            DType::I64 => Self::I64(0i64),
        }
    }

    pub fn min_value(dtype: DType) -> Self {
        match dtype {
            DType::F16 => Self::F16(F16(f16::NEG_INFINITY)),
            DType::BF16 => Self::BF16(Bf16(bf16::NEG_INFINITY)),
            DType::F32 => Self::F32(F32(f32::NEG_INFINITY)),
            DType::I32 => Self::I32(i32::MIN),
            DType::I64 => Self::I64(i64::MIN),
        }
    }

    pub fn max_value(dtype: DType) -> Self {
        match dtype {
            DType::F16 => Self::F16(F16(f16::INFINITY)),
            DType::BF16 => Self::BF16(Bf16(bf16::INFINITY)),
            DType::F32 => Self::F32(F32(f32::INFINITY)),
            DType::I32 => Self::I32(i32::MAX),
            DType::I64 => Self::I64(i64::MAX),
        }
    }
}
