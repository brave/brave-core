// Copyright 2016 Hroi Sigurdsson
//
// Licensed under the MIT license <LICENSE-MIT or http://opensource.org/licenses/MIT>.
// This file may not be copied, modified, or distributed except according to those terms.

#[cfg(not(feature = "alloc"))]
use std::net::{Ipv4Addr, Ipv6Addr};
#[cfg(feature = "alloc")]
pub mod addr {
    #[derive(Copy, Clone)]
    pub struct Ipv4Addr([u8; 4]);
    impl Ipv4Addr {
        pub fn new(a1: u8, a2: u8, a3: u8, a4: u8) -> Self {
            Ipv4Addr([a1, a2, a3, a4])
        }

        pub fn from(num: u32) -> Self {
            Ipv4Addr([
                (num >> 24) as u8,
                (num >> 16) as u8,
                (num >> 8) as u8,
                num as u8,
            ])
        }

        pub fn octets(&self) -> [u8; 4] {
            self.0
        }
    }
    impl ::std::convert::From<Ipv4Addr> for u32 {
        fn from(a: Ipv4Addr) -> u32 {
            (a.0[0] as u32) << 24
                | (a.0[1] as u32) << 16
                | (a.0[2] as u32) << 8
                | (a.0[3] as u32) << 0
        }
    }

    #[derive(Copy, Clone)]
    pub struct Ipv6Addr([u8; 16]);

    impl Ipv6Addr {
        pub fn new(a1: u16, a2: u16, a3: u16, a4: u16, a5: u16, a6: u16, a7: u16, a8: u16) -> Self {
            Ipv6Addr([
                (a1 >> 8) as u8,
                a1 as u8,
                (a2 >> 8) as u8,
                a2 as u8,
                (a3 >> 8) as u8,
                a3 as u8,
                (a4 >> 8) as u8,
                a4 as u8,
                (a5 >> 8) as u8,
                a5 as u8,
                (a6 >> 8) as u8,
                a6 as u8,
                (a7 >> 8) as u8,
                a7 as u8,
                (a8 >> 8) as u8,
                a8 as u8,
            ])
        }

        pub fn octets(&self) -> [u8; 16] {
            self.0
        }

        pub fn segments(&self) -> [u16; 8] {
            [
                (self.0[0] as u16) << 8 | (self.0[1] as u16),
                (self.0[2] as u16) << 8 | (self.0[3] as u16),
                (self.0[4] as u16) << 8 | (self.0[5] as u16),
                (self.0[6] as u16) << 8 | (self.0[7] as u16),
                (self.0[8] as u16) << 8 | (self.0[9] as u16),
                (self.0[10] as u16) << 8 | (self.0[11] as u16),
                (self.0[12] as u16) << 8 | (self.0[13] as u16),
                (self.0[14] as u16) << 8 | (self.0[15] as u16),
            ]
        }
    }
}

#[cfg(feature = "alloc")]
use self::addr::*;

/// Address trait provides methods required for storing in TreeBitmap trie datastructure.
pub trait Address: Copy {
    type Nibbles: AsRef<[u8]>;
    /// Convert to string of nibbles.
    fn nibbles(self) -> Self::Nibbles;
    /// Convert from string of nibbles.
    fn from_nibbles(nibbles: &[u8]) -> Self;
    /// Returns self masked to n bits.
    fn mask(self, masklen: u32) -> Self;
}

impl Address for Ipv4Addr {
    type Nibbles = [u8; 8];

    fn nibbles(self) -> Self::Nibbles {
        let mut ret: Self::Nibbles = [0; 8];
        let bytes: [u8; 4] = self.octets();
        for (i, byte) in bytes.iter().enumerate() {
            ret[i * 2] = byte >> 4;
            ret[i * 2 + 1] = byte & 0xf;
        }
        ret
    }

    fn from_nibbles(nibbles: &[u8]) -> Self {
        let mut ret: [u8; 4] = [0; 4];
        for (i, nibble) in nibbles.iter().enumerate().take(ret.len() * 2) {
            match i % 2 {
                0 => {
                    ret[i / 2] = *nibble << 4;
                }
                _ => {
                    ret[i / 2] |= *nibble;
                }
            }
        }
        Self::new(ret[0], ret[1], ret[2], ret[3])
    }

    fn mask(self, masklen: u32) -> Self {
        debug_assert!(masklen <= 32);
        let ip = u32::from(self);
        let masked = match masklen {
            0 => 0,
            n => ip & (!0 << (32 - n)),
        };
        Ipv4Addr::from(masked)
    }
}

impl Address for Ipv6Addr {
    type Nibbles = [u8; 32];

    fn nibbles(self) -> Self::Nibbles {
        let mut ret: Self::Nibbles = [0; 32];
        let bytes: [u8; 16] = self.octets();
        for (i, byte) in bytes.iter().enumerate() {
            ret[i * 2] = byte >> 4;
            ret[i * 2 + 1] = byte & 0xf;
        }
        ret
    }

    fn from_nibbles(nibbles: &[u8]) -> Self {
        let mut ret: [u16; 8] = [0; 8];
        for (i, nibble) in nibbles.iter().enumerate().take(ret.len() * 4) {
            match i % 4 {
                0 => {
                    ret[i / 4] |= (*nibble as u16) << 12;
                }
                1 => {
                    ret[i / 4] |= (*nibble as u16) << 8;
                }
                2 => {
                    ret[i / 4] |= (*nibble as u16) << 4;
                }
                _ => {
                    ret[i / 4] |= *nibble as u16;
                }
            }
        }
        Self::new(
            ret[0], ret[1], ret[2], ret[3], ret[4], ret[5], ret[6], ret[7],
        )
    }

    fn mask(self, masklen: u32) -> Self {
        debug_assert!(masklen <= 128);
        let mut ret = self.segments();
        for i in ((masklen + 15) / 16)..8 {
            ret[i as usize] = 0;
        }
        if masklen % 16 != 0 {
            ret[masklen as usize / 16] &= !0 << (16 - (masklen % 16));
        }
        Self::new(
            ret[0], ret[1], ret[2], ret[3], ret[4], ret[5], ret[6], ret[7],
        )
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::net::{Ipv4Addr, Ipv6Addr};
    use std::str::FromStr;

    #[test]
    fn address_ipv4_mask() {
        let ip = Ipv4Addr::new(1, 2, 3, 4);
        assert_eq!(ip.mask(0), Ipv4Addr::new(0, 0, 0, 0));
        assert_eq!(ip.mask(24), Ipv4Addr::new(1, 2, 3, 0));
        assert_eq!(ip.mask(23), Ipv4Addr::new(1, 2, 2, 0));
        assert_eq!(ip.mask(32), Ipv4Addr::new(1, 2, 3, 4));
    }

    #[test]
    fn address_ipv6_mask() {
        let ip = Ipv6Addr::from_str("2001:db8:aaaa:bbbb:cccc:dddd:eeee:ffff").unwrap();
        let expected1 = Ipv6Addr::from_str("2001:db8:aaaa::").unwrap();
        let expected2 = Ipv6Addr::from_str("2001:db8:aaaa:bbbb:cccc:dddd::").unwrap();
        let expected3 = Ipv6Addr::from_str("::").unwrap();
        let expected4 = Ipv6Addr::from_str("2000::").unwrap();
        assert_ne!(ip.mask(46), expected1);
        assert_eq!(ip.mask(47), expected1);
        assert_eq!(ip.mask(48), expected1);
        assert_ne!(ip.mask(49), expected1);
        assert_eq!(ip.mask(96), expected2);
        assert_eq!(ip.mask(0), expected3);
        assert_ne!(ip.mask(2), expected4);
        assert_eq!(ip.mask(3), expected4);
        assert_eq!(ip.mask(4), expected4);
        assert_eq!(ip.mask(15), expected4);
        assert_ne!(ip.mask(16), expected4);
        assert_eq!(ip.mask(128), ip);
    }

    #[test]
    fn address_ipv4_nibbles() {
        let ip = Ipv4Addr::from(0x12345678);
        assert_eq!(ip.nibbles(), [1, 2, 3, 4, 5, 6, 7, 8]);
    }

    #[test]
    fn address_ipv6_nibbles() {
        let ip = Ipv6Addr::from_str("2001:db8:aaaa:bbbb:cccc:dddd:eeee:ffff").unwrap();
        assert_eq!(
            ip.nibbles(),
            [
                0x2, 0x0, 0x0, 0x1, 0x0, 0xd, 0xb, 0x8, 0xa, 0xa, 0xa, 0xa, 0xb, 0xb, 0xb, 0xb,
                0xc, 0xc, 0xc, 0xc, 0xd, 0xd, 0xd, 0xd, 0xe, 0xe, 0xe, 0xe, 0xf, 0xf, 0xf, 0xf,
            ]
        );
    }

    #[test]
    fn address_ipv4_from_nibbles() {
        let ip: Ipv4Addr = Address::from_nibbles(&[1, 2, 3, 4, 5, 6, 7, 8]);
        assert_eq!(ip.octets(), [0x12, 0x34, 0x56, 0x78]);
    }

    #[test]
    fn address_ipv4_from_nibbles_short() {
        let ip: Ipv4Addr = Address::from_nibbles(&[]);
        assert_eq!(ip.octets(), [0, 0, 0, 0]);
    }

    #[test]
    fn address_ipv4_from_nibbles_long() {
        let ip: Ipv4Addr = Address::from_nibbles(&[1, 2, 3, 4, 5, 6, 7, 8, 9, 10]);
        assert_eq!(ip.octets(), [0x12, 0x34, 0x56, 0x78]);
    }

    #[test]
    fn address_ipv6_from_nibbles() {
        let ip: Ipv6Addr = Address::from_nibbles(&[
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 15, 14, 13, 12, 11, 10, 9, 8, 7,
            6, 5, 4, 3, 2, 1, 0,
        ]);
        let expected = Ipv6Addr::new(
            0x123, 0x4567, 0x89ab, 0xcdef, 0xfedc, 0xba98, 0x7654, 0x3210,
        );
        assert_eq!(ip, expected);
    }
}
