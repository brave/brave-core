use crate::lang::{self, ssa};
use crate::Result;
use ssa::Instr as SsaI;

// ssa::Instr are indexed based on their line number which is not convenient when
// combining blocks of generated instructions
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct Id(usize);
impl Id {
    #[allow(clippy::new_without_default)]
    pub fn new() -> Self {
        // https://users.rust-lang.org/t/idiomatic-rust-way-to-generate-unique-id/33805
        use std::sync::atomic;
        static COUNTER: atomic::AtomicUsize = atomic::AtomicUsize::new(1);
        Self(COUNTER.fetch_add(1, atomic::Ordering::Relaxed))
    }

    pub fn to_varid(self) -> ssa::VarId {
        ssa::VarId::new(self.0)
    }

    pub fn to_a(self) -> ssa::A {
        ssa::A::Var(ssa::VarId::new(self.0))
    }

    pub fn from_varid(v: ssa::VarId) -> Id {
        Id(v.as_usize())
    }
}

impl From<Id> for ssa::A {
    fn from(val: Id) -> Self {
        val.to_a()
    }
}

// ssa like instructions but with explicit dst
#[derive(Debug)]
pub struct Block(pub Vec<(Id, SsaI)>);

#[derive(Debug)]
pub struct Range {
    range_id: Id,
    erange_id: Id,
}

impl Range {
    pub fn id(&self) -> Id {
        self.range_id
    }
}

impl Block {
    pub fn range<L: Into<ssa::A>, U: Into<ssa::A>>(&mut self, lo: L, up: U, step: usize) -> Range {
        let (range_id, erange_id) = (Id::new(), Id::new());
        let range =
            SsaI::Range { lo: lo.into(), up: up.into(), step, end_idx: erange_id.to_varid() };
        self.0.push((range_id, range));
        Range { range_id, erange_id }
    }

    pub fn end_range(&mut self, range: Range) -> Result<()> {
        let erange = ssa::Instr::EndRange { start_idx: range.range_id.to_varid() };
        self.0.push((range.erange_id, erange));
        Ok(())
    }

    pub fn add(&mut self, src_id: Id, v: i32) -> Id {
        if v == 0 {
            src_id
        } else {
            let dst_id = Id::new();
            self.0.push((
                dst_id,
                SsaI::Binary {
                    op: lang::BinaryOp::Add,
                    lhs: src_id.to_a(),
                    rhs: v.into(),
                    dtype: lang::DType::I32,
                },
            ));
            dst_id
        }
    }

    pub fn mul(&mut self, src_id: Id, v: i32) -> Id {
        if v == 1 {
            src_id
        } else {
            let dst_id = Id::new();
            self.0.push((
                dst_id,
                SsaI::Binary {
                    op: lang::BinaryOp::Mul,
                    lhs: src_id.to_a(),
                    rhs: v.into(),
                    dtype: lang::DType::I32,
                },
            ));
            dst_id
        }
    }

    pub fn cst<C: Into<lang::Const>>(&mut self, c: C) -> Id {
        self.push(SsaI::Const(c.into()))
    }

    pub fn push(&mut self, inst: SsaI) -> Id {
        let id = Id::new();
        self.0.push((id, inst));
        id
    }

    pub fn unary<I: Into<ssa::A>>(&mut self, op: lang::UnaryOp, arg: I, dtype: lang::DType) -> Id {
        let id = Id::new();
        let op = SsaI::Unary { op, arg: arg.into(), dtype };
        self.0.push((id, op));
        id
    }

    pub fn binary<I1: Into<ssa::A>, I2: Into<ssa::A>>(
        &mut self,
        op: lang::BinaryOp,
        lhs: I1,
        rhs: I2,
        dtype: lang::DType,
    ) -> Id {
        let id = Id::new();
        let op = SsaI::Binary { op, lhs: lhs.into(), rhs: rhs.into(), dtype };
        self.0.push((id, op));
        id
    }

    pub fn empty() -> Self {
        Self(vec![])
    }

    pub fn new(instrs: Vec<(Id, SsaI)>) -> Self {
        Self(instrs)
    }

    // This switches all the VarId to be in line number rather than "random" unique identifiers.
    pub fn relocate(&self) -> Result<Vec<SsaI>> {
        let mut per_id = std::collections::HashMap::new();
        for (line_idx, (id, _)) in self.0.iter().enumerate() {
            let line_idx = ssa::VarId::new(line_idx);
            per_id.insert(id, line_idx);
        }
        let mut instrs = vec![];
        for (_, instr) in self.0.iter() {
            let get_id = |id: ssa::VarId| match per_id.get(&Id::from_varid(id)) {
                Some(v) => Ok(*v),
                None => crate::bail!("id not found {id:?}"),
            };
            let get_a = |a: ssa::A| {
                let a = match a {
                    ssa::A::Var(v) => ssa::A::Var(get_id(v)?),
                    ssa::A::Const(c) => ssa::A::Const(c),
                };
                Ok::<_, crate::Error>(a)
            };
            let instr = match instr {
                SsaI::Store { dst, offset, value, dtype } => {
                    let dst = get_id(*dst)?;
                    let offset = get_a(*offset)?;
                    let value = get_a(*value)?;
                    SsaI::Store { dst, offset, value, dtype: *dtype }
                }
                SsaI::If { cond, end_idx } => {
                    let cond = get_a(*cond)?;
                    let end_idx = get_id(*end_idx)?;
                    SsaI::If { cond, end_idx }
                }
                SsaI::Range { lo, up, step, end_idx } => {
                    let lo = get_a(*lo)?;
                    let up = get_a(*up)?;
                    let end_idx = get_id(*end_idx)?;
                    SsaI::Range { lo, up, step: *step, end_idx }
                }
                SsaI::Load { src, offset, dtype } => {
                    let src = get_id(*src)?;
                    let offset = get_a(*offset)?;
                    SsaI::Load { src, offset, dtype: *dtype }
                }
                SsaI::Const(c) => SsaI::Const(*c),
                SsaI::Binary { op, lhs, rhs, dtype } => {
                    let lhs = get_a(*lhs)?;
                    let rhs = get_a(*rhs)?;
                    SsaI::Binary { op: *op, lhs, rhs, dtype: *dtype }
                }
                SsaI::Unary { op, arg, dtype } => {
                    let arg = get_a(*arg)?;
                    SsaI::Unary { op: *op, arg, dtype: *dtype }
                }
                SsaI::DefineAcc(c) => SsaI::DefineAcc(*c),
                SsaI::Assign { dst, src } => {
                    let dst = get_id(*dst)?;
                    let src = get_a(*src)?;
                    SsaI::Assign { dst, src }
                }
                SsaI::Special(s) => SsaI::Special(*s),
                SsaI::DefineLocal { size, dtype } => {
                    SsaI::DefineLocal { size: *size, dtype: *dtype }
                }
                SsaI::DefineGlobal { index, dtype } => {
                    SsaI::DefineGlobal { index: *index, dtype: *dtype }
                }
                SsaI::Barrier => SsaI::Barrier,
                SsaI::ReduceLocal { op, arg, dtype } => {
                    let arg = get_a(*arg)?;
                    SsaI::ReduceLocal { op: *op, arg, dtype: *dtype }
                }
                SsaI::EndIf => SsaI::EndIf,
                SsaI::EndRange { start_idx } => {
                    let start_idx = get_id(*start_idx)?;
                    SsaI::EndRange { start_idx }
                }
            };
            instrs.push(instr)
        }
        Ok(instrs)
    }
}
