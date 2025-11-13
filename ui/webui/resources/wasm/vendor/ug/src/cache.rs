//! Compilation cache utilities.
use crate::lang::op::{self, ArgId, Ast};
use crate::Result;
use std::collections::HashMap;

type Ssa = Vec<crate::lang::ssa::Instr>;

#[derive(Debug, Hash, PartialEq, Eq)]
pub struct NormalizedKernel {
    pub(crate) args: Vec<op::Arg>,
    pub(crate) ops: Vec<op::Store>,
}

impl NormalizedKernel {
    pub fn new(k: &op::Kernel) -> Result<Self> {
        fn walk(ast: &Ast, arg_map: &HashMap<ArgId, ArgId>) -> Result<Ast> {
            use op::AstInner as A;
            match ast.inner.as_ref() {
                A::Id { .. } => crate::bail!("unexpected id node"),
                A::Load { src, layout } => {
                    let src = match arg_map.get(src) {
                        None => crate::bail!("BUG: missing arg id {src:?}"),
                        Some(id) => *id,
                    };
                    op::load(src, layout.clone(), ast.dtype)
                }
                A::Unary { op, arg } => {
                    let arg = walk(arg, arg_map)?;
                    op::unary(*op, arg)
                }
                A::Binary { op, lhs, rhs } => {
                    let lhs = walk(lhs, arg_map)?;
                    let rhs = walk(rhs, arg_map)?;
                    op::binary(*op, lhs, rhs)
                }
                A::Const(cst) => op::cst(*cst),
                A::Reduce { op, arg, dim } => {
                    let arg = walk(arg, arg_map)?;
                    op::reduce(*op, arg, *dim)
                }
                A::Layout { arg, op } => {
                    let arg = walk(arg, arg_map)?;
                    let inner = A::Layout { arg, op: op.clone() };
                    Ok(Ast {
                        inner: std::sync::Arc::new(inner),
                        dtype: ast.dtype(),
                        shape: ast.shape().clone(),
                    })
                }
            }
        }

        let mut arg_map = HashMap::new();
        let mut args = Vec::with_capacity(k.args.len());
        let mut ops = Vec::with_capacity(k.ops.len());
        for (id, arg) in k.args.iter().enumerate() {
            let id = ArgId::from_usize(id);
            arg_map.insert(arg.id(), id);
            args.push(op::Arg::new(id, arg.type_()));
        }
        for op in k.ops.iter() {
            let op::Store { dst, layout, value } = op;
            let dst = match arg_map.get(dst) {
                None => crate::bail!("BUG: missing arg id {dst:?}"),
                Some(id) => *id,
            };
            let value = walk(value, &arg_map)?;
            ops.push(op::store(dst, layout.clone(), value)?)
        }
        Ok(Self { args, ops })
    }
}

pub struct CompilationCache<D: crate::Device> {
    op_cache: HashMap<NormalizedKernel, std::sync::Arc<D::Func>>,
    ssa_cache: HashMap<Ssa, std::sync::Arc<D::Func>>,
}

impl<D: crate::Device> Default for CompilationCache<D> {
    fn default() -> Self {
        Self { op_cache: Default::default(), ssa_cache: Default::default() }
    }
}

impl<D: crate::Device> CompilationCache<D> {
    pub fn get(&self, kernel: &NormalizedKernel) -> Option<std::sync::Arc<D::Func>> {
        self.op_cache.get(kernel).cloned()
    }

    pub fn insert(&mut self, kernel: NormalizedKernel, func: std::sync::Arc<D::Func>) {
        self.op_cache.insert(kernel, func);
    }

    pub fn get_ssa(&self, kernel: &Ssa) -> Option<std::sync::Arc<D::Func>> {
        self.ssa_cache.get(kernel).cloned()
    }

    pub fn insert_ssa(&mut self, kernel: Ssa, func: std::sync::Arc<D::Func>) {
        self.ssa_cache.insert(kernel, func);
    }
}
