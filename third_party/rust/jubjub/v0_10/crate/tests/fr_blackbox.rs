mod common;

use common::{new_rng, MyRandom, NUM_BLACK_BOX_CHECKS};
use jubjub::*;

#[test]
fn test_to_and_from_bytes() {
    let mut rng = new_rng();
    for _ in 0..NUM_BLACK_BOX_CHECKS {
        let a = Fr::new_random(&mut rng);
        assert_eq!(a, Fr::from_bytes(&Fr::to_bytes(&a)).unwrap());
    }
}

#[test]
fn test_additive_associativity() {
    let mut rng = new_rng();
    for _ in 0..NUM_BLACK_BOX_CHECKS {
        let a = Fr::new_random(&mut rng);
        let b = Fr::new_random(&mut rng);
        let c = Fr::new_random(&mut rng);
        assert_eq!((a + b) + c, a + (b + c))
    }
}

#[test]
fn test_additive_identity() {
    let mut rng = new_rng();
    for _ in 0..NUM_BLACK_BOX_CHECKS {
        let a = Fr::new_random(&mut rng);
        assert_eq!(a, a + Fr::zero());
        assert_eq!(a, Fr::zero() + a);
    }
}

#[test]
fn test_subtract_additive_identity() {
    let mut rng = new_rng();
    for _ in 0..NUM_BLACK_BOX_CHECKS {
        let a = Fr::new_random(&mut rng);
        assert_eq!(a, a - Fr::zero());
        assert_eq!(a, Fr::zero() - -&a);
    }
}

#[test]
fn test_additive_inverse() {
    let mut rng = new_rng();
    for _ in 0..NUM_BLACK_BOX_CHECKS {
        let a = Fr::new_random(&mut rng);
        let a_neg = -&a;
        assert_eq!(Fr::zero(), a + a_neg);
        assert_eq!(Fr::zero(), a_neg + a);
    }
}

#[allow(clippy::eq_op)]
#[test]
fn test_additive_commutativity() {
    let mut rng = new_rng();
    for _ in 0..NUM_BLACK_BOX_CHECKS {
        let a = Fr::new_random(&mut rng);
        let b = Fr::new_random(&mut rng);
        assert_eq!(a + b, b + a);
    }
}

#[test]
fn test_multiplicative_associativity() {
    let mut rng = new_rng();
    for _ in 0..NUM_BLACK_BOX_CHECKS {
        let a = Fr::new_random(&mut rng);
        let b = Fr::new_random(&mut rng);
        let c = Fr::new_random(&mut rng);
        assert_eq!((a * b) * c, a * (b * c))
    }
}

#[test]
fn test_multiplicative_identity() {
    let mut rng = new_rng();
    for _ in 0..NUM_BLACK_BOX_CHECKS {
        let a = Fr::new_random(&mut rng);
        assert_eq!(a, a * Fr::one());
        assert_eq!(a, Fr::one() * a);
    }
}

#[test]
fn test_multiplicative_inverse() {
    let mut rng = new_rng();
    for _ in 0..NUM_BLACK_BOX_CHECKS {
        let a = Fr::new_random(&mut rng);
        if a == Fr::zero() {
            continue;
        }
        let a_inv = a.invert().unwrap();
        assert_eq!(Fr::one(), a * a_inv);
        assert_eq!(Fr::one(), a_inv * a);
    }
}

#[test]
fn test_multiplicative_commutativity() {
    let mut rng = new_rng();
    for _ in 0..NUM_BLACK_BOX_CHECKS {
        let a = Fr::new_random(&mut rng);
        let b = Fr::new_random(&mut rng);
        assert_eq!(a * b, b * a);
    }
}

#[test]
fn test_multiply_additive_identity() {
    let mut rng = new_rng();
    for _ in 0..NUM_BLACK_BOX_CHECKS {
        let a = Fr::new_random(&mut rng);
        assert_eq!(Fr::zero(), Fr::zero() * a);
        assert_eq!(Fr::zero(), a * Fr::zero());
    }
}
