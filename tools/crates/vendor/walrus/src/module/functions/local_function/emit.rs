use crate::emit::IdsToIndices;
use crate::map::IdHashMap;
use crate::module::functions::LocalFunction;
use crate::module::memories::MemoryId;
use crate::{ir::*, RefType};
use wasm_encoder::Instruction;

pub(crate) fn run(
    func: &LocalFunction,
    indices: &IdsToIndices,
    local_indices: &IdHashMap<Local, u32>,
    encoder: &mut wasm_encoder::Function,
    map: Option<&mut Vec<(InstrLocId, usize)>>,
) {
    let v = &mut Emit {
        indices,
        blocks: vec![],
        block_kinds: vec![BlockKind::FunctionEntry],
        encoder,
        local_indices,
        map,
    };
    dfs_in_order(v, func, func.entry_block());

    debug_assert!(v.blocks.is_empty());
    debug_assert!(v.block_kinds.is_empty());
}

struct Emit<'a> {
    // Needed so we can map locals to their indices.
    indices: &'a IdsToIndices,
    local_indices: &'a IdHashMap<Local, u32>,

    // Stack of blocks that we are currently emitting instructions for. A branch
    // is only valid if its target is one of these blocks. See also the
    // `branch_target` method.
    blocks: Vec<InstrSeqId>,

    // The kinds of blocks we have on the stack. This is parallel to `blocks`
    // 99% of the time, except we push a new block kind in `visit_instr`, before
    // we push the block in `start_instr_seq`, because this is when we know the
    // kind.
    block_kinds: Vec<BlockKind>,

    // The instruction sequence we are building up to emit.
    encoder: &'a mut wasm_encoder::Function,

    // Encoded ExprId -> offset map.
    map: Option<&'a mut Vec<(InstrLocId, usize)>>,
}

impl<'instr> Visitor<'instr> for Emit<'_> {
    fn start_instr_seq(&mut self, seq: &'instr InstrSeq) {
        self.blocks.push(seq.id());
        debug_assert_eq!(self.blocks.len(), self.block_kinds.len());

        match self.block_kinds.last().unwrap() {
            BlockKind::Block => {
                self.encoder
                    .instruction(&Instruction::Block(self.block_type(seq.ty)));
            }
            BlockKind::Loop => {
                self.encoder
                    .instruction(&Instruction::Loop(self.block_type(seq.ty)));
            }
            BlockKind::If => {
                self.encoder
                    .instruction(&Instruction::If(self.block_type(seq.ty)));
            }
            // Function entries are implicitly started, and don't need any
            // opcode to start them. `Else` blocks are started when `If` blocks
            // end in an `else` opcode, which we handle in `end_instr_seq`
            // below.
            BlockKind::FunctionEntry | BlockKind::Else => {}
        }
    }

    fn end_instr_seq(&mut self, seq: &'instr InstrSeq) {
        let popped_block = self.blocks.pop();
        debug_assert_eq!(popped_block, Some(seq.id()));

        let popped_kind = self.block_kinds.pop();
        debug_assert!(popped_kind.is_some());

        debug_assert_eq!(self.blocks.len(), self.block_kinds.len());

        if let Some(map) = self.map.as_mut() {
            let pos = self.encoder.byte_len();
            // Save the encoded_at position for the specified ExprId.
            map.push((seq.end, pos));
        }

        if let BlockKind::If = popped_kind.unwrap() {
            // We're about to visit the `else` block, so push its kind.
            //
            // TODO: don't emit `else` for empty else blocks
            self.block_kinds.push(BlockKind::Else);
            self.encoder.instruction(&Instruction::Else);
        } else {
            self.encoder.instruction(&Instruction::End);
        }
    }

    fn visit_instr(&mut self, instr: &'instr Instr, instr_loc: &'instr InstrLocId) {
        use self::Instr::*;

        if let Some(map) = self.map.as_mut() {
            let pos = self.encoder.byte_len();
            // Save the encoded_at position for the specified ExprId.
            map.push((*instr_loc, pos));
        }

        let is_block = match instr {
            Block(_) => {
                self.block_kinds.push(BlockKind::Block);
                true
            }
            Loop(_) => {
                self.block_kinds.push(BlockKind::Loop);
                true
            }

            // Push the `if` block kind, and then when we finish encoding the
            // `if` block, we'll pop the `if` kind and push the `else`
            // kind. This allows us to maintain the `self.blocks.len() ==
            // self.block_kinds.len()` invariant.
            IfElse(_) => {
                self.block_kinds.push(BlockKind::If);
                true
            }
            _ => false,
        };

        if is_block {
            return;
        }

        self.encoder.instruction(&match instr {
            Block(_) | Loop(_) | IfElse(_) => unreachable!(),

            BrTable(e) => {
                let default = self.branch_target(e.default);
                let targets: Vec<u32> = e.blocks.iter().map(|b| self.branch_target(*b)).collect();
                Instruction::BrTable(targets.into(), default)
            }

            Const(c) => match &c.value {
                Value::I32(v) => Instruction::I32Const(*v),
                Value::I64(v) => Instruction::I64Const(*v),
                Value::F32(v) => Instruction::F32Const(*v),
                Value::F64(v) => Instruction::F64Const(*v),
                Value::V128(v) => Instruction::V128Const(*v as i128),
            },

            Drop(_) => Instruction::Drop,
            Return(_) => Instruction::Return,

            MemorySize(e) => {
                let idx = self.indices.get_memory_index(e.memory);
                Instruction::MemorySize(idx)
            }

            MemoryGrow(e) => {
                let idx = self.indices.get_memory_index(e.memory);
                Instruction::MemoryGrow(idx)
            }

            MemoryInit(e) => {
                let data_index = self.indices.get_data_index(e.data);
                let mem = self.indices.get_memory_index(e.memory);
                Instruction::MemoryInit { mem, data_index }
            }

            DataDrop(e) => {
                let idx = self.indices.get_data_index(e.data);
                Instruction::DataDrop(idx)
            }

            MemoryCopy(e) => {
                let src_mem = self.indices.get_memory_index(e.src);
                let dst_mem = self.indices.get_memory_index(e.dst);
                Instruction::MemoryCopy { src_mem, dst_mem }
            }

            MemoryFill(e) => {
                let idx = self.indices.get_memory_index(e.memory);
                Instruction::MemoryFill(idx)
            }

            TernOp(e) => {
                use crate::ir::TernaryOp::*;

                match e.op {
                    F32x4RelaxedMadd => Instruction::F32x4RelaxedMadd,
                    F32x4RelaxedNmadd => Instruction::F32x4RelaxedNmadd,
                    F64x2RelaxedMadd => Instruction::F64x2RelaxedMadd,
                    F64x2RelaxedNmadd => Instruction::F64x2RelaxedNmadd,
                    I8x16RelaxedLaneselect => Instruction::I8x16RelaxedLaneselect,
                    I16x8RelaxedLaneselect => Instruction::I16x8RelaxedLaneselect,
                    I32x4RelaxedLaneselect => Instruction::I32x4RelaxedLaneselect,
                    I64x2RelaxedLaneselect => Instruction::I64x2RelaxedLaneselect,
                    I32x4RelaxedDotI8x16I7x16AddS => Instruction::I32x4RelaxedDotI8x16I7x16AddS,
                }
            }

            Binop(e) => {
                use crate::ir::BinaryOp::*;

                match e.op {
                    I32Eq => Instruction::I32Eq,
                    I32Ne => Instruction::I32Ne,
                    I32LtS => Instruction::I32LtS,
                    I32LtU => Instruction::I32LtU,
                    I32GtS => Instruction::I32GtS,
                    I32GtU => Instruction::I32GtU,
                    I32LeS => Instruction::I32LeS,
                    I32LeU => Instruction::I32LeU,
                    I32GeS => Instruction::I32GeS,
                    I32GeU => Instruction::I32GeU,

                    I64Eq => Instruction::I64Eq,
                    I64Ne => Instruction::I64Ne,
                    I64LtS => Instruction::I64LtS,
                    I64LtU => Instruction::I64LtU,
                    I64GtS => Instruction::I64GtS,
                    I64GtU => Instruction::I64GtU,
                    I64LeS => Instruction::I64LeS,
                    I64LeU => Instruction::I64LeU,
                    I64GeS => Instruction::I64GeS,
                    I64GeU => Instruction::I64GeU,

                    F32Eq => Instruction::F32Eq,
                    F32Ne => Instruction::F32Ne,
                    F32Lt => Instruction::F32Lt,
                    F32Gt => Instruction::F32Gt,
                    F32Le => Instruction::F32Le,
                    F32Ge => Instruction::F32Ge,

                    F64Eq => Instruction::F64Eq,
                    F64Ne => Instruction::F64Ne,
                    F64Lt => Instruction::F64Lt,
                    F64Gt => Instruction::F64Gt,
                    F64Le => Instruction::F64Le,
                    F64Ge => Instruction::F64Ge,

                    I32Add => Instruction::I32Add,
                    I32Sub => Instruction::I32Sub,
                    I32Mul => Instruction::I32Mul,
                    I32DivS => Instruction::I32DivS,
                    I32DivU => Instruction::I32DivU,
                    I32RemS => Instruction::I32RemS,
                    I32RemU => Instruction::I32RemU,
                    I32And => Instruction::I32And,
                    I32Or => Instruction::I32Or,
                    I32Xor => Instruction::I32Xor,
                    I32Shl => Instruction::I32Shl,
                    I32ShrS => Instruction::I32ShrS,
                    I32ShrU => Instruction::I32ShrU,
                    I32Rotl => Instruction::I32Rotl,
                    I32Rotr => Instruction::I32Rotr,

                    I64Add => Instruction::I64Add,
                    I64Sub => Instruction::I64Sub,
                    I64Mul => Instruction::I64Mul,
                    I64DivS => Instruction::I64DivS,
                    I64DivU => Instruction::I64DivU,
                    I64RemS => Instruction::I64RemS,
                    I64RemU => Instruction::I64RemU,
                    I64And => Instruction::I64And,
                    I64Or => Instruction::I64Or,
                    I64Xor => Instruction::I64Xor,
                    I64Shl => Instruction::I64Shl,
                    I64ShrS => Instruction::I64ShrS,
                    I64ShrU => Instruction::I64ShrU,
                    I64Rotl => Instruction::I64Rotl,
                    I64Rotr => Instruction::I64Rotr,

                    F32Add => Instruction::F32Add,
                    F32Sub => Instruction::F32Sub,
                    F32Mul => Instruction::F32Mul,
                    F32Div => Instruction::F32Div,
                    F32Min => Instruction::F32Min,
                    F32Max => Instruction::F32Max,
                    F32Copysign => Instruction::F32Copysign,

                    F64Add => Instruction::F64Add,
                    F64Sub => Instruction::F64Sub,
                    F64Mul => Instruction::F64Mul,
                    F64Div => Instruction::F64Div,
                    F64Min => Instruction::F64Min,
                    F64Max => Instruction::F64Max,
                    F64Copysign => Instruction::F64Copysign,

                    I8x16ReplaceLane { idx } => Instruction::I8x16ReplaceLane(idx),
                    I16x8ReplaceLane { idx } => Instruction::I16x8ReplaceLane(idx),
                    I32x4ReplaceLane { idx } => Instruction::I32x4ReplaceLane(idx),
                    I64x2ReplaceLane { idx } => Instruction::I64x2ReplaceLane(idx),
                    F32x4ReplaceLane { idx } => Instruction::F32x4ReplaceLane(idx),
                    F64x2ReplaceLane { idx } => Instruction::F64x2ReplaceLane(idx),

                    I8x16Eq => Instruction::I8x16Eq,
                    I8x16Ne => Instruction::I8x16Ne,
                    I8x16LtS => Instruction::I8x16LtS,
                    I8x16LtU => Instruction::I8x16LtU,
                    I8x16GtS => Instruction::I8x16GtS,
                    I8x16GtU => Instruction::I8x16GtU,
                    I8x16LeS => Instruction::I8x16LeS,
                    I8x16LeU => Instruction::I8x16LeU,
                    I8x16GeS => Instruction::I8x16GeS,
                    I8x16GeU => Instruction::I8x16GeU,

                    I16x8Eq => Instruction::I16x8Eq,
                    I16x8Ne => Instruction::I16x8Ne,
                    I16x8LtS => Instruction::I16x8LtS,
                    I16x8LtU => Instruction::I16x8LtU,
                    I16x8GtS => Instruction::I16x8GtS,
                    I16x8GtU => Instruction::I16x8GtU,
                    I16x8LeS => Instruction::I16x8LeS,
                    I16x8LeU => Instruction::I16x8LeU,
                    I16x8GeS => Instruction::I16x8GeS,
                    I16x8GeU => Instruction::I16x8GeU,

                    I32x4Eq => Instruction::I32x4Eq,
                    I32x4Ne => Instruction::I32x4Ne,
                    I32x4LtS => Instruction::I32x4LtS,
                    I32x4LtU => Instruction::I32x4LtU,
                    I32x4GtS => Instruction::I32x4GtS,
                    I32x4GtU => Instruction::I32x4GtU,
                    I32x4LeS => Instruction::I32x4LeS,
                    I32x4LeU => Instruction::I32x4LeU,
                    I32x4GeS => Instruction::I32x4GeS,
                    I32x4GeU => Instruction::I32x4GeU,

                    I64x2Eq => Instruction::I64x2Eq,
                    I64x2Ne => Instruction::I64x2Ne,
                    I64x2LtS => Instruction::I64x2LtS,
                    I64x2GtS => Instruction::I64x2GtS,
                    I64x2LeS => Instruction::I64x2LeS,
                    I64x2GeS => Instruction::I64x2GeS,

                    F32x4Eq => Instruction::F32x4Eq,
                    F32x4Ne => Instruction::F32x4Ne,
                    F32x4Lt => Instruction::F32x4Lt,
                    F32x4Gt => Instruction::F32x4Gt,
                    F32x4Le => Instruction::F32x4Le,
                    F32x4Ge => Instruction::F32x4Ge,

                    F64x2Eq => Instruction::F64x2Eq,
                    F64x2Ne => Instruction::F64x2Ne,
                    F64x2Lt => Instruction::F64x2Lt,
                    F64x2Gt => Instruction::F64x2Gt,
                    F64x2Le => Instruction::F64x2Le,
                    F64x2Ge => Instruction::F64x2Ge,

                    V128And => Instruction::V128And,
                    V128AndNot => Instruction::V128AndNot,
                    V128Or => Instruction::V128Or,
                    V128Xor => Instruction::V128Xor,

                    I8x16NarrowI16x8S => Instruction::I8x16NarrowI16x8S,
                    I8x16NarrowI16x8U => Instruction::I8x16NarrowI16x8U,
                    I8x16Shl => Instruction::I8x16Shl,
                    I8x16ShrS => Instruction::I8x16ShrS,
                    I8x16ShrU => Instruction::I8x16ShrU,
                    I8x16Add => Instruction::I8x16Add,
                    I8x16AddSatS => Instruction::I8x16AddSatS,
                    I8x16AddSatU => Instruction::I8x16AddSatU,
                    I8x16Sub => Instruction::I8x16Sub,
                    I8x16SubSatS => Instruction::I8x16SubSatS,
                    I8x16SubSatU => Instruction::I8x16SubSatU,
                    I8x16MinS => Instruction::I8x16MinS,
                    I8x16MinU => Instruction::I8x16MinU,
                    I8x16MaxS => Instruction::I8x16MaxS,
                    I8x16MaxU => Instruction::I8x16MaxU,
                    I8x16AvgrU => Instruction::I8x16AvgrU,

                    I16x8NarrowI32x4S => Instruction::I16x8NarrowI32x4S,
                    I16x8NarrowI32x4U => Instruction::I16x8NarrowI32x4U,
                    I16x8Shl => Instruction::I16x8Shl,
                    I16x8ShrS => Instruction::I16x8ShrS,
                    I16x8ShrU => Instruction::I16x8ShrU,
                    I16x8Add => Instruction::I16x8Add,
                    I16x8AddSatS => Instruction::I16x8AddSatS,
                    I16x8AddSatU => Instruction::I16x8AddSatU,
                    I16x8Sub => Instruction::I16x8Sub,
                    I16x8SubSatS => Instruction::I16x8SubSatS,
                    I16x8SubSatU => Instruction::I16x8SubSatU,
                    I16x8Mul => Instruction::I16x8Mul,
                    I16x8MinS => Instruction::I16x8MinS,
                    I16x8MinU => Instruction::I16x8MinU,
                    I16x8MaxS => Instruction::I16x8MaxS,
                    I16x8MaxU => Instruction::I16x8MaxU,
                    I16x8AvgrU => Instruction::I16x8AvgrU,

                    I32x4Shl => Instruction::I32x4Shl,
                    I32x4ShrS => Instruction::I32x4ShrS,
                    I32x4ShrU => Instruction::I32x4ShrU,
                    I32x4Add => Instruction::I32x4Add,
                    I32x4Sub => Instruction::I32x4Sub,
                    I32x4Mul => Instruction::I32x4Mul,
                    I32x4MinS => Instruction::I32x4MinS,
                    I32x4MinU => Instruction::I32x4MinU,
                    I32x4MaxS => Instruction::I32x4MaxS,
                    I32x4MaxU => Instruction::I32x4MaxU,

                    I64x2Shl => Instruction::I64x2Shl,
                    I64x2ShrS => Instruction::I64x2ShrS,
                    I64x2ShrU => Instruction::I64x2ShrU,
                    I64x2Add => Instruction::I64x2Add,
                    I64x2Sub => Instruction::I64x2Sub,
                    I64x2Mul => Instruction::I64x2Mul,

                    F32x4Add => Instruction::F32x4Add,
                    F32x4Sub => Instruction::F32x4Sub,
                    F32x4Mul => Instruction::F32x4Mul,
                    F32x4Div => Instruction::F32x4Div,
                    F32x4Min => Instruction::F32x4Min,
                    F32x4Max => Instruction::F32x4Max,
                    F32x4PMin => Instruction::F32x4PMin,
                    F32x4PMax => Instruction::F32x4PMax,

                    F64x2Add => Instruction::F64x2Add,
                    F64x2Sub => Instruction::F64x2Sub,
                    F64x2Mul => Instruction::F64x2Mul,
                    F64x2Div => Instruction::F64x2Div,
                    F64x2Min => Instruction::F64x2Min,
                    F64x2Max => Instruction::F64x2Max,
                    F64x2PMin => Instruction::F64x2PMin,
                    F64x2PMax => Instruction::F64x2PMax,

                    I32x4DotI16x8S => Instruction::I32x4DotI16x8S,

                    I16x8Q15MulrSatS => Instruction::I16x8Q15MulrSatS,
                    I16x8ExtMulLowI8x16S => Instruction::I16x8ExtMulLowI8x16S,
                    I16x8ExtMulHighI8x16S => Instruction::I16x8ExtMulHighI8x16S,
                    I16x8ExtMulLowI8x16U => Instruction::I16x8ExtMulLowI8x16U,
                    I16x8ExtMulHighI8x16U => Instruction::I16x8ExtMulHighI8x16U,
                    I32x4ExtMulLowI16x8S => Instruction::I32x4ExtMulLowI16x8S,
                    I32x4ExtMulHighI16x8S => Instruction::I32x4ExtMulHighI16x8S,
                    I32x4ExtMulLowI16x8U => Instruction::I32x4ExtMulLowI16x8U,
                    I32x4ExtMulHighI16x8U => Instruction::I32x4ExtMulHighI16x8U,
                    I64x2ExtMulLowI32x4S => Instruction::I64x2ExtMulLowI32x4S,
                    I64x2ExtMulHighI32x4S => Instruction::I64x2ExtMulHighI32x4S,
                    I64x2ExtMulLowI32x4U => Instruction::I64x2ExtMulLowI32x4U,
                    I64x2ExtMulHighI32x4U => Instruction::I64x2ExtMulHighI32x4U,

                    I8x16RelaxedSwizzle => Instruction::I8x16RelaxedSwizzle,
                    F32x4RelaxedMin => Instruction::F32x4RelaxedMin,
                    F32x4RelaxedMax => Instruction::F32x4RelaxedMax,
                    F64x2RelaxedMin => Instruction::F64x2RelaxedMin,
                    F64x2RelaxedMax => Instruction::F64x2RelaxedMax,
                    I16x8RelaxedQ15mulrS => Instruction::I16x8RelaxedQ15mulrS,
                    I16x8RelaxedDotI8x16I7x16S => Instruction::I16x8RelaxedDotI8x16I7x16S,
                }
            }

            Unop(e) => {
                use crate::ir::UnaryOp::*;

                match e.op {
                    I32Eqz => Instruction::I32Eqz,
                    I32Clz => Instruction::I32Clz,
                    I32Ctz => Instruction::I32Ctz,
                    I32Popcnt => Instruction::I32Popcnt,

                    I64Eqz => Instruction::I64Eqz,
                    I64Clz => Instruction::I64Clz,
                    I64Ctz => Instruction::I64Ctz,
                    I64Popcnt => Instruction::I64Popcnt,

                    F32Abs => Instruction::F32Abs,
                    F32Neg => Instruction::F32Neg,
                    F32Ceil => Instruction::F32Ceil,
                    F32Floor => Instruction::F32Floor,
                    F32Trunc => Instruction::F32Trunc,
                    F32Nearest => Instruction::F32Nearest,
                    F32Sqrt => Instruction::F32Sqrt,

                    F64Abs => Instruction::F64Abs,
                    F64Neg => Instruction::F64Neg,
                    F64Ceil => Instruction::F64Ceil,
                    F64Floor => Instruction::F64Floor,
                    F64Trunc => Instruction::F64Trunc,
                    F64Nearest => Instruction::F64Nearest,
                    F64Sqrt => Instruction::F64Sqrt,

                    I32WrapI64 => Instruction::I32WrapI64,
                    I32TruncSF32 => Instruction::I32TruncF32S,
                    I32TruncUF32 => Instruction::I32TruncF32U,
                    I32TruncSF64 => Instruction::I32TruncF64S,
                    I32TruncUF64 => Instruction::I32TruncF64U,
                    I64ExtendSI32 => Instruction::I64ExtendI32S,
                    I64ExtendUI32 => Instruction::I64ExtendI32U,
                    I64TruncSF32 => Instruction::I64TruncF32S,
                    I64TruncUF32 => Instruction::I64TruncF32U,
                    I64TruncSF64 => Instruction::I64TruncF64S,
                    I64TruncUF64 => Instruction::I64TruncF64U,

                    F32ConvertSI32 => Instruction::F32ConvertI32S,
                    F32ConvertUI32 => Instruction::F32ConvertI32U,
                    F32ConvertSI64 => Instruction::F32ConvertI64S,
                    F32ConvertUI64 => Instruction::F32ConvertI64U,
                    F32DemoteF64 => Instruction::F32DemoteF64,
                    F64ConvertSI32 => Instruction::F64ConvertI32S,
                    F64ConvertUI32 => Instruction::F64ConvertI32U,
                    F64ConvertSI64 => Instruction::F64ConvertI64S,
                    F64ConvertUI64 => Instruction::F64ConvertI64U,
                    F64PromoteF32 => Instruction::F64PromoteF32,

                    I32ReinterpretF32 => Instruction::I32ReinterpretF32,
                    I64ReinterpretF64 => Instruction::I64ReinterpretF64,
                    F32ReinterpretI32 => Instruction::F32ReinterpretI32,
                    F64ReinterpretI64 => Instruction::F64ReinterpretI64,

                    I32Extend8S => Instruction::I32Extend8S,
                    I32Extend16S => Instruction::I32Extend16S,
                    I64Extend8S => Instruction::I64Extend8S,
                    I64Extend16S => Instruction::I64Extend16S,
                    I64Extend32S => Instruction::I64Extend32S,

                    I8x16Splat => Instruction::I8x16Splat,
                    I16x8Splat => Instruction::I16x8Splat,
                    I32x4Splat => Instruction::I32x4Splat,
                    I64x2Splat => Instruction::I64x2Splat,
                    F32x4Splat => Instruction::F32x4Splat,
                    F64x2Splat => Instruction::F64x2Splat,
                    I8x16ExtractLaneS { idx } => Instruction::I8x16ExtractLaneS(idx),
                    I8x16ExtractLaneU { idx } => Instruction::I8x16ExtractLaneU(idx),
                    I16x8ExtractLaneS { idx } => Instruction::I16x8ExtractLaneS(idx),
                    I16x8ExtractLaneU { idx } => Instruction::I16x8ExtractLaneU(idx),
                    I32x4ExtractLane { idx } => Instruction::I32x4ExtractLane(idx),
                    I64x2ExtractLane { idx } => Instruction::I64x2ExtractLane(idx),
                    F32x4ExtractLane { idx } => Instruction::F32x4ExtractLane(idx),
                    F64x2ExtractLane { idx } => Instruction::F64x2ExtractLane(idx),

                    V128Not => Instruction::V128Not,

                    V128AnyTrue => Instruction::V128AnyTrue,

                    I8x16Abs => Instruction::I8x16Abs,
                    I8x16Popcnt => Instruction::I8x16Popcnt,
                    I8x16Neg => Instruction::I8x16Neg,
                    I8x16AllTrue => Instruction::I8x16AllTrue,
                    I8x16Bitmask => Instruction::I8x16Bitmask,

                    I16x8Abs => Instruction::I16x8Abs,
                    I16x8Neg => Instruction::I16x8Neg,
                    I16x8AllTrue => Instruction::I16x8AllTrue,
                    I16x8Bitmask => Instruction::I16x8Bitmask,
                    I16x8WidenLowI8x16S => Instruction::I16x8ExtendLowI8x16S,
                    I16x8WidenHighI8x16S => Instruction::I16x8ExtendHighI8x16S,
                    I16x8WidenLowI8x16U => Instruction::I16x8ExtendLowI8x16U,
                    I16x8WidenHighI8x16U => Instruction::I16x8ExtendHighI8x16U,

                    I32x4Abs => Instruction::I32x4Abs,
                    I32x4Neg => Instruction::I32x4Neg,
                    I32x4AllTrue => Instruction::I32x4AllTrue,
                    I32x4Bitmask => Instruction::I32x4Bitmask,
                    I32x4WidenLowI16x8S => Instruction::I32x4ExtendLowI16x8S,
                    I32x4WidenHighI16x8S => Instruction::I32x4ExtendHighI16x8S,
                    I32x4WidenLowI16x8U => Instruction::I32x4ExtendLowI16x8U,
                    I32x4WidenHighI16x8U => Instruction::I32x4ExtendHighI16x8U,

                    I64x2Abs => Instruction::I64x2Abs,
                    I64x2Neg => Instruction::I64x2Neg,
                    I64x2AllTrue => Instruction::I64x2AllTrue,
                    I64x2Bitmask => Instruction::I64x2Bitmask,

                    F32x4Abs => Instruction::F32x4Abs,
                    F32x4Neg => Instruction::F32x4Neg,
                    F32x4Sqrt => Instruction::F32x4Sqrt,
                    F32x4Ceil => Instruction::F32x4Ceil,
                    F32x4Floor => Instruction::F32x4Floor,
                    F32x4Trunc => Instruction::F32x4Trunc,
                    F32x4Nearest => Instruction::F32x4Nearest,

                    F64x2Abs => Instruction::F64x2Abs,
                    F64x2Neg => Instruction::F64x2Neg,
                    F64x2Sqrt => Instruction::F64x2Sqrt,
                    F64x2Ceil => Instruction::F64x2Ceil,
                    F64x2Floor => Instruction::F64x2Floor,
                    F64x2Trunc => Instruction::F64x2Trunc,
                    F64x2Nearest => Instruction::F64x2Nearest,

                    I32x4TruncSatF32x4S => Instruction::I32x4TruncSatF32x4S,
                    I32x4TruncSatF32x4U => Instruction::I32x4TruncSatF32x4U,
                    F32x4ConvertI32x4S => Instruction::F32x4ConvertI32x4S,
                    F32x4ConvertI32x4U => Instruction::F32x4ConvertI32x4U,

                    I32TruncSSatF32 => Instruction::I32TruncSatF32S,
                    I32TruncUSatF32 => Instruction::I32TruncSatF32U,
                    I32TruncSSatF64 => Instruction::I32TruncSatF64S,
                    I32TruncUSatF64 => Instruction::I32TruncSatF64U,
                    I64TruncSSatF32 => Instruction::I64TruncSatF32S,
                    I64TruncUSatF32 => Instruction::I64TruncSatF32U,
                    I64TruncSSatF64 => Instruction::I64TruncSatF64S,
                    I64TruncUSatF64 => Instruction::I64TruncSatF64U,

                    I16x8ExtAddPairwiseI8x16S => Instruction::I16x8ExtAddPairwiseI8x16S,
                    I16x8ExtAddPairwiseI8x16U => Instruction::I16x8ExtAddPairwiseI8x16U,
                    I32x4ExtAddPairwiseI16x8S => Instruction::I32x4ExtAddPairwiseI16x8S,
                    I32x4ExtAddPairwiseI16x8U => Instruction::I32x4ExtAddPairwiseI16x8U,
                    I64x2ExtendLowI32x4S => Instruction::I64x2ExtendLowI32x4S,
                    I64x2ExtendHighI32x4S => Instruction::I64x2ExtendHighI32x4S,
                    I64x2ExtendLowI32x4U => Instruction::I64x2ExtendLowI32x4U,
                    I64x2ExtendHighI32x4U => Instruction::I64x2ExtendHighI32x4U,
                    I32x4TruncSatF64x2SZero => Instruction::I32x4TruncSatF64x2SZero,
                    I32x4TruncSatF64x2UZero => Instruction::I32x4TruncSatF64x2UZero,
                    F64x2ConvertLowI32x4S => Instruction::F64x2ConvertLowI32x4S,
                    F64x2ConvertLowI32x4U => Instruction::F64x2ConvertLowI32x4U,
                    F32x4DemoteF64x2Zero => Instruction::F32x4DemoteF64x2Zero,
                    F64x2PromoteLowF32x4 => Instruction::F64x2PromoteLowF32x4,

                    I32x4RelaxedTruncF32x4S => Instruction::I32x4RelaxedTruncF32x4S,
                    I32x4RelaxedTruncF32x4U => Instruction::I32x4RelaxedTruncF32x4U,
                    I32x4RelaxedTruncF64x2SZero => Instruction::I32x4RelaxedTruncF64x2SZero,
                    I32x4RelaxedTruncF64x2UZero => Instruction::I32x4RelaxedTruncF64x2UZero,
                }
            }

            Select(e) => match e.ty {
                Some(ty) => Instruction::TypedSelect(ty.to_wasmencoder_type()),
                None => Instruction::Select,
            },

            Unreachable(_) => Instruction::Unreachable,

            Br(e) => Instruction::Br(self.branch_target(e.block)),

            BrIf(e) => Instruction::BrIf(self.branch_target(e.block)),

            Call(e) => Instruction::Call(self.indices.get_func_index(e.func)),

            CallIndirect(e) => {
                let type_index = self.indices.get_type_index(e.ty);
                let table_index = self.indices.get_table_index(e.table);
                Instruction::CallIndirect {
                    type_index,
                    table_index,
                }
            }

            LocalGet(e) => Instruction::LocalGet(self.local_indices[&e.local]),

            LocalSet(e) => Instruction::LocalSet(self.local_indices[&e.local]),

            LocalTee(e) => Instruction::LocalTee(self.local_indices[&e.local]),

            GlobalGet(e) => Instruction::GlobalGet(self.indices.get_global_index(e.global)),

            GlobalSet(e) => Instruction::GlobalSet(self.indices.get_global_index(e.global)),

            Load(e) => {
                use crate::ir::ExtendedLoad::*;
                use crate::ir::LoadKind::*;
                let memarg = self.memarg(e.memory, &e.arg);
                match e.kind {
                    I32 { atomic: false } => Instruction::I32Load(memarg),
                    I32 { atomic: true } => Instruction::I32AtomicLoad(memarg),
                    I64 { atomic: false } => Instruction::I64Load(memarg),
                    I64 { atomic: true } => Instruction::I64AtomicLoad(memarg),
                    F32 => Instruction::F32Load(memarg),
                    F64 => Instruction::F64Load(memarg),
                    V128 => Instruction::V128Load(memarg),
                    I32_8 { kind: SignExtend } => Instruction::I32Load8S(memarg),
                    I32_8 { kind: ZeroExtend } => Instruction::I32Load8U(memarg),
                    I32_8 {
                        kind: ZeroExtendAtomic,
                    } => Instruction::I32AtomicLoad8U(memarg),
                    I32_16 { kind: SignExtend } => Instruction::I32Load16S(memarg),
                    I32_16 { kind: ZeroExtend } => Instruction::I32Load16U(memarg),
                    I32_16 {
                        kind: ZeroExtendAtomic,
                    } => Instruction::I32AtomicLoad16U(memarg),
                    I64_8 { kind: SignExtend } => Instruction::I64Load8S(memarg),
                    I64_8 { kind: ZeroExtend } => Instruction::I64Load8U(memarg),
                    I64_8 {
                        kind: ZeroExtendAtomic,
                    } => Instruction::I64AtomicLoad8U(memarg),
                    I64_16 { kind: SignExtend } => Instruction::I64Load16S(memarg),
                    I64_16 { kind: ZeroExtend } => Instruction::I64Load16U(memarg),
                    I64_16 {
                        kind: ZeroExtendAtomic,
                    } => Instruction::I64AtomicLoad16U(memarg),
                    I64_32 { kind: SignExtend } => Instruction::I64Load32S(memarg),
                    I64_32 { kind: ZeroExtend } => Instruction::I64Load32U(memarg),
                    I64_32 {
                        kind: ZeroExtendAtomic,
                    } => Instruction::I64AtomicLoad32U(memarg),
                }
            }

            Store(e) => {
                use crate::ir::StoreKind::*;
                let memarg = self.memarg(e.memory, &e.arg);
                match e.kind {
                    I32 { atomic: false } => Instruction::I32Store(memarg),
                    I32 { atomic: true } => Instruction::I32AtomicStore(memarg),
                    I64 { atomic: false } => Instruction::I64Store(memarg),
                    I64 { atomic: true } => Instruction::I64AtomicStore(memarg),
                    F32 => Instruction::F32Store(memarg),
                    F64 => Instruction::F64Store(memarg),
                    V128 => Instruction::V128Store(memarg),
                    I32_8 { atomic: false } => Instruction::I32Store8(memarg),
                    I32_8 { atomic: true } => Instruction::I32AtomicStore8(memarg),
                    I32_16 { atomic: false } => Instruction::I32Store16(memarg),
                    I32_16 { atomic: true } => Instruction::I32AtomicStore16(memarg),
                    I64_8 { atomic: false } => Instruction::I64Store8(memarg),
                    I64_8 { atomic: true } => Instruction::I64AtomicStore8(memarg),
                    I64_16 { atomic: false } => Instruction::I64Store16(memarg),
                    I64_16 { atomic: true } => Instruction::I64AtomicStore16(memarg),
                    I64_32 { atomic: false } => Instruction::I64Store32(memarg),
                    I64_32 { atomic: true } => Instruction::I64AtomicStore32(memarg),
                }
            }

            AtomicRmw(e) => {
                use crate::ir::AtomicOp::*;
                use crate::ir::AtomicWidth::*;

                let memarg = self.memarg(e.memory, &e.arg);

                match (e.op, e.width) {
                    (Add, I32) => Instruction::I32AtomicRmwAdd(memarg),
                    (Add, I64) => Instruction::I64AtomicRmwAdd(memarg),
                    (Add, I32_8) => Instruction::I32AtomicRmw8AddU(memarg),
                    (Add, I32_16) => Instruction::I32AtomicRmw16AddU(memarg),
                    (Add, I64_8) => Instruction::I64AtomicRmw16AddU(memarg),
                    (Add, I64_16) => Instruction::I64AtomicRmw16AddU(memarg),
                    (Add, I64_32) => Instruction::I64AtomicRmw32AddU(memarg),

                    (Sub, I32) => Instruction::I32AtomicRmwSub(memarg),
                    (Sub, I64) => Instruction::I64AtomicRmwSub(memarg),
                    (Sub, I32_8) => Instruction::I32AtomicRmw8SubU(memarg),
                    (Sub, I32_16) => Instruction::I32AtomicRmw16SubU(memarg),
                    (Sub, I64_8) => Instruction::I64AtomicRmw16SubU(memarg),
                    (Sub, I64_16) => Instruction::I64AtomicRmw16SubU(memarg),
                    (Sub, I64_32) => Instruction::I64AtomicRmw32SubU(memarg),

                    (And, I32) => Instruction::I32AtomicRmwAnd(memarg),
                    (And, I64) => Instruction::I64AtomicRmwAnd(memarg),
                    (And, I32_8) => Instruction::I32AtomicRmw8AndU(memarg),
                    (And, I32_16) => Instruction::I32AtomicRmw16AndU(memarg),
                    (And, I64_8) => Instruction::I64AtomicRmw16AndU(memarg),
                    (And, I64_16) => Instruction::I64AtomicRmw16AndU(memarg),
                    (And, I64_32) => Instruction::I64AtomicRmw32AndU(memarg),

                    (Or, I32) => Instruction::I32AtomicRmwOr(memarg),
                    (Or, I64) => Instruction::I64AtomicRmwOr(memarg),
                    (Or, I32_8) => Instruction::I32AtomicRmw8OrU(memarg),
                    (Or, I32_16) => Instruction::I32AtomicRmw16OrU(memarg),
                    (Or, I64_8) => Instruction::I64AtomicRmw16OrU(memarg),
                    (Or, I64_16) => Instruction::I64AtomicRmw16OrU(memarg),
                    (Or, I64_32) => Instruction::I64AtomicRmw32OrU(memarg),

                    (Xor, I32) => Instruction::I32AtomicRmwXor(memarg),
                    (Xor, I64) => Instruction::I64AtomicRmwXor(memarg),
                    (Xor, I32_8) => Instruction::I32AtomicRmw8XorU(memarg),
                    (Xor, I32_16) => Instruction::I32AtomicRmw16XorU(memarg),
                    (Xor, I64_8) => Instruction::I64AtomicRmw16XorU(memarg),
                    (Xor, I64_16) => Instruction::I64AtomicRmw16XorU(memarg),
                    (Xor, I64_32) => Instruction::I64AtomicRmw32XorU(memarg),

                    (Xchg, I32) => Instruction::I32AtomicRmwXchg(memarg),
                    (Xchg, I64) => Instruction::I64AtomicRmwXchg(memarg),
                    (Xchg, I32_8) => Instruction::I32AtomicRmw8XchgU(memarg),
                    (Xchg, I32_16) => Instruction::I32AtomicRmw16XchgU(memarg),
                    (Xchg, I64_8) => Instruction::I64AtomicRmw16XchgU(memarg),
                    (Xchg, I64_16) => Instruction::I64AtomicRmw16XchgU(memarg),
                    (Xchg, I64_32) => Instruction::I64AtomicRmw32XchgU(memarg),
                }
            }

            Cmpxchg(e) => {
                use crate::ir::AtomicWidth::*;
                let memarg = self.memarg(e.memory, &e.arg);
                match e.width {
                    I32 => Instruction::I32AtomicRmwCmpxchg(memarg),
                    I64 => Instruction::I64AtomicRmwCmpxchg(memarg),
                    I32_8 => Instruction::I32AtomicRmw8CmpxchgU(memarg),
                    I32_16 => Instruction::I32AtomicRmw16CmpxchgU(memarg),
                    I64_8 => Instruction::I64AtomicRmw8CmpxchgU(memarg),
                    I64_16 => Instruction::I64AtomicRmw16CmpxchgU(memarg),
                    I64_32 => Instruction::I64AtomicRmw32CmpxchgU(memarg),
                }
            }

            AtomicNotify(e) => Instruction::MemoryAtomicNotify(self.memarg(e.memory, &e.arg)),

            AtomicWait(e) => {
                let memarg = self.memarg(e.memory, &e.arg);
                if e.sixty_four {
                    Instruction::MemoryAtomicWait64(memarg)
                } else {
                    Instruction::MemoryAtomicWait32(memarg)
                }
            }

            AtomicFence(_) => Instruction::AtomicFence,

            TableGet(e) => Instruction::TableGet(self.indices.get_table_index(e.table)),
            TableSet(e) => Instruction::TableSet(self.indices.get_table_index(e.table)),
            TableGrow(e) => Instruction::TableGrow(self.indices.get_table_index(e.table)),
            TableSize(e) => Instruction::TableSize(self.indices.get_table_index(e.table)),
            TableFill(e) => Instruction::TableFill(self.indices.get_table_index(e.table)),
            RefNull(e) => Instruction::RefNull(match &e.ty {
                RefType::Externref => wasm_encoder::HeapType::Abstract {
                    shared: false,
                    ty: wasm_encoder::AbstractHeapType::Extern,
                },
                RefType::Funcref => wasm_encoder::HeapType::Abstract {
                    shared: false,
                    ty: wasm_encoder::AbstractHeapType::Func,
                },
            }),
            RefIsNull(_) => Instruction::RefIsNull,
            RefFunc(e) => Instruction::RefFunc(self.indices.get_func_index(e.func)),

            V128Bitselect(_) => Instruction::V128Bitselect,
            I8x16Shuffle(e) => Instruction::I8x16Shuffle(e.indices),
            I8x16Swizzle(_) => Instruction::I8x16Swizzle,
            LoadSimd(e) => {
                let memarg = self.memarg(e.memory, &e.arg);
                match e.kind {
                    LoadSimdKind::V128Load8x8S => Instruction::V128Load8x8S(memarg),
                    LoadSimdKind::V128Load8x8U => Instruction::V128Load8x8U(memarg),
                    LoadSimdKind::V128Load16x4S => Instruction::V128Load16x4S(memarg),
                    LoadSimdKind::V128Load16x4U => Instruction::V128Load16x4U(memarg),
                    LoadSimdKind::V128Load32x2S => Instruction::V128Load32x2S(memarg),
                    LoadSimdKind::V128Load32x2U => Instruction::V128Load32x2U(memarg),
                    LoadSimdKind::Splat8 => Instruction::V128Load8Splat(memarg),
                    LoadSimdKind::Splat16 => Instruction::V128Load16Splat(memarg),
                    LoadSimdKind::Splat32 => Instruction::V128Load32Splat(memarg),
                    LoadSimdKind::Splat64 => Instruction::V128Load64Splat(memarg),
                    LoadSimdKind::V128Load32Zero => Instruction::V128Load32Zero(memarg),
                    LoadSimdKind::V128Load64Zero => Instruction::V128Load64Zero(memarg),
                    LoadSimdKind::V128Load8Lane(lane) => {
                        Instruction::V128Load8Lane { memarg, lane }
                    }
                    LoadSimdKind::V128Load16Lane(lane) => {
                        Instruction::V128Load16Lane { memarg, lane }
                    }
                    LoadSimdKind::V128Load32Lane(lane) => {
                        Instruction::V128Load32Lane { memarg, lane }
                    }
                    LoadSimdKind::V128Load64Lane(lane) => {
                        Instruction::V128Load64Lane { memarg, lane }
                    }
                    LoadSimdKind::V128Store8Lane(lane) => {
                        Instruction::V128Store8Lane { memarg, lane }
                    }
                    LoadSimdKind::V128Store16Lane(lane) => {
                        Instruction::V128Store16Lane { memarg, lane }
                    }
                    LoadSimdKind::V128Store32Lane(lane) => {
                        Instruction::V128Store32Lane { memarg, lane }
                    }
                    LoadSimdKind::V128Store64Lane(lane) => {
                        Instruction::V128Store64Lane { memarg, lane }
                    }
                }
            }
            TableInit(e) => {
                let elem_index = self.indices.get_element_index(e.elem);
                let table = self.indices.get_table_index(e.table);
                Instruction::TableInit { elem_index, table }
            }
            TableCopy(e) => {
                let dst_table = self.indices.get_table_index(e.dst);
                let src_table = self.indices.get_table_index(e.src);
                Instruction::TableCopy {
                    src_table,
                    dst_table,
                }
            }
            ElemDrop(e) => Instruction::ElemDrop(self.indices.get_element_index(e.elem)),
            ReturnCall(e) => Instruction::ReturnCall(self.indices.get_func_index(e.func)),
            ReturnCallIndirect(e) => {
                let type_index = self.indices.get_type_index(e.ty);
                let table_index = self.indices.get_table_index(e.table);
                Instruction::ReturnCallIndirect {
                    type_index,
                    table_index,
                }
            }
        });
    }
}

impl Emit<'_> {
    fn branch_target(&self, block: InstrSeqId) -> u32 {
        self.blocks.iter().rev().position(|b| *b == block).expect(
            "attempt to branch to invalid block; bad transformation pass introduced bad branching?",
        ) as u32
    }

    fn block_type(&self, ty: InstrSeqType) -> wasm_encoder::BlockType {
        match ty {
            InstrSeqType::Simple(None) => wasm_encoder::BlockType::Empty,
            InstrSeqType::Simple(Some(ty)) => {
                wasm_encoder::BlockType::Result(ty.to_wasmencoder_type())
            }
            InstrSeqType::MultiValue(ty) => {
                wasm_encoder::BlockType::FunctionType(self.indices.get_type_index(ty))
            }
        }
    }

    fn memarg(&self, id: MemoryId, arg: &MemArg) -> wasm_encoder::MemArg {
        let memory_index = self.indices.get_memory_index(id);
        let MemArg { mut align, offset } = *arg;
        let mut align_exponent: u32 = 0;
        while align > 1 {
            align_exponent += 1;
            align >>= 1;
        }
        wasm_encoder::MemArg {
            offset: offset as u64,
            align: align_exponent,
            memory_index,
        }
    }
}
