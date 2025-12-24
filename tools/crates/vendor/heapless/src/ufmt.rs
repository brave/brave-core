use crate::{string::String, vec::Vec};
use ufmt_write::uWrite;

impl<const N: usize> uWrite for String<N> {
    type Error = ();
    fn write_str(&mut self, s: &str) -> Result<(), Self::Error> {
        self.push_str(s)
    }
}

impl<const N: usize> uWrite for Vec<u8, N> {
    type Error = ();
    fn write_str(&mut self, s: &str) -> Result<(), Self::Error> {
        self.extend_from_slice(s.as_bytes())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    use ufmt::{derive::uDebug, uwrite};

    #[derive(uDebug)]
    struct Pair {
        x: u32,
        y: u32,
    }

    #[test]
    fn test_string() {
        let a = 123;
        let b = Pair { x: 0, y: 1234 };

        let mut s = String::<32>::new();
        uwrite!(s, "{} -> {:?}", a, b).unwrap();

        assert_eq!(s, "123 -> Pair { x: 0, y: 1234 }");
    }

    #[test]
    fn test_string_err() {
        let p = Pair { x: 0, y: 1234 };
        let mut s = String::<4>::new();
        assert!(uwrite!(s, "{:?}", p).is_err());
    }

    #[test]
    fn test_vec() {
        let a = 123;
        let b = Pair { x: 0, y: 1234 };

        let mut v = Vec::<u8, 32>::new();
        uwrite!(v, "{} -> {:?}", a, b).unwrap();

        assert_eq!(v, b"123 -> Pair { x: 0, y: 1234 }");
    }
}
