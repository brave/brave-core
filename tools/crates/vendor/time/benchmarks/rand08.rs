use criterion::Bencher;
use rand08::Rng;
use rand08::rngs::mock::StepRng;
use time::{Date, Month, OffsetDateTime, PlainDateTime, SignedDuration, Time, UtcOffset, Weekday};

macro_rules! bench_rand {
    ($($name:ident : $type:ty),* $(,)?) => {
        setup_benchmark! {
            "Random",
            $(fn $name(ben: &mut Bencher<'_>) {
                iter_batched_ref!(
                    ben,
                    || StepRng::new(0, 1),
                    [|rng| rng.r#gen::<$type>()]
                );
            })*
        }
    }
}

bench_rand![
    time: Time,
    date: Date,
    utc_offset: UtcOffset,
    plain_date_time: PlainDateTime,
    offset_date_time: OffsetDateTime,
    duration: SignedDuration,
    weekday: Weekday,
    month: Month,
];
