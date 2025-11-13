use ug::Result;

#[test]
fn run_add() -> Result<()> {
    let kernel = ug::samples::ssa::simple_add(6)?;
    let cpu = ug::CpuDevice;
    let mut c_code = vec![];
    ug::cpu_code_gen::gen(&mut c_code, "foo", &kernel)?;
    let func = cpu.compile_c(&c_code, "foo".into())?;
    let mut v1 = vec![0i32; 6];
    let mut v2 = vec![3, 1, 4, 1, 5, 9];
    let mut v3 = vec![1, 2, 3, 4, 5, 6];
    unsafe { func.run3(&mut v1, &mut v2, &mut v3)? };
    assert_eq!(v1, [4, 3, 7, 5, 10, 15]);
    Ok(())
}
