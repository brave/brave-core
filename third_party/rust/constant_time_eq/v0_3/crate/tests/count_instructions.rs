#[cfg(feature = "count_instructions_test")]
mod tests {
    use std::io::Result;

    use constant_time_eq::{constant_time_eq, constant_time_eq_n};
    use count_instructions::{count_instructions, Address};

    #[inline(never)]
    fn count(l: &[u8], r: &[u8], capacity: usize) -> Result<Vec<Address>> {
        let mut addresses = Vec::with_capacity(capacity);
        assert!(!count_instructions(
            || constant_time_eq(l, r),
            |instruction| addresses.push(instruction.address())
        )?);
        Ok(addresses)
    }

    #[inline(never)]
    fn count_n<const N: usize>(l: &[u8; N], r: &[u8; N], capacity: usize) -> Result<Vec<Address>> {
        let mut addresses = Vec::with_capacity(capacity);
        assert!(!count_instructions(
            || constant_time_eq_n(l, r),
            |instruction| addresses.push(instruction.address())
        )?);
        Ok(addresses)
    }

    fn test(a: u8, b: u8) -> Result<()> {
        const N: usize = 64;
        let l = vec![a; N];
        let r = vec![b; N];
        let baseline = count(&l, &r, 0)?;

        let mut t = r.clone();
        for n in 0..(N - 1) {
            t[n] = a;
            assert_eq!(count(&l, &t, baseline.len())?, baseline);
        }

        t[N - 1] = a;
        assert!(constant_time_eq(&l, &t));

        let mut t = r.clone();
        for n in 1..N {
            t[N - n] = a;
            assert_eq!(count(&l, &t, baseline.len())?, baseline);
        }

        t[0] = a;
        assert!(constant_time_eq(&l, &t));

        Ok(())
    }

    fn test_n<const N: usize>(a: u8, b: u8) -> Result<()> {
        let l = [a; N];
        let r = [b; N];
        let baseline = count_n(&l, &r, 0)?;

        let mut t = r.clone();
        for n in 0..(N - 1) {
            t[n] = a;
            assert_eq!(count_n(&l, &t, baseline.len())?, baseline);
        }

        t[N - 1] = a;
        assert!(constant_time_eq_n(&l, &t));

        let mut t = r.clone();
        for n in 1..N {
            t[N - n] = a;
            assert_eq!(count_n(&l, &t, baseline.len())?, baseline);
        }

        t[0] = a;
        assert!(constant_time_eq_n(&l, &t));

        Ok(())
    }

    #[test]
    fn count_instructions_test() -> Result<()> {
        test(b'A', b'B')?;
        test(0x55, 0xAA)?;
        Ok(())
    }

    fn count_instructions_test_n<const N: usize>() -> Result<()> {
        test_n::<N>(b'A', b'B')?;
        test_n::<N>(0x55, 0xAA)?;
        Ok(())
    }

    #[test]
    fn count_instructions_test_n_16() -> Result<()> {
        count_instructions_test_n::<16>()
    }

    #[test]
    fn count_instructions_test_n_20() -> Result<()> {
        count_instructions_test_n::<20>()
    }

    #[test]
    fn count_instructions_test_n_24() -> Result<()> {
        count_instructions_test_n::<24>()
    }

    #[test]
    fn count_instructions_test_n_32() -> Result<()> {
        count_instructions_test_n::<32>()
    }

    #[test]
    fn count_instructions_test_n_48() -> Result<()> {
        count_instructions_test_n::<48>()
    }

    #[test]
    fn count_instructions_test_n_64() -> Result<()> {
        count_instructions_test_n::<64>()
    }

    // This silly test shows that count_instructions() can detect early returns.
    #[test]
    fn count_instructions_test_variable() -> Result<()> {
        #[inline(never)]
        fn variable_time_eq(a: &[u8], b: &[u8]) -> bool {
            if a.len() != b.len() {
                false
            } else {
                for i in 0..a.len() {
                    if a[i] != b[i] {
                        return false;
                    }
                }
                true
            }
        }

        #[inline(never)]
        fn count_variable(l: &[u8], r: &[u8], capacity: usize) -> Result<Vec<Address>> {
            let mut addresses = Vec::with_capacity(capacity);
            assert!(!count_instructions(
                || variable_time_eq(l, r),
                |instruction| addresses.push(instruction.address())
            )?);
            Ok(addresses)
        }

        const N: usize = 64;
        let l = vec![b'A'; N];
        let r = vec![b'B'; N];

        let mut t = r.clone();
        t[0] = b'A';
        let short = count_variable(&l, &t, 0)?;

        let mut t = l.clone();
        t[N - 1] = b'B';
        let long = count_variable(&l, &t, short.len())?;

        assert_ne!(short, long);
        Ok(())
    }
}
