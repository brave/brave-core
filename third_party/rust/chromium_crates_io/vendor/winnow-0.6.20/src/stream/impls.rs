macro_rules! impl_partial_eq {
    ($lhs:ty, $rhs:ty) => {
        #[allow(unused_lifetimes)]
        impl<'a> PartialEq<$rhs> for $lhs {
            #[inline]
            fn eq(&self, other: &$rhs) -> bool {
                let l = self.as_ref();
                let r: &Self = other.as_ref();
                PartialEq::eq(l, r)
            }
        }

        #[allow(unused_lifetimes)]
        impl<'a> PartialEq<$lhs> for $rhs {
            #[inline]
            fn eq(&self, other: &$lhs) -> bool {
                PartialEq::eq(other, self)
            }
        }
    };
}

macro_rules! impl_partial_ord {
    ($lhs:ty, $rhs:ty) => {
        #[allow(unused_lifetimes)]
        impl<'a> PartialOrd<$rhs> for $lhs {
            #[inline]
            fn partial_cmp(&self, other: &$rhs) -> Option<Ordering> {
                let l = self.as_ref();
                let r: &Self = other.as_ref();
                PartialOrd::partial_cmp(l, r)
            }
        }

        #[allow(unused_lifetimes)]
        impl<'a> PartialOrd<$lhs> for $rhs {
            #[inline]
            fn partial_cmp(&self, other: &$lhs) -> Option<Ordering> {
                PartialOrd::partial_cmp(other, self)
            }
        }
    };
}

mod bytes {
    use crate::lib::std::{cmp::Ordering, fmt, ops};

    use crate::stream::Bytes;

    impl fmt::Display for Bytes {
        #[inline]
        fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
            <Self as fmt::UpperHex>::fmt(self, f)
        }
    }

    impl fmt::Debug for Bytes {
        #[inline]
        fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
            <Self as fmt::UpperHex>::fmt(self, f)
        }
    }

    impl fmt::LowerHex for Bytes {
        #[inline]
        fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
            for byte in self.as_bytes() {
                write!(f, "{byte:0>2x}")?;
            }
            Ok(())
        }
    }

    impl fmt::UpperHex for Bytes {
        #[inline]
        fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
            for (i, byte) in self.as_bytes().iter().enumerate() {
                if 0 < i {
                    let absolute = (self.as_bytes().as_ptr() as usize) + i;
                    if f.alternate() && absolute != 0 && absolute % 4 == 0 {
                        write!(f, "_")?;
                    }
                }
                write!(f, "{byte:0>2X}")?;
            }
            Ok(())
        }
    }

    impl ops::Deref for Bytes {
        type Target = [u8];

        #[inline]
        fn deref(&self) -> &[u8] {
            self.as_bytes()
        }
    }

    impl ops::Index<usize> for Bytes {
        type Output = u8;

        #[inline]
        fn index(&self, idx: usize) -> &u8 {
            &self.as_bytes()[idx]
        }
    }

    impl ops::Index<ops::RangeFull> for Bytes {
        type Output = Bytes;

        #[inline]
        fn index(&self, _: ops::RangeFull) -> &Bytes {
            self
        }
    }

    impl ops::Index<ops::Range<usize>> for Bytes {
        type Output = Bytes;

        #[inline]
        fn index(&self, r: ops::Range<usize>) -> &Bytes {
            Bytes::new(&self.as_bytes()[r.start..r.end])
        }
    }

    impl ops::Index<ops::RangeInclusive<usize>> for Bytes {
        type Output = Bytes;

        #[inline]
        fn index(&self, r: ops::RangeInclusive<usize>) -> &Bytes {
            Bytes::new(&self.as_bytes()[*r.start()..=*r.end()])
        }
    }

    impl ops::Index<ops::RangeFrom<usize>> for Bytes {
        type Output = Bytes;

        #[inline]
        fn index(&self, r: ops::RangeFrom<usize>) -> &Bytes {
            Bytes::new(&self.as_bytes()[r.start..])
        }
    }

    impl ops::Index<ops::RangeTo<usize>> for Bytes {
        type Output = Bytes;

        #[inline]
        fn index(&self, r: ops::RangeTo<usize>) -> &Bytes {
            Bytes::new(&self.as_bytes()[..r.end])
        }
    }

    impl ops::Index<ops::RangeToInclusive<usize>> for Bytes {
        type Output = Bytes;

        #[inline]
        fn index(&self, r: ops::RangeToInclusive<usize>) -> &Bytes {
            Bytes::new(&self.as_bytes()[..=r.end])
        }
    }

    impl AsRef<[u8]> for Bytes {
        #[inline]
        fn as_ref(&self) -> &[u8] {
            self.as_bytes()
        }
    }

    impl AsRef<Bytes> for [u8] {
        #[inline]
        fn as_ref(&self) -> &Bytes {
            Bytes::new(self)
        }
    }

    impl AsRef<Bytes> for str {
        #[inline]
        fn as_ref(&self) -> &Bytes {
            Bytes::new(self)
        }
    }

    #[cfg(feature = "alloc")]
    impl crate::lib::std::borrow::ToOwned for Bytes {
        type Owned = crate::lib::std::vec::Vec<u8>;

        #[inline]
        fn to_owned(&self) -> Self::Owned {
            crate::lib::std::vec::Vec::from(self.as_bytes())
        }
    }

    #[cfg(feature = "alloc")]
    impl crate::lib::std::borrow::Borrow<Bytes> for crate::lib::std::vec::Vec<u8> {
        #[inline]
        fn borrow(&self) -> &Bytes {
            Bytes::from_bytes(self.as_slice())
        }
    }

    impl<'a> Default for &'a Bytes {
        fn default() -> &'a Bytes {
            Bytes::new(b"")
        }
    }

    impl<'a> From<&'a [u8]> for &'a Bytes {
        #[inline]
        fn from(s: &'a [u8]) -> &'a Bytes {
            Bytes::new(s)
        }
    }

    impl<'a> From<&'a Bytes> for &'a [u8] {
        #[inline]
        fn from(s: &'a Bytes) -> &'a [u8] {
            Bytes::as_bytes(s)
        }
    }

    impl<'a> From<&'a str> for &'a Bytes {
        #[inline]
        fn from(s: &'a str) -> &'a Bytes {
            Bytes::new(s.as_bytes())
        }
    }

    impl Eq for Bytes {}

    impl PartialEq<Bytes> for Bytes {
        #[inline]
        fn eq(&self, other: &Bytes) -> bool {
            self.as_bytes() == other.as_bytes()
        }
    }

    impl_partial_eq!(Bytes, [u8]);
    impl_partial_eq!(Bytes, &'a [u8]);
    impl_partial_eq!(Bytes, str);
    impl_partial_eq!(Bytes, &'a str);

    impl PartialOrd for Bytes {
        #[inline]
        fn partial_cmp(&self, other: &Bytes) -> Option<Ordering> {
            Some(self.cmp(other))
        }
    }

    impl Ord for Bytes {
        #[inline]
        fn cmp(&self, other: &Bytes) -> Ordering {
            Ord::cmp(self.as_bytes(), other.as_bytes())
        }
    }

    impl_partial_ord!(Bytes, [u8]);
    impl_partial_ord!(Bytes, &'a [u8]);
    impl_partial_ord!(Bytes, str);
    impl_partial_ord!(Bytes, &'a str);

    #[cfg(all(test, feature = "std"))]
    mod display {
        use crate::stream::Bytes;

        #[test]
        fn clean() {
            assert_eq!(&format!("{}", Bytes::new(b"abc")), "616263");
            assert_eq!(&format!("{}", Bytes::new(b"\xf0\x28\x8c\xbc")), "F0288CBC");
        }
    }

    #[cfg(all(test, feature = "std"))]
    mod debug {
        use crate::stream::Bytes;

        #[test]
        fn test_debug() {
            assert_eq!(
                "000000206674797069736F6D0000020069736F6D69736F32617663316D70",
                format!(
                    "{:?}",
                    Bytes::new(b"\0\0\0 ftypisom\0\0\x02\0isomiso2avc1mp")
                ),
            );
        }

        #[test]
        fn test_pretty_debug() {
            // Output can change from run-to-run
            format!(
                "{:#?}",
                Bytes::new(b"\0\0\0 ftypisom\0\0\x02\0isomiso2avc1mp")
            );
        }

        #[test]
        fn test_sliced() {
            // Output can change from run-to-run
            let total = Bytes::new(b"12345678901234567890");
            format!("{total:#?}");
            format!("{:#?}", &total[1..]);
            format!("{:#?}", &total[10..]);
        }
    }
}

mod bstr {
    use crate::lib::std::{cmp::Ordering, fmt, ops};

    use crate::stream::BStr;

    #[cfg(feature = "alloc")]
    impl fmt::Display for BStr {
        #[inline]
        fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
            crate::lib::std::string::String::from_utf8_lossy(self.as_bytes()).fmt(f)
        }
    }

    impl fmt::Debug for BStr {
        #[inline]
        fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
            if !f.alternate() {
                write!(f, "\"")?;
            }
            for byte in self.as_bytes() {
                let c = *byte as char;
                write!(f, "{}", c.escape_debug())?;
            }
            if !f.alternate() {
                write!(f, "\"")?;
            }
            Ok(())
        }
    }

    impl ops::Deref for BStr {
        type Target = [u8];

        #[inline]
        fn deref(&self) -> &[u8] {
            self.as_bytes()
        }
    }

    impl ops::Index<usize> for BStr {
        type Output = u8;

        #[inline]
        fn index(&self, idx: usize) -> &u8 {
            &self.as_bytes()[idx]
        }
    }

    impl ops::Index<ops::RangeFull> for BStr {
        type Output = BStr;

        #[inline]
        fn index(&self, _: ops::RangeFull) -> &BStr {
            self
        }
    }

    impl ops::Index<ops::Range<usize>> for BStr {
        type Output = BStr;

        #[inline]
        fn index(&self, r: ops::Range<usize>) -> &BStr {
            BStr::new(&self.as_bytes()[r.start..r.end])
        }
    }

    impl ops::Index<ops::RangeInclusive<usize>> for BStr {
        type Output = BStr;

        #[inline]
        fn index(&self, r: ops::RangeInclusive<usize>) -> &BStr {
            BStr::new(&self.as_bytes()[*r.start()..=*r.end()])
        }
    }

    impl ops::Index<ops::RangeFrom<usize>> for BStr {
        type Output = BStr;

        #[inline]
        fn index(&self, r: ops::RangeFrom<usize>) -> &BStr {
            BStr::new(&self.as_bytes()[r.start..])
        }
    }

    impl ops::Index<ops::RangeTo<usize>> for BStr {
        type Output = BStr;

        #[inline]
        fn index(&self, r: ops::RangeTo<usize>) -> &BStr {
            BStr::new(&self.as_bytes()[..r.end])
        }
    }

    impl ops::Index<ops::RangeToInclusive<usize>> for BStr {
        type Output = BStr;

        #[inline]
        fn index(&self, r: ops::RangeToInclusive<usize>) -> &BStr {
            BStr::new(&self.as_bytes()[..=r.end])
        }
    }

    impl AsRef<[u8]> for BStr {
        #[inline]
        fn as_ref(&self) -> &[u8] {
            self.as_bytes()
        }
    }

    impl AsRef<BStr> for [u8] {
        #[inline]
        fn as_ref(&self) -> &BStr {
            BStr::new(self)
        }
    }

    impl AsRef<BStr> for str {
        #[inline]
        fn as_ref(&self) -> &BStr {
            BStr::new(self)
        }
    }

    #[cfg(feature = "alloc")]
    impl crate::lib::std::borrow::ToOwned for BStr {
        type Owned = crate::lib::std::vec::Vec<u8>;

        #[inline]
        fn to_owned(&self) -> Self::Owned {
            crate::lib::std::vec::Vec::from(self.as_bytes())
        }
    }

    #[cfg(feature = "alloc")]
    impl crate::lib::std::borrow::Borrow<BStr> for crate::lib::std::vec::Vec<u8> {
        #[inline]
        fn borrow(&self) -> &BStr {
            BStr::from_bytes(self.as_slice())
        }
    }

    impl<'a> Default for &'a BStr {
        fn default() -> &'a BStr {
            BStr::new(b"")
        }
    }

    impl<'a> From<&'a [u8]> for &'a BStr {
        #[inline]
        fn from(s: &'a [u8]) -> &'a BStr {
            BStr::new(s)
        }
    }

    impl<'a> From<&'a BStr> for &'a [u8] {
        #[inline]
        fn from(s: &'a BStr) -> &'a [u8] {
            BStr::as_bytes(s)
        }
    }

    impl<'a> From<&'a str> for &'a BStr {
        #[inline]
        fn from(s: &'a str) -> &'a BStr {
            BStr::new(s.as_bytes())
        }
    }

    impl Eq for BStr {}

    impl PartialEq<BStr> for BStr {
        #[inline]
        fn eq(&self, other: &BStr) -> bool {
            self.as_bytes() == other.as_bytes()
        }
    }

    impl_partial_eq!(BStr, [u8]);
    impl_partial_eq!(BStr, &'a [u8]);
    impl_partial_eq!(BStr, str);
    impl_partial_eq!(BStr, &'a str);

    impl PartialOrd for BStr {
        #[inline]
        fn partial_cmp(&self, other: &BStr) -> Option<Ordering> {
            Some(self.cmp(other))
        }
    }

    impl Ord for BStr {
        #[inline]
        fn cmp(&self, other: &BStr) -> Ordering {
            Ord::cmp(self.as_bytes(), other.as_bytes())
        }
    }

    impl_partial_ord!(BStr, [u8]);
    impl_partial_ord!(BStr, &'a [u8]);
    impl_partial_ord!(BStr, str);
    impl_partial_ord!(BStr, &'a str);

    #[cfg(all(test, feature = "std"))]
    mod display {
        use crate::stream::BStr;

        #[test]
        fn clean() {
            assert_eq!(&format!("{}", BStr::new(b"abc")), "abc");
            assert_eq!(&format!("{}", BStr::new(b"\xf0\x28\x8c\xbc")), "�(��");
        }
    }

    #[cfg(all(test, feature = "std"))]
    mod debug {
        use crate::stream::BStr;

        #[test]
        fn test_debug() {
            assert_eq!(&format!("{:?}", BStr::new(b"abc")), "\"abc\"");

            assert_eq!(
                "\"\\0\\0\\0 ftypisom\\0\\0\\u{2}\\0isomiso2avc1mp\"",
                format!(
                    "{:?}",
                    BStr::new(b"\0\0\0 ftypisom\0\0\x02\0isomiso2avc1mp")
                ),
            );
        }

        #[test]
        fn test_pretty_debug() {
            assert_eq!(&format!("{:#?}", BStr::new(b"abc")), "abc");
        }
    }
}
