use criterion::{black_box, criterion_group, criterion_main, Criterion};
use std::convert::TryFrom;

use star_sharks::Share;

#[cfg(feature = "std")]
fn dealer(c: &mut Criterion) {
  let sharks = star_sharks::Sharks(255);
  let mut dealer = sharks.dealer(&[1]).unwrap();

  c.bench_function("obtain_shares_dealer", |b| {
    b.iter(|| sharks.dealer(black_box(&[1])).unwrap())
  });
  c.bench_function("step_shares_dealer", |b| b.iter(|| dealer.next()));
}

#[cfg(feature = "std")]
fn recover(c: &mut Criterion) {
  let sharks = star_sharks::Sharks(255);
  let dealer = sharks.dealer(&[1]).unwrap();
  let shares: Vec<Share> = dealer.take(255).collect();

  c.bench_function("recover_secret", |b| {
    b.iter(|| sharks.recover(black_box(shares.as_slice())))
  });
}

fn share(c: &mut Criterion) {
  let bytes_vec = test_bytes();
  let bytes = bytes_vec.as_slice();
  let share = Share::try_from(bytes).unwrap();

  c.bench_function("share_from_bytes", |b| {
    b.iter(|| Share::try_from(black_box(bytes)))
  });

  c.bench_function("share_to_bytes", |b| {
    b.iter(|| Vec::from(black_box(&share)))
  });
}

fn test_bytes() -> Vec<u8> {
  let suffix = vec![0u8; star_sharks::FIELD_ELEMENT_LEN - 1];
  let mut bytes = vec![1u8; 1];
  bytes.extend(suffix.clone()); // x coord
  bytes.extend(vec![2u8; 1]);
  bytes.extend(suffix.clone()); // y coord #1
  bytes.extend(vec![3u8; 1]);
  bytes.extend(suffix); // y coord #2
  bytes
}

#[cfg(feature = "std")]
criterion_group!(benches, dealer, recover, share);

#[cfg(not(feature = "std"))]
criterion_group!(benches, share);

criterion_main!(benches);
