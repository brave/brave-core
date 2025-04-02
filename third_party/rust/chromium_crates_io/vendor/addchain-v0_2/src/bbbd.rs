//! The Bergeron-Berstel-Brlek-Duboc algorithm for finding short addition chains.
//!
//! References:
//! - Bergeron, Berstel, Brlek, Duboc.
//!   ["Addition chains using continued fractions."][BBBD1989]
//! - [Handbook of Elliptic and Hyperelliptic Curve Cryptography][HEHCC], Chapter 9:
//!   Exponentiation
//!
//! [BBBD1989]: https://doi.org/10.1016/0196-6774(89)90036-9
//! [HEHCC]: https://www.hyperelliptic.org/HEHCC/index.html

use num_bigint::BigUint;
use num_integer::Integer;
use num_traits::{One, Zero};
use std::ops::{Add, Mul};

/// A wrapper around an addition chain. Addition and multiplication operations are defined
/// according to the BBBD algorithm.
#[derive(Debug)]
pub(super) struct Chain(Vec<BigUint>);

impl Add<BigUint> for Chain {
    type Output = Self;

    fn add(mut self, k: BigUint) -> Self {
        self.0.push(k + self.0.last().expect("chain is not empty"));
        self
    }
}

impl Mul<Chain> for Chain {
    type Output = Self;

    fn mul(mut self, mut other: Chain) -> Self {
        let last = self.0.last().expect("chain is not empty");

        // The first element of every chain is 1, so we skip it to prevent duplicate
        // entries in the resulting chain.
        assert!(other.0.remove(0).is_one());

        for w in other.0.iter_mut() {
            *w *= last;
        }
        self.0.append(&mut other.0);

        self
    }
}

pub(super) fn find_shortest_chain(n: BigUint) -> Vec<BigUint> {
    minchain(n).0
}

fn minchain(n: BigUint) -> Chain {
    let log_n = n.bits() - 1;
    if n == BigUint::one() << log_n {
        Chain((0..=log_n).map(|i| BigUint::one() << i).collect())
    } else if n == BigUint::from(3u32) {
        Chain(vec![BigUint::one(), BigUint::from(2u32), n])
    } else {
        // The minchain() algorithm on page 162 of HEHCC indicates that k should be set to
        // 2^(log(n) / 2) in the call to chain(). This is at odds with the definition of k
        // at the bottom of page 161; the latter gives the intended result.
        let k = &n / (BigUint::one() << (log_n / 2));
        chain(n, k)
    }
}

fn chain(n: BigUint, k: BigUint) -> Chain {
    let (q, r) = n.div_rem(&k);
    if r.is_zero() || r.is_one() {
        // We handle the r = 1 case here to prevent unnecessary recursion.
        minchain(k) * minchain(q) + r
    } else {
        chain(k, r.clone()) * minchain(q) + r
    }
}

#[cfg(test)]
mod tests {
    use num_bigint::BigUint;

    use super::minchain;

    #[test]
    fn minchain_87() {
        // Example 9.37 from HEHCC.
        let chain = minchain(BigUint::from(87u32));
        assert_eq!(
            chain.0,
            vec![
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
            ]
        );
    }
}
