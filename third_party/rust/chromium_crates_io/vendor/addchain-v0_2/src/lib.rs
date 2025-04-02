//! *Library for generating addition chains*
//!
//! An addition chain `C` for some positive integer `n` is a sequence of integers that
//! have the following properties:
//!
//! - The first integer is 1.
//! - The last integer is `n`.
//! - Integers only appear once.
//! - Every integer is either the sum of two earlier integers, or double an earlier
//!   integer.
//!
//! An addition chain corresponds to a series of `len(C) - 1` primitive operations
//! (doubling and addition) that can be used to compute a target integer. An *optimal*
//! addition chain for `n` has the shortest possible length, and therefore requires the
//! fewest operations to compute `n`. This is particularly useful in cryptographic
//! algorithms such as modular exponentiation, where `n` is usually at least `2^128`.
//!
//! # Example
//!
//! To compute the number 87, we can represent it in binary as `1010111`, and then using
//! the binary double-and-add algorithm (where we double for every bit, and add 1 for
//! every bit that is set to 1) we have the following steps:
//! ```text
//!  i | n_i | Operation | b_i
//! ---|-----|-----------|-----
//!  0 |  1  |           |  1
//!  1 |  2  | n_0 * 2   |  0
//!  2 |  4  | n_1 * 2   |  1
//!  3 |  5  | n_2 + n_0 |
//!  4 | 10  | n_3 * 2   |  0
//!  5 | 20  | n_4 * 2   |  1
//!  6 | 21  | n_5 + n_0 |
//!  7 | 42  | n_6 * 2   |  1
//!  8 | 43  | n_7 + n_0 |
//!  9 | 86  | n_8 * 2   |  1
//! 10 | 87  | n_9 + n_0 |
//! ```
//!
//! This corresponds to the addition chain `[1, 2, 4, 5, 10, 20, 21, 42, 43, 86, 87]`,
//! which has length 11. However, the optimal addition chain length for 87 is 10, and
//! several addition chains can be constructed with optimal length. One such chain is
//! `[1, 2, 3, 6, 7, 10, 20, 40, 80, 87]`, which corresponds to the following steps:
//! ```text
//!  i | n_i | Operation
//! ---|-----|----------
//!  0 |  1  |
//!  1 |  2  | n_0 * 2
//!  2 |  3  | n_1 + n_0
//!  3 |  6  | n_2 * 2
//!  4 |  7  | n_3 + n_0
//!  5 | 10  | n_4 + n_2
//!  6 | 20  | n_5 * 2
//!  7 | 40  | n_6 * 2
//!  8 | 80  | n_7 * 2
//!  9 | 87  | n_8 + n_4
//! ```
//!
//! # Usage
//!
//! ```
//! use addchain::{build_addition_chain, Step};
//! use num_bigint::BigUint;
//!
//! assert_eq!(
//!     build_addition_chain(BigUint::from(87u32)),
//!     vec![
//!         Step::Double { index: 0 },
//!         Step::Add { left: 1, right: 0 },
//!         Step::Double { index: 2 },
//!         Step::Add { left: 3, right: 0 },
//!         Step::Add { left: 4, right: 2 },
//!         Step::Double { index: 5 },
//!         Step::Double { index: 6 },
//!         Step::Double { index: 7 },
//!         Step::Add { left: 8, right: 4 },
//!     ],
//! );
//! ```

use num_bigint::BigUint;
use num_traits::One;

mod bbbd;

/// The error kinds returned by `addchain` APIs.
#[derive(Debug, PartialEq)]
pub enum Error {
    /// The provided chain is invalid.
    InvalidChain,
}

/// Returns the shortest addition chain we can find for the given number, using all
/// available algorithms.
pub fn find_shortest_chain(n: BigUint) -> Vec<BigUint> {
    bbbd::find_shortest_chain(n)
}

/// A single step in computing an addition chain.
#[derive(Debug, PartialEq)]
pub enum Step {
    Double { index: usize },
    Add { left: usize, right: usize },
}

/// Converts an addition chain into a series of steps.
pub fn build_steps(chain: Vec<BigUint>) -> Result<Vec<Step>, Error> {
    match chain.get(0) {
        Some(n) if n.is_one() => (),
        _ => return Err(Error::InvalidChain),
    }

    let mut steps = vec![];

    for (i, val) in chain.iter().enumerate().skip(1) {
        // Find the pair of previous values that add to this one
        'search: for (j, left) in chain[..i].iter().enumerate() {
            for (k, right) in chain[..=j].iter().enumerate() {
                if val == &(left + right) {
                    // Found the pair!
                    if j == k {
                        steps.push(Step::Double { index: j })
                    } else {
                        steps.push(Step::Add { left: j, right: k });
                    }
                    break 'search;
                }
            }
        }

        // We must always find a matching pair
        if steps.len() != i {
            return Err(Error::InvalidChain);
        }
    }

    Ok(steps)
}

/// Generates a series of steps that will compute an addition chain for the given number.
/// The addition chain is the shortest we can find using all available algorithms.
pub fn build_addition_chain(n: BigUint) -> Vec<Step> {
    build_steps(find_shortest_chain(n)).expect("chain is valid")
}

#[cfg(test)]
mod tests {
    use num_bigint::BigUint;

    use super::{build_steps, Error, Step};

    #[test]
    fn steps_from_valid_chains() {
        assert_eq!(
            build_steps(vec![
                BigUint::from(1u32),
                BigUint::from(2u32),
                BigUint::from(3u32),
            ]),
            Ok(vec![
                Step::Double { index: 0 },
                Step::Add { left: 1, right: 0 }
            ]),
        );

        assert_eq!(
            build_steps(vec![
                BigUint::from(1u32),
                BigUint::from(2u32),
                BigUint::from(4u32),
                BigUint::from(8u32),
            ]),
            Ok(vec![
                Step::Double { index: 0 },
                Step::Double { index: 1 },
                Step::Double { index: 2 },
            ]),
        );

        assert_eq!(
            build_steps(vec![
                BigUint::from(1u32),
                BigUint::from(2u32),
                BigUint::from(3u32),
                BigUint::from(6u32),
                BigUint::from(7u32),
                BigUint::from(10u32),
                BigUint::from(20u32),
                BigUint::from(40u32),
                BigUint::from(80u32),
                BigUint::from(87u32),
            ]),
            Ok(vec![
                Step::Double { index: 0 },
                Step::Add { left: 1, right: 0 },
                Step::Double { index: 2 },
                Step::Add { left: 3, right: 0 },
                Step::Add { left: 4, right: 2 },
                Step::Double { index: 5 },
                Step::Double { index: 6 },
                Step::Double { index: 7 },
                Step::Add { left: 8, right: 4 },
            ]),
        );
    }

    #[test]
    fn invalid_chains() {
        // First element is not one.
        assert_eq!(
            build_steps(vec![BigUint::from(2u32), BigUint::from(3u32),]),
            Err(Error::InvalidChain),
        );

        // Missing an element of a pair.
        assert_eq!(
            build_steps(vec![
                BigUint::from(1u32),
                BigUint::from(4u32),
                BigUint::from(8u32),
            ]),
            Err(Error::InvalidChain),
        );
    }
}
