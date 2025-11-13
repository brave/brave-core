use ug::Result;

fn eval_add() -> Result<()> {
    let kernel = ug::samples::ssa::simple_add(2)?;
    println!("{kernel:?}");
    let mut a = ug::interpreter::Buffer::I32(vec![0i32, 0]);
    let mut b = ug::interpreter::Buffer::I32(vec![3i32, 4]);
    let mut c = ug::interpreter::Buffer::I32(vec![1i32, 2]);
    ug::interpreter::eval_ssa::<1>(&kernel, vec![&mut a, &mut b, &mut c], &[], 0)?;
    println!("a: {a:?}\nb: {b:?}\nc: {c:?}");
    Ok(())
}

fn eval_dotprod() -> Result<()> {
    let kernel = ug::samples::ssa::simple_dotprod(2)?;
    println!("{kernel:?}");
    let mut a = ug::interpreter::Buffer::F32(vec![0f32]);
    let mut b = ug::interpreter::Buffer::F32(vec![3f32, 4.5]);
    let mut c = ug::interpreter::Buffer::F32(vec![1f32, 2.]);
    ug::interpreter::eval_ssa::<1>(&kernel, vec![&mut a, &mut b, &mut c], &[], 0)?;
    println!("a: {a:?}\nb: {b:?}\nc: {c:?}");
    Ok(())
}

fn lower_add() -> Result<()> {
    let kernel = ug::samples::simple_add(2)?;
    let ssa_kernel = kernel.lower()?;
    println!("{ssa_kernel:?}");
    let mut c = ug::interpreter::Buffer::F32(vec![0f32, 0.]);
    let mut b = ug::interpreter::Buffer::F32(vec![3f32, 4.]);
    let mut a = ug::interpreter::Buffer::F32(vec![1f32, 2.]);
    ug::interpreter::eval_ssa::<1>(&ssa_kernel, vec![&mut a, &mut b, &mut c], &[], 0)?;
    println!("a: {a:?}\nb: {b:?}\nc: {c:?}");
    Ok(())
}

fn softmax() -> Result<()> {
    let kernel = ug::samples::op::softmax(2, 4)?;
    println!("{kernel:?}");
    let ssa_kernel = kernel.lower(&Default::default())?;
    println!("{ssa_kernel:?}");
    let mut a = ug::interpreter::Buffer::F32(vec![0., 1., 2., 3., 2., 1., 2., 1.]);
    let mut b = ug::interpreter::Buffer::F32(vec![0f32; 8]);
    ug::interpreter::eval_ssa::<1>(&ssa_kernel, vec![&mut a, &mut b], &[], 0)?;
    println!("a: {a:?}\nb: {b:?}");
    Ok(())
}

fn main() -> Result<()> {
    eval_add()?;
    eval_dotprod()?;
    lower_add()?;
    softmax()?;
    Ok(())
}
