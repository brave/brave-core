use std::marker::PhantomData;

use rstest::rstest;
use time::{Date, Month, OffsetDateTime, PlainDateTime, SignedDuration, Time, UtcOffset, Weekday};

#[rstest]
#[case(7, PhantomData::<Weekday>)]
#[case(12, PhantomData::<Month>)]
#[case(1, PhantomData::<Time>)]
#[case(1, PhantomData::<Date>)]
#[case(1, PhantomData::<UtcOffset>)]
#[case(1, PhantomData::<PlainDateTime>)]
#[case(1, PhantomData::<OffsetDateTime>)]
#[case(1, PhantomData::<SignedDuration>)]
fn support08<T>(#[case] iterations: usize, #[case] _type: PhantomData<T>)
where
    rand08::distributions::Standard: rand08::distributions::Distribution<T>,
{
    use rand08::Rng as _;

    // Work around rust-random/rand#1020.
    let mut rng = rand08::rngs::mock::StepRng::new(0, 2_505_397_590);

    for _ in 0..iterations {
        drop(rng.r#gen::<T>());
    }
}

#[rstest]
#[case(7, PhantomData::<Weekday>)]
#[case(12, PhantomData::<Month>)]
#[case(1, PhantomData::<Time>)]
#[case(1, PhantomData::<Date>)]
#[case(1, PhantomData::<UtcOffset>)]
#[case(1, PhantomData::<PlainDateTime>)]
#[case(1, PhantomData::<OffsetDateTime>)]
#[case(1, PhantomData::<SignedDuration>)]
fn support09<T>(#[case] iterations: usize, #[case] _type: PhantomData<T>)
where
    rand09::distr::StandardUniform: rand09::distr::Distribution<T>,
{
    use rand09::Rng as _;

    // Work around rust-random/rand#1020.
    let mut rng = StepRng::new(0, 2_505_397_590);

    for _ in 0..iterations {
        drop(rng.random::<T>());
    }
}

#[rstest]
#[case(7, PhantomData::<Weekday>)]
#[case(12, PhantomData::<Month>)]
#[case(1, PhantomData::<Time>)]
#[case(1, PhantomData::<Date>)]
#[case(1, PhantomData::<UtcOffset>)]
#[case(1, PhantomData::<PlainDateTime>)]
#[case(1, PhantomData::<OffsetDateTime>)]
#[case(1, PhantomData::<SignedDuration>)]
fn support010<T>(#[case] iterations: usize, #[case] _type: PhantomData<T>)
where
    rand010::distr::StandardUniform: rand010::distr::Distribution<T>,
{
    use rand010::RngExt as _;

    // Work around rust-random/rand#1020.
    let mut rng = StepRng::new(0, 2_505_397_590);

    for _ in 0..iterations {
        drop(rng.random::<T>());
    }
}

// copy of `StepRng` from rand 0.8 to avoid deprecation warnings
#[derive(Debug, Clone)]
struct StepRng {
    v: u64,
    a: u64,
}

impl StepRng {
    fn new(initial: u64, increment: u64) -> Self {
        Self {
            v: initial,
            a: increment,
        }
    }
}

impl rand09::RngCore for StepRng {
    fn next_u32(&mut self) -> u32 {
        self.next_u64() as u32
    }

    fn next_u64(&mut self) -> u64 {
        let res = self.v;
        self.v = self.v.wrapping_add(self.a);
        res
    }

    fn fill_bytes(&mut self, dst: &mut [u8]) {
        rand09::rand_core::impls::fill_bytes_via_next(self, dst)
    }
}

impl rand010::TryRng for StepRng {
    type Error = core::convert::Infallible;

    fn try_next_u32(&mut self) -> Result<u32, Self::Error> {
        self.try_next_u64().map(|v| v as u32)
    }

    fn try_next_u64(&mut self) -> Result<u64, Self::Error> {
        let res = self.v;
        self.v = self.v.wrapping_add(self.a);
        Ok(res)
    }

    #[expect(clippy::unimplemented)]
    fn try_fill_bytes(&mut self, _dst: &mut [u8]) -> Result<(), Self::Error> {
        unimplemented!("not used in testing, so not implemented");
    }
}
