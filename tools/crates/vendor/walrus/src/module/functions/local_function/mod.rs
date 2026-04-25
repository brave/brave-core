//! Functions defined locally within a wasm module.

mod context;
mod emit;

use self::context::ValidationContext;
use crate::emit::IdsToIndices;
use crate::map::{IdHashMap, IdHashSet};
use crate::parse::IndicesToIds;
use crate::{ir::*, RefType};
use crate::{Data, DataId, FunctionBuilder, FunctionId, MemoryId, Module, Result, TypeId, ValType};
use std::collections::BTreeMap;
use std::ops::Range;
use wasmparser::{FuncValidator, Operator, ValidatorResources};

/// A function defined locally within the wasm module.
#[derive(Debug)]
pub struct LocalFunction {
    /// All of this function's instructions, contained in the arena.
    builder: FunctionBuilder,

    /// Arguments to this function, and the locals that they're assigned to.
    pub args: Vec<LocalId>,

    /// InstrSeqId list mapping to original instruction
    pub instruction_mapping: Vec<(usize, InstrLocId)>,

    /// Original function binary range.
    pub original_range: Option<Range<usize>>,
}

impl LocalFunction {
    /// Creates a new definition of a local function from its components.
    pub(crate) fn new(args: Vec<LocalId>, builder: FunctionBuilder) -> LocalFunction {
        LocalFunction {
            args,
            builder,
            instruction_mapping: Vec::new(),
            original_range: None,
        }
    }

    /// Construct a new `LocalFunction`.
    ///
    /// Validates the given function body and constructs the `Instr` IR at the
    /// same time.
    pub(crate) fn parse(
        module: &Module,
        indices: &IndicesToIds,
        id: FunctionId,
        ty: TypeId,
        args: Vec<LocalId>,
        mut body: wasmparser::BinaryReader<'_>,
        on_instr_pos: Option<&(dyn Fn(&usize) -> InstrLocId + Sync + Send + 'static)>,
        mut validator: FuncValidator<ValidatorResources>,
    ) -> Result<LocalFunction> {
        let code_address_offset = module.funcs.code_section_offset;
        let function_body_size = body.range().end - body.range().start;
        let function_body_size_bit =
            (std::mem::size_of::<usize>() as u32 * 8 - function_body_size.leading_zeros() - 1) / 7
                + 1;
        let mut func = LocalFunction {
            builder: FunctionBuilder::without_entry(ty),
            args,
            instruction_mapping: Vec::new(),
            original_range: Some(Range::<usize> {
                start: body.range().start - code_address_offset - (function_body_size_bit as usize),
                end: body.range().end - code_address_offset,
            }),
        };

        let result: Vec<_> = module.types.get(ty).results().to_vec();
        let result = result.into_boxed_slice();

        let controls = &mut context::ControlStack::new();

        let mut ctx = ValidationContext::new(module, indices, id, &mut func, controls);

        let ty = module.types.find_for_function_entry(&result).expect(
            "the function entry type should have already been created before parsing the body",
        );
        let entry = ctx.push_control_with_ty(BlockKind::FunctionEntry, ty);
        let mut instruction_mapping = BTreeMap::new();
        ctx.func.builder.entry = Some(entry);
        while !body.eof() {
            let pos = body.original_position();
            let inst = body.read_operator()?;
            let loc = if let Some(ref on_instr_pos) = on_instr_pos {
                on_instr_pos(&pos)
            } else {
                InstrLocId::new(pos as u32)
            };
            validator.op(pos, &inst)?;
            append_instruction(&mut ctx, inst, loc);
            instruction_mapping.insert(pos - code_address_offset, loc);
        }
        ctx.func.instruction_mapping = instruction_mapping.into_iter().collect();
        validator.finish(body.original_position())?;

        debug_assert!(ctx.controls.is_empty());

        Ok(func)
    }

    /// Get this function's type.
    #[inline]
    pub fn ty(&self) -> TypeId {
        self.builder.ty
    }

    pub(crate) fn add_block(
        &mut self,
        make_block: impl FnOnce(InstrSeqId) -> InstrSeq,
    ) -> InstrSeqId {
        self.builder.arena.alloc_with_id(make_block)
    }

    /// Get the id of this function's entry block.
    pub fn entry_block(&self) -> InstrSeqId {
        self.builder.entry.unwrap()
    }

    /// Get the block associated with the given id.
    pub fn block(&self, id: InstrSeqId) -> &InstrSeq {
        &self.builder.arena[id]
    }

    /// Get the block associated with the given id.
    pub fn block_mut(&mut self, id: InstrSeqId) -> &mut InstrSeq {
        &mut self.builder.arena[id]
    }

    /// Get access to a `FunctionBuilder` to continue adding instructions to
    /// this function.
    pub fn builder(&self) -> &FunctionBuilder {
        &self.builder
    }

    /// Get access to a `FunctionBuilder` to continue adding instructions to
    /// this function.
    pub fn builder_mut(&mut self) -> &mut FunctionBuilder {
        &mut self.builder
    }

    /// Get the size of this function, in number of instructions.
    pub fn size(&self) -> u64 {
        let mut v = SizeVisitor::default();
        dfs_in_order(&mut v, self, self.entry_block());
        return v.size;

        #[derive(Default)]
        struct SizeVisitor {
            size: u64,
        }

        impl<'instr> Visitor<'instr> for SizeVisitor {
            fn start_instr_seq(&mut self, seq: &'instr InstrSeq) {
                self.size += seq.len() as u64;
            }
        }
    }

    /// Is this function's body a [constant
    /// instruction](https://webassembly.github.io/spec/core/valid/instructions.html#constant-instructions)?
    pub fn is_const(&self) -> bool {
        self.block(self.entry_block())
            .instrs
            .iter()
            .all(|(e, _)| e.is_const())
    }

    /// Collect the set of data segments that are used in this function via
    /// `memory.init` or `data.drop` instructions.
    pub fn used_data_segments(&self) -> IdHashSet<Data> {
        let mut visitor = DataSegmentsVisitor::default();
        dfs_in_order(&mut visitor, self, self.entry_block());
        return visitor.segments;

        #[derive(Default)]
        struct DataSegmentsVisitor {
            segments: IdHashSet<Data>,
        }

        impl<'a> Visitor<'a> for DataSegmentsVisitor {
            fn visit_data_id(&mut self, id: &DataId) {
                self.segments.insert(*id);
            }
        }
    }

    fn used_locals(&self) -> IdHashSet<Local> {
        let mut locals = Used::default();
        dfs_in_order(&mut locals, self, self.entry_block());
        return locals.locals;

        #[derive(Default)]
        struct Used {
            locals: IdHashSet<Local>,
        }

        impl<'a> Visitor<'a> for Used {
            fn visit_local_id(&mut self, id: &LocalId) {
                self.locals.insert(*id);
            }
        }
    }

    /// Emit this function's compact locals declarations.
    pub(crate) fn emit_locals(
        &self,
        module: &Module,
    ) -> (
        Vec<(u32, wasm_encoder::ValType)>,
        IdHashSet<Local>,
        IdHashMap<Local, u32>,
    ) {
        let used_set = self.used_locals();
        let mut used_locals = used_set.iter().cloned().collect::<Vec<_>>();
        // Sort to ensure we assign local indexes deterministically, and
        // everything is distinct so we can use a faster unstable sort.
        used_locals.sort_unstable();

        // NB: Use `BTreeMap` to make compilation deterministic by emitting
        // types in the same order
        let mut ty_to_locals = BTreeMap::new();
        let args = self.args.iter().cloned().collect::<IdHashSet<_>>();

        // Partition all locals by their type as we'll create at most one entry
        // for each type. Skip all arguments to the function because they're
        // handled separately.
        for local in used_locals.iter() {
            if !args.contains(local) {
                let ty = module.locals.get(*local).ty();
                ty_to_locals.entry(ty).or_insert_with(Vec::new).push(*local);
            }
        }

        let mut local_map = IdHashMap::default();
        local_map.reserve(used_locals.len());

        // Allocate an index to all the function arguments, as these are all
        // unconditionally used and are implicit locals in wasm.
        let mut idx = 0;
        for &arg in self.args.iter() {
            local_map.insert(arg, idx);
            idx += 1;
        }

        // Assign an index to all remaining locals
        for (_, locals) in ty_to_locals.iter() {
            for l in locals {
                local_map.insert(*l, idx);
                idx += 1;
            }
        }

        // Use our type map to emit a compact representation of all locals now
        (
            ty_to_locals
                .iter()
                .map(|(ty, locals)| (locals.len() as u32, ty.to_wasmencoder_type()))
                .collect(),
            used_set,
            local_map,
        )
    }

    /// Emit this function's instruction sequence.
    pub(crate) fn emit_instructions(
        &self,
        indices: &IdsToIndices,
        local_indices: &IdHashMap<Local, u32>,
        dst: &mut wasm_encoder::Function,
        map: Option<&mut Vec<(InstrLocId, usize)>>,
    ) {
        emit::run(self, indices, local_indices, dst, map)
    }
}

fn block_result_tys(ctx: &ValidationContext, ty: wasmparser::BlockType) -> Result<Box<[ValType]>> {
    match ty {
        wasmparser::BlockType::Type(ty) => ValType::from_wasmparser_type(ty).map(Into::into),
        wasmparser::BlockType::FuncType(idx) => {
            let ty = ctx.indices.get_type(idx)?;
            Ok(ctx.module.types.results(ty).into())
        }
        wasmparser::BlockType::Empty => Ok(Box::new([])),
    }
}

fn block_param_tys(ctx: &ValidationContext, ty: wasmparser::BlockType) -> Result<Box<[ValType]>> {
    match ty {
        wasmparser::BlockType::Type(_) => Ok([][..].into()),
        wasmparser::BlockType::FuncType(idx) => {
            let ty = ctx.indices.get_type(idx)?;
            Ok(ctx.module.types.params(ty).into())
        }
        wasmparser::BlockType::Empty => Ok(Box::new([])),
    }
}

fn append_instruction(ctx: &mut ValidationContext, inst: Operator, loc: InstrLocId) {
    // NB. there's a lot of `unwrap()` here in this function, and that's because
    // the `Operator` was validated above to already be valid, so everything
    // should succeed.
    use crate::ir::ExtendedLoad::*;

    log::trace!("validate instruction: {:?}", inst);

    let const_ = |ctx: &mut ValidationContext, value| {
        ctx.alloc_instr(Const { value }, loc);
    };

    let unop = |ctx: &mut ValidationContext, op| {
        ctx.alloc_instr(Unop { op }, loc);
    };
    let binop = |ctx: &mut ValidationContext, op| {
        ctx.alloc_instr(Binop { op }, loc);
    };
    let ternop = |ctx: &mut ValidationContext, op| {
        ctx.alloc_instr(TernOp { op }, loc);
    };

    let mem_arg = |ctx: &mut ValidationContext, arg: &wasmparser::MemArg| -> (MemoryId, MemArg) {
        (
            ctx.indices.get_memory(arg.memory).unwrap(),
            MemArg {
                align: 1 << (arg.align as i32),
                offset: arg.offset as u32,
            },
        )
    };

    let load = |ctx: &mut ValidationContext, arg, kind| {
        let (memory, arg) = mem_arg(ctx, &arg);
        ctx.alloc_instr(Load { arg, kind, memory }, loc);
    };

    let store = |ctx: &mut ValidationContext, arg, kind| {
        let (memory, arg) = mem_arg(ctx, &arg);
        ctx.alloc_instr(Store { arg, kind, memory }, loc);
    };

    let atomicrmw = |ctx: &mut ValidationContext, arg, op, width| {
        let (memory, arg) = mem_arg(ctx, &arg);
        ctx.alloc_instr(
            AtomicRmw {
                arg,
                memory,
                op,
                width,
            },
            loc,
        );
    };

    let cmpxchg = |ctx: &mut ValidationContext, arg, width| {
        let (memory, arg) = mem_arg(ctx, &arg);
        ctx.alloc_instr(Cmpxchg { arg, memory, width }, loc);
    };

    let load_simd = |ctx: &mut ValidationContext, arg, kind| {
        let (memory, arg) = mem_arg(ctx, &arg);
        ctx.alloc_instr(LoadSimd { memory, arg, kind }, loc);
    };
    match inst {
        Operator::Call { function_index } => {
            let func = ctx.indices.get_func(function_index).unwrap();
            ctx.alloc_instr(Call { func }, loc);
        }
        Operator::CallIndirect {
            type_index,
            table_index,
        } => {
            let type_id = ctx.indices.get_type(type_index).unwrap();
            let table = ctx.indices.get_table(table_index).unwrap();
            ctx.alloc_instr(CallIndirect { table, ty: type_id }, loc);
        }
        Operator::LocalGet { local_index } => {
            let local = ctx.indices.get_local(ctx.func_id, local_index).unwrap();
            ctx.alloc_instr(LocalGet { local }, loc);
        }
        Operator::LocalSet { local_index } => {
            let local = ctx.indices.get_local(ctx.func_id, local_index).unwrap();
            ctx.alloc_instr(LocalSet { local }, loc);
        }
        Operator::LocalTee { local_index } => {
            let local = ctx.indices.get_local(ctx.func_id, local_index).unwrap();
            ctx.alloc_instr(LocalTee { local }, loc);
        }
        Operator::GlobalGet { global_index } => {
            let global = ctx.indices.get_global(global_index).unwrap();
            ctx.alloc_instr(GlobalGet { global }, loc);
        }
        Operator::GlobalSet { global_index } => {
            let global = ctx.indices.get_global(global_index).unwrap();
            ctx.alloc_instr(GlobalSet { global }, loc);
        }
        Operator::I32Const { value } => const_(ctx, Value::I32(value)),
        Operator::I64Const { value } => const_(ctx, Value::I64(value)),
        Operator::F32Const { value } => const_(ctx, Value::F32(f32::from_bits(value.bits()))),
        Operator::F64Const { value } => const_(ctx, Value::F64(f64::from_bits(value.bits()))),
        Operator::V128Const { value } => {
            let val = crate::const_expr::v128_to_u128(&value);
            const_(ctx, Value::V128(val))
        }
        Operator::I32Eqz => unop(ctx, UnaryOp::I32Eqz),
        Operator::I32Eq => binop(ctx, BinaryOp::I32Eq),
        Operator::I32Ne => binop(ctx, BinaryOp::I32Ne),
        Operator::I32LtS => binop(ctx, BinaryOp::I32LtS),
        Operator::I32LtU => binop(ctx, BinaryOp::I32LtU),
        Operator::I32GtS => binop(ctx, BinaryOp::I32GtS),
        Operator::I32GtU => binop(ctx, BinaryOp::I32GtU),
        Operator::I32LeS => binop(ctx, BinaryOp::I32LeS),
        Operator::I32LeU => binop(ctx, BinaryOp::I32LeU),
        Operator::I32GeS => binop(ctx, BinaryOp::I32GeS),
        Operator::I32GeU => binop(ctx, BinaryOp::I32GeU),

        Operator::I64Eqz => unop(ctx, UnaryOp::I64Eqz),
        Operator::I64Eq => binop(ctx, BinaryOp::I64Eq),
        Operator::I64Ne => binop(ctx, BinaryOp::I64Ne),
        Operator::I64LtS => binop(ctx, BinaryOp::I64LtS),
        Operator::I64LtU => binop(ctx, BinaryOp::I64LtU),
        Operator::I64GtS => binop(ctx, BinaryOp::I64GtS),
        Operator::I64GtU => binop(ctx, BinaryOp::I64GtU),
        Operator::I64LeS => binop(ctx, BinaryOp::I64LeS),
        Operator::I64LeU => binop(ctx, BinaryOp::I64LeU),
        Operator::I64GeS => binop(ctx, BinaryOp::I64GeS),
        Operator::I64GeU => binop(ctx, BinaryOp::I64GeU),

        Operator::F32Eq => binop(ctx, BinaryOp::F32Eq),
        Operator::F32Ne => binop(ctx, BinaryOp::F32Ne),
        Operator::F32Lt => binop(ctx, BinaryOp::F32Lt),
        Operator::F32Gt => binop(ctx, BinaryOp::F32Gt),
        Operator::F32Le => binop(ctx, BinaryOp::F32Le),
        Operator::F32Ge => binop(ctx, BinaryOp::F32Ge),

        Operator::F64Eq => binop(ctx, BinaryOp::F64Eq),
        Operator::F64Ne => binop(ctx, BinaryOp::F64Ne),
        Operator::F64Lt => binop(ctx, BinaryOp::F64Lt),
        Operator::F64Gt => binop(ctx, BinaryOp::F64Gt),
        Operator::F64Le => binop(ctx, BinaryOp::F64Le),
        Operator::F64Ge => binop(ctx, BinaryOp::F64Ge),

        Operator::I32Clz => unop(ctx, UnaryOp::I32Clz),
        Operator::I32Ctz => unop(ctx, UnaryOp::I32Ctz),
        Operator::I32Popcnt => unop(ctx, UnaryOp::I32Popcnt),
        Operator::I32Add => binop(ctx, BinaryOp::I32Add),
        Operator::I32Sub => binop(ctx, BinaryOp::I32Sub),
        Operator::I32Mul => binop(ctx, BinaryOp::I32Mul),
        Operator::I32DivS => binop(ctx, BinaryOp::I32DivS),
        Operator::I32DivU => binop(ctx, BinaryOp::I32DivU),
        Operator::I32RemS => binop(ctx, BinaryOp::I32RemS),
        Operator::I32RemU => binop(ctx, BinaryOp::I32RemU),
        Operator::I32And => binop(ctx, BinaryOp::I32And),
        Operator::I32Or => binop(ctx, BinaryOp::I32Or),
        Operator::I32Xor => binop(ctx, BinaryOp::I32Xor),
        Operator::I32Shl => binop(ctx, BinaryOp::I32Shl),
        Operator::I32ShrS => binop(ctx, BinaryOp::I32ShrS),
        Operator::I32ShrU => binop(ctx, BinaryOp::I32ShrU),
        Operator::I32Rotl => binop(ctx, BinaryOp::I32Rotl),
        Operator::I32Rotr => binop(ctx, BinaryOp::I32Rotr),

        Operator::I64Clz => unop(ctx, UnaryOp::I64Clz),
        Operator::I64Ctz => unop(ctx, UnaryOp::I64Ctz),
        Operator::I64Popcnt => unop(ctx, UnaryOp::I64Popcnt),
        Operator::I64Add => binop(ctx, BinaryOp::I64Add),
        Operator::I64Sub => binop(ctx, BinaryOp::I64Sub),
        Operator::I64Mul => binop(ctx, BinaryOp::I64Mul),
        Operator::I64DivS => binop(ctx, BinaryOp::I64DivS),
        Operator::I64DivU => binop(ctx, BinaryOp::I64DivU),
        Operator::I64RemS => binop(ctx, BinaryOp::I64RemS),
        Operator::I64RemU => binop(ctx, BinaryOp::I64RemU),
        Operator::I64And => binop(ctx, BinaryOp::I64And),
        Operator::I64Or => binop(ctx, BinaryOp::I64Or),
        Operator::I64Xor => binop(ctx, BinaryOp::I64Xor),
        Operator::I64Shl => binop(ctx, BinaryOp::I64Shl),
        Operator::I64ShrS => binop(ctx, BinaryOp::I64ShrS),
        Operator::I64ShrU => binop(ctx, BinaryOp::I64ShrU),
        Operator::I64Rotl => binop(ctx, BinaryOp::I64Rotl),
        Operator::I64Rotr => binop(ctx, BinaryOp::I64Rotr),

        Operator::F32Abs => unop(ctx, UnaryOp::F32Abs),
        Operator::F32Neg => unop(ctx, UnaryOp::F32Neg),
        Operator::F32Ceil => unop(ctx, UnaryOp::F32Ceil),
        Operator::F32Floor => unop(ctx, UnaryOp::F32Floor),
        Operator::F32Trunc => unop(ctx, UnaryOp::F32Trunc),
        Operator::F32Nearest => unop(ctx, UnaryOp::F32Nearest),
        Operator::F32Sqrt => unop(ctx, UnaryOp::F32Sqrt),
        Operator::F32Add => binop(ctx, BinaryOp::F32Add),
        Operator::F32Sub => binop(ctx, BinaryOp::F32Sub),
        Operator::F32Mul => binop(ctx, BinaryOp::F32Mul),
        Operator::F32Div => binop(ctx, BinaryOp::F32Div),
        Operator::F32Min => binop(ctx, BinaryOp::F32Min),
        Operator::F32Max => binop(ctx, BinaryOp::F32Max),
        Operator::F32Copysign => binop(ctx, BinaryOp::F32Copysign),

        Operator::F64Abs => unop(ctx, UnaryOp::F64Abs),
        Operator::F64Neg => unop(ctx, UnaryOp::F64Neg),
        Operator::F64Ceil => unop(ctx, UnaryOp::F64Ceil),
        Operator::F64Floor => unop(ctx, UnaryOp::F64Floor),
        Operator::F64Trunc => unop(ctx, UnaryOp::F64Trunc),
        Operator::F64Nearest => unop(ctx, UnaryOp::F64Nearest),
        Operator::F64Sqrt => unop(ctx, UnaryOp::F64Sqrt),
        Operator::F64Add => binop(ctx, BinaryOp::F64Add),
        Operator::F64Sub => binop(ctx, BinaryOp::F64Sub),
        Operator::F64Mul => binop(ctx, BinaryOp::F64Mul),
        Operator::F64Div => binop(ctx, BinaryOp::F64Div),
        Operator::F64Min => binop(ctx, BinaryOp::F64Min),
        Operator::F64Max => binop(ctx, BinaryOp::F64Max),
        Operator::F64Copysign => binop(ctx, BinaryOp::F64Copysign),

        Operator::I32WrapI64 => unop(ctx, UnaryOp::I32WrapI64),
        Operator::I32TruncF32S => unop(ctx, UnaryOp::I32TruncSF32),
        Operator::I32TruncF32U => unop(ctx, UnaryOp::I32TruncUF32),
        Operator::I32TruncF64S => unop(ctx, UnaryOp::I32TruncSF64),
        Operator::I32TruncF64U => unop(ctx, UnaryOp::I32TruncUF64),

        Operator::I64ExtendI32S => unop(ctx, UnaryOp::I64ExtendSI32),
        Operator::I64ExtendI32U => unop(ctx, UnaryOp::I64ExtendUI32),
        Operator::I64TruncF32S => unop(ctx, UnaryOp::I64TruncSF32),
        Operator::I64TruncF32U => unop(ctx, UnaryOp::I64TruncUF32),
        Operator::I64TruncF64S => unop(ctx, UnaryOp::I64TruncSF64),
        Operator::I64TruncF64U => unop(ctx, UnaryOp::I64TruncUF64),

        Operator::F32ConvertI32S => unop(ctx, UnaryOp::F32ConvertSI32),
        Operator::F32ConvertI32U => unop(ctx, UnaryOp::F32ConvertUI32),
        Operator::F32ConvertI64S => unop(ctx, UnaryOp::F32ConvertSI64),
        Operator::F32ConvertI64U => unop(ctx, UnaryOp::F32ConvertUI64),
        Operator::F32DemoteF64 => unop(ctx, UnaryOp::F32DemoteF64),

        Operator::F64ConvertI32S => unop(ctx, UnaryOp::F64ConvertSI32),
        Operator::F64ConvertI32U => unop(ctx, UnaryOp::F64ConvertUI32),
        Operator::F64ConvertI64S => unop(ctx, UnaryOp::F64ConvertSI64),
        Operator::F64ConvertI64U => unop(ctx, UnaryOp::F64ConvertUI64),
        Operator::F64PromoteF32 => unop(ctx, UnaryOp::F64PromoteF32),

        Operator::I32ReinterpretF32 => unop(ctx, UnaryOp::I32ReinterpretF32),
        Operator::I64ReinterpretF64 => unop(ctx, UnaryOp::I64ReinterpretF64),
        Operator::F32ReinterpretI32 => unop(ctx, UnaryOp::F32ReinterpretI32),
        Operator::F64ReinterpretI64 => unop(ctx, UnaryOp::F64ReinterpretI64),

        Operator::I32Extend8S => unop(ctx, UnaryOp::I32Extend8S),
        Operator::I32Extend16S => unop(ctx, UnaryOp::I32Extend16S),
        Operator::I64Extend8S => unop(ctx, UnaryOp::I64Extend8S),
        Operator::I64Extend16S => unop(ctx, UnaryOp::I64Extend16S),
        Operator::I64Extend32S => unop(ctx, UnaryOp::I64Extend32S),

        Operator::Drop => ctx.alloc_instr(Drop {}, loc),
        Operator::Select => ctx.alloc_instr(Select { ty: None }, loc),
        Operator::TypedSelect { ty } => {
            let ty = ValType::parse(&ty).unwrap();
            ctx.alloc_instr(Select { ty: Some(ty) }, loc);
        }
        Operator::Return => {
            ctx.alloc_instr(Return {}, loc);
            ctx.unreachable();
        }
        Operator::Unreachable => {
            ctx.alloc_instr(Unreachable {}, loc);
            ctx.unreachable();
        }
        Operator::Block { blockty } => {
            let param_tys = block_param_tys(ctx, blockty).unwrap();
            let result_tys = block_result_tys(ctx, blockty).unwrap();
            let seq = ctx
                .push_control(BlockKind::Block, param_tys, result_tys)
                .unwrap();
            ctx.alloc_instr_in_control(1, Block { seq }, loc).unwrap();
        }
        Operator::Loop { blockty } => {
            let result_tys = block_result_tys(ctx, blockty).unwrap();
            let param_tys = block_param_tys(ctx, blockty).unwrap();
            let seq = ctx
                .push_control(BlockKind::Loop, param_tys, result_tys)
                .unwrap();
            ctx.alloc_instr_in_control(1, Loop { seq }, loc).unwrap();
        }
        Operator::If { blockty } => {
            let result_tys = block_result_tys(ctx, blockty).unwrap();
            let param_tys = block_param_tys(ctx, blockty).unwrap();

            let consequent = ctx
                .push_control(BlockKind::If, param_tys, result_tys)
                .unwrap();
            ctx.if_else.push(context::IfElseState {
                start: loc,
                consequent,
                alternative: None,
            });
        }
        Operator::End => {
            let (frame, block) = ctx.pop_control().unwrap();

            ctx.func.block_mut(block).end = loc;

            // If we just finished an if/else block then the actual
            // instruction which produces the value will be an `IfElse` node,
            // not the block itself. Do some postprocessing here to create
            // such a node.
            match frame.kind {
                BlockKind::If | BlockKind::Else => {
                    let context::IfElseState {
                        start,
                        consequent,
                        alternative,
                    } = ctx.if_else.pop().unwrap();

                    let alternative = match alternative {
                        Some(alt) => {
                            debug_assert_eq!(frame.kind, BlockKind::Else);
                            alt
                        }
                        None => {
                            debug_assert_eq!(frame.kind, BlockKind::If);
                            let alternative = ctx
                                .push_control(
                                    BlockKind::Else,
                                    frame.start_types.clone(),
                                    frame.end_types.clone(),
                                )
                                .unwrap();
                            ctx.pop_control().unwrap();
                            alternative
                        }
                    };

                    ctx.alloc_instr(
                        IfElse {
                            consequent,
                            alternative,
                        },
                        start,
                    );
                }
                _ => {}
            }
        }
        Operator::Else => {
            let (frame, consequent) = ctx.pop_control().unwrap();
            // An `else` instruction is only valid immediately inside an if/else
            // block which is denoted by the `IfElse` block kind.
            match frame.kind {
                BlockKind::If => {}
                _ => panic!("`else` without a leading `if`"),
            }

            ctx.func.block_mut(consequent).end = loc;

            // But we still need to parse the alternative block, so allocate the
            // block here to parse.
            let alternative = ctx
                .push_control(BlockKind::Else, frame.start_types, frame.end_types)
                .unwrap();
            let last = ctx.if_else.last_mut().unwrap();
            if last.alternative.is_some() {
                panic!("`else` without a leading `if`")
            }
            last.alternative = Some(alternative);
        }
        Operator::Br { relative_depth } => {
            let n = relative_depth as usize;
            let block = ctx.control(n).unwrap().block;
            ctx.alloc_instr(Br { block }, loc);
            ctx.unreachable();
        }
        Operator::BrIf { relative_depth } => {
            let n = relative_depth as usize;
            let block = ctx.control(n).unwrap().block;
            ctx.alloc_instr(BrIf { block }, loc);
        }

        Operator::BrTable { targets } => {
            let mut blocks = Vec::with_capacity(targets.len() as usize);
            let default = ctx.control(targets.default() as usize).unwrap().block;
            for target in targets.targets() {
                let target = target.unwrap();
                let control = ctx.control(target as usize).unwrap();

                blocks.push(control.block);
            }
            ctx.alloc_instr(
                BrTable {
                    blocks: blocks.into(),
                    default,
                },
                loc,
            );
            ctx.unreachable();
        }

        Operator::MemorySize { mem, .. } => {
            let memory = ctx.indices.get_memory(mem).unwrap();
            ctx.alloc_instr(MemorySize { memory }, loc);
        }
        Operator::MemoryGrow { mem, .. } => {
            let memory = ctx.indices.get_memory(mem).unwrap();
            ctx.alloc_instr(MemoryGrow { memory }, loc);
        }
        Operator::MemoryInit { data_index, mem } => {
            let memory = ctx.indices.get_memory(mem).unwrap();
            let data = ctx.indices.get_data(data_index).unwrap();
            ctx.alloc_instr(MemoryInit { memory, data }, loc);
        }
        Operator::DataDrop { data_index } => {
            let data = ctx.indices.get_data(data_index).unwrap();
            ctx.alloc_instr(DataDrop { data }, loc);
        }
        Operator::MemoryCopy { dst_mem, src_mem } => {
            let src = ctx.indices.get_memory(src_mem).unwrap();
            let dst = ctx.indices.get_memory(dst_mem).unwrap();
            ctx.alloc_instr(MemoryCopy { src, dst }, loc);
        }
        Operator::MemoryFill { mem } => {
            let memory = ctx.indices.get_memory(mem).unwrap();
            ctx.alloc_instr(MemoryFill { memory }, loc);
        }

        Operator::Nop => {}

        Operator::I32Load { memarg } => load(ctx, memarg, LoadKind::I32 { atomic: false }),
        Operator::I64Load { memarg } => load(ctx, memarg, LoadKind::I64 { atomic: false }),
        Operator::F32Load { memarg } => load(ctx, memarg, LoadKind::F32),
        Operator::F64Load { memarg } => load(ctx, memarg, LoadKind::F64),
        Operator::V128Load { memarg } => load(ctx, memarg, LoadKind::V128),
        Operator::I32Load8S { memarg } => load(ctx, memarg, LoadKind::I32_8 { kind: SignExtend }),
        Operator::I32Load8U { memarg } => load(ctx, memarg, LoadKind::I32_8 { kind: ZeroExtend }),
        Operator::I32Load16S { memarg } => load(ctx, memarg, LoadKind::I32_16 { kind: SignExtend }),
        Operator::I32Load16U { memarg } => load(ctx, memarg, LoadKind::I32_16 { kind: ZeroExtend }),
        Operator::I64Load8S { memarg } => load(ctx, memarg, LoadKind::I64_8 { kind: SignExtend }),
        Operator::I64Load8U { memarg } => load(ctx, memarg, LoadKind::I64_8 { kind: ZeroExtend }),
        Operator::I64Load16S { memarg } => load(ctx, memarg, LoadKind::I64_16 { kind: SignExtend }),
        Operator::I64Load16U { memarg } => load(ctx, memarg, LoadKind::I64_16 { kind: ZeroExtend }),
        Operator::I64Load32S { memarg } => load(ctx, memarg, LoadKind::I64_32 { kind: SignExtend }),
        Operator::I64Load32U { memarg } => load(ctx, memarg, LoadKind::I64_32 { kind: ZeroExtend }),

        Operator::I32Store { memarg } => store(ctx, memarg, StoreKind::I32 { atomic: false }),
        Operator::I64Store { memarg } => store(ctx, memarg, StoreKind::I64 { atomic: false }),
        Operator::F32Store { memarg } => store(ctx, memarg, StoreKind::F32),
        Operator::F64Store { memarg } => store(ctx, memarg, StoreKind::F64),
        Operator::V128Store { memarg } => store(ctx, memarg, StoreKind::V128),
        Operator::I32Store8 { memarg } => store(ctx, memarg, StoreKind::I32_8 { atomic: false }),
        Operator::I32Store16 { memarg } => store(ctx, memarg, StoreKind::I32_16 { atomic: false }),
        Operator::I64Store8 { memarg } => store(ctx, memarg, StoreKind::I64_8 { atomic: false }),
        Operator::I64Store16 { memarg } => store(ctx, memarg, StoreKind::I64_16 { atomic: false }),
        Operator::I64Store32 { memarg } => store(ctx, memarg, StoreKind::I64_32 { atomic: false }),

        Operator::AtomicFence => ctx.alloc_instr(AtomicFence {}, loc),

        Operator::I32AtomicLoad { memarg } => load(ctx, memarg, LoadKind::I32 { atomic: true }),
        Operator::I64AtomicLoad { memarg } => load(ctx, memarg, LoadKind::I64 { atomic: true }),
        Operator::I32AtomicLoad8U { memarg } => load(
            ctx,
            memarg,
            LoadKind::I32_8 {
                kind: ZeroExtendAtomic,
            },
        ),
        Operator::I32AtomicLoad16U { memarg } => load(
            ctx,
            memarg,
            LoadKind::I32_16 {
                kind: ZeroExtendAtomic,
            },
        ),
        Operator::I64AtomicLoad8U { memarg } => load(
            ctx,
            memarg,
            LoadKind::I64_8 {
                kind: ZeroExtendAtomic,
            },
        ),
        Operator::I64AtomicLoad16U { memarg } => load(
            ctx,
            memarg,
            LoadKind::I64_16 {
                kind: ZeroExtendAtomic,
            },
        ),
        Operator::I64AtomicLoad32U { memarg } => load(
            ctx,
            memarg,
            LoadKind::I64_32 {
                kind: ZeroExtendAtomic,
            },
        ),

        Operator::I32AtomicStore { memarg } => store(ctx, memarg, StoreKind::I32 { atomic: true }),
        Operator::I64AtomicStore { memarg } => store(ctx, memarg, StoreKind::I64 { atomic: true }),
        Operator::I32AtomicStore8 { memarg } => {
            store(ctx, memarg, StoreKind::I32_8 { atomic: true })
        }
        Operator::I32AtomicStore16 { memarg } => {
            store(ctx, memarg, StoreKind::I32_16 { atomic: true })
        }
        Operator::I64AtomicStore8 { memarg } => {
            store(ctx, memarg, StoreKind::I64_8 { atomic: true })
        }
        Operator::I64AtomicStore16 { memarg } => {
            store(ctx, memarg, StoreKind::I64_16 { atomic: true })
        }
        Operator::I64AtomicStore32 { memarg } => {
            store(ctx, memarg, StoreKind::I64_32 { atomic: true })
        }

        Operator::I32AtomicRmwAdd { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Add, AtomicWidth::I32);
        }
        Operator::I64AtomicRmwAdd { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Add, AtomicWidth::I64);
        }
        Operator::I32AtomicRmw8AddU { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Add, AtomicWidth::I32_8);
        }
        Operator::I32AtomicRmw16AddU { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Add, AtomicWidth::I32_16);
        }
        Operator::I64AtomicRmw8AddU { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Add, AtomicWidth::I64_8);
        }
        Operator::I64AtomicRmw16AddU { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Add, AtomicWidth::I64_16);
        }
        Operator::I64AtomicRmw32AddU { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Add, AtomicWidth::I64_32);
        }

        Operator::I32AtomicRmwSub { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Sub, AtomicWidth::I32);
        }
        Operator::I64AtomicRmwSub { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Sub, AtomicWidth::I64);
        }
        Operator::I32AtomicRmw8SubU { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Sub, AtomicWidth::I32_8);
        }
        Operator::I32AtomicRmw16SubU { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Sub, AtomicWidth::I32_16);
        }
        Operator::I64AtomicRmw8SubU { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Sub, AtomicWidth::I64_8);
        }
        Operator::I64AtomicRmw16SubU { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Sub, AtomicWidth::I64_16);
        }
        Operator::I64AtomicRmw32SubU { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Sub, AtomicWidth::I64_32);
        }

        Operator::I32AtomicRmwAnd { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::And, AtomicWidth::I32);
        }
        Operator::I64AtomicRmwAnd { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::And, AtomicWidth::I64);
        }
        Operator::I32AtomicRmw8AndU { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::And, AtomicWidth::I32_8);
        }
        Operator::I32AtomicRmw16AndU { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::And, AtomicWidth::I32_16);
        }
        Operator::I64AtomicRmw8AndU { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::And, AtomicWidth::I64_8);
        }
        Operator::I64AtomicRmw16AndU { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::And, AtomicWidth::I64_16);
        }
        Operator::I64AtomicRmw32AndU { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::And, AtomicWidth::I64_32);
        }

        Operator::I32AtomicRmwOr { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Or, AtomicWidth::I32);
        }
        Operator::I64AtomicRmwOr { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Or, AtomicWidth::I64);
        }
        Operator::I32AtomicRmw8OrU { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Or, AtomicWidth::I32_8);
        }
        Operator::I32AtomicRmw16OrU { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Or, AtomicWidth::I32_16);
        }
        Operator::I64AtomicRmw8OrU { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Or, AtomicWidth::I64_8);
        }
        Operator::I64AtomicRmw16OrU { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Or, AtomicWidth::I64_16);
        }
        Operator::I64AtomicRmw32OrU { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Or, AtomicWidth::I64_32);
        }

        Operator::I32AtomicRmwXor { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Xor, AtomicWidth::I32);
        }
        Operator::I64AtomicRmwXor { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Xor, AtomicWidth::I64);
        }
        Operator::I32AtomicRmw8XorU { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Xor, AtomicWidth::I32_8);
        }
        Operator::I32AtomicRmw16XorU { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Xor, AtomicWidth::I32_16);
        }
        Operator::I64AtomicRmw8XorU { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Xor, AtomicWidth::I64_8);
        }
        Operator::I64AtomicRmw16XorU { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Xor, AtomicWidth::I64_16);
        }
        Operator::I64AtomicRmw32XorU { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Xor, AtomicWidth::I64_32);
        }

        Operator::I32AtomicRmwXchg { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Xchg, AtomicWidth::I32);
        }
        Operator::I64AtomicRmwXchg { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Xchg, AtomicWidth::I64);
        }
        Operator::I32AtomicRmw8XchgU { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Xchg, AtomicWidth::I32_8);
        }
        Operator::I32AtomicRmw16XchgU { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Xchg, AtomicWidth::I32_16);
        }
        Operator::I64AtomicRmw8XchgU { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Xchg, AtomicWidth::I64_8);
        }
        Operator::I64AtomicRmw16XchgU { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Xchg, AtomicWidth::I64_16);
        }
        Operator::I64AtomicRmw32XchgU { memarg } => {
            atomicrmw(ctx, memarg, AtomicOp::Xchg, AtomicWidth::I64_32);
        }

        Operator::I32AtomicRmwCmpxchg { memarg } => {
            cmpxchg(ctx, memarg, AtomicWidth::I32);
        }
        Operator::I64AtomicRmwCmpxchg { memarg } => {
            cmpxchg(ctx, memarg, AtomicWidth::I64);
        }
        Operator::I32AtomicRmw8CmpxchgU { memarg } => {
            cmpxchg(ctx, memarg, AtomicWidth::I32_8);
        }
        Operator::I32AtomicRmw16CmpxchgU { memarg } => {
            cmpxchg(ctx, memarg, AtomicWidth::I32_16);
        }
        Operator::I64AtomicRmw8CmpxchgU { memarg } => {
            cmpxchg(ctx, memarg, AtomicWidth::I64_8);
        }
        Operator::I64AtomicRmw16CmpxchgU { memarg } => {
            cmpxchg(ctx, memarg, AtomicWidth::I64_16);
        }
        Operator::I64AtomicRmw32CmpxchgU { memarg } => {
            cmpxchg(ctx, memarg, AtomicWidth::I64_32);
        }
        Operator::MemoryAtomicNotify { ref memarg } => {
            let (memory, arg) = mem_arg(ctx, memarg);
            ctx.alloc_instr(AtomicNotify { memory, arg }, loc);
        }
        Operator::MemoryAtomicWait32 { ref memarg }
        | Operator::MemoryAtomicWait64 { ref memarg } => {
            let sixty_four = !matches!(inst, Operator::MemoryAtomicWait32 { .. });
            let (memory, arg) = mem_arg(ctx, memarg);
            ctx.alloc_instr(
                AtomicWait {
                    sixty_four,
                    memory,
                    arg,
                },
                loc,
            );
        }

        Operator::TableGet { table } => {
            let table = ctx.indices.get_table(table).unwrap();
            ctx.alloc_instr(TableGet { table }, loc);
        }
        Operator::TableSet { table } => {
            let table = ctx.indices.get_table(table).unwrap();
            ctx.alloc_instr(TableSet { table }, loc);
        }
        Operator::TableGrow { table } => {
            let table = ctx.indices.get_table(table).unwrap();
            ctx.alloc_instr(TableGrow { table }, loc);
        }
        Operator::TableSize { table } => {
            let table = ctx.indices.get_table(table).unwrap();
            ctx.alloc_instr(TableSize { table }, loc);
        }
        Operator::TableFill { table } => {
            let table = ctx.indices.get_table(table).unwrap();
            ctx.alloc_instr(TableFill { table }, loc);
        }
        Operator::RefNull { hty } => {
            let ty = match hty {
                wasmparser::HeapType::Abstract { shared: _, ty } => match ty {
                    wasmparser::AbstractHeapType::Func => RefType::Funcref,
                    wasmparser::AbstractHeapType::Extern => RefType::Externref,
                    other => {
                        panic!("unsupported abstract heap type for ref.null: {:?}", other)
                    }
                },
                wasmparser::HeapType::Concrete(_) => {
                    panic!("unsupported concrete heap type for ref.null")
                }
            };
            ctx.alloc_instr(RefNull { ty }, loc);
        }
        Operator::RefIsNull => {
            ctx.alloc_instr(RefIsNull {}, loc);
        }
        Operator::RefFunc { function_index } => {
            let func = ctx.indices.get_func(function_index).unwrap();
            ctx.alloc_instr(RefFunc { func }, loc);
        }

        Operator::I8x16Swizzle => {
            ctx.alloc_instr(I8x16Swizzle {}, loc);
        }

        Operator::I8x16Shuffle { lanes } => {
            ctx.alloc_instr(I8x16Shuffle { indices: lanes }, loc);
        }

        Operator::I8x16Splat => unop(ctx, UnaryOp::I8x16Splat),
        Operator::I8x16ExtractLaneS { lane: idx } => unop(ctx, UnaryOp::I8x16ExtractLaneS { idx }),
        Operator::I8x16ExtractLaneU { lane: idx } => unop(ctx, UnaryOp::I8x16ExtractLaneU { idx }),
        Operator::I8x16ReplaceLane { lane: idx } => binop(ctx, BinaryOp::I8x16ReplaceLane { idx }),
        Operator::I16x8Splat => unop(ctx, UnaryOp::I16x8Splat),
        Operator::I16x8ExtractLaneS { lane: idx } => unop(ctx, UnaryOp::I16x8ExtractLaneS { idx }),
        Operator::I16x8ExtractLaneU { lane: idx } => unop(ctx, UnaryOp::I16x8ExtractLaneU { idx }),
        Operator::I16x8ReplaceLane { lane: idx } => binop(ctx, BinaryOp::I16x8ReplaceLane { idx }),
        Operator::I32x4Splat => unop(ctx, UnaryOp::I32x4Splat),
        Operator::I32x4ExtractLane { lane: idx } => unop(ctx, UnaryOp::I32x4ExtractLane { idx }),
        Operator::I32x4ReplaceLane { lane: idx } => binop(ctx, BinaryOp::I32x4ReplaceLane { idx }),
        Operator::I64x2Splat => unop(ctx, UnaryOp::I64x2Splat),
        Operator::I64x2ExtractLane { lane: idx } => unop(ctx, UnaryOp::I64x2ExtractLane { idx }),
        Operator::I64x2ReplaceLane { lane: idx } => binop(ctx, BinaryOp::I64x2ReplaceLane { idx }),
        Operator::F32x4Splat => unop(ctx, UnaryOp::F32x4Splat),
        Operator::F32x4ExtractLane { lane: idx } => unop(ctx, UnaryOp::F32x4ExtractLane { idx }),
        Operator::F32x4ReplaceLane { lane: idx } => binop(ctx, BinaryOp::F32x4ReplaceLane { idx }),
        Operator::F64x2Splat => unop(ctx, UnaryOp::F64x2Splat),
        Operator::F64x2ExtractLane { lane: idx } => unop(ctx, UnaryOp::F64x2ExtractLane { idx }),
        Operator::F64x2ReplaceLane { lane: idx } => binop(ctx, BinaryOp::F64x2ReplaceLane { idx }),

        Operator::I8x16Eq => binop(ctx, BinaryOp::I8x16Eq),
        Operator::I8x16Ne => binop(ctx, BinaryOp::I8x16Ne),
        Operator::I8x16LtS => binop(ctx, BinaryOp::I8x16LtS),
        Operator::I8x16LtU => binop(ctx, BinaryOp::I8x16LtU),
        Operator::I8x16GtS => binop(ctx, BinaryOp::I8x16GtS),
        Operator::I8x16GtU => binop(ctx, BinaryOp::I8x16GtU),
        Operator::I8x16LeS => binop(ctx, BinaryOp::I8x16LeS),
        Operator::I8x16LeU => binop(ctx, BinaryOp::I8x16LeU),
        Operator::I8x16GeS => binop(ctx, BinaryOp::I8x16GeS),
        Operator::I8x16GeU => binop(ctx, BinaryOp::I8x16GeU),
        Operator::I16x8Eq => binop(ctx, BinaryOp::I16x8Eq),
        Operator::I16x8Ne => binop(ctx, BinaryOp::I16x8Ne),
        Operator::I16x8LtS => binop(ctx, BinaryOp::I16x8LtS),
        Operator::I16x8LtU => binop(ctx, BinaryOp::I16x8LtU),
        Operator::I16x8GtS => binop(ctx, BinaryOp::I16x8GtS),
        Operator::I16x8GtU => binop(ctx, BinaryOp::I16x8GtU),
        Operator::I16x8LeS => binop(ctx, BinaryOp::I16x8LeS),
        Operator::I16x8LeU => binop(ctx, BinaryOp::I16x8LeU),
        Operator::I16x8GeS => binop(ctx, BinaryOp::I16x8GeS),
        Operator::I16x8GeU => binop(ctx, BinaryOp::I16x8GeU),
        Operator::I32x4Eq => binop(ctx, BinaryOp::I32x4Eq),
        Operator::I32x4Ne => binop(ctx, BinaryOp::I32x4Ne),
        Operator::I32x4LtS => binop(ctx, BinaryOp::I32x4LtS),
        Operator::I32x4LtU => binop(ctx, BinaryOp::I32x4LtU),
        Operator::I32x4GtS => binop(ctx, BinaryOp::I32x4GtS),
        Operator::I32x4GtU => binop(ctx, BinaryOp::I32x4GtU),
        Operator::I32x4LeS => binop(ctx, BinaryOp::I32x4LeS),
        Operator::I32x4LeU => binop(ctx, BinaryOp::I32x4LeU),
        Operator::I32x4GeS => binop(ctx, BinaryOp::I32x4GeS),
        Operator::I32x4GeU => binop(ctx, BinaryOp::I32x4GeU),
        Operator::I64x2Eq => binop(ctx, BinaryOp::I64x2Eq),
        Operator::I64x2Ne => binop(ctx, BinaryOp::I64x2Ne),
        Operator::I64x2LtS => binop(ctx, BinaryOp::I64x2LtS),
        Operator::I64x2GtS => binop(ctx, BinaryOp::I64x2GtS),
        Operator::I64x2LeS => binop(ctx, BinaryOp::I64x2LeS),
        Operator::I64x2GeS => binop(ctx, BinaryOp::I64x2GeS),
        Operator::F32x4Eq => binop(ctx, BinaryOp::F32x4Eq),
        Operator::F32x4Ne => binop(ctx, BinaryOp::F32x4Ne),
        Operator::F32x4Lt => binop(ctx, BinaryOp::F32x4Lt),
        Operator::F32x4Gt => binop(ctx, BinaryOp::F32x4Gt),
        Operator::F32x4Le => binop(ctx, BinaryOp::F32x4Le),
        Operator::F32x4Ge => binop(ctx, BinaryOp::F32x4Ge),
        Operator::F64x2Eq => binop(ctx, BinaryOp::F64x2Eq),
        Operator::F64x2Ne => binop(ctx, BinaryOp::F64x2Ne),
        Operator::F64x2Lt => binop(ctx, BinaryOp::F64x2Lt),
        Operator::F64x2Gt => binop(ctx, BinaryOp::F64x2Gt),
        Operator::F64x2Le => binop(ctx, BinaryOp::F64x2Le),
        Operator::F64x2Ge => binop(ctx, BinaryOp::F64x2Ge),

        Operator::V128Not => unop(ctx, UnaryOp::V128Not),
        Operator::V128AnyTrue => unop(ctx, UnaryOp::V128AnyTrue),
        Operator::V128And => binop(ctx, BinaryOp::V128And),
        Operator::V128AndNot => binop(ctx, BinaryOp::V128AndNot),
        Operator::V128Or => binop(ctx, BinaryOp::V128Or),
        Operator::V128Xor => binop(ctx, BinaryOp::V128Xor),

        Operator::V128Bitselect => ctx.alloc_instr(V128Bitselect {}, loc),

        Operator::I8x16Abs => unop(ctx, UnaryOp::I8x16Abs),
        Operator::I8x16Popcnt => unop(ctx, UnaryOp::I8x16Popcnt),
        Operator::I8x16Neg => unop(ctx, UnaryOp::I8x16Neg),
        Operator::I8x16AllTrue => unop(ctx, UnaryOp::I8x16AllTrue),
        Operator::I8x16Shl => binop(ctx, BinaryOp::I8x16Shl),
        Operator::I8x16ShrS => binop(ctx, BinaryOp::I8x16ShrS),
        Operator::I8x16ShrU => binop(ctx, BinaryOp::I8x16ShrU),
        Operator::I8x16Add => binop(ctx, BinaryOp::I8x16Add),
        Operator::I8x16AddSatS => binop(ctx, BinaryOp::I8x16AddSatS),
        Operator::I8x16AddSatU => binop(ctx, BinaryOp::I8x16AddSatU),
        Operator::I8x16Sub => binop(ctx, BinaryOp::I8x16Sub),
        Operator::I8x16SubSatS => binop(ctx, BinaryOp::I8x16SubSatS),
        Operator::I8x16SubSatU => binop(ctx, BinaryOp::I8x16SubSatU),

        Operator::I16x8Abs => unop(ctx, UnaryOp::I16x8Abs),
        Operator::I16x8Neg => unop(ctx, UnaryOp::I16x8Neg),
        Operator::I16x8AllTrue => unop(ctx, UnaryOp::I16x8AllTrue),
        Operator::I16x8Shl => binop(ctx, BinaryOp::I16x8Shl),
        Operator::I16x8ShrS => binop(ctx, BinaryOp::I16x8ShrS),
        Operator::I16x8ShrU => binop(ctx, BinaryOp::I16x8ShrU),
        Operator::I16x8Add => binop(ctx, BinaryOp::I16x8Add),
        Operator::I16x8AddSatS => binop(ctx, BinaryOp::I16x8AddSatS),
        Operator::I16x8AddSatU => binop(ctx, BinaryOp::I16x8AddSatU),
        Operator::I16x8Sub => binop(ctx, BinaryOp::I16x8Sub),
        Operator::I16x8SubSatS => binop(ctx, BinaryOp::I16x8SubSatS),
        Operator::I16x8SubSatU => binop(ctx, BinaryOp::I16x8SubSatU),
        Operator::I16x8Mul => binop(ctx, BinaryOp::I16x8Mul),

        Operator::I32x4Abs => unop(ctx, UnaryOp::I32x4Abs),
        Operator::I32x4Neg => unop(ctx, UnaryOp::I32x4Neg),
        Operator::I32x4AllTrue => unop(ctx, UnaryOp::I32x4AllTrue),
        Operator::I32x4Shl => binop(ctx, BinaryOp::I32x4Shl),
        Operator::I32x4ShrS => binop(ctx, BinaryOp::I32x4ShrS),
        Operator::I32x4ShrU => binop(ctx, BinaryOp::I32x4ShrU),
        Operator::I32x4Add => binop(ctx, BinaryOp::I32x4Add),
        Operator::I32x4Sub => binop(ctx, BinaryOp::I32x4Sub),
        Operator::I32x4Mul => binop(ctx, BinaryOp::I32x4Mul),

        Operator::I64x2Abs => unop(ctx, UnaryOp::I64x2Abs),
        Operator::I64x2AllTrue => unop(ctx, UnaryOp::I64x2AllTrue),
        Operator::I64x2Neg => unop(ctx, UnaryOp::I64x2Neg),
        Operator::I64x2Shl => binop(ctx, BinaryOp::I64x2Shl),
        Operator::I64x2ShrS => binop(ctx, BinaryOp::I64x2ShrS),
        Operator::I64x2ShrU => binop(ctx, BinaryOp::I64x2ShrU),
        Operator::I64x2Add => binop(ctx, BinaryOp::I64x2Add),
        Operator::I64x2Sub => binop(ctx, BinaryOp::I64x2Sub),
        Operator::I64x2Mul => binop(ctx, BinaryOp::I64x2Mul),

        Operator::F32x4Abs => unop(ctx, UnaryOp::F32x4Abs),
        Operator::F32x4Neg => unop(ctx, UnaryOp::F32x4Neg),
        Operator::F32x4Sqrt => unop(ctx, UnaryOp::F32x4Sqrt),
        Operator::F32x4Add => binop(ctx, BinaryOp::F32x4Add),
        Operator::F32x4Sub => binop(ctx, BinaryOp::F32x4Sub),
        Operator::F32x4Mul => binop(ctx, BinaryOp::F32x4Mul),
        Operator::F32x4Div => binop(ctx, BinaryOp::F32x4Div),
        Operator::F32x4Min => binop(ctx, BinaryOp::F32x4Min),
        Operator::F32x4Max => binop(ctx, BinaryOp::F32x4Max),
        Operator::F32x4Ceil => unop(ctx, UnaryOp::F32x4Ceil),
        Operator::F32x4Floor => unop(ctx, UnaryOp::F32x4Floor),
        Operator::F32x4Trunc => unop(ctx, UnaryOp::F32x4Trunc),
        Operator::F32x4Nearest => unop(ctx, UnaryOp::F32x4Nearest),
        Operator::F32x4PMin => binop(ctx, BinaryOp::F32x4PMin),
        Operator::F32x4PMax => binop(ctx, BinaryOp::F32x4PMax),

        Operator::I16x8ExtAddPairwiseI8x16S => unop(ctx, UnaryOp::I16x8ExtAddPairwiseI8x16S),
        Operator::I16x8ExtAddPairwiseI8x16U => unop(ctx, UnaryOp::I16x8ExtAddPairwiseI8x16U),
        Operator::I32x4ExtAddPairwiseI16x8S => unop(ctx, UnaryOp::I32x4ExtAddPairwiseI16x8S),
        Operator::I32x4ExtAddPairwiseI16x8U => unop(ctx, UnaryOp::I32x4ExtAddPairwiseI16x8U),
        Operator::I64x2ExtendLowI32x4S => unop(ctx, UnaryOp::I64x2ExtendLowI32x4S),
        Operator::I64x2ExtendHighI32x4S => unop(ctx, UnaryOp::I64x2ExtendHighI32x4S),
        Operator::I64x2ExtendLowI32x4U => unop(ctx, UnaryOp::I64x2ExtendLowI32x4U),
        Operator::I64x2ExtendHighI32x4U => unop(ctx, UnaryOp::I64x2ExtendHighI32x4U),
        Operator::I32x4TruncSatF64x2SZero => unop(ctx, UnaryOp::I32x4TruncSatF64x2SZero),
        Operator::I32x4TruncSatF64x2UZero => unop(ctx, UnaryOp::I32x4TruncSatF64x2UZero),
        Operator::F64x2ConvertLowI32x4S => unop(ctx, UnaryOp::F64x2ConvertLowI32x4S),
        Operator::F64x2ConvertLowI32x4U => unop(ctx, UnaryOp::F64x2ConvertLowI32x4U),
        Operator::F32x4DemoteF64x2Zero => unop(ctx, UnaryOp::F32x4DemoteF64x2Zero),
        Operator::F64x2PromoteLowF32x4 => unop(ctx, UnaryOp::F64x2PromoteLowF32x4),

        Operator::I16x8Q15MulrSatS => binop(ctx, BinaryOp::I16x8Q15MulrSatS),
        Operator::I16x8ExtMulLowI8x16S => binop(ctx, BinaryOp::I16x8ExtMulLowI8x16S),
        Operator::I16x8ExtMulHighI8x16S => binop(ctx, BinaryOp::I16x8ExtMulHighI8x16S),
        Operator::I16x8ExtMulLowI8x16U => binop(ctx, BinaryOp::I16x8ExtMulLowI8x16U),
        Operator::I16x8ExtMulHighI8x16U => binop(ctx, BinaryOp::I16x8ExtMulHighI8x16U),
        Operator::I32x4ExtMulLowI16x8S => binop(ctx, BinaryOp::I32x4ExtMulLowI16x8S),
        Operator::I32x4ExtMulHighI16x8S => binop(ctx, BinaryOp::I32x4ExtMulHighI16x8S),
        Operator::I32x4ExtMulLowI16x8U => binop(ctx, BinaryOp::I32x4ExtMulLowI16x8U),
        Operator::I32x4ExtMulHighI16x8U => binop(ctx, BinaryOp::I32x4ExtMulHighI16x8U),
        Operator::I64x2ExtMulLowI32x4S => binop(ctx, BinaryOp::I64x2ExtMulLowI32x4S),
        Operator::I64x2ExtMulHighI32x4S => binop(ctx, BinaryOp::I64x2ExtMulHighI32x4S),
        Operator::I64x2ExtMulLowI32x4U => binop(ctx, BinaryOp::I64x2ExtMulLowI32x4U),
        Operator::I64x2ExtMulHighI32x4U => binop(ctx, BinaryOp::I64x2ExtMulHighI32x4U),

        Operator::F64x2Abs => unop(ctx, UnaryOp::F64x2Abs),
        Operator::F64x2Neg => unop(ctx, UnaryOp::F64x2Neg),
        Operator::F64x2Sqrt => unop(ctx, UnaryOp::F64x2Sqrt),
        Operator::F64x2Add => binop(ctx, BinaryOp::F64x2Add),
        Operator::F64x2Sub => binop(ctx, BinaryOp::F64x2Sub),
        Operator::F64x2Mul => binop(ctx, BinaryOp::F64x2Mul),
        Operator::F64x2Div => binop(ctx, BinaryOp::F64x2Div),
        Operator::F64x2Min => binop(ctx, BinaryOp::F64x2Min),
        Operator::F64x2Max => binop(ctx, BinaryOp::F64x2Max),
        Operator::F64x2Ceil => unop(ctx, UnaryOp::F64x2Ceil),
        Operator::F64x2Floor => unop(ctx, UnaryOp::F64x2Floor),
        Operator::F64x2Trunc => unop(ctx, UnaryOp::F64x2Trunc),
        Operator::F64x2Nearest => unop(ctx, UnaryOp::F64x2Nearest),
        Operator::F64x2PMin => binop(ctx, BinaryOp::F64x2PMin),
        Operator::F64x2PMax => binop(ctx, BinaryOp::F64x2PMax),

        Operator::I32x4TruncSatF32x4S => unop(ctx, UnaryOp::I32x4TruncSatF32x4S),
        Operator::I32x4TruncSatF32x4U => unop(ctx, UnaryOp::I32x4TruncSatF32x4U),
        Operator::F32x4ConvertI32x4S => unop(ctx, UnaryOp::F32x4ConvertI32x4S),
        Operator::F32x4ConvertI32x4U => unop(ctx, UnaryOp::F32x4ConvertI32x4U),

        Operator::I32TruncSatF32S => unop(ctx, UnaryOp::I32TruncSSatF32),
        Operator::I32TruncSatF32U => unop(ctx, UnaryOp::I32TruncUSatF32),
        Operator::I32TruncSatF64S => unop(ctx, UnaryOp::I32TruncSSatF64),
        Operator::I32TruncSatF64U => unop(ctx, UnaryOp::I32TruncUSatF64),
        Operator::I64TruncSatF32S => unop(ctx, UnaryOp::I64TruncSSatF32),
        Operator::I64TruncSatF32U => unop(ctx, UnaryOp::I64TruncUSatF32),
        Operator::I64TruncSatF64S => unop(ctx, UnaryOp::I64TruncSSatF64),
        Operator::I64TruncSatF64U => unop(ctx, UnaryOp::I64TruncUSatF64),

        Operator::V128Load8Splat { memarg } => load_simd(ctx, memarg, LoadSimdKind::Splat8),
        Operator::V128Load16Splat { memarg } => load_simd(ctx, memarg, LoadSimdKind::Splat16),
        Operator::V128Load32Splat { memarg } => load_simd(ctx, memarg, LoadSimdKind::Splat32),
        Operator::V128Load64Splat { memarg } => load_simd(ctx, memarg, LoadSimdKind::Splat64),
        Operator::V128Load32Zero { memarg } => load_simd(ctx, memarg, LoadSimdKind::V128Load32Zero),
        Operator::V128Load64Zero { memarg } => load_simd(ctx, memarg, LoadSimdKind::V128Load64Zero),

        Operator::V128Load8Lane { memarg, lane } => {
            load_simd(ctx, memarg, LoadSimdKind::V128Load8Lane(lane))
        }
        Operator::V128Load16Lane { memarg, lane } => {
            load_simd(ctx, memarg, LoadSimdKind::V128Load16Lane(lane))
        }
        Operator::V128Load32Lane { memarg, lane } => {
            load_simd(ctx, memarg, LoadSimdKind::V128Load32Lane(lane))
        }
        Operator::V128Load64Lane { memarg, lane } => {
            load_simd(ctx, memarg, LoadSimdKind::V128Load64Lane(lane))
        }
        Operator::V128Store8Lane { memarg, lane } => {
            load_simd(ctx, memarg, LoadSimdKind::V128Store8Lane(lane))
        }
        Operator::V128Store16Lane { memarg, lane } => {
            load_simd(ctx, memarg, LoadSimdKind::V128Store16Lane(lane))
        }
        Operator::V128Store32Lane { memarg, lane } => {
            load_simd(ctx, memarg, LoadSimdKind::V128Store32Lane(lane))
        }
        Operator::V128Store64Lane { memarg, lane } => {
            load_simd(ctx, memarg, LoadSimdKind::V128Store64Lane(lane))
        }
        Operator::I8x16NarrowI16x8S => binop(ctx, BinaryOp::I8x16NarrowI16x8S),
        Operator::I8x16NarrowI16x8U => binop(ctx, BinaryOp::I8x16NarrowI16x8U),
        Operator::I16x8NarrowI32x4S => binop(ctx, BinaryOp::I16x8NarrowI32x4S),
        Operator::I16x8NarrowI32x4U => binop(ctx, BinaryOp::I16x8NarrowI32x4U),
        Operator::I16x8ExtendLowI8x16S => unop(ctx, UnaryOp::I16x8WidenLowI8x16S),
        Operator::I16x8ExtendLowI8x16U => unop(ctx, UnaryOp::I16x8WidenLowI8x16U),
        Operator::I16x8ExtendHighI8x16S => unop(ctx, UnaryOp::I16x8WidenHighI8x16S),
        Operator::I16x8ExtendHighI8x16U => unop(ctx, UnaryOp::I16x8WidenHighI8x16U),
        Operator::I32x4ExtendLowI16x8S => unop(ctx, UnaryOp::I32x4WidenLowI16x8S),
        Operator::I32x4ExtendLowI16x8U => unop(ctx, UnaryOp::I32x4WidenLowI16x8U),
        Operator::I32x4ExtendHighI16x8S => unop(ctx, UnaryOp::I32x4WidenHighI16x8S),
        Operator::I32x4ExtendHighI16x8U => unop(ctx, UnaryOp::I32x4WidenHighI16x8U),
        Operator::V128Load8x8S { memarg } => load_simd(ctx, memarg, LoadSimdKind::V128Load8x8S),
        Operator::V128Load8x8U { memarg } => load_simd(ctx, memarg, LoadSimdKind::V128Load8x8U),
        Operator::V128Load16x4S { memarg } => load_simd(ctx, memarg, LoadSimdKind::V128Load16x4S),
        Operator::V128Load16x4U { memarg } => load_simd(ctx, memarg, LoadSimdKind::V128Load16x4U),
        Operator::V128Load32x2S { memarg } => load_simd(ctx, memarg, LoadSimdKind::V128Load32x2S),
        Operator::V128Load32x2U { memarg } => load_simd(ctx, memarg, LoadSimdKind::V128Load32x2U),
        Operator::I8x16AvgrU => binop(ctx, BinaryOp::I8x16AvgrU),
        Operator::I16x8AvgrU => binop(ctx, BinaryOp::I16x8AvgrU),

        Operator::I8x16MinS => binop(ctx, BinaryOp::I8x16MinS),
        Operator::I8x16MinU => binop(ctx, BinaryOp::I8x16MinU),
        Operator::I8x16MaxS => binop(ctx, BinaryOp::I8x16MaxS),
        Operator::I8x16MaxU => binop(ctx, BinaryOp::I8x16MaxU),
        Operator::I16x8MinS => binop(ctx, BinaryOp::I16x8MinS),
        Operator::I16x8MinU => binop(ctx, BinaryOp::I16x8MinU),
        Operator::I16x8MaxS => binop(ctx, BinaryOp::I16x8MaxS),
        Operator::I16x8MaxU => binop(ctx, BinaryOp::I16x8MaxU),
        Operator::I32x4MinS => binop(ctx, BinaryOp::I32x4MinS),
        Operator::I32x4MinU => binop(ctx, BinaryOp::I32x4MinU),
        Operator::I32x4MaxS => binop(ctx, BinaryOp::I32x4MaxS),
        Operator::I32x4MaxU => binop(ctx, BinaryOp::I32x4MaxU),

        Operator::I8x16Bitmask => unop(ctx, UnaryOp::I8x16Bitmask),
        Operator::I16x8Bitmask => unop(ctx, UnaryOp::I16x8Bitmask),
        Operator::I32x4Bitmask => unop(ctx, UnaryOp::I32x4Bitmask),
        Operator::I64x2Bitmask => unop(ctx, UnaryOp::I64x2Bitmask),

        Operator::I32x4DotI16x8S => binop(ctx, BinaryOp::I32x4DotI16x8S),

        Operator::TableCopy {
            src_table,
            dst_table,
        } => {
            let src = ctx.indices.get_table(src_table).unwrap();
            let dst = ctx.indices.get_table(dst_table).unwrap();
            ctx.alloc_instr(TableCopy { src, dst }, loc);
        }

        Operator::TableInit { elem_index, table } => {
            let elem = ctx.indices.get_element(elem_index).unwrap();
            let table = ctx.indices.get_table(table).unwrap();
            ctx.alloc_instr(TableInit { elem, table }, loc);
        }

        Operator::ElemDrop { elem_index } => {
            let elem = ctx.indices.get_element(elem_index).unwrap();
            ctx.alloc_instr(ElemDrop { elem }, loc);
        }

        Operator::ReturnCall { function_index } => {
            let func = ctx.indices.get_func(function_index).unwrap();
            ctx.alloc_instr(ReturnCall { func }, loc);
        }

        Operator::ReturnCallIndirect {
            type_index,
            table_index,
        } => {
            let ty = ctx.indices.get_type(type_index).unwrap();
            let table = ctx.indices.get_table(table_index).unwrap();
            ctx.alloc_instr(ReturnCallIndirect { ty, table }, loc);
        }

        Operator::I8x16RelaxedSwizzle => binop(ctx, BinaryOp::I8x16RelaxedSwizzle),
        Operator::I32x4RelaxedTruncF32x4S => unop(ctx, UnaryOp::I32x4RelaxedTruncF32x4S),
        Operator::I32x4RelaxedTruncF32x4U => unop(ctx, UnaryOp::I32x4RelaxedTruncF32x4U),
        Operator::I32x4RelaxedTruncF64x2SZero => unop(ctx, UnaryOp::I32x4RelaxedTruncF64x2SZero),
        Operator::I32x4RelaxedTruncF64x2UZero => unop(ctx, UnaryOp::I32x4RelaxedTruncF64x2UZero),
        Operator::F32x4RelaxedMadd => ternop(ctx, TernaryOp::F32x4RelaxedMadd),
        Operator::F32x4RelaxedNmadd => ternop(ctx, TernaryOp::F32x4RelaxedNmadd),
        Operator::F64x2RelaxedMadd => ternop(ctx, TernaryOp::F64x2RelaxedMadd),
        Operator::F64x2RelaxedNmadd => ternop(ctx, TernaryOp::F64x2RelaxedNmadd),
        Operator::I8x16RelaxedLaneselect => ternop(ctx, TernaryOp::I8x16RelaxedLaneselect),
        Operator::I16x8RelaxedLaneselect => ternop(ctx, TernaryOp::I16x8RelaxedLaneselect),
        Operator::I32x4RelaxedLaneselect => ternop(ctx, TernaryOp::I32x4RelaxedLaneselect),
        Operator::I64x2RelaxedLaneselect => ternop(ctx, TernaryOp::I64x2RelaxedLaneselect),
        Operator::F32x4RelaxedMin => binop(ctx, BinaryOp::F32x4RelaxedMin),
        Operator::F32x4RelaxedMax => binop(ctx, BinaryOp::F32x4RelaxedMax),
        Operator::F64x2RelaxedMin => binop(ctx, BinaryOp::F64x2RelaxedMin),
        Operator::F64x2RelaxedMax => binop(ctx, BinaryOp::F64x2RelaxedMax),
        Operator::I16x8RelaxedQ15mulrS => binop(ctx, BinaryOp::I16x8RelaxedQ15mulrS),
        Operator::I16x8RelaxedDotI8x16I7x16S => binop(ctx, BinaryOp::I16x8RelaxedDotI8x16I7x16S),
        Operator::I32x4RelaxedDotI8x16I7x16AddS => {
            ternop(ctx, TernaryOp::I32x4RelaxedDotI8x16I7x16AddS)
        }

        // List all unimplmented operators instead of have a catch-all arm.
        // So that future upgrades won't miss additions to this list that may be important to know.
        Operator::TryTable { try_table: _ }
        | Operator::Throw { tag_index: _ }
        | Operator::ThrowRef
        | Operator::Try { blockty: _ }
        | Operator::Catch { tag_index: _ }
        | Operator::Rethrow { relative_depth: _ }
        | Operator::Delegate { relative_depth: _ }
        | Operator::CatchAll
        | Operator::RefEq
        | Operator::StructNew {
            struct_type_index: _,
        }
        | Operator::StructNewDefault {
            struct_type_index: _,
        }
        | Operator::StructGet {
            struct_type_index: _,
            field_index: _,
        }
        | Operator::StructGetS {
            struct_type_index: _,
            field_index: _,
        }
        | Operator::StructGetU {
            struct_type_index: _,
            field_index: _,
        }
        | Operator::StructSet {
            struct_type_index: _,
            field_index: _,
        }
        | Operator::ArrayNew {
            array_type_index: _,
        }
        | Operator::ArrayNewDefault {
            array_type_index: _,
        }
        | Operator::ArrayNewFixed {
            array_type_index: _,
            array_size: _,
        }
        | Operator::ArrayNewData {
            array_type_index: _,
            array_data_index: _,
        }
        | Operator::ArrayNewElem {
            array_type_index: _,
            array_elem_index: _,
        }
        | Operator::ArrayGet {
            array_type_index: _,
        }
        | Operator::ArrayGetS {
            array_type_index: _,
        }
        | Operator::ArrayGetU {
            array_type_index: _,
        }
        | Operator::ArraySet {
            array_type_index: _,
        }
        | Operator::ArrayLen
        | Operator::ArrayFill {
            array_type_index: _,
        }
        | Operator::ArrayCopy {
            array_type_index_dst: _,
            array_type_index_src: _,
        }
        | Operator::ArrayInitData {
            array_type_index: _,
            array_data_index: _,
        }
        | Operator::ArrayInitElem {
            array_type_index: _,
            array_elem_index: _,
        }
        | Operator::RefTestNonNull { hty: _ }
        | Operator::RefTestNullable { hty: _ }
        | Operator::RefCastNonNull { hty: _ }
        | Operator::RefCastNullable { hty: _ }
        | Operator::BrOnCast {
            relative_depth: _,
            from_ref_type: _,
            to_ref_type: _,
        }
        | Operator::BrOnCastFail {
            relative_depth: _,
            from_ref_type: _,
            to_ref_type: _,
        }
        | Operator::AnyConvertExtern
        | Operator::ExternConvertAny
        | Operator::RefI31
        | Operator::I31GetS
        | Operator::I31GetU
        | Operator::MemoryDiscard { mem: _ }
        | Operator::GlobalAtomicGet {
            ordering: _,
            global_index: _,
        }
        | Operator::GlobalAtomicSet {
            ordering: _,
            global_index: _,
        }
        | Operator::GlobalAtomicRmwAdd {
            ordering: _,
            global_index: _,
        }
        | Operator::GlobalAtomicRmwSub {
            ordering: _,
            global_index: _,
        }
        | Operator::GlobalAtomicRmwAnd {
            ordering: _,
            global_index: _,
        }
        | Operator::GlobalAtomicRmwOr {
            ordering: _,
            global_index: _,
        }
        | Operator::GlobalAtomicRmwXor {
            ordering: _,
            global_index: _,
        }
        | Operator::GlobalAtomicRmwXchg {
            ordering: _,
            global_index: _,
        }
        | Operator::GlobalAtomicRmwCmpxchg {
            ordering: _,
            global_index: _,
        }
        | Operator::TableAtomicGet {
            ordering: _,
            table_index: _,
        }
        | Operator::TableAtomicSet {
            ordering: _,
            table_index: _,
        }
        | Operator::TableAtomicRmwXchg {
            ordering: _,
            table_index: _,
        }
        | Operator::TableAtomicRmwCmpxchg {
            ordering: _,
            table_index: _,
        }
        | Operator::StructAtomicGet {
            ordering: _,
            struct_type_index: _,
            field_index: _,
        }
        | Operator::StructAtomicGetS {
            ordering: _,
            struct_type_index: _,
            field_index: _,
        }
        | Operator::StructAtomicGetU {
            ordering: _,
            struct_type_index: _,
            field_index: _,
        }
        | Operator::StructAtomicSet {
            ordering: _,
            struct_type_index: _,
            field_index: _,
        }
        | Operator::StructAtomicRmwAdd {
            ordering: _,
            struct_type_index: _,
            field_index: _,
        }
        | Operator::StructAtomicRmwSub {
            ordering: _,
            struct_type_index: _,
            field_index: _,
        }
        | Operator::StructAtomicRmwAnd {
            ordering: _,
            struct_type_index: _,
            field_index: _,
        }
        | Operator::StructAtomicRmwOr {
            ordering: _,
            struct_type_index: _,
            field_index: _,
        }
        | Operator::StructAtomicRmwXor {
            ordering: _,
            struct_type_index: _,
            field_index: _,
        }
        | Operator::StructAtomicRmwXchg {
            ordering: _,
            struct_type_index: _,
            field_index: _,
        }
        | Operator::StructAtomicRmwCmpxchg {
            ordering: _,
            struct_type_index: _,
            field_index: _,
        }
        | Operator::ArrayAtomicGet {
            ordering: _,
            array_type_index: _,
        }
        | Operator::ArrayAtomicGetS {
            ordering: _,
            array_type_index: _,
        }
        | Operator::ArrayAtomicGetU {
            ordering: _,
            array_type_index: _,
        }
        | Operator::ArrayAtomicSet {
            ordering: _,
            array_type_index: _,
        }
        | Operator::ArrayAtomicRmwAdd {
            ordering: _,
            array_type_index: _,
        }
        | Operator::ArrayAtomicRmwSub {
            ordering: _,
            array_type_index: _,
        }
        | Operator::ArrayAtomicRmwAnd {
            ordering: _,
            array_type_index: _,
        }
        | Operator::ArrayAtomicRmwOr {
            ordering: _,
            array_type_index: _,
        }
        | Operator::ArrayAtomicRmwXor {
            ordering: _,
            array_type_index: _,
        }
        | Operator::ArrayAtomicRmwXchg {
            ordering: _,
            array_type_index: _,
        }
        | Operator::ArrayAtomicRmwCmpxchg {
            ordering: _,
            array_type_index: _,
        }
        | Operator::RefI31Shared
        | Operator::CallRef { type_index: _ }
        | Operator::ReturnCallRef { type_index: _ }
        | Operator::RefAsNonNull
        | Operator::BrOnNull { relative_depth: _ }
        | Operator::BrOnNonNull { relative_depth: _ } => {
            unimplemented!("unsupported operator: {:?}", inst)
        }
    }
}
