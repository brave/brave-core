use ug::Result;

type LB = ug::LazyBuffer<ug::CpuDevice>;

fn main() -> Result<()> {
    let dev = ug::CpuDevice;
    let lb = LB::copy([1f32, 2f32, 3f32, 4f32, 5f32, 6f32].as_slice(), (3, 2), &dev)?;
    println!("{lb}");
    Ok(())
}
