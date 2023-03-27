#[cfg(not(feature = "std"))]
use alloc::vec::Vec;
use bigint::BigUint;

pub(crate) fn encode<T>(alpha: &[T], input: &[u8]) -> Vec<T>
where
    T: Copy,
{
    if input.is_empty() {
        return Vec::new();
    }

    let base = alpha.len() as u32;

    // Convert the input byte array to a BigUint
    let mut big = BigUint::from_bytes_be(input);
    let mut out = Vec::with_capacity(input.len());

    // Find the highest power of `base` that fits in `u32`
    let big_pow = 32 / (32 - base.leading_zeros());
    let big_base = base.pow(big_pow);

    'fast: loop {
        // Instead of diving by `base`, we divide by the `big_base`,
        // giving us a bigger remainder that we can further subdivide
        // by the original `base`. This greatly (in case of base58 it's
        // a factor of 5) reduces the amount of divisions that need to
        // be done on BigUint, delegating the hard work to regular `u32`
        // operations, which are blazing fast.
        let mut big_rem = big.div_mod(big_base);

        if big.is_zero() {
            loop {
                let (result, remainder) = (big_rem / base, big_rem % base);
                out.push(alpha[remainder as usize]);
                big_rem = result;

                if big_rem == 0 {
                    break 'fast; // teehee
                }
            }
        } else {
            for _ in 0..big_pow {
                let (result, remainder) = (big_rem / base, big_rem % base);
                out.push(alpha[remainder as usize]);
                big_rem = result;
            }
        }
    }

    let leaders = input
        .iter()
        .take(input.len() - 1)
        .take_while(|i| **i == 0)
        .map(|_| alpha[0]);

    out.extend(leaders);
    out
}
