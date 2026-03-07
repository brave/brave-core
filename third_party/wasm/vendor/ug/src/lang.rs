use crate::Result;
pub use crate::{Const, DType};
use std::sync::Arc;

#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub enum ReduceOp {
    Sum,
    Max,
    Min,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub enum UnaryOp {
    Id,
    Exp,
    Sin,
    Cos,
    Neg,
    Sqrt,
    Cast(DType),
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub enum BinaryOp {
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    Min,
    Max,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct LaunchConfig {
    pub grid_dim: u32,
    pub block_dim: u32,
    pub shared_mem: u32,
}

impl LaunchConfig {
    pub fn for_num_elems(n: u32) -> Self {
        LaunchConfig { grid_dim: n.div_ceil(32), block_dim: 32, shared_mem: 0 }
    }
}

impl BinaryOp {
    fn as_str(&self) -> &'static str {
        match self {
            Self::Add => "add",
            Self::Sub => "sub",
            Self::Mul => "mul",
            Self::Div => "div",
            Self::Min => "min",
            Self::Max => "max",
            Self::Mod => "mod",
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub enum Type {
    Value(DType),
    Ptr(DType),
}

impl From<DType> for Type {
    fn from(value: DType) -> Self {
        Self::Value(value)
    }
}

impl Type {
    pub fn size_in_bytes(&self) -> usize {
        match self {
            Self::Ptr(_) => 8,
            Self::Value(v) => v.size_in_bytes(),
        }
    }
}

/// Unique identifier for arguments.
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct ArgId(usize);

impl ArgId {
    #[allow(clippy::new_without_default)]
    pub fn new() -> Self {
        // https://users.rust-lang.org/t/idiomatic-rust-way-to-generate-unique-id/33805
        use std::sync::atomic;
        static COUNTER: atomic::AtomicUsize = atomic::AtomicUsize::new(1);
        Self(COUNTER.fetch_add(1, atomic::Ordering::Relaxed))
    }

    pub fn from_usize(v: usize) -> Self {
        Self(v)
    }

    pub fn as_usize(&self) -> usize {
        self.0
    }
}

/// Unique identifier for nodes.
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct NodeId(usize);

impl NodeId {
    fn new() -> Self {
        // https://users.rust-lang.org/t/idiomatic-rust-way-to-generate-unique-id/33805
        use std::sync::atomic;
        static COUNTER: atomic::AtomicUsize = atomic::AtomicUsize::new(1);
        Self(COUNTER.fetch_add(1, atomic::Ordering::Relaxed))
    }

    pub fn as_usize(&self) -> usize {
        self.0
    }
}

/// Unique identifier for index nodes.
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct IndexNodeId(usize);

impl IndexNodeId {
    fn new() -> Self {
        // https://users.rust-lang.org/t/idiomatic-rust-way-to-generate-unique-id/33805
        use std::sync::atomic;
        static COUNTER: atomic::AtomicUsize = atomic::AtomicUsize::new(1);
        Self(COUNTER.fetch_add(1, atomic::Ordering::Relaxed))
    }

    pub fn as_usize(&self) -> usize {
        self.0
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct Arg {
    id: ArgId,
    type_: Type,
}

impl PartialOrd for Arg {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.cmp(other))
    }
}

impl Ord for Arg {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        self.id.cmp(&other.id)
    }
}

impl Arg {
    pub fn new(id: ArgId, type_: Type) -> Self {
        Self { id, type_ }
    }

    pub fn value(dtype: DType) -> Self {
        let id = ArgId::new();
        Self { id, type_: Type::Value(dtype) }
    }

    pub fn ptr(dtype: DType) -> Self {
        let id = ArgId::new();
        Self { id, type_: Type::Ptr(dtype) }
    }

    pub fn id(&self) -> ArgId {
        self.id
    }

    pub fn type_(&self) -> Type {
        self.type_
    }
}

#[derive(Debug, Clone)]
pub enum IndexExpr {
    ProgramId,
    Const(usize),
    Add(IndexExprNode, IndexExprNode),
    Mul(IndexExprNode, IndexExprNode),
}

#[derive(Debug, Clone)]
pub(crate) struct IndexExprNodeInner {
    #[allow(unused)]
    pub(crate) id: IndexNodeId,
    pub(crate) expr: IndexExpr,
}

#[derive(Debug, Clone)]
pub struct IndexExprNode {
    pub(crate) inner: Arc<IndexExprNodeInner>,
}

#[derive(Debug, Clone)]
pub(crate) struct ExprNodeInner {
    #[allow(unused)]
    pub(crate) id: NodeId,
    pub(crate) expr: Expr,
    pub(crate) dtype: DType,
}

impl IndexExprNode {
    fn from_expr(expr: IndexExpr) -> Self {
        let id = IndexNodeId::new();
        let inner = IndexExprNodeInner { id, expr };
        Self { inner: Arc::new(inner) }
    }

    pub fn cst(v: usize) -> Self {
        Self::from_expr(IndexExpr::Const(v))
    }

    pub fn program_id() -> Self {
        Self::from_expr(IndexExpr::ProgramId)
    }

    pub fn add(&self, rhs: &Self) -> Self {
        Self::from_expr(IndexExpr::Add(self.clone(), rhs.clone()))
    }

    pub fn mul(&self, rhs: &Self) -> Self {
        Self::from_expr(IndexExpr::Mul(self.clone(), rhs.clone()))
    }

    pub fn as_const(&self) -> Option<usize> {
        match &self.inner.as_ref().expr {
            IndexExpr::Const(c) => Some(*c),
            IndexExpr::ProgramId => None,
            IndexExpr::Add(lhs, rhs) => lhs.as_const().zip(rhs.as_const()).map(|(u, v)| u + v),
            IndexExpr::Mul(lhs, rhs) => lhs.as_const().zip(rhs.as_const()).map(|(u, v)| u * v),
        }
    }
}

#[derive(Debug, Clone)]
pub struct ExprNode {
    pub(crate) inner: Arc<ExprNodeInner>,
}

#[derive(Debug, Clone)]
pub struct StridedSlice {
    ptr: Arg,
    offset: IndexExprNode,
    len: IndexExprNode,
    stride: IndexExprNode,
}

impl StridedSlice {
    pub fn ptr(&self) -> &Arg {
        &self.ptr
    }
    pub fn offset(&self) -> &IndexExprNode {
        &self.offset
    }
    pub fn len(&self) -> &IndexExprNode {
        &self.len
    }
    pub fn stride(&self) -> &IndexExprNode {
        &self.stride
    }
}

#[derive(Debug, Clone)]
pub enum Expr {
    Const(Const),
    Range(usize, usize),
    Load(StridedSlice),
    Unary(UnaryOp, ExprNode),
    Binary(BinaryOp, ExprNode, ExprNode),
}

// Language in the style of triton.
#[derive(Debug, Clone)]
pub enum Ops {
    Store { dst: StridedSlice, src: ExprNode },
}

impl ExprNode {
    pub fn dtype(&self) -> DType {
        self.inner.dtype
    }

    fn from_expr(expr: Expr, dtype: DType) -> Self {
        let id = NodeId::new();
        let inner = ExprNodeInner { id, expr, dtype };
        Self { inner: Arc::new(inner) }
    }

    pub fn cst<C: Into<Const>>(c: C) -> Self {
        let c = c.into();
        Self::from_expr(Expr::Const(c), c.dtype())
    }

    pub fn load(
        ptr: &Arg,
        offset: &IndexExprNode,
        len: &IndexExprNode,
        stride: &IndexExprNode,
    ) -> Result<Self> {
        let dtype = match ptr.type_() {
            ssa::Type::Ptr(v) => v,
            ssa::Type::Value(_) => crate::bail!("non-pointer arguments are not supported yet"),
        };
        let ss = StridedSlice {
            ptr: *ptr,
            offset: offset.clone(),
            len: len.clone(),
            stride: stride.clone(),
        };
        Ok(Self::from_expr(Expr::Load(ss), dtype))
    }

    pub fn unary(&self, op: UnaryOp) -> Self {
        Self::from_expr(Expr::Unary(op, self.clone()), self.dtype())
    }

    pub fn binary(&self, rhs: &Self, op: BinaryOp) -> Self {
        let dtype = self.dtype();
        Self::from_expr(Expr::Binary(op, self.clone(), rhs.clone()), dtype)
    }

    pub fn add(&self, rhs: &Self) -> Self {
        let dtype = self.dtype();
        Self::from_expr(Expr::Binary(BinaryOp::Add, self.clone(), rhs.clone()), dtype)
    }

    pub fn mul(&self, rhs: &Self) -> Self {
        let dtype = self.dtype();
        Self::from_expr(Expr::Binary(BinaryOp::Mul, self.clone(), rhs.clone()), dtype)
    }

    pub fn all_args(&self, args: &mut std::collections::BTreeSet<Arg>) {
        match &self.inner.expr {
            Expr::Load(ss) => {
                args.insert(*ss.ptr());
            }
            Expr::Binary(_, lhs, rhs) => {
                lhs.all_args(args);
                rhs.all_args(args);
            }
            Expr::Unary(_, e) => e.all_args(args),
            Expr::Range(..) | Expr::Const(_) => {}
        }
    }
}

impl Ops {
    pub fn store(
        dst_ptr: &Arg,
        dst_offset: &IndexExprNode,
        dst_len: &IndexExprNode,
        dst_stride: &IndexExprNode,
        src: &ExprNode,
    ) -> Self {
        let dst = StridedSlice {
            ptr: *dst_ptr,
            offset: dst_offset.clone(),
            len: dst_len.clone(),
            stride: dst_stride.clone(),
        };
        Self::Store { dst, src: src.clone() }
    }

    pub fn src(&self) -> &ExprNode {
        match self {
            Self::Store { dst: _, src } => src,
        }
    }

    pub fn dst(&self) -> &StridedSlice {
        match self {
            Self::Store { dst, src: _ } => dst,
        }
    }

    pub fn all_args(&self, args: &mut std::collections::BTreeSet<Arg>) {
        args.insert(*self.dst().ptr());
        self.src().all_args(args);
    }
}

#[derive(Debug, Clone)]
pub struct Kernel {
    #[allow(unused)]
    pub(crate) name: String,
    pub(crate) args: Vec<Arg>,
    pub(crate) ops: Vec<Ops>,
}

impl Kernel {
    pub fn new(name: String, args: Vec<Arg>, ops: Vec<Ops>) -> Self {
        Self { name, args, ops }
    }
}

#[derive(Debug, Clone, Copy)]
pub struct FlopsMem {
    pub flops: usize,
    pub mem_in_bytes: usize,
}

// AST version of the SSA ops
pub mod op {
    pub use super::{Arg, ArgId, BinaryOp, Const, DType, ReduceOp, UnaryOp};
    use crate::Result;
    pub use crate::{Layout, Shape};
    use std::sync::Arc;

    #[derive(Debug, Clone, PartialEq, PartialOrd, Ord, Eq, Hash)]
    pub struct Ast {
        pub(crate) inner: Arc<AstInner>,
        pub(crate) dtype: DType,
        pub(crate) shape: Shape,
    }

    #[derive(Debug, Clone, PartialEq, PartialOrd, Ord, Eq, Hash)]
    pub enum LayoutOp {
        Broadcast { inserted_dims: usize, broadcasted_dims: Vec<usize> },
        Narrow { dim: usize, offset: usize },
        Transpose { dim1: usize, dim2: usize },
        SplitDim { dim: usize, lhs: usize, rhs: usize },
        MergeDims { dim: usize, lhs: usize, rhs: usize },
    }

    #[derive(Debug, Clone, PartialEq, PartialOrd, Ord, Eq, Hash)]
    pub enum AstInner {
        // Id nodes are used to share common parts when linearizing the code. Maybe this should be
        // part of a separate type.
        Id { src: crate::block::Id },
        Load { src: ArgId, layout: Layout },
        Reduce { op: ReduceOp, arg: Ast, dim: usize },
        Unary { op: UnaryOp, arg: Ast },
        Binary { op: BinaryOp, lhs: Ast, rhs: Ast },
        Const(Const),

        // Layout operations
        Layout { op: LayoutOp, arg: Ast },
    }

    pub fn load(src: ArgId, layout: Layout, dtype: DType) -> Result<Ast> {
        let shape = layout.shape().clone();
        let inner = AstInner::Load { src, layout };
        Ok(Ast { inner: Arc::new(inner), dtype, shape })
    }

    pub fn broadcast<S: Into<Shape>>(arg: Ast, shape: S) -> Result<Ast> {
        let shape: Shape = shape.into();
        let dtype = arg.dtype;
        if arg.shape.rank() > shape.rank() {
            crate::bail!(
                "target shape {:?} has less dimensions than original shape {:?}",
                shape,
                arg.shape
            )
        }
        let inserted_dims = shape.rank() - arg.shape.rank();
        let mut broadcasted_dims = (0..inserted_dims).collect::<Vec<_>>();
        for (dim_idx, &dim_len) in arg.shape.dims().iter().enumerate() {
            let dim_idx = dim_idx + inserted_dims;
            if shape.dims()[dim_idx] != dim_len {
                if dim_len == 1 {
                    broadcasted_dims.push(dim_idx)
                } else {
                    crate::bail!("cannot broadcast from {:?} to {shape:?}", arg.shape)
                }
            }
        }
        let op = LayoutOp::Broadcast { inserted_dims, broadcasted_dims };
        let inner = AstInner::Layout { op, arg };
        Ok(Ast { inner: Arc::new(inner), dtype, shape })
    }

    pub fn layout<S: Into<Shape>>(op: LayoutOp, arg: Ast, shape: S) -> Result<Ast> {
        let shape = shape.into();
        let dtype = arg.dtype;
        let inner = AstInner::Layout { op, arg };
        Ok(Ast { inner: Arc::new(inner), dtype, shape })
    }

    pub fn unary(op: UnaryOp, arg: Ast) -> Result<Ast> {
        let dtype = match op {
            UnaryOp::Cast(dtype) => dtype,
            _ => arg.dtype,
        };
        let shape = arg.shape.clone();
        let inner = AstInner::Unary { op, arg };
        Ok(Ast { inner: Arc::new(inner), dtype, shape })
    }

    pub fn reduce(op: ReduceOp, arg: Ast, dim: usize) -> Result<Ast> {
        let dtype = arg.dtype;
        let mut shape = arg.shape.dims().to_vec();
        if dim >= shape.len() {
            crate::bail!("no dim {dim} in shape {shape:?}")
        }
        shape[dim] = 1; // keepdim by default.
        let inner = AstInner::Reduce { op, arg, dim };
        Ok(Ast { inner: Arc::new(inner), dtype, shape: Shape::from(shape) })
    }

    pub fn binary(op: BinaryOp, lhs: Ast, rhs: Ast) -> Result<Ast> {
        let dtype = if lhs.dtype != rhs.dtype {
            crate::bail!("dtype mismatch in {op:?}, lhs: {:?}, rhs: {:?}", lhs.dtype, rhs.dtype)
        } else {
            lhs.dtype
        };
        // Broadcasting has to be done explicitely except for scalar values.
        if lhs.shape != rhs.shape && lhs.shape.rank() != 0 && rhs.shape.rank() != 0 {
            crate::bail!("shape mismatch in {op:?}, lhs: {:?}, rhs: {:?}", lhs.shape, rhs.shape)
        }
        let shape = lhs.shape.clone();
        let inner = AstInner::Binary { op, lhs, rhs };
        Ok(Ast { inner: Arc::new(inner), dtype, shape })
    }

    pub fn broadcast_binary(op: BinaryOp, lhs: Ast, rhs: Ast) -> Result<Ast> {
        let shape = lhs.shape.broadcast_shape_binary_op(rhs.shape(), op.as_str())?;
        let lhs = broadcast(lhs, shape.clone())?;
        let rhs = broadcast(rhs, shape)?;
        binary(op, lhs, rhs)
    }

    pub fn cst<I: TryInto<Const> + std::fmt::Debug + Copy>(v: I) -> Result<Ast> {
        let v: Const = match v.try_into() {
            Err(_) => crate::bail!("unable to create const for {v:?}"),
            Ok(v) => v,
        };
        let dtype = v.dtype();
        let inner = AstInner::Const(v);
        Ok(Ast { inner: Arc::new(inner), dtype, shape: Shape::from(()) })
    }

    impl Ast {
        pub fn dtype(&self) -> DType {
            self.dtype
        }

        pub fn shape(&self) -> &Shape {
            &self.shape
        }

        // We use a BTreeSet rather than a HashSet here to ensure consistent ordering
        // when converting the result to a Vec.
        pub fn arg_ids(&self) -> std::collections::BTreeSet<ArgId> {
            fn visit(ast: &Ast, ids: &mut std::collections::BTreeSet<ArgId>) {
                match ast.inner.as_ref() {
                    AstInner::Layout { arg, .. }
                    | AstInner::Reduce { op: _, dim: _, arg }
                    | AstInner::Unary { op: _, arg } => visit(arg, ids),
                    AstInner::Binary { op: _, lhs, rhs } => {
                        visit(lhs, ids);
                        visit(rhs, ids)
                    }
                    AstInner::Load { src, layout: _ } => {
                        ids.insert(*src);
                    }
                    AstInner::Id { .. } | AstInner::Const(_) => {}
                }
            }
            let mut ids = std::collections::BTreeSet::new();
            visit(self, &mut ids);
            ids
        }
    }

    #[derive(Debug, Clone, PartialEq, Eq, Hash)]
    pub struct Store {
        pub(crate) dst: ArgId,
        pub(crate) layout: Layout,
        pub(crate) value: Ast,
    }

    pub fn store(dst: ArgId, layout: Layout, value: Ast) -> Result<Store> {
        Ok(Store { dst, layout, value })
    }

    impl Store {
        pub fn dtype(&self) -> DType {
            self.value.dtype
        }
    }

    #[derive(Debug, Clone, PartialEq, Eq, Hash)]
    pub struct Kernel {
        name: String,
        pub(crate) args: Vec<Arg>,
        pub(crate) ops: Vec<Store>,
    }

    impl Kernel {
        pub fn new(name: String, args: Vec<Arg>, ops: Vec<Store>) -> Self {
            Self { name, args, ops }
        }

        pub fn name(&self) -> &str {
            self.name.as_str()
        }
    }
}

// Very untyped almost SSA language.
// There are no phi symbols, instead Range is used.
// This is currently close to the UOps setup from tinygrad:
// https://github.com/tinygrad/tinygrad/blob/13846930cd43b1cfd8f7bb2967529f08c08cb6d6/tinygrad/ops.py#L98
pub mod ssa {
    use super::LaunchConfig;
    use crate::Result;

    pub use super::{Arg, ArgId, BinaryOp, Const, ReduceOp, Type, UnaryOp};
    pub use crate::DType;

    #[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
    pub struct VarId(usize);

    impl VarId {
        pub fn new(v: usize) -> Self {
            Self(v)
        }

        pub fn as_usize(&self) -> usize {
            self.0
        }

        /// A placeholder value, never to be used for anything else.
        pub fn null() -> Self {
            Self(usize::MAX)
        }
    }

    #[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
    pub enum Special {
        ThreadIdx,
        BlockIdx,
    }

    #[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
    pub enum A {
        Var(VarId),
        Const(Const),
    }

    impl From<VarId> for A {
        fn from(value: VarId) -> Self {
            Self::Var(value)
        }
    }

    impl From<i32> for A {
        fn from(value: i32) -> Self {
            Self::Const(Const::I32(value))
        }
    }

    impl From<i64> for A {
        fn from(value: i64) -> Self {
            Self::Const(Const::I64(value))
        }
    }

    impl TryFrom<half::bf16> for A {
        type Error = crate::Error;
        fn try_from(value: half::bf16) -> std::result::Result<Self, Self::Error> {
            Ok(Self::Const(value.try_into()?))
        }
    }

    impl TryFrom<half::f16> for A {
        type Error = crate::Error;
        fn try_from(value: half::f16) -> std::result::Result<Self, Self::Error> {
            Ok(Self::Const(value.try_into()?))
        }
    }

    impl TryFrom<f32> for A {
        type Error = crate::Error;
        fn try_from(value: f32) -> std::result::Result<Self, Self::Error> {
            Ok(Self::Const(value.try_into()?))
        }
    }

    #[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord, Hash)]
    /// The loop start_idx and end_idx are using VarIds as these are derived from line numbers in
    /// the final SSA.
    pub enum Instr {
        DefineAcc(Const),
        DefineGlobal { index: usize, dtype: DType },
        DefineLocal { size: usize, dtype: DType },
        Special(Special),
        Const(Const),
        Unary { op: UnaryOp, arg: A, dtype: DType },
        Binary { op: BinaryOp, lhs: A, rhs: A, dtype: DType },
        Range { lo: A, up: A, step: usize, end_idx: VarId },
        Load { src: VarId, offset: A, dtype: DType },
        Assign { dst: VarId, src: A },
        EndRange { start_idx: VarId },
        If { cond: A, end_idx: VarId },
        EndIf,
        Store { dst: VarId, offset: A, value: A, dtype: DType },
        Barrier,
        // This is not part of the tinygrad SSA but is convenient to handle warp reduce without
        // shared memory.
        ReduceLocal { op: ReduceOp, arg: A, dtype: DType },
    }

    #[derive(Clone)]
    pub struct Kernel {
        instrs: Vec<Instr>,
        args: Vec<(Arg, usize)>,
        launch_config: LaunchConfig,
    }

    impl Kernel {
        pub fn launch_config(&self) -> &LaunchConfig {
            &self.launch_config
        }

        pub fn instrs(&self) -> &Vec<Instr> {
            &self.instrs
        }

        pub fn args(&self) -> &[(Arg, usize)] {
            &self.args
        }

        pub fn new(
            instrs: Vec<Instr>,
            args: Vec<(Arg, usize)>,
            launch_config: LaunchConfig,
        ) -> Self {
            Self { instrs, args, launch_config }
        }

        pub fn from_instrs(instrs: Vec<Instr>) -> crate::Result<Self> {
            let mut args = vec![];
            for (line_no, instr) in instrs.iter().enumerate() {
                if let Instr::DefineGlobal { index, dtype } = instr {
                    if *index != args.len() {
                        crate::bail!("unexpected order for arguments in kernel {instrs:?}")
                    }
                    let arg_id = ArgId::new();
                    let arg = Arg::new(arg_id, Type::Ptr(*dtype));
                    args.push((arg, line_no));
                }
            }
            let launch_config = LaunchConfig { grid_dim: 1, block_dim: 1, shared_mem: 0 };
            Ok(Self { instrs, args, launch_config })
        }

        pub fn flops_mem_per_thread(&self) -> Result<super::FlopsMem> {
            let mut flops = 0usize;
            let mut mem = 0usize;
            let mut mults = vec![];
            let mut mult = 1usize;
            for instr in self.instrs.iter() {
                match instr {
                    Instr::Load { src: _, offset: _, dtype }
                    | Instr::Store { dst: _, offset: _, value: _, dtype } => {
                        mem += mult * dtype.size_in_bytes()
                    }
                    Instr::Range { lo, up, step, end_idx: _ } => {
                        mults.push(mult);
                        let lo = match lo {
                            A::Const(Const::I64(lo)) => *lo,
                            A::Const(Const::I32(lo)) => *lo as i64,
                            A::Const(_) => {
                                crate::bail!("range lo is not a const i32/i64")
                            }
                            A::Var(lo) => match self.instrs[lo.0] {
                                Instr::Const(Const::I64(lo)) => lo,
                                Instr::Const(Const::I32(lo)) => lo as i64,
                                _ => crate::bail!("range lo is not a const"),
                            },
                        };
                        let up = match up {
                            A::Const(Const::I32(up)) => *up as i64,
                            A::Const(Const::I64(up)) => *up,
                            A::Const(_) => {
                                crate::bail!("range up is not a const i32/i64")
                            }
                            A::Var(up) => match self.instrs[up.0] {
                                Instr::Const(Const::I32(up)) => up as i64,
                                Instr::Const(Const::I64(up)) => up,
                                _ => crate::bail!("range lo is not a const"),
                            },
                        };
                        mult *= (up - lo).max(0) as usize / step;
                    }
                    Instr::EndRange { .. } => match mults.pop() {
                        None => crate::bail!("unexpected EndRange"),
                        Some(m) => mult = m,
                    },
                    Instr::Unary { .. } | Instr::Binary { .. } => flops += mult,
                    Instr::DefineGlobal { .. }
                    | Instr::DefineLocal { .. }
                    | Instr::DefineAcc(_)
                    | Instr::Special(_)
                    | Instr::Assign { .. }
                    | Instr::Const(_)
                    | Instr::If { .. }
                    | Instr::EndIf
                    | Instr::Barrier
                    | Instr::ReduceLocal { .. } => {}
                }
            }
            Ok(super::FlopsMem { flops, mem_in_bytes: mem })
        }
    }

    impl std::fmt::Debug for Kernel {
        fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
            for (var_id, instr) in self.instrs.iter().enumerate() {
                writeln!(f, "{var_id:03} {instr:?}")?
            }
            Ok(())
        }
    }
}
