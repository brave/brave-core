use core::hash::Hasher;

use {hash_seeded, helper};

/// The streaming version of the algorithm.
pub struct SeaHasher {
    /// The state of the hasher.
    state: u64,
    /// The first key.
    k1: u64,
    /// The second key.
    k2: u64,
    /// The third key.
    k3: u64,
    /// The fourth key.
    k4: u64,
}

impl Default for SeaHasher {
    fn default() -> SeaHasher {
        SeaHasher::with_seeds(0xe7b0c93ca8525013, 0x011d02b854ae8182, 0x7bcc5cf9c39cec76, 0xfa336285d102d083)
    }
}

impl SeaHasher {
    /// Create a new `SeaHasher` with default state.
    pub fn new() -> SeaHasher {
        SeaHasher::default()
    }

    /// Construct a new `SeaHasher` given some seed.
    ///
    /// For maximum quality, these seeds should be chosen at random.
    pub fn with_seeds(k1: u64, k2: u64, k3: u64, k4: u64) -> SeaHasher {
        SeaHasher {
            state: k1 ^ k3,
            k1: k1,
            k2: k2,
            k3: k3,
            k4: k4,
        }
    }

    /// Write some integer in.
    ///
    /// This applies XEX key whitening with the keys given as argument.
    fn write(&mut self, n: u64, k1: u64, k2: u64) {
        self.state ^= n ^ k1;
        self.state = helper::diffuse(self.state) ^ k2;
    }
}

impl Hasher for SeaHasher {
    fn finish(&self) -> u64 {
        helper::diffuse(self.state ^ self.k3) ^ self.k4
    }

    fn write(&mut self, bytes: &[u8]) {
        self.state ^= hash_seeded(bytes, self.k1, self.k2, self.k3, self.k4);
        self.state = helper::diffuse(self.state);
    }

    fn write_u64(&mut self, n: u64) {
        let k1 = self.k1;
        let k2 = self.k2;
        self.write(n, k1, k2)
    }

    fn write_u8(&mut self, n: u8) {
        let k1 = self.k1;
        let k3 = self.k3;
        self.write(n as u64, k1, k3)
    }

    fn write_u16(&mut self, n: u16) {
        let k1 = self.k1;
        let k2 = self.k2;
        self.write(n as u64, k2, k1)
    }

    fn write_u32(&mut self, n: u32) {
        let k2 = self.k2;
        let k3 = self.k3;
        self.write(n as u64, k2, k3)
    }

    fn write_usize(&mut self, n: usize) {
        let k2 = self.k2;
        let k3 = self.k3;
        self.write(n as u64, k3, k2)
    }

    fn write_i64(&mut self, n: i64) {
        let k1 = self.k1;
        let k2 = self.k2;
        self.write(n as u64, !k1, !k2)
    }

    fn write_i8(&mut self, n: i8) {
        let k1 = self.k1;
        let k3 = self.k3;
        self.write(n as u64, !k1, !k3)
    }

    fn write_i16(&mut self, n: i16) {
        let k1 = self.k1;
        let k2 = self.k2;
        self.write(n as u64, !k2, !k1)
    }

    fn write_i32(&mut self, n: i32) {
        let k2 = self.k2;
        let k3 = self.k3;
        self.write(n as u64, !k2, !k3)
    }

    fn write_isize(&mut self, n: isize) {
        let k2 = self.k2;
        let k3 = self.k3;
        self.write(n as u64, !k3, !k2)
    }
}
