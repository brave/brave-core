//! WebAssembly function and value types.

use crate::error::Result;
use crate::tombstone_arena::Tombstone;
use anyhow::bail;
use id_arena::Id;
use std::cmp::Ordering;
use std::convert::TryFrom;
use std::fmt;
use std::hash;

/// An identifier for types.
pub type TypeId = Id<Type>;

/// A function type.
#[derive(Debug, Clone)]
pub struct Type {
    id: TypeId,
    params: Box<[ValType]>,
    results: Box<[ValType]>,

    // Whether or not this type is for a multi-value function entry block, and
    // therefore is for internal use only and shouldn't be emitted when we
    // serialize the Type section.
    is_for_function_entry: bool,

    /// An optional name for debugging.
    ///
    /// This is not really used by anything currently, but a theoretical WAT to
    /// walrus parser could keep track of the original name in the WAT.
    pub name: Option<String>,
}

impl PartialEq for Type {
    #[inline]
    fn eq(&self, rhs: &Type) -> bool {
        // NB: do not compare id or name.
        self.params == rhs.params
            && self.results == rhs.results
            && self.is_for_function_entry == rhs.is_for_function_entry
    }
}

impl Eq for Type {}

impl PartialOrd for Type {
    fn partial_cmp(&self, rhs: &Type) -> Option<Ordering> {
        Some(self.cmp(rhs))
    }
}

impl Ord for Type {
    fn cmp(&self, rhs: &Type) -> Ordering {
        self.params()
            .cmp(rhs.params())
            .then_with(|| self.results().cmp(rhs.results()))
    }
}

impl hash::Hash for Type {
    #[inline]
    fn hash<H: hash::Hasher>(&self, h: &mut H) {
        // Do not hash id or name.
        self.params.hash(h);
        self.results.hash(h);
        self.is_for_function_entry.hash(h);
    }
}

impl Tombstone for Type {
    fn on_delete(&mut self) {
        self.params = Box::new([]);
        self.results = Box::new([]);
    }
}

impl Type {
    /// Construct a new function type.
    #[inline]
    pub(crate) fn new(id: TypeId, params: Box<[ValType]>, results: Box<[ValType]>) -> Type {
        Type {
            id,
            params,
            results,
            is_for_function_entry: false,
            name: None,
        }
    }

    /// Construct a new type for function entry blocks.
    #[inline]
    pub(crate) fn for_function_entry(id: TypeId, results: Box<[ValType]>) -> Type {
        let params = vec![].into();
        Type {
            id,
            params,
            results,
            is_for_function_entry: true,
            name: None,
        }
    }

    /// Get the id of this type.
    #[inline]
    pub fn id(&self) -> TypeId {
        self.id
    }

    /// Get the parameters to this function type.
    #[inline]
    pub fn params(&self) -> &[ValType] {
        &self.params
    }

    /// Get the results of this function type.
    #[inline]
    pub fn results(&self) -> &[ValType] {
        &self.results
    }

    pub(crate) fn is_for_function_entry(&self) -> bool {
        self.is_for_function_entry
    }
}

/// A value type.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, PartialOrd, Ord)]
pub enum ValType {
    /// 32-bit integer.
    I32,
    /// 64-bit integer.
    I64,
    /// 32-bit float.
    F32,
    /// 64-bit float.
    F64,
    /// 128-bit vector.
    V128,
    /// Reference.
    Ref(RefType),
}

/// A reference type.
///
/// The "function references" and "gc" proposals will add more reference types.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, PartialOrd, Ord)]
#[non_exhaustive]
pub enum RefType {
    /// A nullable reference to an untyped function
    Funcref,
    /// A nullable reference to an extern object
    Externref,
}

impl TryFrom<wasmparser::RefType> for RefType {
    type Error = anyhow::Error;

    fn try_from(ref_type: wasmparser::RefType) -> Result<RefType> {
        match ref_type {
            wasmparser::RefType::FUNCREF => Ok(RefType::Funcref),
            wasmparser::RefType::EXTERNREF => Ok(RefType::Externref),
            _ => bail!("unsupported ref type {:?}", ref_type),
        }
    }
}

impl ValType {
    pub(crate) fn from_wasmparser_type(ty: wasmparser::ValType) -> Result<Box<[ValType]>> {
        let v = vec![ValType::parse(&ty)?];
        Ok(v.into_boxed_slice())
    }

    pub(crate) fn to_wasmencoder_type(&self) -> wasm_encoder::ValType {
        match self {
            ValType::I32 => wasm_encoder::ValType::I32,
            ValType::I64 => wasm_encoder::ValType::I64,
            ValType::F32 => wasm_encoder::ValType::F32,
            ValType::F64 => wasm_encoder::ValType::F64,
            ValType::V128 => wasm_encoder::ValType::V128,
            ValType::Ref(ref_type) => match ref_type {
                RefType::Externref => wasm_encoder::ValType::Ref(wasm_encoder::RefType::EXTERNREF),
                RefType::Funcref => wasm_encoder::ValType::Ref(wasm_encoder::RefType::FUNCREF),
            },
        }
    }

    pub(crate) fn parse(input: &wasmparser::ValType) -> Result<ValType> {
        match input {
            wasmparser::ValType::I32 => Ok(ValType::I32),
            wasmparser::ValType::I64 => Ok(ValType::I64),
            wasmparser::ValType::F32 => Ok(ValType::F32),
            wasmparser::ValType::F64 => Ok(ValType::F64),
            wasmparser::ValType::V128 => Ok(ValType::V128),
            wasmparser::ValType::Ref(ref_type) => match *ref_type {
                wasmparser::RefType::EXTERNREF => Ok(ValType::Ref(RefType::Externref)),
                wasmparser::RefType::FUNCREF => Ok(ValType::Ref(RefType::Funcref)),
                _ => bail!("unsupported ref type {:?}", ref_type),
            },
        }
    }
}

impl fmt::Display for ValType {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(
            f,
            "{}",
            match self {
                ValType::I32 => "i32",
                ValType::I64 => "i64",
                ValType::F32 => "f32",
                ValType::F64 => "f64",
                ValType::V128 => "v128",
                ValType::Ref(RefType::Externref) => "externref",
                ValType::Ref(RefType::Funcref) => "funcref",
            }
        )
    }
}
