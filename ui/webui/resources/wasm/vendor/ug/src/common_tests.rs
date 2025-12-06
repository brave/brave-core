use crate::{Device, LazyBuffer as LB, Result};

pub mod lb {
    use super::*;

    pub fn add<D: Device>(dev: &D) -> Result<()> {
        let lhs = LB::cst(40., (5, 2), dev)?;
        let rhs = LB::cst(2., (5, 2), dev)?;
        let lb = lhs.binary(crate::lang::BinaryOp::Add, rhs)?;
        let data = lb.data_vec::<f32>()?;
        assert_eq!(data, [42., 42., 42., 42., 42., 42., 42., 42., 42., 42.]);
        Ok(())
    }

    pub fn mm<D: Device>(dev: &D) -> Result<()> {
        let lhs = LB::cst(1., (4, 2), dev)?;
        let rhs = LB::cst(2., (2, 3), dev)?;
        let lb = lhs.matmul(rhs)?;
        let data = lb.data_vec::<f32>()?;
        assert_eq!(data, [4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0]);
        Ok(())
    }

    pub fn cat<D: Device>(dev: &D) -> Result<()> {
        let arg0 = LB::copy([1f32, 2.0, 3.0, 4.0].as_slice(), (2, 2), dev)?;
        let arg1 = LB::copy([1.1f32, 2.1, 3.1, 4.1].as_slice(), (2, 2), dev)?;
        let arg2 = LB::copy([1.2f32, 2.2, 3.2, 4.2].as_slice(), (2, 2), dev)?;
        let lb = LB::<D>::cat(&[&arg0, &arg1, &arg2], 0)?;
        let data = lb.data_vec::<f32>()?;
        assert_eq!(data, [1.0, 2.0, 3.0, 4.0, 1.1, 2.1, 3.1, 4.1, 1.2, 2.2, 3.2, 4.2]);
        let lb = LB::<D>::cat(&[&arg0, &arg1, &arg2], 1)?;
        let data = lb.data_vec::<f32>()?;
        assert_eq!(data, [1.0, 2.0, 1.1, 2.1, 1.2, 2.2, 3.2, 4.2, 0.0, 0.0, 0.0, 0.0]);
        Ok(())
    }
}
