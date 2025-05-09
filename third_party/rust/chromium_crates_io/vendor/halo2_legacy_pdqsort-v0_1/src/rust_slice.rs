// From https://raw.githubusercontent.com/rust-lang/rust/1.56.1/library/core/tests/slice.rs
// (which has not changed from 1.56.1 to 1.67.1).

#[test]
#[cfg(not(target_arch = "wasm32"))]
fn sort_unstable() {
    // MODIFIED: test the implementations in this crate.
    use crate::sort::{heapsort, quicksort};
    use rand::{rngs::StdRng, seq::SliceRandom, Rng, SeedableRng};

    // Miri is too slow (but still need to `chain` to make the types match)
    let lens = if cfg!(miri) {
        (2..20).chain(0..0)
    } else {
        (2..25).chain(500..510)
    };
    let rounds = if cfg!(miri) { 1 } else { 100 };

    let mut v = [0; 600];
    let mut tmp = [0; 600];
    let mut rng = StdRng::from_entropy();

    for len in lens {
        let v = &mut v[0..len];
        let tmp = &mut tmp[0..len];

        for &modulus in &[5, 10, 100, 1000] {
            for _ in 0..rounds {
                for i in 0..len {
                    v[i] = rng.gen::<i32>() % modulus;
                }

                // MODIFIED: "Sort in default order" test removed since the code
                // in this crate requires a specific order.

                // Sort in ascending order.
                tmp.copy_from_slice(v);
                // MODIFIED: test quicksort directly rather than via the slice API.
                quicksort(tmp, |a, b| a < b);
                assert!(tmp.windows(2).all(|w| w[0] <= w[1]));

                // Sort in descending order.
                tmp.copy_from_slice(v);
                // MODIFIED: test quicksort directly rather than via the slice API.
                quicksort(tmp, |a, b| a > b);
                assert!(tmp.windows(2).all(|w| w[0] >= w[1]));

                // Test heapsort using `<` operator.
                tmp.copy_from_slice(v);
                heapsort(tmp, |a, b| a < b);
                assert!(tmp.windows(2).all(|w| w[0] <= w[1]));

                // Test heapsort using `>` operator.
                tmp.copy_from_slice(v);
                heapsort(tmp, |a, b| a > b);
                assert!(tmp.windows(2).all(|w| w[0] >= w[1]));
            }
        }
    }

    // Sort using a completely random comparison function.
    // This will reorder the elements *somehow*, but won't panic.
    for i in 0..v.len() {
        v[i] = i as i32;
    }

    // MODIFIED: test quicksort directly rather than via the slice API.
    quicksort(&mut v, |_, _| *[true, false].choose(&mut rng).unwrap());
    quicksort(&mut v, |a, b| a < b);
    for i in 0..v.len() {
        assert_eq!(v[i], i as i32);
    }

    // Should not panic.
    // MODIFIED: test quicksort directly rather than via the slice API.
    quicksort(&mut [0i32; 0], |a, b| a < b);
    quicksort(&mut [(); 10], |a, b| a < b);
    quicksort(&mut [(); 100], |a, b| a < b);

    let mut v = [0xDEADBEEFu64];
    // MODIFIED: test quicksort directly rather than via the slice API.
    quicksort(&mut v, |a, b| a < b);
    assert!(v == [0xDEADBEEF]);
}
