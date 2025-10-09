pub mod ssa {
    use crate::lang::ssa;
    use crate::lang::ssa::{BinaryOp, Const, DType, Instr as I, Kernel, VarId, A};
    use crate::Result;

    fn arg(index: usize, dtype: DType) -> I {
        I::DefineGlobal { index, dtype }
    }

    pub fn simple_add(vec_len: usize) -> Result<Kernel> {
        let v = VarId::new;
        let a = |i| A::Var(VarId::new(i));
        let i32 = |i| A::Const(Const::I32(i));
        let dtype = DType::I32;
        let instrs = vec![
            /* 0 */ arg(0, DType::I32),
            /* 1 */ arg(1, DType::I32),
            /* 2 */ arg(2, DType::I32),
            /* 3 */ I::Range { lo: i32(0), up: i32(vec_len as i32), end_idx: v(8), step: 1 },
            /* 4 */ I::Load { src: v(1), offset: a(3), dtype },
            /* 5 */ I::Load { src: v(2), offset: a(3), dtype },
            /* 6 */ I::Binary { op: self::BinaryOp::Add, lhs: a(4), rhs: a(5), dtype },
            /* 7 */ I::Store { dst: v(0), offset: a(3), value: a(6), dtype },
            /* 8 */ I::EndRange { start_idx: v(3) },
        ];
        Kernel::from_instrs(instrs)
    }

    pub fn simple_dotprod(vec_len: usize) -> Result<Kernel> {
        let v = VarId::new;
        let a = |i| A::Var(VarId::new(i));
        let dtype = DType::F32;
        let instrs = vec![
            /* 0 */ arg(0, DType::F32),
            /* 1 */ arg(1, DType::F32),
            /* 2 */ arg(2, DType::F32),
            /* 3 */ I::Const(Const::I32(0)),
            /* 4 */ I::Const(Const::I32(vec_len as i32)),
            /* 5 */ I::DefineAcc(0f32.try_into()?),
            /* 6 */ I::Range { lo: a(3), up: a(4), end_idx: v(12), step: 1 },
            /* 7 */ I::Load { src: v(1), offset: a(6), dtype },
            /* 8 */ I::Load { src: v(2), offset: a(6), dtype },
            /* 9 */ I::Binary { op: self::BinaryOp::Mul, lhs: a(7), rhs: a(8), dtype },
            /* 10*/ I::Binary { op: self::BinaryOp::Add, lhs: a(9), rhs: a(5), dtype },
            /* 11*/ I::Assign { dst: v(5), src: a(10) },
            /* 12*/ I::EndRange { start_idx: v(6) },
            /* 13*/ I::Store { dst: v(0), offset: a(3), value: a(5), dtype },
        ];
        Kernel::from_instrs(instrs)
    }

    pub fn exp(block_size: usize) -> Result<Kernel> {
        let mut b = crate::block::Block::empty();
        let dtype = DType::F32;
        let src_i = b.push(arg(0, DType::F32));
        let dst_i = b.push(arg(1, DType::F32));
        let g_i = b.push(I::Special(ssa::Special::BlockIdx));
        let l_i = b.push(I::Special(ssa::Special::ThreadIdx));
        let off_i = b.mul(g_i, block_size as i32);
        let off_i = b.binary(BinaryOp::Add, off_i, l_i, DType::I32);
        let load_i = b.push(I::Load { src: src_i.to_varid(), offset: off_i.to_a(), dtype });
        let value_i = b.unary(ssa::UnaryOp::Exp, load_i, dtype);
        b.push(I::Store {
            dst: dst_i.to_varid(),
            offset: off_i.to_a(),
            value: value_i.to_a(),
            dtype,
        });
        let instrs = b.relocate()?;
        Kernel::from_instrs(instrs)
    }

    pub fn exp_block(dim2: usize, block_size: usize) -> Result<Kernel> {
        if dim2 % block_size != 0 {
            crate::bail!("last-dim {dim2} must be divisible by block size {block_size}")
        }

        let mut b = crate::block::Block::empty();
        let dtype = DType::F32;
        let src_i = b.push(arg(0, DType::F32));
        let dst_i = b.push(arg(1, DType::F32));
        let g_i = b.push(I::Special(ssa::Special::BlockIdx));
        let l_i = b.push(I::Special(ssa::Special::ThreadIdx));
        let off_i = b.mul(g_i, dim2 as i32);
        let off_i = b.binary(BinaryOp::Add, off_i, l_i, DType::I32);

        for i in (0..dim2).step_by(block_size) {
            let off_i = b.add(off_i, i as i32);
            let load_i = b.push(I::Load { src: src_i.to_varid(), offset: off_i.to_a(), dtype });
            let value_i = b.unary(ssa::UnaryOp::Exp, load_i, dtype);
            b.push(I::Store {
                dst: dst_i.to_varid(),
                offset: off_i.to_a(),
                value: value_i.to_a(),
                dtype,
            });
        }
        Kernel::from_instrs(b.relocate()?)
    }

    pub fn softmax_barrier(_dim1: usize, dim2: usize) -> Result<Kernel> {
        let mut b = crate::block::Block::empty();
        let dtype = DType::F32;
        let src_i = b.push(arg(0, DType::F32));
        let dst_i = b.push(arg(1, DType::F32));
        let sto_i = b.push(I::DefineLocal { size: (2 * dim2), dtype: DType::F32 }).to_varid();
        let g_i = b.push(I::Special(ssa::Special::BlockIdx));
        let l_i = b.push(I::Special(ssa::Special::ThreadIdx));
        let base_off_i = b.mul(g_i, dim2 as i32);
        let global_off_i = b.binary(BinaryOp::Add, base_off_i, l_i, DType::I32);
        let load_i = b.push(I::Load { src: src_i.to_varid(), offset: global_off_i.to_a(), dtype });

        // Compute the max value over dim2.
        // This implementation uses shared memory which is likely slower than using warp-reduce
        // primitives but these are harder to scale to more than 32 threads.
        b.push(I::Store { dst: sto_i, offset: l_i.to_a(), value: load_i.to_a(), dtype });
        b.push(I::Barrier);
        let mut offset = 1;
        let mut v_i = load_i;
        // This is only ok if dim2 is a power of 2.
        while offset < dim2 {
            let off_i = b.add(l_i, offset as i32);
            let m_i = b.push(I::Load { src: sto_i, offset: off_i.to_a(), dtype });
            v_i = b.binary(BinaryOp::Max, m_i, v_i, dtype);
            b.push(I::Store { dst: sto_i, offset: l_i.to_a(), value: v_i.to_a(), dtype });
            b.push(I::Barrier);
            offset *= 2
        }
        let max_i = b.push(I::Load { src: sto_i, offset: 0.into(), dtype });

        let value_i = b.binary(BinaryOp::Sub, load_i, max_i, dtype);
        let value_i = b.unary(ssa::UnaryOp::Exp, value_i, dtype);

        // Compute the sum of the exps
        b.push(I::Store { dst: sto_i, offset: l_i.to_a(), value: value_i.to_a(), dtype });
        b.push(I::Barrier);
        let mut offset = 1;
        let mut v_i = value_i;
        while offset < dim2 {
            let off_i = b.add(l_i, offset as i32);
            let m_i = b.push(I::Load { src: sto_i, offset: off_i.to_a(), dtype });
            v_i = b.binary(BinaryOp::Add, m_i, v_i, dtype);
            b.push(I::Store { dst: sto_i, offset: l_i.to_a(), value: v_i.to_a(), dtype });
            b.push(I::Barrier);
            offset *= 2
        }
        let sum_i = b.push(I::Load { src: sto_i, offset: 0.into(), dtype });

        // Normalize by sum_exp
        let value_i = b.binary(BinaryOp::Div, value_i, sum_i, dtype);
        b.push(I::Store {
            dst: dst_i.to_varid(),
            offset: global_off_i.to_a(),
            value: value_i.to_a(),
            dtype,
        });
        Kernel::from_instrs(b.relocate()?)
    }

    pub fn softmax_reduce(_dim1: usize, dim2: usize) -> Result<Kernel> {
        let mut b = crate::block::Block::empty();
        let dtype = DType::F32;
        let src_i = b.push(arg(0, dtype));
        let dst_i = b.push(arg(1, dtype));
        let g_i = b.push(I::Special(ssa::Special::BlockIdx));
        let l_i = b.push(I::Special(ssa::Special::ThreadIdx));
        let base_off_i = b.mul(g_i, dim2 as i32);
        let global_off_i = b.binary(BinaryOp::Add, base_off_i, l_i, DType::I32);
        let load_i = b.push(I::Load { src: src_i.to_varid(), offset: global_off_i.to_a(), dtype });
        let max_i = b.push(I::ReduceLocal { op: ssa::ReduceOp::Max, arg: load_i.to_a(), dtype });
        let value_i = b.binary(BinaryOp::Sub, load_i, max_i, dtype);
        let value_i = b.unary(ssa::UnaryOp::Exp, value_i, dtype);
        let sum_i = b.push(I::ReduceLocal { op: ssa::ReduceOp::Sum, arg: value_i.to_a(), dtype });
        // Normalize by sum_exp
        let value_i = b.binary(BinaryOp::Div, value_i, sum_i, dtype);
        b.push(I::Store {
            dst: dst_i.to_varid(),
            offset: global_off_i.to_a(),
            value: value_i.to_a(),
            dtype,
        });
        Kernel::from_instrs(b.relocate()?)
    }

    pub fn softmax_block(_dim1: usize, dim2: usize, block_size: usize) -> Result<Kernel> {
        if dim2 % block_size != 0 {
            crate::bail!("last-dim {dim2} must be divisible by block size {block_size}")
        }
        let per_block = dim2 / block_size;
        let mut b = crate::block::Block::empty();
        let dtype = DType::F32;
        let src_i = b.push(arg(0, dtype));
        let dst_i = b.push(arg(1, dtype));
        let g_i = b.push(I::Special(ssa::Special::BlockIdx));
        let l_i = b.push(I::Special(ssa::Special::ThreadIdx));
        let base_off_i = b.mul(g_i, dim2 as i32);
        let global_off_i = b.binary(BinaryOp::Add, base_off_i, l_i, DType::I32);

        let mut load_is = Vec::with_capacity(per_block);

        let mut max_i = b.push(I::Const(f32::NEG_INFINITY.try_into()?));
        for i in (0..dim2).step_by(block_size) {
            let offset = b.add(global_off_i, i as i32).to_a();
            let load_i = b.push(I::Load { src: src_i.to_varid(), offset, dtype });
            max_i = b.binary(BinaryOp::Max, max_i, load_i, dtype);
            load_is.push(load_i)
        }
        let max_i = b.push(I::ReduceLocal { op: ssa::ReduceOp::Max, arg: max_i.to_a(), dtype });

        let mut value_is = Vec::with_capacity(per_block);
        let mut sum_i = b.push(I::Const(Const::I32(0)));
        for load_i in load_is.into_iter() {
            let value_i = b.binary(BinaryOp::Sub, load_i, max_i, dtype);
            let value_i = b.unary(ssa::UnaryOp::Exp, value_i, dtype);
            sum_i = b.binary(BinaryOp::Add, value_i, sum_i, dtype);
            value_is.push(value_i);
        }
        let sum_i = b.push(I::ReduceLocal { op: ssa::ReduceOp::Sum, arg: sum_i.to_a(), dtype });

        for (i, value_i) in value_is.into_iter().enumerate() {
            let i = i * block_size;
            let offset = b.add(global_off_i, i as i32).to_a();
            // Normalize by sum_exp
            let value_i = b.binary(BinaryOp::Div, value_i, sum_i, dtype);
            b.push(I::Store { dst: dst_i.to_varid(), offset, value: value_i.to_a(), dtype });
        }
        Kernel::from_instrs(b.relocate()?)
    }

    pub fn softmax(dim1: usize, dim2: usize) -> Result<Kernel> {
        softmax_barrier(dim1, dim2)
    }
}

pub mod op {
    use crate::lang::op::{self, Arg, DType, Kernel, Layout};
    use crate::Result;

    pub fn softmax(dim1: usize, dim2: usize) -> Result<Kernel> {
        let layout = Layout::from_shape(&[dim1, dim2]);
        let src_ptr = Arg::ptr(DType::F32);
        let dst_ptr = Arg::ptr(DType::F32);
        let src = op::load(src_ptr.id(), layout.clone(), DType::F32)?;
        let src_max = op::reduce(op::ReduceOp::Max, src.clone(), 1)?;
        let src_max = op::broadcast(src_max, (dim1, dim2))?;
        let diff = op::binary(op::BinaryOp::Sub, src, src_max)?;
        let exp = op::unary(op::UnaryOp::Exp, diff)?;
        let sum_exp = op::reduce(op::ReduceOp::Sum, exp.clone(), 1)?;
        let sum_exp = op::broadcast(sum_exp, (dim1, dim2))?;
        let sm = op::binary(op::BinaryOp::Div, exp, sum_exp)?;
        let st = op::store(dst_ptr.id(), layout, sm)?;
        let kernel =
            Kernel::new(format!("softmax_{dim1}_{dim2}"), vec![src_ptr, dst_ptr], vec![st]);
        Ok(kernel)
    }
}

use crate::lang::{Arg, DType, ExprNode as E, IndexExprNode as I, Kernel, Ops};

pub fn simple_add(block_size: usize) -> crate::Result<Kernel> {
    let lhs_ptr = Arg::ptr(DType::F32);
    let rhs_ptr = Arg::ptr(DType::F32);
    let dst_ptr = Arg::ptr(DType::F32);
    let offset = I::mul(&I::program_id(), &I::cst(block_size));
    let stride = I::cst(1);
    let len = I::cst(block_size);
    let lhs = E::load(&lhs_ptr, &offset, &len, &stride)?;
    let rhs = E::load(&rhs_ptr, &offset, &len, &stride)?;
    let op = Ops::store(&dst_ptr, &offset, &len, &stride, &lhs.add(&rhs));
    let k =
        Kernel::new(format!("simple_add_{block_size}"), vec![lhs_ptr, rhs_ptr, dst_ptr], vec![op]);
    Ok(k)
}
