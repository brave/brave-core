//! Handling wasm constant values

use crate::emit::EmitContext;
use crate::ir::Value;
use crate::parse::IndicesToIds;
use crate::RefType;
use crate::{FunctionId, GlobalId, Result};
use anyhow::bail;

/// A constant which is produced in WebAssembly, typically used in global
/// initializers or element/data offsets.
#[derive(Debug, Copy, Clone)]
pub enum ConstExpr {
    /// An immediate constant value
    Value(Value),
    /// A constant value referenced by the global specified
    Global(GlobalId),
    /// A null reference
    RefNull(RefType),
    /// A function initializer
    RefFunc(FunctionId),
}

impl ConstExpr {
    pub(crate) fn eval(init: &wasmparser::ConstExpr, ids: &IndicesToIds) -> Result<ConstExpr> {
        use wasmparser::Operator::*;
        let mut reader = init.get_operators_reader();
        let val = match reader.read()? {
            I32Const { value } => ConstExpr::Value(Value::I32(value)),
            I64Const { value } => ConstExpr::Value(Value::I64(value)),
            F32Const { value } => ConstExpr::Value(Value::F32(f32::from_bits(value.bits()))),
            F64Const { value } => ConstExpr::Value(Value::F64(f64::from_bits(value.bits()))),
            V128Const { value } => ConstExpr::Value(Value::V128(v128_to_u128(&value))),
            GlobalGet { global_index } => ConstExpr::Global(ids.get_global(global_index)?),
            RefNull { hty } => {
                let val_type = match hty {
                    wasmparser::HeapType::Abstract { shared: _, ty } => match ty {
                        wasmparser::AbstractHeapType::Func => RefType::Funcref,
                        wasmparser::AbstractHeapType::Extern => RefType::Externref,
                        other => bail!(
                            "unsupported abstract heap type in constant expression: {other:?}"
                        ),
                    },
                    wasmparser::HeapType::Concrete(_) => {
                        bail!("unsupported concrete heap type in constant expression")
                    }
                };
                ConstExpr::RefNull(val_type)
            }
            RefFunc { function_index } => ConstExpr::RefFunc(ids.get_func(function_index)?),
            _ => bail!("invalid constant expression"),
        };
        match reader.read()? {
            End => {}
            _ => bail!("invalid constant expression"),
        }
        reader.ensure_end()?;
        Ok(val)
    }

    pub(crate) fn to_wasmencoder_type(&self, cx: &EmitContext) -> wasm_encoder::ConstExpr {
        match self {
            ConstExpr::Value(v) => match v {
                Value::I32(v) => wasm_encoder::ConstExpr::i32_const(*v),
                Value::I64(v) => wasm_encoder::ConstExpr::i64_const(*v),
                Value::F32(v) => wasm_encoder::ConstExpr::f32_const(*v),
                Value::F64(v) => wasm_encoder::ConstExpr::f64_const(*v),
                Value::V128(v) => wasm_encoder::ConstExpr::v128_const(*v as i128),
            },
            ConstExpr::Global(g) => {
                wasm_encoder::ConstExpr::global_get(cx.indices.get_global_index(*g))
            }
            ConstExpr::RefNull(ty) => wasm_encoder::ConstExpr::ref_null(match ty {
                RefType::Externref => wasm_encoder::HeapType::Abstract {
                    shared: false,
                    ty: wasm_encoder::AbstractHeapType::Extern,
                },
                RefType::Funcref => wasm_encoder::HeapType::Abstract {
                    shared: false,
                    ty: wasm_encoder::AbstractHeapType::Func,
                },
            }),
            ConstExpr::RefFunc(f) => {
                wasm_encoder::ConstExpr::ref_func(cx.indices.get_func_index(*f))
            }
        }
    }
}

pub(crate) fn v128_to_u128(value: &wasmparser::V128) -> u128 {
    let n = value.bytes();
    ((n[0] as u128) << 0)
        | ((n[1] as u128) << 8)
        | ((n[2] as u128) << 16)
        | ((n[3] as u128) << 24)
        | ((n[4] as u128) << 32)
        | ((n[5] as u128) << 40)
        | ((n[6] as u128) << 48)
        | ((n[7] as u128) << 56)
        | ((n[8] as u128) << 64)
        | ((n[9] as u128) << 72)
        | ((n[10] as u128) << 80)
        | ((n[11] as u128) << 88)
        | ((n[12] as u128) << 96)
        | ((n[13] as u128) << 104)
        | ((n[14] as u128) << 112)
        | ((n[15] as u128) << 120)
}
