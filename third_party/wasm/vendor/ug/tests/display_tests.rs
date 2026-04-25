use ug::{CpuDevice, DType, Device, LazyBuffer as LB, Result};

fn mul_cst<D: Device>(lb: &LB<D>, v: f32) -> Result<LB<D>> {
    let t = LB::cst(v, lb.shape(), lb.device())?;
    println!(">>> {:?} {:?}", lb.dtype(), t.dtype());
    lb.binary(ug::lang::BinaryOp::Mul, t)
}

#[test]
fn display_scalar() -> Result<()> {
    let dev = &CpuDevice;
    let t = LB::copy([1234i32].as_slice(), (), dev)?;
    let s = format!("{t}");
    assert_eq!(&s, "[1234]\nTensor[[], i32]");
    let t = t.cast(DType::F32)?.unary(ug::lang::UnaryOp::Neg)?;
    let s = format!("{}", mul_cst(&t, 0.1)?);
    assert_eq!(&s, "[-123.4000]\nTensor[[], f32]");
    let s = format!("{}", mul_cst(&t, 1e-8)?);
    assert_eq!(&s, "[-1.2340e-5]\nTensor[[], f32]");
    let s = format!("{}", mul_cst(&t, 1e8)?);
    assert_eq!(&s, "[-1.2340e11]\nTensor[[], f32]");
    let s = format!("{}", mul_cst(&t, 0.0)?);
    assert_eq!(&s, "[0.]\nTensor[[], f32]");
    Ok(())
}

#[test]
fn display_vector() -> Result<()> {
    let dev = &CpuDevice;
    let t = LB::copy::<&[i32], _>([].as_slice(), 0, dev)?;
    let s = format!("{t}");
    assert_eq!(&s, "[]\nTensor[[0], i32]");
    let t = LB::copy([0.1234567, 1.0, -1.2, 4.1, f32::NAN].as_slice(), 5, dev)?;
    let s = format!("{t}");
    assert_eq!(&s, "[ 0.1235,  1.0000, -1.2000,  4.1000,     NaN]\nTensor[[5], f32]");
    let t = LB::cst(42., 50, dev)?;
    let s = format!("\n{t}");
    let expected = r#"
[42., 42., 42., 42., 42., 42., 42., 42., 42., 42., 42., 42., 42., 42., 42., 42.,
 42., 42., 42., 42., 42., 42., 42., 42., 42., 42., 42., 42., 42., 42., 42., 42.,
 42., 42., 42., 42., 42., 42., 42., 42., 42., 42., 42., 42., 42., 42., 42., 42.,
 42., 42.]
Tensor[[50], f32]"#;
    assert_eq!(&s, expected);
    let t = LB::cst(42., 11000, dev)?;
    let s = format!("{t}");
    assert_eq!(&s, "[42., 42., 42., ..., 42., 42., 42.]\nTensor[[11000], f32]");
    Ok(())
}

#[test]
fn display_multi_dim() -> Result<()> {
    let t = LB::cst(42f32, (200, 100), &CpuDevice)?;
    let s = format!("\n{t}");
    let expected = r#"
[[42., 42., 42., ..., 42., 42., 42.],
 [42., 42., 42., ..., 42., 42., 42.],
 [42., 42., 42., ..., 42., 42., 42.],
 ...
 [42., 42., 42., ..., 42., 42., 42.],
 [42., 42., 42., ..., 42., 42., 42.],
 [42., 42., 42., ..., 42., 42., 42.]]
Tensor[[200, 100], f32]"#;
    assert_eq!(&s, expected);
    let t = t.reshape(&[2, 1, 1, 100, 100])?;
    let t = format!("\n{t}");
    let expected = r#"
[[[[[42., 42., 42., ..., 42., 42., 42.],
    [42., 42., 42., ..., 42., 42., 42.],
    [42., 42., 42., ..., 42., 42., 42.],
    ...
    [42., 42., 42., ..., 42., 42., 42.],
    [42., 42., 42., ..., 42., 42., 42.],
    [42., 42., 42., ..., 42., 42., 42.]]]],
 [[[[42., 42., 42., ..., 42., 42., 42.],
    [42., 42., 42., ..., 42., 42., 42.],
    [42., 42., 42., ..., 42., 42., 42.],
    ...
    [42., 42., 42., ..., 42., 42., 42.],
    [42., 42., 42., ..., 42., 42., 42.],
    [42., 42., 42., ..., 42., 42., 42.]]]]]
Tensor[[2, 1, 1, 100, 100], f32]"#;
    assert_eq!(&t, expected);
    Ok(())
}
