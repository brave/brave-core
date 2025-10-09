use crate::block::{Block, Id};
use crate::lang::{self, ssa};
use crate::Result;
use ssa::Instr as SsaI;

impl lang::ExprNode {
    fn lower(
        &self,
        range_id: Id,
        per_arg: &std::collections::HashMap<lang::ArgId, ssa::VarId>,
    ) -> Result<(Id, Block)> {
        use lang::Expr as E;
        let dst_id = Id::new();
        let dtype = self.dtype();
        let block = match &self.inner.expr {
            E::Load(src) => {
                let (ptr_i, off_i, src_b) = src.lower(range_id, per_arg)?;
                let instr = SsaI::Load { src: ptr_i, offset: off_i.into(), dtype };
                let mut src_b = src_b.0;
                src_b.push((dst_id, instr));
                src_b
            }
            E::Const(c) => vec![(dst_id, SsaI::Const(*c))],
            E::Range(_, _) => crate::bail!("TODO range is not supported yet"),
            E::Unary(op, arg) => {
                let (arg_id, arg_b) = arg.lower(range_id, per_arg)?;
                let instr = SsaI::Unary { op: *op, arg: arg_id.to_a(), dtype };
                let last = vec![(dst_id, instr)];
                [arg_b.0.as_slice(), last.as_slice()].concat()
            }
            E::Binary(op, lhs, rhs) => {
                let (lhs_id, lhs_b) = lhs.lower(range_id, per_arg)?;
                let (rhs_id, rhs_b) = rhs.lower(range_id, per_arg)?;
                let instr = SsaI::Binary { op: *op, lhs: lhs_id.to_a(), rhs: rhs_id.to_a(), dtype };
                let last = vec![(dst_id, instr)];
                [lhs_b.0.as_slice(), rhs_b.0.as_slice(), last.as_slice()].concat()
            }
        };
        Ok((dst_id, Block(block)))
    }
}

impl lang::StridedSlice {
    fn lower(
        &self,
        range_id: Id,
        per_arg: &std::collections::HashMap<lang::ArgId, ssa::VarId>,
    ) -> Result<(ssa::VarId, ssa::VarId, Block)> {
        let (off_i, off_b) = self.offset().lower()?;
        let (stride_i, stride_b) = self.stride().lower()?;
        let ptr_i = match per_arg.get(&self.ptr().id()) {
            None => crate::bail!("unknown arg {:?}", self.ptr().id()),
            Some(id) => *id,
        };
        // TODO(laurent): remove this when we have some proper optimization pass.
        if self.offset().as_const() == Some(0) && self.stride().as_const() == Some(1) {
            Ok((ptr_i, range_id.to_varid(), Block(vec![])))
        } else if self.stride().as_const() == Some(1) {
            let index_i = Id::new();
            let index_b = vec![(
                index_i,
                SsaI::Binary {
                    op: ssa::BinaryOp::Add,
                    lhs: range_id.to_a(),
                    rhs: off_i.to_a(),
                    dtype: ssa::DType::I32,
                },
            )];
            let instrs = [off_b.0.as_slice(), stride_b.0.as_slice(), index_b.as_slice()].concat();
            Ok((ptr_i, index_i.to_varid(), Block(instrs)))
        } else {
            let mul_i = Id::new();
            let index_i = Id::new();
            let index_b = vec![
                (
                    mul_i,
                    SsaI::Binary {
                        op: ssa::BinaryOp::Mul,
                        lhs: range_id.to_a(),
                        rhs: stride_i.to_a(),
                        dtype: ssa::DType::I32,
                    },
                ),
                (
                    index_i,
                    SsaI::Binary {
                        op: ssa::BinaryOp::Add,
                        lhs: mul_i.to_a(),
                        rhs: off_i.to_a(),
                        dtype: ssa::DType::I32,
                    },
                ),
            ];
            let instrs = [off_b.0.as_slice(), stride_b.0.as_slice(), index_b.as_slice()].concat();
            Ok((ptr_i, index_i.to_varid(), Block(instrs)))
        }
    }
}

impl lang::IndexExprNode {
    fn lower(&self) -> Result<(Id, Block)> {
        use lang::IndexExpr as E;
        let dst_id = Id::new();
        let block = match &self.inner.expr {
            E::Add(lhs, rhs) => {
                let (lhs_id, lhs_b) = lhs.lower()?;
                let (rhs_id, rhs_b) = rhs.lower()?;
                let instr = SsaI::Binary {
                    op: ssa::BinaryOp::Add,
                    lhs: lhs_id.to_a(),
                    rhs: rhs_id.to_a(),
                    dtype: ssa::DType::I32,
                };
                let last = vec![(dst_id, instr)];
                [lhs_b.0.as_slice(), rhs_b.0.as_slice(), last.as_slice()].concat()
            }
            E::Mul(lhs, rhs) => {
                let (lhs_id, lhs_b) = lhs.lower()?;
                let (rhs_id, rhs_b) = rhs.lower()?;
                let instr = SsaI::Binary {
                    op: ssa::BinaryOp::Mul,
                    lhs: lhs_id.to_a(),
                    rhs: rhs_id.to_a(),
                    dtype: ssa::DType::I32,
                };
                let last = vec![(dst_id, instr)];
                [lhs_b.0.as_slice(), rhs_b.0.as_slice(), last.as_slice()].concat()
            }
            E::Const(v) => {
                let instr = SsaI::Const(ssa::Const::I32(*v as i32));
                vec![(dst_id, instr)]
            }
            E::ProgramId => {
                let instr = SsaI::Special(ssa::Special::BlockIdx);
                vec![(dst_id, instr)]
            }
        };
        Ok((dst_id, Block(block)))
    }
}

impl lang::Kernel {
    fn lower_b(&self) -> Result<Block> {
        let mut instrs = vec![];
        let mut per_arg = std::collections::HashMap::new();
        for (index, arg) in self.args.iter().enumerate() {
            let id = Id::new();
            let dtype = match arg.type_() {
                ssa::Type::Ptr(v) => v,
                ssa::Type::Value(_) => crate::bail!("non-pointer arguments are not supported yet"),
            };
            instrs.push((id, SsaI::DefineGlobal { index, dtype }));
            per_arg.insert(arg.id(), id.to_varid());
        }
        for lang::Ops::Store { dst, src } in self.ops.iter() {
            let len = dst.len();

            let (len_i, len_b) = len.lower()?;
            instrs.extend_from_slice(len_b.0.as_slice());
            let lo_id = Id::new();
            instrs.push((lo_id, SsaI::Const(ssa::Const::I32(0))));

            let (range_id, erange_id) = (Id::new(), Id::new());
            let range = SsaI::Range {
                lo: lo_id.to_a(),
                up: len_i.to_a(),
                step: 1,
                end_idx: erange_id.to_varid(),
            };
            instrs.push((range_id, range));

            let (src_i, src_b) = src.lower(range_id, &per_arg)?;
            instrs.extend_from_slice(src_b.0.as_slice());
            let (ptr_i, off_i, dst_b) = dst.lower(range_id, &per_arg)?;
            instrs.extend_from_slice(dst_b.0.as_slice());
            let store = SsaI::Store {
                dst: ptr_i,
                offset: off_i.into(),
                value: src_i.to_a(),
                dtype: src.dtype(),
            };
            instrs.push((Id::new(), store));

            let erange = ssa::Instr::EndRange { start_idx: range_id.to_varid() };
            instrs.push((erange_id, erange));
        }
        Ok(Block(instrs))
    }

    pub fn lower(&self) -> Result<ssa::Kernel> {
        let block = self.lower_b()?;
        let instrs = block.relocate()?;
        ssa::Kernel::from_instrs(instrs)
    }
}
