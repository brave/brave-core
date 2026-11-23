use std::hint::black_box;

use criterion::Bencher;
use time::format_description;

const FORMAT_DESCRIPTION: &str =
    "[year]-[month]-[day]T[hour]:[minute]:[second].[subsecond digits:3]";

setup_benchmark! {
    "Parse format description",

    fn parse_borrowed_v2(ben: &mut Bencher<'_>) {
        ben.iter(|| format_description::parse_borrowed::<2>(black_box(FORMAT_DESCRIPTION)));
    }

    fn parse_borrowed_v3(ben: &mut Bencher<'_>) {
        ben.iter(|| format_description::parse_borrowed::<3>(black_box(FORMAT_DESCRIPTION)));
    }

    fn parse_owned_v2(ben: &mut Bencher<'_>) {
        ben.iter(|| format_description::parse_owned::<2>(black_box(FORMAT_DESCRIPTION)));
    }

    fn parse_owned_v3(ben: &mut Bencher<'_>) {
        ben.iter(|| format_description::parse_owned::<3>(black_box(FORMAT_DESCRIPTION)));
    }
}
