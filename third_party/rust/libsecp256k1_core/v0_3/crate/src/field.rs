use core::{
    cmp::Ordering,
    ops::{Add, AddAssign, Mul, MulAssign},
};

macro_rules! debug_assert_bits {
    ($x: expr, $n: expr) => {
        debug_assert!($x >> $n == 0);
    };
}

#[derive(Debug, Clone, Copy)]
/// Field element for secp256k1.
pub struct Field {
    /// Store representation of X.
    /// X = sum(i=0..9, n[i]*2^(i*26)) mod p
    /// where p = 2^256 - 0x1000003D1
    ///
    /// The least signifiant byte is in the front.
    n: [u32; 10],
    magnitude: u32,
    normalized: bool,
}

impl Field {
    pub const fn new_raw(
        d9: u32,
        d8: u32,
        d7: u32,
        d6: u32,
        d5: u32,
        d4: u32,
        d3: u32,
        d2: u32,
        d1: u32,
        d0: u32,
    ) -> Self {
        Self {
            n: [d0, d1, d2, d3, d4, d5, d6, d7, d8, d9],
            magnitude: 1,
            normalized: false,
        }
    }

    pub const fn new(
        d7: u32,
        d6: u32,
        d5: u32,
        d4: u32,
        d3: u32,
        d2: u32,
        d1: u32,
        d0: u32,
    ) -> Self {
        Self {
            n: [
                d0 & 0x3ffffff,
                (d0 >> 26) | ((d1 & 0xfffff) << 6),
                (d1 >> 20) | ((d2 & 0x3fff) << 12),
                (d2 >> 14) | ((d3 & 0xff) << 18),
                (d3 >> 8) | ((d4 & 0x3) << 24),
                (d4 >> 2) & 0x3ffffff,
                (d4 >> 28) | ((d5 & 0x3fffff) << 4),
                (d5 >> 22) | ((d6 & 0xffff) << 10),
                (d6 >> 16) | ((d7 & 0x3ff) << 16),
                (d7 >> 10),
            ],
            magnitude: 1,
            normalized: true,
        }
    }

    pub fn from_int(a: u32) -> Field {
        let mut f = Field::default();
        f.set_int(a);
        f
    }

    fn verify(&self) -> bool {
        let m = if self.normalized { 1 } else { 2 } * self.magnitude;
        let mut r = true;
        r = r && (self.n[0] <= 0x3ffffff * m);
        r = r && (self.n[1] <= 0x3ffffff * m);
        r = r && (self.n[2] <= 0x3ffffff * m);
        r = r && (self.n[3] <= 0x3ffffff * m);
        r = r && (self.n[4] <= 0x3ffffff * m);
        r = r && (self.n[5] <= 0x3ffffff * m);
        r = r && (self.n[6] <= 0x3ffffff * m);
        r = r && (self.n[7] <= 0x3ffffff * m);
        r = r && (self.n[8] <= 0x3ffffff * m);
        r = r && (self.n[9] <= 0x03fffff * m);
        r = r && (self.magnitude <= 32);
        if self.normalized {
            r = r && self.magnitude <= 1;
            if r && (self.n[9] == 0x03fffff) {
                let mid = self.n[8]
                    & self.n[7]
                    & self.n[6]
                    & self.n[5]
                    & self.n[4]
                    & self.n[3]
                    & self.n[2];
                if mid == 0x3ffffff {
                    r = r && ((self.n[1] + 0x40 + ((self.n[0] + 0x3d1) >> 26)) <= 0x3ffffff)
                }
            }
        }
        r
    }

    /// Normalize a field element.
    pub fn normalize(&mut self) {
        let mut t0 = self.n[0];
        let mut t1 = self.n[1];
        let mut t2 = self.n[2];
        let mut t3 = self.n[3];
        let mut t4 = self.n[4];
        let mut t5 = self.n[5];
        let mut t6 = self.n[6];
        let mut t7 = self.n[7];
        let mut t8 = self.n[8];
        let mut t9 = self.n[9];

        let mut m: u32;
        let mut x = t9 >> 22;
        t9 &= 0x03fffff;

        t0 += x * 0x3d1;
        t1 += x << 6;
        t1 += t0 >> 26;
        t0 &= 0x3ffffff;
        t2 += t1 >> 26;
        t1 &= 0x3ffffff;
        t3 += t2 >> 26;
        t2 &= 0x3ffffff;
        m = t2;
        t4 += t3 >> 26;
        t3 &= 0x3ffffff;
        m &= t3;
        t5 += t4 >> 26;
        t4 &= 0x3ffffff;
        m &= t4;
        t6 += t5 >> 26;
        t5 &= 0x3ffffff;
        m &= t5;
        t7 += t6 >> 26;
        t6 &= 0x3ffffff;
        m &= t6;
        t8 += t7 >> 26;
        t7 &= 0x3ffffff;
        m &= t7;
        t9 += t8 >> 26;
        t8 &= 0x3ffffff;
        m &= t8;

        debug_assert!(t9 >> 23 == 0);

        x = (t9 >> 22)
            | (if t9 == 0x03fffff { 1 } else { 0 }
                & if m == 0x3ffffff { 1 } else { 0 }
                & (if (t1 + 0x40 + ((t0 + 0x3d1) >> 26)) > 0x3ffffff {
                    1
                } else {
                    0
                }));

        t0 += x * 0x3d1;
        t1 += x << 6;
        t1 += t0 >> 26;
        t0 &= 0x3ffffff;
        t2 += t1 >> 26;
        t1 &= 0x3ffffff;
        t3 += t2 >> 26;
        t2 &= 0x3ffffff;
        t4 += t3 >> 26;
        t3 &= 0x3ffffff;
        t5 += t4 >> 26;
        t4 &= 0x3ffffff;
        t6 += t5 >> 26;
        t5 &= 0x3ffffff;
        t7 += t6 >> 26;
        t6 &= 0x3ffffff;
        t8 += t7 >> 26;
        t7 &= 0x3ffffff;
        t9 += t8 >> 26;
        t8 &= 0x3ffffff;

        debug_assert!(t9 >> 22 == x);

        t9 &= 0x03fffff;

        self.n = [t0, t1, t2, t3, t4, t5, t6, t7, t8, t9];
        self.magnitude = 1;
        self.normalized = true;
        debug_assert!(self.verify());
    }

    /// Weakly normalize a field element: reduce it magnitude to 1,
    /// but don't fully normalize.
    pub fn normalize_weak(&mut self) {
        let mut t0 = self.n[0];
        let mut t1 = self.n[1];
        let mut t2 = self.n[2];
        let mut t3 = self.n[3];
        let mut t4 = self.n[4];
        let mut t5 = self.n[5];
        let mut t6 = self.n[6];
        let mut t7 = self.n[7];
        let mut t8 = self.n[8];
        let mut t9 = self.n[9];

        let x = t9 >> 22;
        t9 &= 0x03fffff;

        t0 += x * 0x3d1;
        t1 += x << 6;
        t1 += t0 >> 26;
        t0 &= 0x3ffffff;
        t2 += t1 >> 26;
        t1 &= 0x3ffffff;
        t3 += t2 >> 26;
        t2 &= 0x3ffffff;
        t4 += t3 >> 26;
        t3 &= 0x3ffffff;
        t5 += t4 >> 26;
        t4 &= 0x3ffffff;
        t6 += t5 >> 26;
        t5 &= 0x3ffffff;
        t7 += t6 >> 26;
        t6 &= 0x3ffffff;
        t8 += t7 >> 26;
        t7 &= 0x3ffffff;
        t9 += t8 >> 26;
        t8 &= 0x3ffffff;

        debug_assert!(t9 >> 23 == 0);

        self.n = [t0, t1, t2, t3, t4, t5, t6, t7, t8, t9];
        self.magnitude = 1;
        debug_assert!(self.verify());
    }

    /// Normalize a field element, without constant-time guarantee.
    pub fn normalize_var(&mut self) {
        let mut t0 = self.n[0];
        let mut t1 = self.n[1];
        let mut t2 = self.n[2];
        let mut t3 = self.n[3];
        let mut t4 = self.n[4];
        let mut t5 = self.n[5];
        let mut t6 = self.n[6];
        let mut t7 = self.n[7];
        let mut t8 = self.n[8];
        let mut t9 = self.n[9];

        let mut m: u32;
        let mut x = t9 >> 22;
        t9 &= 0x03fffff;

        t0 += x * 0x3d1;
        t1 += x << 6;
        t1 += t0 >> 26;
        t0 &= 0x3ffffff;
        t2 += t1 >> 26;
        t1 &= 0x3ffffff;
        t3 += t2 >> 26;
        t2 &= 0x3ffffff;
        m = t2;
        t4 += t3 >> 26;
        t3 &= 0x3ffffff;
        m &= t3;
        t5 += t4 >> 26;
        t4 &= 0x3ffffff;
        m &= t4;
        t6 += t5 >> 26;
        t5 &= 0x3ffffff;
        m &= t5;
        t7 += t6 >> 26;
        t6 &= 0x3ffffff;
        m &= t6;
        t8 += t7 >> 26;
        t7 &= 0x3ffffff;
        m &= t7;
        t9 += t8 >> 26;
        t8 &= 0x3ffffff;
        m &= t8;

        debug_assert!(t9 >> 23 == 0);

        x = (t9 >> 22)
            | (if t9 == 0x03fffff { 1 } else { 0 }
                & if m == 0x3ffffff { 1 } else { 0 }
                & (if (t1 + 0x40 + ((t0 + 0x3d1) >> 26)) > 0x3ffffff {
                    1
                } else {
                    0
                }));

        if x > 0 {
            t0 += 0x3d1;
            t1 += x << 6;
            t1 += t0 >> 26;
            t0 &= 0x3ffffff;
            t2 += t1 >> 26;
            t1 &= 0x3ffffff;
            t3 += t2 >> 26;
            t2 &= 0x3ffffff;
            t4 += t3 >> 26;
            t3 &= 0x3ffffff;
            t5 += t4 >> 26;
            t4 &= 0x3ffffff;
            t6 += t5 >> 26;
            t5 &= 0x3ffffff;
            t7 += t6 >> 26;
            t6 &= 0x3ffffff;
            t8 += t7 >> 26;
            t7 &= 0x3ffffff;
            t9 += t8 >> 26;
            t8 &= 0x3ffffff;

            debug_assert!(t9 >> 22 == x);

            t9 &= 0x03fffff;
        }

        self.n = [t0, t1, t2, t3, t4, t5, t6, t7, t8, t9];
        self.magnitude = 1;
        self.normalized = true;
        debug_assert!(self.verify());
    }

    /// Verify whether a field element represents zero i.e. would
    /// normalize to a zero value. The field implementation may
    /// optionally normalize the input, but this should not be relied
    /// upon.
    pub fn normalizes_to_zero(&self) -> bool {
        let mut t0 = self.n[0];
        let mut t1 = self.n[1];
        let mut t2 = self.n[2];
        let mut t3 = self.n[3];
        let mut t4 = self.n[4];
        let mut t5 = self.n[5];
        let mut t6 = self.n[6];
        let mut t7 = self.n[7];
        let mut t8 = self.n[8];
        let mut t9 = self.n[9];

        let mut z0: u32;
        let mut z1: u32;

        let x = t9 >> 22;
        t9 &= 0x03fffff;

        t0 += x * 0x3d1;
        t1 += x << 6;
        t1 += t0 >> 26;
        t0 &= 0x3ffffff;
        z0 = t0;
        z1 = t0 ^ 0x3d0;
        t2 += t1 >> 26;
        t1 &= 0x3ffffff;
        z0 |= t1;
        z1 &= t1 ^ 0x40;
        t3 += t2 >> 26;
        t2 &= 0x3ffffff;
        z0 |= t2;
        z1 &= t2;
        t4 += t3 >> 26;
        t3 &= 0x3ffffff;
        z0 |= t3;
        z1 &= t3;
        t5 += t4 >> 26;
        t4 &= 0x3ffffff;
        z0 |= t4;
        z1 &= t4;
        t6 += t5 >> 26;
        t5 &= 0x3ffffff;
        z0 |= t5;
        z1 &= t5;
        t7 += t6 >> 26;
        t6 &= 0x3ffffff;
        z0 |= t6;
        z1 &= t6;
        t8 += t7 >> 26;
        t7 &= 0x3ffffff;
        z0 |= t7;
        z1 &= t7;
        t9 += t8 >> 26;
        t8 &= 0x3ffffff;
        z0 |= t8;
        z1 &= t8;
        z0 |= t9;
        z1 &= t9 ^ 0x3c00000;

        debug_assert!(t9 >> 23 == 0);

        z0 == 0 || z1 == 0x3ffffff
    }

    /// Verify whether a field element represents zero i.e. would
    /// normalize to a zero value. The field implementation may
    /// optionally normalize the input, but this should not be relied
    /// upon.
    pub fn normalizes_to_zero_var(&self) -> bool {
        let mut t0: u32;
        let mut t1: u32;
        let mut t2: u32;
        let mut t3: u32;
        let mut t4: u32;
        let mut t5: u32;
        let mut t6: u32;
        let mut t7: u32;
        let mut t8: u32;
        let mut t9: u32;
        let mut z0: u32;
        let mut z1: u32;
        let x: u32;

        t0 = self.n[0];
        t9 = self.n[9];

        x = t9 >> 22;
        t0 += x * 0x3d1;

        z0 = t0 & 0x3ffffff;
        z1 = z0 ^ 0x3d0;

        if z0 != 0 && z1 != 0x3ffffff {
            return false;
        }

        t1 = self.n[1];
        t2 = self.n[2];
        t3 = self.n[3];
        t4 = self.n[4];
        t5 = self.n[5];
        t6 = self.n[6];
        t7 = self.n[7];
        t8 = self.n[8];

        t9 &= 0x03fffff;
        t1 += x << 6;

        t1 += t0 >> 26;
        t2 += t1 >> 26;
        t1 &= 0x3ffffff;
        z0 |= t1;
        z1 &= t1 ^ 0x40;
        t3 += t2 >> 26;
        t2 &= 0x3ffffff;
        z0 |= t2;
        z1 &= t2;
        t4 += t3 >> 26;
        t3 &= 0x3ffffff;
        z0 |= t3;
        z1 &= t3;
        t5 += t4 >> 26;
        t4 &= 0x3ffffff;
        z0 |= t4;
        z1 &= t4;
        t6 += t5 >> 26;
        t5 &= 0x3ffffff;
        z0 |= t5;
        z1 &= t5;
        t7 += t6 >> 26;
        t6 &= 0x3ffffff;
        z0 |= t6;
        z1 &= t6;
        t8 += t7 >> 26;
        t7 &= 0x3ffffff;
        z0 |= t7;
        z1 &= t7;
        t9 += t8 >> 26;
        t8 &= 0x3ffffff;
        z0 |= t8;
        z1 &= t8;
        z0 |= t9;
        z1 &= t9 ^ 0x3c00000;

        debug_assert!(t9 >> 23 == 0);

        z0 == 0 || z1 == 0x3ffffff
    }

    /// Set a field element equal to a small integer. Resulting field
    /// element is normalized.
    pub fn set_int(&mut self, a: u32) {
        self.n = [a, 0, 0, 0, 0, 0, 0, 0, 0, 0];
        self.magnitude = 1;
        self.normalized = true;
        debug_assert!(self.verify());
    }

    /// Verify whether a field element is zero. Requires the input to
    /// be normalized.
    pub fn is_zero(&self) -> bool {
        debug_assert!(self.normalized);
        debug_assert!(self.verify());
        (self.n[0]
            | self.n[1]
            | self.n[2]
            | self.n[3]
            | self.n[4]
            | self.n[5]
            | self.n[6]
            | self.n[7]
            | self.n[8]
            | self.n[9])
            == 0
    }

    /// Check the "oddness" of a field element. Requires the input to
    /// be normalized.
    pub fn is_odd(&self) -> bool {
        debug_assert!(self.normalized);
        debug_assert!(self.verify());
        self.n[0] & 1 != 0
    }

    /// Sets a field element equal to zero, initializing all fields.
    pub fn clear(&mut self) {
        self.magnitude = 0;
        self.normalized = true;
        self.n = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    }

    /// Set a field element equal to 32-byte big endian value. If
    /// successful, the resulting field element is normalized.
    #[must_use]
    pub fn set_b32(&mut self, a: &[u8; 32]) -> bool {
        self.n[0] = (a[31] as u32)
            | ((a[30] as u32) << 8)
            | ((a[29] as u32) << 16)
            | (((a[28] & 0x3) as u32) << 24);
        self.n[1] = (((a[28] >> 2) & 0x3f) as u32)
            | ((a[27] as u32) << 6)
            | ((a[26] as u32) << 14)
            | (((a[25] & 0xf) as u32) << 22);
        self.n[2] = (((a[25] >> 4) & 0xf) as u32)
            | ((a[24] as u32) << 4)
            | ((a[23] as u32) << 12)
            | (((a[22] as u32) & 0x3f) << 20);
        self.n[3] = (((a[22] >> 6) & 0x3) as u32)
            | ((a[21] as u32) << 2)
            | ((a[20] as u32) << 10)
            | ((a[19] as u32) << 18);
        self.n[4] = (a[18] as u32)
            | ((a[17] as u32) << 8)
            | ((a[16] as u32) << 16)
            | (((a[15] & 0x3) as u32) << 24);
        self.n[5] = (((a[15] >> 2) & 0x3f) as u32)
            | ((a[14] as u32) << 6)
            | ((a[13] as u32) << 14)
            | (((a[12] as u32) & 0xf) << 22);
        self.n[6] = (((a[12] >> 4) & 0xf) as u32)
            | ((a[11] as u32) << 4)
            | ((a[10] as u32) << 12)
            | (((a[9] & 0x3f) as u32) << 20);
        self.n[7] = (((a[9] >> 6) & 0x3) as u32)
            | ((a[8] as u32) << 2)
            | ((a[7] as u32) << 10)
            | ((a[6] as u32) << 18);
        self.n[8] = (a[5] as u32)
            | ((a[4] as u32) << 8)
            | ((a[3] as u32) << 16)
            | (((a[2] & 0x3) as u32) << 24);
        self.n[9] = (((a[2] >> 2) & 0x3f) as u32) | ((a[1] as u32) << 6) | ((a[0] as u32) << 14);

        if self.n[9] == 0x03fffff
            && (self.n[8] & self.n[7] & self.n[6] & self.n[5] & self.n[4] & self.n[3] & self.n[2])
                == 0x3ffffff
            && (self.n[1] + 0x40 + ((self.n[0] + 0x3d1) >> 26)) > 0x3ffffff
        {
            return false;
        }

        self.magnitude = 1;
        self.normalized = true;
        debug_assert!(self.verify());

        true
    }

    pub fn fill_b32(&self, r: &mut [u8; 32]) {
        debug_assert!(self.normalized);
        debug_assert!(self.verify());

        r[0] = ((self.n[9] >> 14) & 0xff) as u8;
        r[1] = ((self.n[9] >> 6) & 0xff) as u8;
        r[2] = (((self.n[9] & 0x3f) << 2) | ((self.n[8] >> 24) & 0x3)) as u8;
        r[3] = ((self.n[8] >> 16) & 0xff) as u8;
        r[4] = ((self.n[8] >> 8) & 0xff) as u8;
        r[5] = (self.n[8] & 0xff) as u8;
        r[6] = ((self.n[7] >> 18) & 0xff) as u8;
        r[7] = ((self.n[7] >> 10) & 0xff) as u8;
        r[8] = ((self.n[7] >> 2) & 0xff) as u8;
        r[9] = (((self.n[7] & 0x3) << 6) | ((self.n[6] >> 20) & 0x3f)) as u8;
        r[10] = ((self.n[6] >> 12) & 0xff) as u8;
        r[11] = ((self.n[6] >> 4) & 0xff) as u8;
        r[12] = (((self.n[6] & 0xf) << 4) | ((self.n[5] >> 22) & 0xf)) as u8;
        r[13] = ((self.n[5] >> 14) & 0xff) as u8;
        r[14] = ((self.n[5] >> 6) & 0xff) as u8;
        r[15] = (((self.n[5] & 0x3f) << 2) | ((self.n[4] >> 24) & 0x3)) as u8;
        r[16] = ((self.n[4] >> 16) & 0xff) as u8;
        r[17] = ((self.n[4] >> 8) & 0xff) as u8;
        r[18] = (self.n[4] & 0xff) as u8;
        r[19] = ((self.n[3] >> 18) & 0xff) as u8;
        r[20] = ((self.n[3] >> 10) & 0xff) as u8;
        r[21] = ((self.n[3] >> 2) & 0xff) as u8;
        r[22] = (((self.n[3] & 0x3) << 6) | ((self.n[2] >> 20) & 0x3f)) as u8;
        r[23] = ((self.n[2] >> 12) & 0xff) as u8;
        r[24] = ((self.n[2] >> 4) & 0xff) as u8;
        r[25] = (((self.n[2] & 0xf) << 4) | ((self.n[1] >> 22) & 0xf)) as u8;
        r[26] = ((self.n[1] >> 14) & 0xff) as u8;
        r[27] = ((self.n[1] >> 6) & 0xff) as u8;
        r[28] = (((self.n[1] & 0x3f) << 2) | ((self.n[0] >> 24) & 0x3)) as u8;
        r[29] = ((self.n[0] >> 16) & 0xff) as u8;
        r[30] = ((self.n[0] >> 8) & 0xff) as u8;
        r[31] = (self.n[0] & 0xff) as u8;
    }

    /// Convert a field element to a 32-byte big endian
    /// value. Requires the input to be normalized.
    pub fn b32(&self) -> [u8; 32] {
        let mut r = [0u8; 32];
        self.fill_b32(&mut r);
        r
    }

    /// Set a field element equal to the additive inverse of
    /// another. Takes a maximum magnitude of the input as an
    /// argument. The magnitude of the output is one higher.
    pub fn neg_in_place(&mut self, other: &Field, m: u32) {
        debug_assert!(other.magnitude <= m);
        debug_assert!(other.verify());

        self.n[0] = 0x3fffc2f * 2 * (m + 1) - other.n[0];
        self.n[1] = 0x3ffffbf * 2 * (m + 1) - other.n[1];
        self.n[2] = 0x3ffffff * 2 * (m + 1) - other.n[2];
        self.n[3] = 0x3ffffff * 2 * (m + 1) - other.n[3];
        self.n[4] = 0x3ffffff * 2 * (m + 1) - other.n[4];
        self.n[5] = 0x3ffffff * 2 * (m + 1) - other.n[5];
        self.n[6] = 0x3ffffff * 2 * (m + 1) - other.n[6];
        self.n[7] = 0x3ffffff * 2 * (m + 1) - other.n[7];
        self.n[8] = 0x3ffffff * 2 * (m + 1) - other.n[8];
        self.n[9] = 0x03fffff * 2 * (m + 1) - other.n[9];

        self.magnitude = m + 1;
        self.normalized = false;
        debug_assert!(self.verify());
    }

    /// Compute the additive inverse of this element. Takes the maximum
    /// expected magnitude of this element as an argument.
    pub fn neg(&self, m: u32) -> Field {
        let mut ret = Field::default();
        ret.neg_in_place(self, m);
        ret
    }

    /// Multiplies the passed field element with a small integer
    /// constant. Multiplies the magnitude by that small integer.
    pub fn mul_int(&mut self, a: u32) {
        self.n[0] *= a;
        self.n[1] *= a;
        self.n[2] *= a;
        self.n[3] *= a;
        self.n[4] *= a;
        self.n[5] *= a;
        self.n[6] *= a;
        self.n[7] *= a;
        self.n[8] *= a;
        self.n[9] *= a;

        self.magnitude *= a;
        self.normalized = false;
        debug_assert!(self.verify());
    }

    /// Compare two field elements. Requires both inputs to be
    /// normalized.
    pub fn cmp_var(&self, other: &Field) -> Ordering {
        // Variable time compare implementation.
        debug_assert!(self.normalized);
        debug_assert!(other.normalized);
        debug_assert!(self.verify());
        debug_assert!(other.verify());

        for i in (0..10).rev() {
            if self.n[i] > other.n[i] {
                return Ordering::Greater;
            }
            if self.n[i] < other.n[i] {
                return Ordering::Less;
            }
        }
        Ordering::Equal
    }

    pub fn eq_var(&self, other: &Field) -> bool {
        let mut na = self.neg(1);
        na += other;
        na.normalizes_to_zero_var()
    }

    fn mul_inner(&mut self, a: &Field, b: &Field) {
        const M: u64 = 0x3ffffff;
        const R0: u64 = 0x3d10;
        const R1: u64 = 0x400;

        let (mut c, mut d): (u64, u64);
        let (v0, v1, v2, v3, v4, v5, v6, v7, v8): (u64, u64, u64, u64, u64, u64, u64, u64, u64);
        let (t9, t1, t0, t2, t3, t4, t5, t6, t7): (u32, u32, u32, u32, u32, u32, u32, u32, u32);

        debug_assert_bits!(a.n[0], 30);
        debug_assert_bits!(a.n[1], 30);
        debug_assert_bits!(a.n[2], 30);
        debug_assert_bits!(a.n[3], 30);
        debug_assert_bits!(a.n[4], 30);
        debug_assert_bits!(a.n[5], 30);
        debug_assert_bits!(a.n[6], 30);
        debug_assert_bits!(a.n[7], 30);
        debug_assert_bits!(a.n[8], 30);
        debug_assert_bits!(a.n[9], 26);
        debug_assert_bits!(b.n[0], 30);
        debug_assert_bits!(b.n[1], 30);
        debug_assert_bits!(b.n[2], 30);
        debug_assert_bits!(b.n[3], 30);
        debug_assert_bits!(b.n[4], 30);
        debug_assert_bits!(b.n[5], 30);
        debug_assert_bits!(b.n[6], 30);
        debug_assert_bits!(b.n[7], 30);
        debug_assert_bits!(b.n[8], 30);
        debug_assert_bits!(b.n[9], 26);

        // [... a b c] is a shorthand for ... + a<<52 + b<<26 + c<<0 mod n.
        // px is a shorthand for sum(a[i]*b[x-i], i=0..x).
        // Note that [x 0 0 0 0 0 0 0 0 0 0] = [x*R1 x*R0].

        d = ((a.n[0] as u64) * (b.n[9] as u64))
            .wrapping_add((a.n[1] as u64) * (b.n[8] as u64))
            .wrapping_add((a.n[2] as u64) * (b.n[7] as u64))
            .wrapping_add((a.n[3] as u64) * (b.n[6] as u64))
            .wrapping_add((a.n[4] as u64) * (b.n[5] as u64))
            .wrapping_add((a.n[5] as u64) * (b.n[4] as u64))
            .wrapping_add((a.n[6] as u64) * (b.n[3] as u64))
            .wrapping_add((a.n[7] as u64) * (b.n[2] as u64))
            .wrapping_add((a.n[8] as u64) * (b.n[1] as u64))
            .wrapping_add((a.n[9] as u64) * (b.n[0] as u64));
        // debug_assert_bits!(d, 64);

        /* [d 0 0 0 0 0 0 0 0 0] = [p9 0 0 0 0 0 0 0 0 0] */
        t9 = (d & M) as u32;
        d >>= 26;
        debug_assert_bits!(t9, 26);
        debug_assert_bits!(d, 38);
        /* [d t9 0 0 0 0 0 0 0 0 0] = [p9 0 0 0 0 0 0 0 0 0] */

        c = (a.n[0] as u64) * (b.n[0] as u64);
        debug_assert_bits!(c, 60);
        /* [d t9 0 0 0 0 0 0 0 0 c] = [p9 0 0 0 0 0 0 0 0 p0] */

        d = d
            .wrapping_add((a.n[1] as u64) * (b.n[9] as u64))
            .wrapping_add((a.n[2] as u64) * (b.n[8] as u64))
            .wrapping_add((a.n[3] as u64) * (b.n[7] as u64))
            .wrapping_add((a.n[4] as u64) * (b.n[6] as u64))
            .wrapping_add((a.n[5] as u64) * (b.n[5] as u64))
            .wrapping_add((a.n[6] as u64) * (b.n[4] as u64))
            .wrapping_add((a.n[7] as u64) * (b.n[3] as u64))
            .wrapping_add((a.n[8] as u64) * (b.n[2] as u64))
            .wrapping_add((a.n[9] as u64) * (b.n[1] as u64));
        debug_assert_bits!(d, 63);
        /* [d t9 0 0 0 0 0 0 0 0 c] = [p10 p9 0 0 0 0 0 0 0 0 p0] */
        v0 = d & M;
        d >>= 26;
        c += v0 * R0;
        debug_assert_bits!(v0, 26);
        debug_assert_bits!(d, 37);
        debug_assert_bits!(c, 61);
        /* [d u0 t9 0 0 0 0 0 0 0 0 c-u0*R0] = [p10 p9 0 0 0 0 0 0 0 0 p0] */
        t0 = (c & M) as u32;
        c >>= 26;
        c += v0 * R1;

        debug_assert_bits!(t0, 26);
        debug_assert_bits!(c, 37);
        /* [d u0 t9 0 0 0 0 0 0 0 c-u0*R1 t0-u0*R0] = [p10 p9 0 0 0 0 0 0 0 0 p0] */
        /* [d 0 t9 0 0 0 0 0 0 0 c t0] = [p10 p9 0 0 0 0 0 0 0 0 p0] */

        c = c
            .wrapping_add((a.n[0] as u64) * (b.n[1] as u64))
            .wrapping_add((a.n[1] as u64) * (b.n[0] as u64));
        debug_assert_bits!(c, 62);
        /* [d 0 t9 0 0 0 0 0 0 0 c t0] = [p10 p9 0 0 0 0 0 0 0 p1 p0] */
        d = d
            .wrapping_add((a.n[2] as u64) * (b.n[9] as u64))
            .wrapping_add((a.n[3] as u64) * (b.n[8] as u64))
            .wrapping_add((a.n[4] as u64) * (b.n[7] as u64))
            .wrapping_add((a.n[5] as u64) * (b.n[6] as u64))
            .wrapping_add((a.n[6] as u64) * (b.n[5] as u64))
            .wrapping_add((a.n[7] as u64) * (b.n[4] as u64))
            .wrapping_add((a.n[8] as u64) * (b.n[3] as u64))
            .wrapping_add((a.n[9] as u64) * (b.n[2] as u64));
        debug_assert_bits!(d, 63);
        /* [d 0 t9 0 0 0 0 0 0 0 c t0] = [p11 p10 p9 0 0 0 0 0 0 0 p1 p0] */
        v1 = d & M;
        d >>= 26;
        c += v1 * R0;
        debug_assert_bits!(v1, 26);
        debug_assert_bits!(d, 37);
        debug_assert_bits!(c, 63);
        /* [d u1 0 t9 0 0 0 0 0 0 0 c-u1*R0 t0] = [p11 p10 p9 0 0 0 0 0 0 0 p1 p0] */
        t1 = (c & M) as u32;
        c >>= 26;
        c += v1 * R1;
        debug_assert_bits!(t1, 26);
        debug_assert_bits!(c, 38);
        /* [d u1 0 t9 0 0 0 0 0 0 c-u1*R1 t1-u1*R0 t0] = [p11 p10 p9 0 0 0 0 0 0 0 p1 p0] */
        /* [d 0 0 t9 0 0 0 0 0 0 c t1 t0] = [p11 p10 p9 0 0 0 0 0 0 0 p1 p0] */

        c = c
            .wrapping_add((a.n[0] as u64) * (b.n[2] as u64))
            .wrapping_add((a.n[1] as u64) * (b.n[1] as u64))
            .wrapping_add((a.n[2] as u64) * (b.n[0] as u64));
        debug_assert_bits!(c, 62);
        /* [d 0 0 t9 0 0 0 0 0 0 c t1 t0] = [p11 p10 p9 0 0 0 0 0 0 p2 p1 p0] */
        d = d
            .wrapping_add((a.n[3] as u64) * (b.n[9] as u64))
            .wrapping_add((a.n[4] as u64) * (b.n[8] as u64))
            .wrapping_add((a.n[5] as u64) * (b.n[7] as u64))
            .wrapping_add((a.n[6] as u64) * (b.n[6] as u64))
            .wrapping_add((a.n[7] as u64) * (b.n[5] as u64))
            .wrapping_add((a.n[8] as u64) * (b.n[4] as u64))
            .wrapping_add((a.n[9] as u64) * (b.n[3] as u64));
        debug_assert_bits!(d, 63);
        /* [d 0 0 t9 0 0 0 0 0 0 c t1 t0] = [p12 p11 p10 p9 0 0 0 0 0 0 p2 p1 p0] */
        v2 = d & M;
        d >>= 26;
        c += v2 * R0;
        debug_assert_bits!(v2, 26);
        debug_assert_bits!(d, 37);
        debug_assert_bits!(c, 63);
        /* [d u2 0 0 t9 0 0 0 0 0 0 c-u2*R0 t1 t0] = [p12 p11 p10 p9 0 0 0 0 0 0 p2 p1 p0] */
        t2 = (c & M) as u32;
        c >>= 26;
        c += v2 * R1;
        debug_assert_bits!(t2, 26);
        debug_assert_bits!(c, 38);
        /* [d u2 0 0 t9 0 0 0 0 0 c-u2*R1 t2-u2*R0 t1 t0] = [p12 p11 p10 p9 0 0 0 0 0 0 p2 p1 p0] */
        /* [d 0 0 0 t9 0 0 0 0 0 c t2 t1 t0] = [p12 p11 p10 p9 0 0 0 0 0 0 p2 p1 p0] */

        c = c
            .wrapping_add((a.n[0] as u64) * (b.n[3] as u64))
            .wrapping_add((a.n[1] as u64) * (b.n[2] as u64))
            .wrapping_add((a.n[2] as u64) * (b.n[1] as u64))
            .wrapping_add((a.n[3] as u64) * (b.n[0] as u64));
        debug_assert_bits!(c, 63);
        /* [d 0 0 0 t9 0 0 0 0 0 c t2 t1 t0] = [p12 p11 p10 p9 0 0 0 0 0 p3 p2 p1 p0] */
        d = d
            .wrapping_add((a.n[4] as u64) * (b.n[9] as u64))
            .wrapping_add((a.n[5] as u64) * (b.n[8] as u64))
            .wrapping_add((a.n[6] as u64) * (b.n[7] as u64))
            .wrapping_add((a.n[7] as u64) * (b.n[6] as u64))
            .wrapping_add((a.n[8] as u64) * (b.n[5] as u64))
            .wrapping_add((a.n[9] as u64) * (b.n[4] as u64));
        debug_assert_bits!(d, 63);
        /* [d 0 0 0 t9 0 0 0 0 0 c t2 t1 t0] = [p13 p12 p11 p10 p9 0 0 0 0 0 p3 p2 p1 p0] */
        v3 = d & M;
        d >>= 26;
        c += v3 * R0;
        debug_assert_bits!(v3, 26);
        debug_assert_bits!(d, 37);
        // debug_assert_bits!(c, 64);
        /* [d u3 0 0 0 t9 0 0 0 0 0 c-u3*R0 t2 t1 t0] = [p13 p12 p11 p10 p9 0 0 0 0 0 p3 p2 p1 p0] */
        t3 = (c & M) as u32;
        c >>= 26;
        c += v3 * R1;
        debug_assert_bits!(t3, 26);
        debug_assert_bits!(c, 39);
        /* [d u3 0 0 0 t9 0 0 0 0 c-u3*R1 t3-u3*R0 t2 t1 t0] = [p13 p12 p11 p10 p9 0 0 0 0 0 p3 p2 p1 p0] */
        /* [d 0 0 0 0 t9 0 0 0 0 c t3 t2 t1 t0] = [p13 p12 p11 p10 p9 0 0 0 0 0 p3 p2 p1 p0] */

        c = c
            .wrapping_add((a.n[0] as u64) * (b.n[4] as u64))
            .wrapping_add((a.n[1] as u64) * (b.n[3] as u64))
            .wrapping_add((a.n[2] as u64) * (b.n[2] as u64))
            .wrapping_add((a.n[3] as u64) * (b.n[1] as u64))
            .wrapping_add((a.n[4] as u64) * (b.n[0] as u64));
        debug_assert_bits!(c, 63);
        /* [d 0 0 0 0 t9 0 0 0 0 c t3 t2 t1 t0] = [p13 p12 p11 p10 p9 0 0 0 0 p4 p3 p2 p1 p0] */
        d = d
            .wrapping_add((a.n[5] as u64) * (b.n[9] as u64))
            .wrapping_add((a.n[6] as u64) * (b.n[8] as u64))
            .wrapping_add((a.n[7] as u64) * (b.n[7] as u64))
            .wrapping_add((a.n[8] as u64) * (b.n[6] as u64))
            .wrapping_add((a.n[9] as u64) * (b.n[5] as u64));
        debug_assert_bits!(d, 62);
        /* [d 0 0 0 0 t9 0 0 0 0 c t3 t2 t1 t0] = [p14 p13 p12 p11 p10 p9 0 0 0 0 p4 p3 p2 p1 p0] */
        v4 = d & M;
        d >>= 26;
        c += v4 * R0;
        debug_assert_bits!(v4, 26);
        debug_assert_bits!(d, 36);
        // debug_assert_bits!(c, 64);
        /* [d u4 0 0 0 0 t9 0 0 0 0 c-u4*R0 t3 t2 t1 t0] = [p14 p13 p12 p11 p10 p9 0 0 0 0 p4 p3 p2 p1 p0] */
        t4 = (c & M) as u32;
        c >>= 26;
        c += v4 * R1;
        debug_assert_bits!(t4, 26);
        debug_assert_bits!(c, 39);
        /* [d u4 0 0 0 0 t9 0 0 0 c-u4*R1 t4-u4*R0 t3 t2 t1 t0] = [p14 p13 p12 p11 p10 p9 0 0 0 0 p4 p3 p2 p1 p0] */
        /* [d 0 0 0 0 0 t9 0 0 0 c t4 t3 t2 t1 t0] = [p14 p13 p12 p11 p10 p9 0 0 0 0 p4 p3 p2 p1 p0] */

        c = c
            .wrapping_add((a.n[0] as u64) * (b.n[5] as u64))
            .wrapping_add((a.n[1] as u64) * (b.n[4] as u64))
            .wrapping_add((a.n[2] as u64) * (b.n[3] as u64))
            .wrapping_add((a.n[3] as u64) * (b.n[2] as u64))
            .wrapping_add((a.n[4] as u64) * (b.n[1] as u64))
            .wrapping_add((a.n[5] as u64) * (b.n[0] as u64));
        debug_assert_bits!(c, 63);
        /* [d 0 0 0 0 0 t9 0 0 0 c t4 t3 t2 t1 t0] = [p14 p13 p12 p11 p10 p9 0 0 0 p5 p4 p3 p2 p1 p0] */
        d = d
            .wrapping_add((a.n[6] as u64) * (b.n[9] as u64))
            .wrapping_add((a.n[7] as u64) * (b.n[8] as u64))
            .wrapping_add((a.n[8] as u64) * (b.n[7] as u64))
            .wrapping_add((a.n[9] as u64) * (b.n[6] as u64));
        debug_assert_bits!(d, 62);
        /* [d 0 0 0 0 0 t9 0 0 0 c t4 t3 t2 t1 t0] = [p15 p14 p13 p12 p11 p10 p9 0 0 0 p5 p4 p3 p2 p1 p0] */
        v5 = d & M;
        d >>= 26;
        c += v5 * R0;
        debug_assert_bits!(v5, 26);
        debug_assert_bits!(d, 36);
        // debug_assert_bits!(c, 64);
        /* [d u5 0 0 0 0 0 t9 0 0 0 c-u5*R0 t4 t3 t2 t1 t0] = [p15 p14 p13 p12 p11 p10 p9 0 0 0 p5 p4 p3 p2 p1 p0] */
        t5 = (c & M) as u32;
        c >>= 26;
        c += v5 * R1;
        debug_assert_bits!(t5, 26);
        debug_assert_bits!(c, 39);
        /* [d u5 0 0 0 0 0 t9 0 0 c-u5*R1 t5-u5*R0 t4 t3 t2 t1 t0] = [p15 p14 p13 p12 p11 p10 p9 0 0 0 p5 p4 p3 p2 p1 p0] */
        /* [d 0 0 0 0 0 0 t9 0 0 c t5 t4 t3 t2 t1 t0] = [p15 p14 p13 p12 p11 p10 p9 0 0 0 p5 p4 p3 p2 p1 p0] */

        c = c
            .wrapping_add((a.n[0] as u64) * (b.n[6] as u64))
            .wrapping_add((a.n[1] as u64) * (b.n[5] as u64))
            .wrapping_add((a.n[2] as u64) * (b.n[4] as u64))
            .wrapping_add((a.n[3] as u64) * (b.n[3] as u64))
            .wrapping_add((a.n[4] as u64) * (b.n[2] as u64))
            .wrapping_add((a.n[5] as u64) * (b.n[1] as u64))
            .wrapping_add((a.n[6] as u64) * (b.n[0] as u64));
        debug_assert_bits!(c, 63);
        /* [d 0 0 0 0 0 0 t9 0 0 c t5 t4 t3 t2 t1 t0] = [p15 p14 p13 p12 p11 p10 p9 0 0 p6 p5 p4 p3 p2 p1 p0] */
        d = d
            .wrapping_add((a.n[7] as u64) * (b.n[9] as u64))
            .wrapping_add((a.n[8] as u64) * (b.n[8] as u64))
            .wrapping_add((a.n[9] as u64) * (b.n[7] as u64));
        debug_assert_bits!(d, 61);
        /* [d 0 0 0 0 0 0 t9 0 0 c t5 t4 t3 t2 t1 t0] = [p16 p15 p14 p13 p12 p11 p10 p9 0 0 p6 p5 p4 p3 p2 p1 p0] */
        v6 = d & M;
        d >>= 26;
        c += v6 * R0;
        debug_assert_bits!(v6, 26);
        debug_assert_bits!(d, 35);
        // debug_assert_bits!(c, 64);
        /* [d u6 0 0 0 0 0 0 t9 0 0 c-u6*R0 t5 t4 t3 t2 t1 t0] = [p16 p15 p14 p13 p12 p11 p10 p9 0 0 p6 p5 p4 p3 p2 p1 p0] */
        t6 = (c & M) as u32;
        c >>= 26;
        c += v6 * R1;
        debug_assert_bits!(t6, 26);
        debug_assert_bits!(c, 39);
        /* [d u6 0 0 0 0 0 0 t9 0 c-u6*R1 t6-u6*R0 t5 t4 t3 t2 t1 t0] = [p16 p15 p14 p13 p12 p11 p10 p9 0 0 p6 p5 p4 p3 p2 p1 p0] */
        /* [d 0 0 0 0 0 0 0 t9 0 c t6 t5 t4 t3 t2 t1 t0] = [p16 p15 p14 p13 p12 p11 p10 p9 0 0 p6 p5 p4 p3 p2 p1 p0] */

        c = c
            .wrapping_add((a.n[0] as u64) * (b.n[7] as u64))
            .wrapping_add((a.n[1] as u64) * (b.n[6] as u64))
            .wrapping_add((a.n[2] as u64) * (b.n[5] as u64))
            .wrapping_add((a.n[3] as u64) * (b.n[4] as u64))
            .wrapping_add((a.n[4] as u64) * (b.n[3] as u64))
            .wrapping_add((a.n[5] as u64) * (b.n[2] as u64))
            .wrapping_add((a.n[6] as u64) * (b.n[1] as u64))
            .wrapping_add((a.n[7] as u64) * (b.n[0] as u64));
        // debug_assert_bits!(c, 64);
        debug_assert!(c <= 0x8000007c00000007);
        /* [d 0 0 0 0 0 0 0 t9 0 c t6 t5 t4 t3 t2 t1 t0] = [p16 p15 p14 p13 p12 p11 p10 p9 0 p7 p6 p5 p4 p3 p2 p1 p0] */
        d = d
            .wrapping_add((a.n[8] as u64) * (b.n[9] as u64))
            .wrapping_add((a.n[9] as u64) * (b.n[8] as u64));
        debug_assert_bits!(d, 58);
        /* [d 0 0 0 0 0 0 0 t9 0 c t6 t5 t4 t3 t2 t1 t0] = [p17 p16 p15 p14 p13 p12 p11 p10 p9 0 p7 p6 p5 p4 p3 p2 p1 p0] */
        v7 = d & M;
        d >>= 26;
        c += v7 * R0;
        debug_assert_bits!(v7, 26);
        debug_assert_bits!(d, 32);
        // debug_assert_bits!(c, 64);
        debug_assert!(c <= 0x800001703fffc2f7);
        /* [d u7 0 0 0 0 0 0 0 t9 0 c-u7*R0 t6 t5 t4 t3 t2 t1 t0] = [p17 p16 p15 p14 p13 p12 p11 p10 p9 0 p7 p6 p5 p4 p3 p2 p1 p0] */
        t7 = (c & M) as u32;
        c >>= 26;
        c += v7 * R1;
        debug_assert_bits!(t7, 26);
        debug_assert_bits!(c, 38);
        /* [d u7 0 0 0 0 0 0 0 t9 c-u7*R1 t7-u7*R0 t6 t5 t4 t3 t2 t1 t0] = [p17 p16 p15 p14 p13 p12 p11 p10 p9 0 p7 p6 p5 p4 p3 p2 p1 p0] */
        /* [d 0 0 0 0 0 0 0 0 t9 c t7 t6 t5 t4 t3 t2 t1 t0] = [p17 p16 p15 p14 p13 p12 p11 p10 p9 0 p7 p6 p5 p4 p3 p2 p1 p0] */

        c = c
            .wrapping_add((a.n[0] as u64) * (b.n[8] as u64))
            .wrapping_add((a.n[1] as u64) * (b.n[7] as u64))
            .wrapping_add((a.n[2] as u64) * (b.n[6] as u64))
            .wrapping_add((a.n[3] as u64) * (b.n[5] as u64))
            .wrapping_add((a.n[4] as u64) * (b.n[4] as u64))
            .wrapping_add((a.n[5] as u64) * (b.n[3] as u64))
            .wrapping_add((a.n[6] as u64) * (b.n[2] as u64))
            .wrapping_add((a.n[7] as u64) * (b.n[1] as u64))
            .wrapping_add((a.n[8] as u64) * (b.n[0] as u64));
        // debug_assert_bits!(c, 64);
        debug_assert!(c <= 0x9000007b80000008);
        /* [d 0 0 0 0 0 0 0 0 t9 c t7 t6 t5 t4 t3 t2 t1 t0] = [p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        d = d.wrapping_add((a.n[9] as u64) * (b.n[9] as u64));
        debug_assert_bits!(d, 57);
        /* [d 0 0 0 0 0 0 0 0 t9 c t7 t6 t5 t4 t3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        v8 = d & M;
        d >>= 26;
        c += v8 * R0;
        debug_assert_bits!(v8, 26);
        debug_assert_bits!(d, 31);
        // debug_assert_bits!(c, 64);
        debug_assert!(c <= 0x9000016fbfffc2f8);
        /* [d u8 0 0 0 0 0 0 0 0 t9 c-u8*R0 t7 t6 t5 t4 t3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */

        self.n[3] = t3;
        debug_assert_bits!(self.n[3], 26);
        /* [d u8 0 0 0 0 0 0 0 0 t9 c-u8*R0 t7 t6 t5 t4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        self.n[4] = t4;
        debug_assert_bits!(self.n[4], 26);
        /* [d u8 0 0 0 0 0 0 0 0 t9 c-u8*R0 t7 t6 t5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        self.n[5] = t5;
        debug_assert_bits!(self.n[5], 26);
        /* [d u8 0 0 0 0 0 0 0 0 t9 c-u8*R0 t7 t6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        self.n[6] = t6;
        debug_assert_bits!(self.n[6], 26);
        /* [d u8 0 0 0 0 0 0 0 0 t9 c-u8*R0 t7 r6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        self.n[7] = t7;
        debug_assert_bits!(self.n[7], 26);
        /* [d u8 0 0 0 0 0 0 0 0 t9 c-u8*R0 r7 r6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */

        self.n[8] = (c & M) as u32;
        c >>= 26;
        c += v8 * R1;
        debug_assert_bits!(self.n[8], 26);
        debug_assert_bits!(c, 39);
        /* [d u8 0 0 0 0 0 0 0 0 t9+c-u8*R1 r8-u8*R0 r7 r6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        /* [d 0 0 0 0 0 0 0 0 0 t9+c r8 r7 r6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        c += d * R0 + t9 as u64;
        debug_assert_bits!(c, 45);
        /* [d 0 0 0 0 0 0 0 0 0 c-d*R0 r8 r7 r6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        self.n[9] = (c & (M >> 4)) as u32;
        c >>= 22;
        c += d * (R1 << 4);
        debug_assert_bits!(self.n[9], 22);
        debug_assert_bits!(c, 46);
        /* [d 0 0 0 0 0 0 0 0 r9+((c-d*R1<<4)<<22)-d*R0 r8 r7 r6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        /* [d 0 0 0 0 0 0 0 -d*R1 r9+(c<<22)-d*R0 r8 r7 r6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        /* [r9+(c<<22) r8 r7 r6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */

        d = c * (R0 >> 4) + t0 as u64;
        debug_assert_bits!(d, 56);
        /* [r9+(c<<22) r8 r7 r6 r5 r4 r3 t2 t1 d-c*R0>>4] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        self.n[0] = (d & M) as u32;
        d >>= 26;
        debug_assert_bits!(self.n[0], 26);
        debug_assert_bits!(d, 30);
        /* [r9+(c<<22) r8 r7 r6 r5 r4 r3 t2 t1+d r0-c*R0>>4] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        d += c * (R1 >> 4) + t1 as u64;
        debug_assert_bits!(d, 53);
        debug_assert!(d <= 0x10000003ffffbf);
        /* [r9+(c<<22) r8 r7 r6 r5 r4 r3 t2 d-c*R1>>4 r0-c*R0>>4] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        /* [r9 r8 r7 r6 r5 r4 r3 t2 d r0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        self.n[1] = (d & M) as u32;
        d >>= 26;
        debug_assert_bits!(self.n[1], 26);
        debug_assert_bits!(d, 27);
        debug_assert!(d <= 0x4000000);
        /* [r9 r8 r7 r6 r5 r4 r3 t2+d r1 r0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        d += t2 as u64;
        debug_assert_bits!(d, 27);
        /* [r9 r8 r7 r6 r5 r4 r3 d r1 r0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        self.n[2] = d as u32;
        debug_assert_bits!(self.n[2], 27);
        /* [r9 r8 r7 r6 r5 r4 r3 r2 r1 r0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    }

    fn sqr_inner(&mut self, a: &Field) {
        const M: u64 = 0x3ffffff;
        const R0: u64 = 0x3d10;
        const R1: u64 = 0x400;

        let (mut c, mut d): (u64, u64);
        let (v0, v1, v2, v3, v4, v5, v6, v7, v8): (u64, u64, u64, u64, u64, u64, u64, u64, u64);
        let (t9, t0, t1, t2, t3, t4, t5, t6, t7): (u32, u32, u32, u32, u32, u32, u32, u32, u32);

        debug_assert_bits!(a.n[0], 30);
        debug_assert_bits!(a.n[1], 30);
        debug_assert_bits!(a.n[2], 30);
        debug_assert_bits!(a.n[3], 30);
        debug_assert_bits!(a.n[4], 30);
        debug_assert_bits!(a.n[5], 30);
        debug_assert_bits!(a.n[6], 30);
        debug_assert_bits!(a.n[7], 30);
        debug_assert_bits!(a.n[8], 30);
        debug_assert_bits!(a.n[9], 26);

        // [... a b c] is a shorthand for ... + a<<52 + b<<26 + c<<0 mod n.
        // px is a shorthand for sum(a.n[i]*a.n[x-i], i=0..x).
        // Note that [x 0 0 0 0 0 0 0 0 0 0] = [x*R1 x*R0].

        d = (((a.n[0] * 2) as u64) * (a.n[9] as u64))
            .wrapping_add(((a.n[1] * 2) as u64) * (a.n[8] as u64))
            .wrapping_add(((a.n[2] * 2) as u64) * (a.n[7] as u64))
            .wrapping_add(((a.n[3] * 2) as u64) * (a.n[6] as u64))
            .wrapping_add(((a.n[4] * 2) as u64) * (a.n[5] as u64));
        // debug_assert_bits!(d, 64);
        /* [d 0 0 0 0 0 0 0 0 0] = [p9 0 0 0 0 0 0 0 0 0] */
        t9 = (d & M) as u32;
        d >>= 26;
        debug_assert_bits!(t9, 26);
        debug_assert_bits!(d, 38);
        /* [d t9 0 0 0 0 0 0 0 0 0] = [p9 0 0 0 0 0 0 0 0 0] */

        c = (a.n[0] as u64) * (a.n[0] as u64);
        debug_assert_bits!(c, 60);
        /* [d t9 0 0 0 0 0 0 0 0 c] = [p9 0 0 0 0 0 0 0 0 p0] */
        d = d
            .wrapping_add(((a.n[1] * 2) as u64) * (a.n[9] as u64))
            .wrapping_add(((a.n[2] * 2) as u64) * (a.n[8] as u64))
            .wrapping_add(((a.n[3] * 2) as u64) * (a.n[7] as u64))
            .wrapping_add(((a.n[4] * 2) as u64) * (a.n[6] as u64))
            .wrapping_add((a.n[5] as u64) * (a.n[5] as u64));
        debug_assert_bits!(d, 63);
        /* [d t9 0 0 0 0 0 0 0 0 c] = [p10 p9 0 0 0 0 0 0 0 0 p0] */
        v0 = d & M;
        d >>= 26;
        c += v0 * R0;
        debug_assert_bits!(v0, 26);
        debug_assert_bits!(d, 37);
        debug_assert_bits!(c, 61);
        /* [d u0 t9 0 0 0 0 0 0 0 0 c-u0*R0] = [p10 p9 0 0 0 0 0 0 0 0 p0] */
        t0 = (c & M) as u32;
        c >>= 26;
        c += v0 * R1;
        debug_assert_bits!(t0, 26);
        debug_assert_bits!(c, 37);
        /* [d u0 t9 0 0 0 0 0 0 0 c-u0*R1 t0-u0*R0] = [p10 p9 0 0 0 0 0 0 0 0 p0] */
        /* [d 0 t9 0 0 0 0 0 0 0 c t0] = [p10 p9 0 0 0 0 0 0 0 0 p0] */

        c = c.wrapping_add(((a.n[0] * 2) as u64) * (a.n[1] as u64));
        debug_assert_bits!(c, 62);
        /* [d 0 t9 0 0 0 0 0 0 0 c t0] = [p10 p9 0 0 0 0 0 0 0 p1 p0] */
        d = d
            .wrapping_add(((a.n[2] * 2) as u64) * (a.n[9] as u64))
            .wrapping_add(((a.n[3] * 2) as u64) * (a.n[8] as u64))
            .wrapping_add(((a.n[4] * 2) as u64) * (a.n[7] as u64))
            .wrapping_add(((a.n[5] * 2) as u64) * (a.n[6] as u64));
        debug_assert_bits!(d, 63);
        /* [d 0 t9 0 0 0 0 0 0 0 c t0] = [p11 p10 p9 0 0 0 0 0 0 0 p1 p0] */
        v1 = d & M;
        d >>= 26;
        c += v1 * R0;
        debug_assert_bits!(v1, 26);
        debug_assert_bits!(d, 37);
        debug_assert_bits!(c, 63);
        /* [d u1 0 t9 0 0 0 0 0 0 0 c-u1*R0 t0] = [p11 p10 p9 0 0 0 0 0 0 0 p1 p0] */
        t1 = (c & M) as u32;
        c >>= 26;
        c += v1 * R1;
        debug_assert_bits!(t1, 26);
        debug_assert_bits!(c, 38);
        /* [d u1 0 t9 0 0 0 0 0 0 c-u1*R1 t1-u1*R0 t0] = [p11 p10 p9 0 0 0 0 0 0 0 p1 p0] */
        /* [d 0 0 t9 0 0 0 0 0 0 c t1 t0] = [p11 p10 p9 0 0 0 0 0 0 0 p1 p0] */

        c = c
            .wrapping_add(((a.n[0] * 2) as u64) * (a.n[2] as u64))
            .wrapping_add((a.n[1] as u64) * (a.n[1] as u64));
        debug_assert_bits!(c, 62);
        /* [d 0 0 t9 0 0 0 0 0 0 c t1 t0] = [p11 p10 p9 0 0 0 0 0 0 p2 p1 p0] */
        d = d
            .wrapping_add(((a.n[3] * 2) as u64) * (a.n[9] as u64))
            .wrapping_add(((a.n[4] * 2) as u64) * (a.n[8] as u64))
            .wrapping_add(((a.n[5] * 2) as u64) * (a.n[7] as u64))
            .wrapping_add((a.n[6] as u64) * (a.n[6] as u64));
        debug_assert_bits!(d, 63);
        /* [d 0 0 t9 0 0 0 0 0 0 c t1 t0] = [p12 p11 p10 p9 0 0 0 0 0 0 p2 p1 p0] */
        v2 = d & M;
        d >>= 26;
        c += v2 * R0;
        debug_assert_bits!(v2, 26);
        debug_assert_bits!(d, 37);
        debug_assert_bits!(c, 63);
        /* [d u2 0 0 t9 0 0 0 0 0 0 c-u2*R0 t1 t0] = [p12 p11 p10 p9 0 0 0 0 0 0 p2 p1 p0] */
        t2 = (c & M) as u32;
        c >>= 26;
        c += v2 * R1;
        debug_assert_bits!(t2, 26);
        debug_assert_bits!(c, 38);
        /* [d u2 0 0 t9 0 0 0 0 0 c-u2*R1 t2-u2*R0 t1 t0] = [p12 p11 p10 p9 0 0 0 0 0 0 p2 p1 p0] */
        /* [d 0 0 0 t9 0 0 0 0 0 c t2 t1 t0] = [p12 p11 p10 p9 0 0 0 0 0 0 p2 p1 p0] */

        c = c
            .wrapping_add(((a.n[0] * 2) as u64) * (a.n[3] as u64))
            .wrapping_add(((a.n[1] * 2) as u64) * (a.n[2] as u64));
        debug_assert_bits!(c, 63);
        /* [d 0 0 0 t9 0 0 0 0 0 c t2 t1 t0] = [p12 p11 p10 p9 0 0 0 0 0 p3 p2 p1 p0] */
        d = d
            .wrapping_add(((a.n[4] * 2) as u64) * (a.n[9] as u64))
            .wrapping_add(((a.n[5] * 2) as u64) * (a.n[8] as u64))
            .wrapping_add(((a.n[6] * 2) as u64) * (a.n[7] as u64));
        debug_assert_bits!(d, 63);
        /* [d 0 0 0 t9 0 0 0 0 0 c t2 t1 t0] = [p13 p12 p11 p10 p9 0 0 0 0 0 p3 p2 p1 p0] */
        v3 = d & M;
        d >>= 26;
        c += v3 * R0;
        debug_assert_bits!(v3, 26);
        debug_assert_bits!(d, 37);
        // debug_assert_bits!(c, 64);
        /* [d u3 0 0 0 t9 0 0 0 0 0 c-u3*R0 t2 t1 t0] = [p13 p12 p11 p10 p9 0 0 0 0 0 p3 p2 p1 p0] */
        t3 = (c & M) as u32;
        c >>= 26;
        c += v3 * R1;
        debug_assert_bits!(t3, 26);
        debug_assert_bits!(c, 39);
        /* [d u3 0 0 0 t9 0 0 0 0 c-u3*R1 t3-u3*R0 t2 t1 t0] = [p13 p12 p11 p10 p9 0 0 0 0 0 p3 p2 p1 p0] */
        /* [d 0 0 0 0 t9 0 0 0 0 c t3 t2 t1 t0] = [p13 p12 p11 p10 p9 0 0 0 0 0 p3 p2 p1 p0] */

        c = c
            .wrapping_add(((a.n[0] * 2) as u64) * (a.n[4] as u64))
            .wrapping_add(((a.n[1] * 2) as u64) * (a.n[3] as u64))
            .wrapping_add((a.n[2] as u64) * (a.n[2] as u64));
        debug_assert_bits!(c, 63);
        /* [d 0 0 0 0 t9 0 0 0 0 c t3 t2 t1 t0] = [p13 p12 p11 p10 p9 0 0 0 0 p4 p3 p2 p1 p0] */
        d = d
            .wrapping_add(((a.n[5] * 2) as u64) * (a.n[9] as u64))
            .wrapping_add(((a.n[6] * 2) as u64) * (a.n[8] as u64))
            .wrapping_add((a.n[7] as u64) * (a.n[7] as u64));
        debug_assert_bits!(d, 62);
        /* [d 0 0 0 0 t9 0 0 0 0 c t3 t2 t1 t0] = [p14 p13 p12 p11 p10 p9 0 0 0 0 p4 p3 p2 p1 p0] */
        v4 = d & M;
        d >>= 26;
        c += v4 * R0;
        debug_assert_bits!(v4, 26);
        debug_assert_bits!(d, 36);
        // debug_assert_bits!(c, 64);
        /* [d u4 0 0 0 0 t9 0 0 0 0 c-u4*R0 t3 t2 t1 t0] = [p14 p13 p12 p11 p10 p9 0 0 0 0 p4 p3 p2 p1 p0] */
        t4 = (c & M) as u32;
        c >>= 26;
        c += v4 * R1;
        debug_assert_bits!(t4, 26);
        debug_assert_bits!(c, 39);
        /* [d u4 0 0 0 0 t9 0 0 0 c-u4*R1 t4-u4*R0 t3 t2 t1 t0] = [p14 p13 p12 p11 p10 p9 0 0 0 0 p4 p3 p2 p1 p0] */
        /* [d 0 0 0 0 0 t9 0 0 0 c t4 t3 t2 t1 t0] = [p14 p13 p12 p11 p10 p9 0 0 0 0 p4 p3 p2 p1 p0] */

        c = c
            .wrapping_add(((a.n[0] * 2) as u64) * (a.n[5] as u64))
            .wrapping_add(((a.n[1] * 2) as u64) * (a.n[4] as u64))
            .wrapping_add(((a.n[2] * 2) as u64) * (a.n[3] as u64));
        debug_assert_bits!(c, 63);
        /* [d 0 0 0 0 0 t9 0 0 0 c t4 t3 t2 t1 t0] = [p14 p13 p12 p11 p10 p9 0 0 0 p5 p4 p3 p2 p1 p0] */
        d = d
            .wrapping_add(((a.n[6] * 2) as u64) * (a.n[9] as u64))
            .wrapping_add(((a.n[7] * 2) as u64) * (a.n[8] as u64));
        debug_assert_bits!(d, 62);
        /* [d 0 0 0 0 0 t9 0 0 0 c t4 t3 t2 t1 t0] = [p15 p14 p13 p12 p11 p10 p9 0 0 0 p5 p4 p3 p2 p1 p0] */
        v5 = d & M;
        d >>= 26;
        c += v5 * R0;
        debug_assert_bits!(v5, 26);
        debug_assert_bits!(d, 36);
        // debug_assert_bits!(c, 64);
        /* [d u5 0 0 0 0 0 t9 0 0 0 c-u5*R0 t4 t3 t2 t1 t0] = [p15 p14 p13 p12 p11 p10 p9 0 0 0 p5 p4 p3 p2 p1 p0] */
        t5 = (c & M) as u32;
        c >>= 26;
        c += v5 * R1;
        debug_assert_bits!(t5, 26);
        debug_assert_bits!(c, 39);
        /* [d u5 0 0 0 0 0 t9 0 0 c-u5*R1 t5-u5*R0 t4 t3 t2 t1 t0] = [p15 p14 p13 p12 p11 p10 p9 0 0 0 p5 p4 p3 p2 p1 p0] */
        /* [d 0 0 0 0 0 0 t9 0 0 c t5 t4 t3 t2 t1 t0] = [p15 p14 p13 p12 p11 p10 p9 0 0 0 p5 p4 p3 p2 p1 p0] */

        c = c
            .wrapping_add(((a.n[0] * 2) as u64) * (a.n[6] as u64))
            .wrapping_add(((a.n[1] * 2) as u64) * (a.n[5] as u64))
            .wrapping_add(((a.n[2] * 2) as u64) * (a.n[4] as u64))
            .wrapping_add((a.n[3] as u64) * (a.n[3] as u64));
        debug_assert_bits!(c, 63);
        /* [d 0 0 0 0 0 0 t9 0 0 c t5 t4 t3 t2 t1 t0] = [p15 p14 p13 p12 p11 p10 p9 0 0 p6 p5 p4 p3 p2 p1 p0] */
        d = d
            .wrapping_add(((a.n[7] * 2) as u64) * (a.n[9] as u64))
            .wrapping_add((a.n[8] as u64) * (a.n[8] as u64));
        debug_assert_bits!(d, 61);
        /* [d 0 0 0 0 0 0 t9 0 0 c t5 t4 t3 t2 t1 t0] = [p16 p15 p14 p13 p12 p11 p10 p9 0 0 p6 p5 p4 p3 p2 p1 p0] */
        v6 = d & M;
        d >>= 26;
        c += v6 * R0;
        debug_assert_bits!(v6, 26);
        debug_assert_bits!(d, 35);
        // debug_assert_bits!(c, 64);
        /* [d u6 0 0 0 0 0 0 t9 0 0 c-u6*R0 t5 t4 t3 t2 t1 t0] = [p16 p15 p14 p13 p12 p11 p10 p9 0 0 p6 p5 p4 p3 p2 p1 p0] */
        t6 = (c & M) as u32;
        c >>= 26;
        c += v6 * R1;
        debug_assert_bits!(t6, 26);
        debug_assert_bits!(c, 39);
        /* [d u6 0 0 0 0 0 0 t9 0 c-u6*R1 t6-u6*R0 t5 t4 t3 t2 t1 t0] = [p16 p15 p14 p13 p12 p11 p10 p9 0 0 p6 p5 p4 p3 p2 p1 p0] */
        /* [d 0 0 0 0 0 0 0 t9 0 c t6 t5 t4 t3 t2 t1 t0] = [p16 p15 p14 p13 p12 p11 p10 p9 0 0 p6 p5 p4 p3 p2 p1 p0] */

        c = c
            .wrapping_add(((a.n[0] * 2) as u64) * (a.n[7] as u64))
            .wrapping_add(((a.n[1] * 2) as u64) * (a.n[6] as u64))
            .wrapping_add(((a.n[2] * 2) as u64) * (a.n[5] as u64))
            .wrapping_add(((a.n[3] * 2) as u64) * (a.n[4] as u64));
        // debug_assert_bits!(c, 64);
        debug_assert!(c <= 0x8000007C00000007);
        /* [d 0 0 0 0 0 0 0 t9 0 c t6 t5 t4 t3 t2 t1 t0] = [p16 p15 p14 p13 p12 p11 p10 p9 0 p7 p6 p5 p4 p3 p2 p1 p0] */
        d = d.wrapping_add(((a.n[8] * 2) as u64) * (a.n[9] as u64));
        debug_assert_bits!(d, 58);
        /* [d 0 0 0 0 0 0 0 t9 0 c t6 t5 t4 t3 t2 t1 t0] = [p17 p16 p15 p14 p13 p12 p11 p10 p9 0 p7 p6 p5 p4 p3 p2 p1 p0] */
        v7 = d & M;
        d >>= 26;
        c += v7 * R0;
        debug_assert_bits!(v7, 26);
        debug_assert_bits!(d, 32);
        /* debug_assert_bits!(c, 64); */
        debug_assert!(c <= 0x800001703FFFC2F7);
        /* [d u7 0 0 0 0 0 0 0 t9 0 c-u7*R0 t6 t5 t4 t3 t2 t1 t0] = [p17 p16 p15 p14 p13 p12 p11 p10 p9 0 p7 p6 p5 p4 p3 p2 p1 p0] */
        t7 = (c & M) as u32;
        c >>= 26;
        c += v7 * R1;
        debug_assert_bits!(t7, 26);
        debug_assert_bits!(c, 38);
        /* [d u7 0 0 0 0 0 0 0 t9 c-u7*R1 t7-u7*R0 t6 t5 t4 t3 t2 t1 t0] = [p17 p16 p15 p14 p13 p12 p11 p10 p9 0 p7 p6 p5 p4 p3 p2 p1 p0] */
        /* [d 0 0 0 0 0 0 0 0 t9 c t7 t6 t5 t4 t3 t2 t1 t0] = [p17 p16 p15 p14 p13 p12 p11 p10 p9 0 p7 p6 p5 p4 p3 p2 p1 p0] */

        c = c
            .wrapping_add(((a.n[0] * 2) as u64) * (a.n[8] as u64))
            .wrapping_add(((a.n[1] * 2) as u64) * (a.n[7] as u64))
            .wrapping_add(((a.n[2] * 2) as u64) * (a.n[6] as u64))
            .wrapping_add(((a.n[3] * 2) as u64) * (a.n[5] as u64))
            .wrapping_add((a.n[4] as u64) * (a.n[4] as u64));
        // debug_assert_bits!(c, 64);
        debug_assert!(c <= 0x9000007B80000008);
        /* [d 0 0 0 0 0 0 0 0 t9 c t7 t6 t5 t4 t3 t2 t1 t0] = [p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        d = d.wrapping_add((a.n[9] as u64) * (a.n[9] as u64));
        debug_assert_bits!(d, 57);
        /* [d 0 0 0 0 0 0 0 0 t9 c t7 t6 t5 t4 t3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        v8 = d & M;
        d >>= 26;
        c += v8 * R0;
        debug_assert_bits!(v8, 26);
        debug_assert_bits!(d, 31);
        /* debug_assert_bits!(c, 64); */
        debug_assert!(c <= 0x9000016FBFFFC2F8);
        /* [d u8 0 0 0 0 0 0 0 0 t9 c-u8*R0 t7 t6 t5 t4 t3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */

        self.n[3] = t3;
        debug_assert_bits!(self.n[3], 26);
        /* [d u8 0 0 0 0 0 0 0 0 t9 c-u8*R0 t7 t6 t5 t4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        self.n[4] = t4;
        debug_assert_bits!(self.n[4], 26);
        /* [d u8 0 0 0 0 0 0 0 0 t9 c-u8*R0 t7 t6 t5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        self.n[5] = t5;
        debug_assert_bits!(self.n[5], 26);
        /* [d u8 0 0 0 0 0 0 0 0 t9 c-u8*R0 t7 t6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        self.n[6] = t6;
        debug_assert_bits!(self.n[6], 26);
        /* [d u8 0 0 0 0 0 0 0 0 t9 c-u8*R0 t7 r6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        self.n[7] = t7;
        debug_assert_bits!(self.n[7], 26);
        /* [d u8 0 0 0 0 0 0 0 0 t9 c-u8*R0 r7 r6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */

        self.n[8] = (c & M) as u32;
        c >>= 26;
        c += v8 * R1;
        debug_assert_bits!(self.n[8], 26);
        debug_assert_bits!(c, 39);
        /* [d u8 0 0 0 0 0 0 0 0 t9+c-u8*R1 r8-u8*R0 r7 r6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        /* [d 0 0 0 0 0 0 0 0 0 t9+c r8 r7 r6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        c += d * R0 + t9 as u64;
        debug_assert_bits!(c, 45);
        /* [d 0 0 0 0 0 0 0 0 0 c-d*R0 r8 r7 r6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        self.n[9] = (c & (M >> 4)) as u32;
        c >>= 22;
        c += d * (R1 << 4);
        debug_assert_bits!(self.n[9], 22);
        debug_assert_bits!(c, 46);
        /* [d 0 0 0 0 0 0 0 0 r9+((c-d*R1<<4)<<22)-d*R0 r8 r7 r6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        /* [d 0 0 0 0 0 0 0 -d*R1 r9+(c<<22)-d*R0 r8 r7 r6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        /* [r9+(c<<22) r8 r7 r6 r5 r4 r3 t2 t1 t0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */

        d = c * (R0 >> 4) + t0 as u64;
        debug_assert_bits!(d, 56);
        /* [r9+(c<<22) r8 r7 r6 r5 r4 r3 t2 t1 d-c*R0>>4] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        self.n[0] = (d & M) as u32;
        d >>= 26;
        debug_assert_bits!(self.n[0], 26);
        debug_assert_bits!(d, 30);
        /* [r9+(c<<22) r8 r7 r6 r5 r4 r3 t2 t1+d r0-c*R0>>4] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        d += c * (R1 >> 4) + t1 as u64;
        debug_assert_bits!(d, 53);
        debug_assert!(d <= 0x10000003FFFFBF);
        /* [r9+(c<<22) r8 r7 r6 r5 r4 r3 t2 d-c*R1>>4 r0-c*R0>>4] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        /* [r9 r8 r7 r6 r5 r4 r3 t2 d r0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        self.n[1] = (d & M) as u32;
        d >>= 26;
        debug_assert_bits!(self.n[1], 26);
        debug_assert_bits!(d, 27);
        debug_assert!(d <= 0x4000000);
        /* [r9 r8 r7 r6 r5 r4 r3 t2+d r1 r0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        d += t2 as u64;
        debug_assert_bits!(d, 27);
        /* [r9 r8 r7 r6 r5 r4 r3 d r1 r0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
        self.n[2] = d as u32;
        debug_assert_bits!(self.n[2], 27);
        /* [r9 r8 r7 r6 r5 r4 r3 r2 r1 r0] = [p18 p17 p16 p15 p14 p13 p12 p11 p10 p9 p8 p7 p6 p5 p4 p3 p2 p1 p0] */
    }

    /// Sets a field element to be the product of two others. Requires
    /// the inputs' magnitudes to be at most 8. The output magnitude
    /// is 1 (but not guaranteed to be normalized).
    pub fn mul_in_place(&mut self, a: &Field, b: &Field) {
        debug_assert!(a.magnitude <= 8);
        debug_assert!(b.magnitude <= 8);
        debug_assert!(a.verify());
        debug_assert!(b.verify());
        self.mul_inner(a, b);
        self.magnitude = 1;
        self.normalized = false;
        debug_assert!(self.verify());
    }

    /// Sets a field element to be the square of another. Requires the
    /// input's magnitude to be at most 8. The output magnitude is 1
    /// (but not guaranteed to be normalized).
    pub fn sqr_in_place(&mut self, a: &Field) {
        debug_assert!(a.magnitude <= 8);
        debug_assert!(a.verify());
        self.sqr_inner(a);
        self.magnitude = 1;
        self.normalized = false;
        debug_assert!(a.verify());
    }

    pub fn sqr(&self) -> Field {
        let mut ret = Field::default();
        ret.sqr_in_place(self);
        ret
    }

    /// If a has a square root, it is computed in r and 1 is
    /// returned. If a does not have a square root, the root of its
    /// negation is computed and 0 is returned. The input's magnitude
    /// can be at most 8. The output magnitude is 1 (but not
    /// guaranteed to be normalized). The result in r will always be a
    /// square itself.
    pub fn sqrt(&self) -> (Field, bool) {
        let mut x2 = self.sqr();
        x2 *= self;

        let mut x3 = x2.sqr();
        x3 *= self;

        let mut x6 = x3;
        for _ in 0..3 {
            x6 = x6.sqr();
        }
        x6 *= &x3;

        let mut x9 = x6;
        for _ in 0..3 {
            x9 = x9.sqr();
        }
        x9 *= &x3;

        let mut x11 = x9;
        for _ in 0..2 {
            x11 = x11.sqr();
        }
        x11 *= &x2;

        let mut x22 = x11;
        for _ in 0..11 {
            x22 = x22.sqr();
        }
        x22 *= &x11;

        let mut x44 = x22;
        for _ in 0..22 {
            x44 = x44.sqr();
        }
        x44 *= &x22;

        let mut x88 = x44;
        for _ in 0..44 {
            x88 = x88.sqr();
        }
        x88 *= &x44;

        let mut x176 = x88;
        for _ in 0..88 {
            x176 = x176.sqr();
        }
        x176 *= &x88;

        let mut x220 = x176;
        for _ in 0..44 {
            x220 = x220.sqr();
        }
        x220 *= &x44;

        let mut x223 = x220;
        for _ in 0..3 {
            x223 = x223.sqr();
        }
        x223 *= &x3;

        let mut t1 = x223;
        for _ in 0..23 {
            t1 = t1.sqr();
        }
        t1 *= &x22;
        for _ in 0..6 {
            t1 = t1.sqr();
        }
        t1 *= &x2;
        t1 = t1.sqr();
        let r = t1.sqr();

        t1 = r.sqr();
        (r, &t1 == self)
    }

    /// Sets a field element to be the (modular) inverse of
    /// another. Requires the input's magnitude to be at most 8. The
    /// output magnitude is 1 (but not guaranteed to be normalized).
    pub fn inv(&self) -> Field {
        let mut x2 = self.sqr();
        x2 *= self;

        let mut x3 = x2.sqr();
        x3 *= self;

        let mut x6 = x3;
        for _ in 0..3 {
            x6 = x6.sqr();
        }
        x6 *= &x3;

        let mut x9 = x6;
        for _ in 0..3 {
            x9 = x9.sqr();
        }
        x9 *= &x3;

        let mut x11 = x9;
        for _ in 0..2 {
            x11 = x11.sqr();
        }
        x11 *= &x2;

        let mut x22 = x11;
        for _ in 0..11 {
            x22 = x22.sqr();
        }
        x22 *= &x11;

        let mut x44 = x22;
        for _ in 0..22 {
            x44 = x44.sqr();
        }
        x44 *= &x22;

        let mut x88 = x44;
        for _ in 0..44 {
            x88 = x88.sqr();
        }
        x88 *= &x44;

        let mut x176 = x88;
        for _ in 0..88 {
            x176 = x176.sqr();
        }
        x176 *= &x88;

        let mut x220 = x176;
        for _ in 0..44 {
            x220 = x220.sqr();
        }
        x220 *= &x44;

        let mut x223 = x220;
        for _ in 0..3 {
            x223 = x223.sqr();
        }
        x223 *= &x3;

        let mut t1 = x223;
        for _ in 0..23 {
            t1 = t1.sqr();
        }
        t1 *= &x22;
        for _ in 0..5 {
            t1 = t1.sqr();
        }
        t1 *= self;
        for _ in 0..3 {
            t1 = t1.sqr();
        }
        t1 *= &x2;
        for _ in 0..2 {
            t1 = t1.sqr();
        }
        self * &t1
    }

    /// Potentially faster version of secp256k1_fe_inv, without
    /// constant-time guarantee.
    pub fn inv_var(&self) -> Field {
        self.inv()
    }

    /// Checks whether a field element is a quadratic residue.
    pub fn is_quad_var(&self) -> bool {
        let (_, ret) = self.sqrt();
        ret
    }

    /// If flag is true, set *r equal to *a; otherwise leave
    /// it. Constant-time.
    pub fn cmov(&mut self, other: &Field, flag: bool) {
        self.n[0] = if flag { other.n[0] } else { self.n[0] };
        self.n[1] = if flag { other.n[1] } else { self.n[1] };
        self.n[2] = if flag { other.n[2] } else { self.n[2] };
        self.n[3] = if flag { other.n[3] } else { self.n[3] };
        self.n[4] = if flag { other.n[4] } else { self.n[4] };
        self.n[5] = if flag { other.n[5] } else { self.n[5] };
        self.n[6] = if flag { other.n[6] } else { self.n[6] };
        self.n[7] = if flag { other.n[7] } else { self.n[7] };
        self.n[8] = if flag { other.n[8] } else { self.n[8] };
        self.n[9] = if flag { other.n[9] } else { self.n[9] };
        self.magnitude = if flag {
            other.magnitude
        } else {
            self.magnitude
        };
        self.normalized = if flag {
            other.normalized
        } else {
            self.normalized
        };
    }
}

impl Default for Field {
    fn default() -> Field {
        Self {
            n: [0u32; 10],
            magnitude: 0,
            normalized: true,
        }
    }
}

impl Add<Field> for Field {
    type Output = Field;
    fn add(self, other: Field) -> Field {
        let mut ret = self;
        ret.add_assign(&other);
        ret
    }
}

impl<'a, 'b> Add<&'a Field> for &'b Field {
    type Output = Field;
    fn add(self, other: &'a Field) -> Field {
        let mut ret = *self;
        ret.add_assign(other);
        ret
    }
}

impl<'a> AddAssign<&'a Field> for Field {
    fn add_assign(&mut self, other: &'a Field) {
        self.n[0] += other.n[0];
        self.n[1] += other.n[1];
        self.n[2] += other.n[2];
        self.n[3] += other.n[3];
        self.n[4] += other.n[4];
        self.n[5] += other.n[5];
        self.n[6] += other.n[6];
        self.n[7] += other.n[7];
        self.n[8] += other.n[8];
        self.n[9] += other.n[9];

        self.magnitude += other.magnitude;
        self.normalized = false;
        debug_assert!(self.verify());
    }
}

impl AddAssign<Field> for Field {
    fn add_assign(&mut self, other: Field) {
        self.add_assign(&other)
    }
}

impl Mul<Field> for Field {
    type Output = Field;
    fn mul(self, other: Field) -> Field {
        let mut ret = Field::default();
        ret.mul_in_place(&self, &other);
        ret
    }
}

impl<'a, 'b> Mul<&'a Field> for &'b Field {
    type Output = Field;
    fn mul(self, other: &'a Field) -> Field {
        let mut ret = Field::default();
        ret.mul_in_place(self, other);
        ret
    }
}

impl<'a> MulAssign<&'a Field> for Field {
    fn mul_assign(&mut self, other: &'a Field) {
        let mut ret = Field::default();
        ret.mul_in_place(self, other);
        *self = ret;
    }
}

impl MulAssign<Field> for Field {
    fn mul_assign(&mut self, other: Field) {
        self.mul_assign(&other)
    }
}

impl PartialEq for Field {
    fn eq(&self, other: &Field) -> bool {
        let mut na = self.neg(self.magnitude);
        na += other;
        na.normalizes_to_zero()
    }
}

impl Eq for Field {}

impl Ord for Field {
    fn cmp(&self, other: &Field) -> Ordering {
        self.cmp_var(other)
    }
}

impl PartialOrd for Field {
    fn partial_cmp(&self, other: &Field) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}

#[derive(Debug, Clone, Copy, Eq, PartialEq)]
/// Compact field element storage.
pub struct FieldStorage(pub [u32; 8]);

impl Default for FieldStorage {
    fn default() -> FieldStorage {
        FieldStorage([0; 8])
    }
}

impl FieldStorage {
    pub const fn new(
        d7: u32,
        d6: u32,
        d5: u32,
        d4: u32,
        d3: u32,
        d2: u32,
        d1: u32,
        d0: u32,
    ) -> Self {
        Self([d0, d1, d2, d3, d4, d5, d6, d7])
    }

    pub fn cmov(&mut self, other: &FieldStorage, flag: bool) {
        self.0[0] = if flag { other.0[0] } else { self.0[0] };
        self.0[1] = if flag { other.0[1] } else { self.0[1] };
        self.0[2] = if flag { other.0[2] } else { self.0[2] };
        self.0[3] = if flag { other.0[3] } else { self.0[3] };
        self.0[4] = if flag { other.0[4] } else { self.0[4] };
        self.0[5] = if flag { other.0[5] } else { self.0[5] };
        self.0[6] = if flag { other.0[6] } else { self.0[6] };
        self.0[7] = if flag { other.0[7] } else { self.0[7] };
    }
}

impl From<FieldStorage> for Field {
    fn from(a: FieldStorage) -> Field {
        let mut r = Field::default();

        r.n[0] = a.0[0] & 0x3FFFFFF;
        r.n[1] = a.0[0] >> 26 | ((a.0[1] << 6) & 0x3FFFFFF);
        r.n[2] = a.0[1] >> 20 | ((a.0[2] << 12) & 0x3FFFFFF);
        r.n[3] = a.0[2] >> 14 | ((a.0[3] << 18) & 0x3FFFFFF);
        r.n[4] = a.0[3] >> 8 | ((a.0[4] << 24) & 0x3FFFFFF);
        r.n[5] = (a.0[4] >> 2) & 0x3FFFFFF;
        r.n[6] = a.0[4] >> 28 | ((a.0[5] << 4) & 0x3FFFFFF);
        r.n[7] = a.0[5] >> 22 | ((a.0[6] << 10) & 0x3FFFFFF);
        r.n[8] = a.0[6] >> 16 | ((a.0[7] << 16) & 0x3FFFFFF);
        r.n[9] = a.0[7] >> 10;

        r.magnitude = 1;
        r.normalized = true;

        r
    }
}

impl Into<FieldStorage> for Field {
    fn into(self) -> FieldStorage {
        debug_assert!(self.normalized);
        let mut r = FieldStorage::default();

        r.0[0] = self.n[0] | self.n[1] << 26;
        r.0[1] = self.n[1] >> 6 | self.n[2] << 20;
        r.0[2] = self.n[2] >> 12 | self.n[3] << 14;
        r.0[3] = self.n[3] >> 18 | self.n[4] << 8;
        r.0[4] = self.n[4] >> 24 | self.n[5] << 2 | self.n[6] << 28;
        r.0[5] = self.n[6] >> 4 | self.n[7] << 22;
        r.0[6] = self.n[7] >> 10 | self.n[8] << 16;
        r.0[7] = self.n[8] >> 16 | self.n[9] << 10;

        r
    }
}
