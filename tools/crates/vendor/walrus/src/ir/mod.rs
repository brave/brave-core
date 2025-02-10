//! Intermediate representation for instructions.
//!
//! The goal is to match wasm instructions as closely as possible, but translate
//! the stack machine into an instruction tree. Additionally all control frames
//! are representd as `Block`s.

mod traversals;
pub use self::traversals::*;

use crate::{
    DataId, ElementId, FunctionId, GlobalId, LocalFunction, MemoryId, ModuleTypes, RefType,
    TableId, TypeId, ValType,
};
use id_arena::Id;
use std::fmt;
use std::ops::{Deref, DerefMut};
use walrus_macro::walrus_instr;

/// The id of a local.
pub type LocalId = Id<Local>;

/// A local variable or parameter.
#[derive(Clone, Debug, PartialEq, Eq, Hash)]
pub struct Local {
    id: LocalId,
    ty: ValType,
    /// A human-readable name for this local, often useful when debugging
    pub name: Option<String>,
}

impl Local {
    /// Construct a new local from the given id and type.
    pub fn new(id: LocalId, ty: ValType) -> Local {
        Local { id, ty, name: None }
    }

    /// Get this local's id that is unique across the whole module.
    pub fn id(&self) -> LocalId {
        self.id
    }

    /// Get this local's type.
    pub fn ty(&self) -> ValType {
        self.ty
    }
}

/// The identifier for a `InstrSeq` within some `LocalFunction`.
pub type InstrSeqId = Id<InstrSeq>;

/// The type of an instruction sequence.
///
// NB: We purposefully match the encoding for block types here, with MVP Wasm
// types inlined and multi-value types outlined. If we tried to simplify this
// type representation by always using `TypeId`, then the `used` pass would
// think that a bunch of types that are only internally used by `InstrSeq`s are
// generally used, and we would emit them in the module's "Types" section. We
// don't want to bloat the modules we emit, nor do we want to make the used/GC
// passes convoluted, so we intentionally let the shape of this type guide us.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum InstrSeqType {
    /// MVP Wasm blocks/loops/ifs can only push zero or one resulting value onto
    /// the stack. They cannot take parameters on the stack.
    Simple(Option<ValType>),
    /// The multi-value extension to Wasm allows arbitrary stack parameters and
    /// results, which are expressed via the same mechanism as function types.
    MultiValue(TypeId),
}

impl InstrSeqType {
    /// Construct a new `InstrSeqType` of the correct form for the given
    /// parameter and result types.
    pub fn new(types: &mut ModuleTypes, params: &[ValType], results: &[ValType]) -> InstrSeqType {
        match (params.len(), results.len()) {
            (0, 0) => InstrSeqType::Simple(None),
            (0, 1) => InstrSeqType::Simple(Some(results[0])),
            _ => InstrSeqType::MultiValue(types.add(params, results)),
        }
    }

    /// Construct an `InstrSeqType` with a signature that is known to either be
    /// `Simple` or uses a `Type` that has already been inserted into the
    /// `ModuleTypes`.
    ///
    /// Returns `None` if this is an instruction sequence signature that
    /// requires multi-value and `ModuleTypes` does not already have a `Type`
    /// for it.
    pub fn existing(
        types: &ModuleTypes,
        params: &[ValType],
        results: &[ValType],
    ) -> Option<InstrSeqType> {
        Some(match (params.len(), results.len()) {
            (0, 0) => InstrSeqType::Simple(None),
            (0, 1) => InstrSeqType::Simple(Some(results[0])),
            _ => InstrSeqType::MultiValue(types.find(params, results)?),
        })
    }
}

impl From<Option<ValType>> for InstrSeqType {
    #[inline]
    fn from(x: Option<ValType>) -> InstrSeqType {
        InstrSeqType::Simple(x)
    }
}

impl From<ValType> for InstrSeqType {
    #[inline]
    fn from(x: ValType) -> InstrSeqType {
        InstrSeqType::Simple(Some(x))
    }
}

impl From<TypeId> for InstrSeqType {
    #[inline]
    fn from(x: TypeId) -> InstrSeqType {
        InstrSeqType::MultiValue(x)
    }
}

/// A symbolic original wasm operator source location.
#[derive(Debug, Copy, Clone, PartialEq, PartialOrd, Eq, Ord)]
pub struct InstrLocId(u32);

const DEFAULT_INSTR_LOC_ID: u32 = 0xffff_ffff;

impl InstrLocId {
    /// Create `InstrLocId` from provided data. Normaly the data is
    /// wasm bytecode offset. (0xffff_ffff is reserved for default value).
    pub fn new(data: u32) -> Self {
        assert!(data != DEFAULT_INSTR_LOC_ID);
        InstrLocId(data)
    }

    /// Check if default value.
    pub fn is_default(&self) -> bool {
        self.0 == DEFAULT_INSTR_LOC_ID
    }

    /// The data
    pub fn data(&self) -> u32 {
        assert!(self.0 != DEFAULT_INSTR_LOC_ID);
        self.0
    }
}

impl Default for InstrLocId {
    fn default() -> Self {
        InstrLocId(DEFAULT_INSTR_LOC_ID)
    }
}

/// A sequence of instructions.
#[derive(Debug)]
pub struct InstrSeq {
    id: InstrSeqId,

    /// This block's type: its the types of values that are expected on the
    /// stack when entering this instruction sequence and the types that are
    /// left on the stack afterwards.
    pub ty: InstrSeqType,

    /// The instructions that make up the body of this block.
    pub instrs: Vec<(Instr, InstrLocId)>,

    /// For code address mapping
    pub end: InstrLocId,
}

impl Deref for InstrSeq {
    type Target = Vec<(Instr, InstrLocId)>;

    #[inline]
    fn deref(&self) -> &Vec<(Instr, InstrLocId)> {
        &self.instrs
    }
}

impl DerefMut for InstrSeq {
    #[inline]
    fn deref_mut(&mut self) -> &mut Vec<(Instr, InstrLocId)> {
        &mut self.instrs
    }
}

impl InstrSeq {
    /// Construct a new instruction sequence.
    pub(crate) fn new(id: InstrSeqId, ty: InstrSeqType) -> InstrSeq {
        let instrs = vec![];
        let end = Default::default();
        InstrSeq {
            id,
            ty,
            instrs,
            end,
        }
    }

    /// Get the id of this instruction sequence.
    #[inline]
    pub fn id(&self) -> InstrSeqId {
        self.id
    }
}

/// Different kinds of blocks.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub(crate) enum BlockKind {
    /// A `block` block.
    Block,

    /// A `loop` block.
    Loop,

    /// An `if` block
    If,

    /// An `Else` block
    Else,

    /// The entry to a function.
    FunctionEntry,
}

/// An enum of all the different kinds of wasm instructions.
///
/// Note that the `#[walrus_expr]` macro rewrites this enum's variants from
///
/// ```ignore
/// enum Instr {
///     Variant { field: Ty, .. },
///     ...
/// }
/// ```
///
/// into
///
/// ```ignore
/// enum Instr {
///     Variant(Variant),
///     ...
/// }
///
/// struct Variant {
///     field: Ty,
///     ...
/// }
/// ```
#[walrus_instr]
#[derive(Clone, Debug)]
pub enum Instr {
    /// `block ... end`
    #[walrus(skip_builder)]
    Block {
        /// The id of this `block` instruction's inner `InstrSeq`.
        seq: InstrSeqId,
    },

    /// `loop ... end`
    #[walrus(skip_builder)]
    Loop {
        /// The id of this `loop` instruction's inner `InstrSeq`.
        seq: InstrSeqId,
    },

    /// `call`
    Call {
        /// The function being invoked.
        func: FunctionId,
    },

    /// `call_indirect`
    CallIndirect {
        /// The type signature of the function we're calling
        ty: TypeId,
        /// The table which `func` below is indexing into
        table: TableId,
    },

    /// `local.get n`
    LocalGet {
        /// The local being got.
        local: LocalId,
    },

    /// `local.set n`
    LocalSet {
        /// The local being set.
        local: LocalId,
    },

    /// `local.tee n`
    LocalTee {
        /// The local being set.
        local: LocalId,
    },

    /// `global.get n`
    GlobalGet {
        /// The global being got.
        global: GlobalId,
    },

    /// `global.set n`
    GlobalSet {
        /// The global being set.
        global: GlobalId,
    },

    /// `*.const`
    Const {
        /// The constant value.
        value: Value,
    },

    /// Ternary operations, those requiring three operands
    TernOp {
        /// The operation being performed
        #[walrus(skip_visit)]
        op: TernaryOp,
    },

    /// Binary operations, those requiring two operands
    Binop {
        /// The operation being performed
        #[walrus(skip_visit)]
        op: BinaryOp,
    },

    /// Unary operations, those requiring one operand
    Unop {
        /// The operation being performed
        #[walrus(skip_visit)]
        op: UnaryOp,
    },

    /// `select`
    Select {
        /// Optionally listed type that the `select` instruction is expected to
        /// produce, used in subtyping relations with the gc proposal.
        #[walrus(skip_visit)]
        ty: Option<ValType>,
    },

    /// `unreachable`
    Unreachable {},

    /// `br`
    Br {
        /// The target block to branch to.
        #[walrus(skip_visit)] // should have already been visited
        block: InstrSeqId,
    },

    /// `br_if`
    BrIf {
        /// The target block to branch to when the condition is met.
        #[walrus(skip_visit)] // should have already been visited
        block: InstrSeqId,
    },

    /// `if <consequent> else <alternative> end`
    #[walrus(skip_builder)]
    IfElse {
        /// The block to execute when the condition is true.
        consequent: InstrSeqId,
        /// The block to execute when the condition is false.
        alternative: InstrSeqId,
    },

    /// `br_table`
    BrTable {
        /// The table of target blocks.
        #[walrus(skip_visit)] // should have already been visited
        blocks: Box<[InstrSeqId]>,
        /// The block that is branched to by default when `which` is out of the
        /// table's bounds.
        #[walrus(skip_visit)] // should have already been visited
        default: InstrSeqId,
    },

    /// `drop`
    Drop {},

    /// `return`
    Return {},

    /// `memory.size`
    MemorySize {
        /// The memory we're fetching the current size of.
        memory: MemoryId,
    },

    /// `memory.grow`
    MemoryGrow {
        /// The memory we're growing.
        memory: MemoryId,
    },

    /// `memory.init`
    MemoryInit {
        /// The memory we're growing.
        memory: MemoryId,
        /// The data to copy in
        data: DataId,
    },

    /// `data.drop`
    DataDrop {
        /// The data to drop
        data: DataId,
    },

    /// `memory.copy`
    MemoryCopy {
        /// The source memory
        src: MemoryId,
        /// The destination memory
        dst: MemoryId,
    },

    /// `memory.fill`
    MemoryFill {
        /// The memory to fill
        memory: MemoryId,
    },

    /// `*.load`
    ///
    /// Loading a value from memory.
    Load {
        /// The memory we're loading from.
        memory: MemoryId,
        /// The kind of memory load this is performing
        #[walrus(skip_visit)]
        kind: LoadKind,
        /// The alignment and offset of this memory load
        #[walrus(skip_visit)]
        arg: MemArg,
    },

    /// `*.store`
    ///
    /// Storing a value to memory.
    Store {
        /// The memory we're storing to
        memory: MemoryId,
        /// The kind of memory store this is performing
        #[walrus(skip_visit)]
        kind: StoreKind,
        /// The alignment and offset of this memory store
        #[walrus(skip_visit)]
        arg: MemArg,
    },

    /// An atomic read/modify/write operation.
    AtomicRmw {
        /// The memory we're modifying
        memory: MemoryId,
        /// The atomic operation being performed
        #[walrus(skip_visit)]
        op: AtomicOp,
        /// The atomic operation being performed
        #[walrus(skip_visit)]
        width: AtomicWidth,
        /// The alignment and offset from the base address
        #[walrus(skip_visit)]
        arg: MemArg,
    },

    /// An atomic compare-and-exchange operation.
    Cmpxchg {
        /// The memory we're modifying
        memory: MemoryId,
        /// The atomic operation being performed
        #[walrus(skip_visit)]
        width: AtomicWidth,
        /// The alignment and offset from the base address
        #[walrus(skip_visit)]
        arg: MemArg,
    },

    /// The `atomic.notify` instruction to wake up threads.
    AtomicNotify {
        /// The memory we're notifying through
        memory: MemoryId,
        /// The alignment and offset from the base address
        #[walrus(skip_visit)]
        arg: MemArg,
    },

    /// The `*.atomic.wait` instruction to block threads.
    AtomicWait {
        /// The memory we're waiting through.
        memory: MemoryId,
        /// The alignment and offset from the base address.
        #[walrus(skip_visit)]
        arg: MemArg,
        /// Whether or not this is an `i32` or `i64` wait.
        #[walrus(skip_visit)]
        sixty_four: bool,
    },

    /// The `atomic.fence` instruction
    AtomicFence {},

    /// `table.get`
    TableGet {
        /// The table we're fetching from.
        table: TableId,
    },

    /// `table.set`
    TableSet {
        /// The table we're storing to.
        table: TableId,
    },

    /// `table.grow`
    TableGrow {
        /// The table we're growing
        table: TableId,
    },

    /// `table.size`
    TableSize {
        /// The table we're getting the size of
        table: TableId,
    },

    /// `table.fill`
    TableFill {
        /// The table we're filling
        table: TableId,
    },

    /// `ref.null $ty`
    RefNull {
        /// The type of null that we're producing
        #[walrus(skip_visit)]
        ty: RefType,
    },

    /// `ref.is_null`
    RefIsNull {},

    /// `ref.func`
    RefFunc {
        /// The function that this instruction is referencing
        func: FunctionId,
    },

    /// `v128.bitselect`
    V128Bitselect {},

    /// `i8x16.swizzle`
    I8x16Swizzle {},

    /// `i8x16.shuffle`
    I8x16Shuffle {
        /// The indices that are used to create the final vector of this
        /// instruction
        #[walrus(skip_visit)]
        indices: ShuffleIndices,
    },

    /// Various instructions to load a simd vector from memory
    LoadSimd {
        /// The memory we're loading from.
        memory: MemoryId,
        /// The size of load this is performing
        #[walrus(skip_visit)]
        kind: LoadSimdKind,
        /// The alignment and offset of this memory load
        #[walrus(skip_visit)]
        arg: MemArg,
    },

    /// `table.init`
    TableInit {
        /// The table we're copying into.
        table: TableId,
        /// The element we're getting items from.
        elem: ElementId,
    },

    /// `elem.drop`
    ElemDrop {
        /// The elem segment to drop
        elem: ElementId,
    },

    /// `table.copy`
    TableCopy {
        /// The source table
        src: TableId,
        /// The destination table
        dst: TableId,
    },

    /// `return_call`
    ReturnCall {
        /// The function being invoked.
        func: FunctionId,
    },

    /// `return_call_indirect`
    ReturnCallIndirect {
        /// The type signature of the function we're calling
        ty: TypeId,
        /// The table which `func` below is indexing into
        table: TableId,
    },
}

/// Argument in `V128Shuffle` of lane indices to select
pub type ShuffleIndices = [u8; 16];

/// Constant values that can show up in WebAssembly
#[derive(Debug, Clone, Copy)]
pub enum Value {
    /// A constant 32-bit integer
    I32(i32),
    /// A constant 64-bit integer
    I64(i64),
    /// A constant 32-bit float
    F32(f32),
    /// A constant 64-bit float
    F64(f64),
    /// A constant 128-bit vector register
    V128(u128),
}

impl fmt::Display for Value {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            Value::I32(i) => i.fmt(f),
            Value::I64(i) => i.fmt(f),
            Value::F32(i) => i.fmt(f),
            Value::F64(i) => i.fmt(f),
            Value::V128(i) => i.fmt(f),
        }
    }
}

/// Possible ternary operations in wasm
#[allow(missing_docs)]
#[derive(Copy, Clone, Debug)]
pub enum TernaryOp {
    F32x4RelaxedMadd,
    F32x4RelaxedNmadd,
    F64x2RelaxedMadd,
    F64x2RelaxedNmadd,
    I8x16RelaxedLaneselect,
    I16x8RelaxedLaneselect,
    I32x4RelaxedLaneselect,
    I64x2RelaxedLaneselect,
    I32x4RelaxedDotI8x16I7x16AddS,
}

/// Possible binary operations in wasm
#[allow(missing_docs)]
#[derive(Copy, Clone, Debug)]
pub enum BinaryOp {
    I32Eq,
    I32Ne,
    I32LtS,
    I32LtU,
    I32GtS,
    I32GtU,
    I32LeS,
    I32LeU,
    I32GeS,
    I32GeU,

    I64Eq,
    I64Ne,
    I64LtS,
    I64LtU,
    I64GtS,
    I64GtU,
    I64LeS,
    I64LeU,
    I64GeS,
    I64GeU,

    F32Eq,
    F32Ne,
    F32Lt,
    F32Gt,
    F32Le,
    F32Ge,

    F64Eq,
    F64Ne,
    F64Lt,
    F64Gt,
    F64Le,
    F64Ge,

    I32Add,
    I32Sub,
    I32Mul,
    I32DivS,
    I32DivU,
    I32RemS,
    I32RemU,
    I32And,
    I32Or,
    I32Xor,
    I32Shl,
    I32ShrS,
    I32ShrU,
    I32Rotl,
    I32Rotr,

    I64Add,
    I64Sub,
    I64Mul,
    I64DivS,
    I64DivU,
    I64RemS,
    I64RemU,
    I64And,
    I64Or,
    I64Xor,
    I64Shl,
    I64ShrS,
    I64ShrU,
    I64Rotl,
    I64Rotr,

    F32Add,
    F32Sub,
    F32Mul,
    F32Div,
    F32Min,
    F32Max,
    F32Copysign,

    F64Add,
    F64Sub,
    F64Mul,
    F64Div,
    F64Min,
    F64Max,
    F64Copysign,

    I8x16ReplaceLane { idx: u8 },
    I16x8ReplaceLane { idx: u8 },
    I32x4ReplaceLane { idx: u8 },
    I64x2ReplaceLane { idx: u8 },
    F32x4ReplaceLane { idx: u8 },
    F64x2ReplaceLane { idx: u8 },

    I8x16Eq,
    I8x16Ne,
    I8x16LtS,
    I8x16LtU,
    I8x16GtS,
    I8x16GtU,
    I8x16LeS,
    I8x16LeU,
    I8x16GeS,
    I8x16GeU,

    I16x8Eq,
    I16x8Ne,
    I16x8LtS,
    I16x8LtU,
    I16x8GtS,
    I16x8GtU,
    I16x8LeS,
    I16x8LeU,
    I16x8GeS,
    I16x8GeU,

    I32x4Eq,
    I32x4Ne,
    I32x4LtS,
    I32x4LtU,
    I32x4GtS,
    I32x4GtU,
    I32x4LeS,
    I32x4LeU,
    I32x4GeS,
    I32x4GeU,

    I64x2Eq,
    I64x2Ne,
    I64x2LtS,
    I64x2GtS,
    I64x2LeS,
    I64x2GeS,

    F32x4Eq,
    F32x4Ne,
    F32x4Lt,
    F32x4Gt,
    F32x4Le,
    F32x4Ge,

    F64x2Eq,
    F64x2Ne,
    F64x2Lt,
    F64x2Gt,
    F64x2Le,
    F64x2Ge,

    V128And,
    V128Or,
    V128Xor,
    V128AndNot,

    I8x16Shl,
    I8x16ShrS,
    I8x16ShrU,
    I8x16Add,
    I8x16AddSatS,
    I8x16AddSatU,
    I8x16Sub,
    I8x16SubSatS,
    I8x16SubSatU,
    I16x8Shl,
    I16x8ShrS,
    I16x8ShrU,
    I16x8Add,
    I16x8AddSatS,
    I16x8AddSatU,
    I16x8Sub,
    I16x8SubSatS,
    I16x8SubSatU,
    I16x8Mul,
    I32x4Shl,
    I32x4ShrS,
    I32x4ShrU,
    I32x4Add,
    I32x4Sub,
    I32x4Mul,
    I64x2Shl,
    I64x2ShrS,
    I64x2ShrU,
    I64x2Add,
    I64x2Sub,
    I64x2Mul,

    F32x4Add,
    F32x4Sub,
    F32x4Mul,
    F32x4Div,
    F32x4Min,
    F32x4Max,
    F32x4PMin,
    F32x4PMax,
    F64x2Add,
    F64x2Sub,
    F64x2Mul,
    F64x2Div,
    F64x2Min,
    F64x2Max,
    F64x2PMin,
    F64x2PMax,

    I8x16NarrowI16x8S,
    I8x16NarrowI16x8U,
    I16x8NarrowI32x4S,
    I16x8NarrowI32x4U,
    I8x16AvgrU,
    I16x8AvgrU,

    I8x16MinS,
    I8x16MinU,
    I8x16MaxS,
    I8x16MaxU,
    I16x8MinS,
    I16x8MinU,
    I16x8MaxS,
    I16x8MaxU,
    I32x4MinS,
    I32x4MinU,
    I32x4MaxS,
    I32x4MaxU,

    I32x4DotI16x8S,

    I16x8Q15MulrSatS,
    I16x8ExtMulLowI8x16S,
    I16x8ExtMulHighI8x16S,
    I16x8ExtMulLowI8x16U,
    I16x8ExtMulHighI8x16U,
    I32x4ExtMulLowI16x8S,
    I32x4ExtMulHighI16x8S,
    I32x4ExtMulLowI16x8U,
    I32x4ExtMulHighI16x8U,
    I64x2ExtMulLowI32x4S,
    I64x2ExtMulHighI32x4S,
    I64x2ExtMulLowI32x4U,
    I64x2ExtMulHighI32x4U,

    I8x16RelaxedSwizzle,
    F32x4RelaxedMin,
    F32x4RelaxedMax,
    F64x2RelaxedMin,
    F64x2RelaxedMax,
    I16x8RelaxedQ15mulrS,
    I16x8RelaxedDotI8x16I7x16S,
}

/// Possible unary operations in wasm
#[allow(missing_docs)]
#[derive(Copy, Clone, Debug)]
pub enum UnaryOp {
    I32Eqz,
    I32Clz,
    I32Ctz,
    I32Popcnt,

    I64Eqz,
    I64Clz,
    I64Ctz,
    I64Popcnt,

    F32Abs,
    F32Neg,
    F32Ceil,
    F32Floor,
    F32Trunc,
    F32Nearest,
    F32Sqrt,

    F64Abs,
    F64Neg,
    F64Ceil,
    F64Floor,
    F64Trunc,
    F64Nearest,
    F64Sqrt,

    I32WrapI64,
    I32TruncSF32,
    I32TruncUF32,
    I32TruncSF64,
    I32TruncUF64,
    I64ExtendSI32,
    I64ExtendUI32,
    I64TruncSF32,
    I64TruncUF32,
    I64TruncSF64,
    I64TruncUF64,

    F32ConvertSI32,
    F32ConvertUI32,
    F32ConvertSI64,
    F32ConvertUI64,
    F32DemoteF64,
    F64ConvertSI32,
    F64ConvertUI32,
    F64ConvertSI64,
    F64ConvertUI64,
    F64PromoteF32,

    I32ReinterpretF32,
    I64ReinterpretF64,
    F32ReinterpretI32,
    F64ReinterpretI64,

    I32Extend8S,
    I32Extend16S,
    I64Extend8S,
    I64Extend16S,
    I64Extend32S,

    I8x16Splat,
    I8x16ExtractLaneS { idx: u8 },
    I8x16ExtractLaneU { idx: u8 },
    I16x8Splat,
    I16x8ExtractLaneS { idx: u8 },
    I16x8ExtractLaneU { idx: u8 },
    I32x4Splat,
    I32x4ExtractLane { idx: u8 },
    I64x2Splat,
    I64x2ExtractLane { idx: u8 },
    F32x4Splat,
    F32x4ExtractLane { idx: u8 },
    F64x2Splat,
    F64x2ExtractLane { idx: u8 },

    V128Not,
    V128AnyTrue,

    I8x16Abs,
    I8x16Popcnt,
    I8x16Neg,
    I8x16AllTrue,
    I8x16Bitmask,
    I16x8Abs,
    I16x8Neg,
    I16x8AllTrue,
    I16x8Bitmask,
    I32x4Abs,
    I32x4Neg,
    I32x4AllTrue,
    I32x4Bitmask,
    I64x2Abs,
    I64x2Neg,
    I64x2AllTrue,
    I64x2Bitmask,

    F32x4Abs,
    F32x4Neg,
    F32x4Sqrt,
    F32x4Ceil,
    F32x4Floor,
    F32x4Trunc,
    F32x4Nearest,
    F64x2Abs,
    F64x2Neg,
    F64x2Sqrt,
    F64x2Ceil,
    F64x2Floor,
    F64x2Trunc,
    F64x2Nearest,

    I16x8ExtAddPairwiseI8x16S,
    I16x8ExtAddPairwiseI8x16U,
    I32x4ExtAddPairwiseI16x8S,
    I32x4ExtAddPairwiseI16x8U,
    I64x2ExtendLowI32x4S,
    I64x2ExtendHighI32x4S,
    I64x2ExtendLowI32x4U,
    I64x2ExtendHighI32x4U,
    I32x4TruncSatF64x2SZero,
    I32x4TruncSatF64x2UZero,
    F64x2ConvertLowI32x4S,
    F64x2ConvertLowI32x4U,
    F32x4DemoteF64x2Zero,
    F64x2PromoteLowF32x4,

    I32x4TruncSatF32x4S,
    I32x4TruncSatF32x4U,
    F32x4ConvertI32x4S,
    F32x4ConvertI32x4U,

    I32TruncSSatF32,
    I32TruncUSatF32,
    I32TruncSSatF64,
    I32TruncUSatF64,
    I64TruncSSatF32,
    I64TruncUSatF32,
    I64TruncSSatF64,
    I64TruncUSatF64,

    I16x8WidenLowI8x16S,
    I16x8WidenLowI8x16U,
    I16x8WidenHighI8x16S,
    I16x8WidenHighI8x16U,
    I32x4WidenLowI16x8S,
    I32x4WidenLowI16x8U,
    I32x4WidenHighI16x8S,
    I32x4WidenHighI16x8U,

    I32x4RelaxedTruncF32x4S,
    I32x4RelaxedTruncF32x4U,
    I32x4RelaxedTruncF64x2SZero,
    I32x4RelaxedTruncF64x2UZero,
}

/// The different kinds of load instructions that are part of a `Load` IR node
#[derive(Debug, Copy, Clone)]
#[allow(missing_docs)]
pub enum LoadKind {
    // TODO: much of this is probably redundant with type information already
    // ambiently available, we probably want to trim this down to just "value"
    // and then maybe some sign extensions. We'd then use the type of the node
    // to figure out what kind of store it actually is.
    I32 { atomic: bool },
    I64 { atomic: bool },
    F32,
    F64,
    V128,
    I32_8 { kind: ExtendedLoad },
    I32_16 { kind: ExtendedLoad },
    I64_8 { kind: ExtendedLoad },
    I64_16 { kind: ExtendedLoad },
    I64_32 { kind: ExtendedLoad },
}

/// The different kinds of load instructions that are part of a `LoadSimd` IR node
#[derive(Debug, Copy, Clone)]
#[allow(missing_docs)]
pub enum LoadSimdKind {
    Splat8,
    Splat16,
    Splat32,
    Splat64,

    V128Load8x8S,
    V128Load8x8U,
    V128Load16x4S,
    V128Load16x4U,
    V128Load32x2S,
    V128Load32x2U,
    V128Load32Zero,
    V128Load64Zero,

    V128Load8Lane(u8),
    V128Load16Lane(u8),
    V128Load32Lane(u8),
    V128Load64Lane(u8),
    V128Store8Lane(u8),
    V128Store16Lane(u8),
    V128Store32Lane(u8),
    V128Store64Lane(u8),
}

/// The kinds of extended loads which can happen
#[derive(Debug, Copy, Clone)]
#[allow(missing_docs)]
pub enum ExtendedLoad {
    SignExtend,
    ZeroExtend,
    ZeroExtendAtomic,
}

impl LoadKind {
    /// Returns the number of bytes loaded
    pub fn width(&self) -> u32 {
        use self::LoadKind::*;
        match self {
            I32_8 { .. } | I64_8 { .. } => 1,
            I32_16 { .. } | I64_16 { .. } => 2,
            I32 { .. } | F32 | I64_32 { .. } => 4,
            I64 { .. } | F64 => 8,
            V128 => 16,
        }
    }

    /// Returns if this is an atomic load
    pub fn atomic(&self) -> bool {
        use self::LoadKind::*;
        match self {
            I32_8 { kind }
            | I32_16 { kind }
            | I64_8 { kind }
            | I64_16 { kind }
            | I64_32 { kind } => kind.atomic(),
            I32 { atomic } | I64 { atomic } => *atomic,
            F32 | F64 | V128 => false,
        }
    }
}

impl ExtendedLoad {
    /// Returns whether this is an atomic extended load
    pub fn atomic(&self) -> bool {
        match self {
            ExtendedLoad::SignExtend | ExtendedLoad::ZeroExtend => false,
            ExtendedLoad::ZeroExtendAtomic => true,
        }
    }
}

/// The different kinds of store instructions that are part of a `Store` IR node
#[derive(Debug, Copy, Clone)]
#[allow(missing_docs)]
pub enum StoreKind {
    I32 { atomic: bool },
    I64 { atomic: bool },
    F32,
    F64,
    V128,
    I32_8 { atomic: bool },
    I32_16 { atomic: bool },
    I64_8 { atomic: bool },
    I64_16 { atomic: bool },
    I64_32 { atomic: bool },
}

impl StoreKind {
    /// Returns the number of bytes stored
    pub fn width(&self) -> u32 {
        use self::StoreKind::*;
        match self {
            I32_8 { .. } | I64_8 { .. } => 1,
            I32_16 { .. } | I64_16 { .. } => 2,
            I32 { .. } | F32 | I64_32 { .. } => 4,
            I64 { .. } | F64 => 8,
            V128 => 16,
        }
    }

    /// Returns whether this is an atomic store
    pub fn atomic(&self) -> bool {
        use self::StoreKind::*;

        match self {
            I32 { atomic }
            | I64 { atomic }
            | I32_8 { atomic }
            | I32_16 { atomic }
            | I64_8 { atomic }
            | I64_16 { atomic }
            | I64_32 { atomic } => *atomic,
            F32 | F64 | V128 => false,
        }
    }
}

/// Arguments to memory operations, containing a constant offset from a dynamic
/// address as well as a predicted alignment.
#[derive(Debug, Copy, Clone)]
pub struct MemArg {
    /// The alignment of the memory operation, must be a power of two
    pub align: u32,
    /// The offset of the memory operation, in bytes from the source address
    pub offset: u32,
}

/// The different kinds of atomic rmw operations
#[derive(Debug, Copy, Clone)]
#[allow(missing_docs)]
pub enum AtomicOp {
    Add,
    Sub,
    And,
    Or,
    Xor,
    Xchg,
}

/// The different kinds of atomic rmw operations
#[derive(Debug, Copy, Clone)]
#[allow(missing_docs)]
pub enum AtomicWidth {
    I32,
    I32_8,
    I32_16,
    I64,
    I64_8,
    I64_16,
    I64_32,
}

impl AtomicWidth {
    /// Returns the size, in bytes, of this atomic operation
    pub fn bytes(&self) -> u32 {
        use self::AtomicWidth::*;
        match self {
            I32_8 | I64_8 => 1,
            I32_16 | I64_16 => 2,
            I32 | I64_32 => 4,
            I64 => 8,
        }
    }
}

impl Instr {
    /// Are any instructions that follow this instruction's instruction (within
    /// the current block) unreachable?
    ///
    /// Returns `true` for unconditional branches (`br`, `return`, etc...) and
    /// `unreachable`. Returns `false` for all other "normal" instructions
    /// (`i32.add`, etc...).
    pub fn following_instructions_are_unreachable(&self) -> bool {
        match *self {
            Instr::Unreachable(..)
            | Instr::Br(..)
            | Instr::BrTable(..)
            | Instr::Return(..)
            | Instr::ReturnCall(..)
            | Instr::ReturnCallIndirect(..) => true,

            // No `_` arm to make sure that we properly update this function as
            // we add support for new instructions.
            Instr::Block(..)
            | Instr::Loop(..)
            | Instr::Call(..)
            | Instr::LocalGet(..)
            | Instr::LocalSet(..)
            | Instr::LocalTee(..)
            | Instr::GlobalGet(..)
            | Instr::GlobalSet(..)
            | Instr::Const(..)
            | Instr::TernOp(..)
            | Instr::Binop(..)
            | Instr::Unop(..)
            | Instr::Select(..)
            | Instr::BrIf(..)
            | Instr::IfElse(..)
            | Instr::MemorySize(..)
            | Instr::MemoryGrow(..)
            | Instr::MemoryInit(..)
            | Instr::DataDrop(..)
            | Instr::MemoryCopy(..)
            | Instr::MemoryFill(..)
            | Instr::CallIndirect(..)
            | Instr::Load(..)
            | Instr::Store(..)
            | Instr::AtomicRmw(..)
            | Instr::Cmpxchg(..)
            | Instr::AtomicNotify(..)
            | Instr::AtomicWait(..)
            | Instr::TableGet(..)
            | Instr::TableSet(..)
            | Instr::TableGrow(..)
            | Instr::TableSize(..)
            | Instr::TableFill(..)
            | Instr::RefNull(..)
            | Instr::RefIsNull(..)
            | Instr::RefFunc(..)
            | Instr::V128Bitselect(..)
            | Instr::I8x16Swizzle(..)
            | Instr::I8x16Shuffle(..)
            | Instr::LoadSimd(..)
            | Instr::AtomicFence(..)
            | Instr::TableInit(..)
            | Instr::TableCopy(..)
            | Instr::ElemDrop(..)
            | Instr::Drop(..) => false,
        }
    }
}

/// Anything that can be visited by a `Visitor`.
pub(crate) trait Visit<'instr> {
    /// Visit this thing with the given visitor.
    fn visit<V>(&self, visitor: &mut V)
    where
        V: Visitor<'instr>;
}

/// Anything that can be mutably visited by a `VisitorMut`.
pub(crate) trait VisitMut {
    /// Visit this thing with the given visitor.
    fn visit_mut<V>(&mut self, visitor: &mut V)
    where
        V: VisitorMut;
}

impl<'instr> Visit<'instr> for InstrSeq {
    fn visit<V>(&self, visitor: &mut V)
    where
        V: Visitor<'instr>,
    {
        if let InstrSeqType::MultiValue(ref ty) = self.ty {
            visitor.visit_type_id(ty);
        }
    }
}

impl VisitMut for InstrSeq {
    fn visit_mut<V>(&mut self, visitor: &mut V)
    where
        V: VisitorMut,
    {
        if let InstrSeqType::MultiValue(ref mut ty) = self.ty {
            visitor.visit_type_id_mut(ty);
        }
    }
}
