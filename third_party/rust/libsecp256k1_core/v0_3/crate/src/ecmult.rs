use crate::{
    field::Field,
    group::{globalz_set_table_gej, set_table_gej_var, Affine, AffineStorage, Jacobian, AFFINE_G},
    scalar::Scalar,
};
use alloc::{
    alloc::{alloc, Layout},
    boxed::Box,
    vec,
    vec::Vec,
};
use subtle::Choice;

pub const WINDOW_A: usize = 5;
pub const WINDOW_G: usize = 16;
pub const ECMULT_TABLE_SIZE_A: usize = 1 << (WINDOW_A - 2);
pub const ECMULT_TABLE_SIZE_G: usize = 1 << (WINDOW_G - 2);
pub const WNAF_BITS: usize = 256;

fn odd_multiples_table_storage_var(pre: &mut [AffineStorage], a: &Jacobian) {
    let mut prej: Vec<Jacobian> = Vec::with_capacity(pre.len());
    for _ in 0..pre.len() {
        prej.push(Jacobian::default());
    }
    let mut prea: Vec<Affine> = Vec::with_capacity(pre.len());
    for _ in 0..pre.len() {
        prea.push(Affine::default());
    }
    let mut zr: Vec<Field> = Vec::with_capacity(pre.len());
    for _ in 0..pre.len() {
        zr.push(Field::default());
    }

    odd_multiples_table(&mut prej, &mut zr, a);
    set_table_gej_var(&mut prea, &prej, &zr);

    for i in 0..pre.len() {
        pre[i] = prea[i].into();
    }
}

/// Context for accelerating the computation of a*P + b*G.
pub struct ECMultContext {
    pre_g: [AffineStorage; ECMULT_TABLE_SIZE_G],
}

impl ECMultContext {
    /// Create a new `ECMultContext` from raw values.
    ///
    /// # Safety
    /// The function is unsafe because incorrect value of `pre_g` can lead to
    /// crypto logic failure. You most likely do not want to use this function,
    /// but `ECMultContext::new_boxed`.
    pub const unsafe fn new_from_raw(pre_g: [AffineStorage; ECMULT_TABLE_SIZE_G]) -> Self {
        Self { pre_g }
    }

    /// Inspect raw values of `ECMultContext`.
    pub fn inspect_raw(&self) -> &[AffineStorage; ECMULT_TABLE_SIZE_G] {
        &self.pre_g
    }

    /// Generate a new `ECMultContext` on the heap. Note that this function is expensive.
    pub fn new_boxed() -> Box<Self> {
        // This unsafe block allocates a new, unitialized `ECMultContext` and
        // then fills in the value. This is to avoid allocating it on stack
        // because the struct is big. All values in `ECMultContext` are manually
        // initialized after allocation.
        let mut this = unsafe {
            let ptr = alloc(Layout::new::<ECMultContext>()) as *mut ECMultContext;
            let mut this = Box::from_raw(ptr);

            for i in 0..ECMULT_TABLE_SIZE_G {
                this.pre_g[i] = AffineStorage::default();
            }

            this
        };

        let mut gj = Jacobian::default();
        gj.set_ge(&AFFINE_G);
        odd_multiples_table_storage_var(&mut this.pre_g, &gj);

        this
    }
}

/// Set a batch of group elements equal to the inputs given in jacobian
/// coordinates. Not constant time.
pub fn set_all_gej_var(a: &[Jacobian]) -> Vec<Affine> {
    let mut az: Vec<Field> = Vec::with_capacity(a.len());
    for point in a {
        if !point.is_infinity() {
            az.push(point.z);
        }
    }
    let azi: Vec<Field> = inv_all_var(&az);

    let mut ret = vec![Affine::default(); a.len()];

    let mut count = 0;
    for i in 0..a.len() {
        ret[i].infinity = a[i].infinity;
        if !a[i].is_infinity() {
            ret[i].set_gej_zinv(&a[i], &azi[count]);
            count += 1;
        }
    }
    ret
}

/// Calculate the (modular) inverses of a batch of field
/// elements. Requires the inputs' magnitudes to be at most 8. The
/// output magnitudes are 1 (but not guaranteed to be
/// normalized).
pub fn inv_all_var(fields: &[Field]) -> Vec<Field> {
    if fields.is_empty() {
        return Vec::new();
    }

    let mut ret = Vec::with_capacity(fields.len());
    ret.push(fields[0]);

    for i in 1..fields.len() {
        ret.push(Field::default());
        ret[i] = ret[i - 1] * fields[i];
    }

    let mut u = ret[fields.len() - 1].inv_var();

    for i in (1..fields.len()).rev() {
        let j = i;
        let i = i - 1;
        ret[j] = ret[i] * u;
        u *= fields[j];
    }

    ret[0] = u;
    ret
}

const GEN_BLIND: Scalar = Scalar([
    2217680822, 850875797, 1046150361, 1330484644, 4015777837, 2466086288, 2052467175, 2084507480,
]);
const GEN_INITIAL: Jacobian = Jacobian {
    x: Field::new_raw(
        586608, 43357028, 207667908, 262670128, 142222828, 38529388, 267186148, 45417712,
        115291924, 13447464,
    ),
    y: Field::new_raw(
        12696548, 208302564, 112025180, 191752716, 143238548, 145482948, 228906000, 69755164,
        243572800, 210897016,
    ),
    z: Field::new_raw(
        3685368, 75404844, 20246216, 5748944, 73206666, 107661790, 110806176, 73488774, 5707384,
        104448710,
    ),
    infinity: false,
};

/// Context for accelerating the computation of a*G.
pub struct ECMultGenContext {
    prec: [[AffineStorage; 16]; 64],
    blind: Scalar,
    initial: Jacobian,
}

impl ECMultGenContext {
    /// Create a new `ECMultGenContext` from raw values.
    ///
    /// # Safety
    /// The function is unsafe because incorrect value of `pre_g` can lead to
    /// crypto logic failure. You most likely do not want to use this function,
    /// but `ECMultGenContext::new_boxed`.
    pub const unsafe fn new_from_raw(prec: [[AffineStorage; 16]; 64]) -> Self {
        Self {
            prec,
            blind: GEN_BLIND,
            initial: GEN_INITIAL,
        }
    }

    /// Inspect `ECMultGenContext` values.
    pub fn inspect_raw(&self) -> &[[AffineStorage; 16]; 64] {
        &self.prec
    }

    /// Generate a new `ECMultGenContext` on the heap. Note that this function is expensive.
    pub fn new_boxed() -> Box<Self> {
        // This unsafe block allocates a new, unitialized `ECMultGenContext` and
        // then fills in the value. This is to avoid allocating it on stack
        // because the struct is big. All values in `ECMultGenContext` are
        // manually initialized after allocation.
        let mut this = unsafe {
            let ptr = alloc(Layout::new::<ECMultGenContext>()) as *mut ECMultGenContext;
            let mut this = Box::from_raw(ptr);

            for j in 0..64 {
                for i in 0..16 {
                    this.prec[j][i] = AffineStorage::default();
                }
            }

            this.blind = GEN_BLIND;
            this.initial = GEN_INITIAL;

            this
        };

        let mut gj = Jacobian::default();
        gj.set_ge(&AFFINE_G);

        // Construct a group element with no known corresponding scalar (nothing up my sleeve).
        let mut nums_32 = [0u8; 32];
        debug_assert!(b"The scalar for this x is unknown".len() == 32);
        for (i, v) in b"The scalar for this x is unknown".iter().enumerate() {
            nums_32[i] = *v;
        }
        let mut nums_x = Field::default();
        assert!(nums_x.set_b32(&nums_32));
        let mut nums_ge = Affine::default();
        assert!(nums_ge.set_xo_var(&nums_x, false));
        let mut nums_gej = Jacobian::default();
        nums_gej.set_ge(&nums_ge);
        nums_gej = nums_gej.add_ge_var(&AFFINE_G, None);

        // Compute prec.
        let mut precj: Vec<Jacobian> = Vec::with_capacity(1024);
        for _ in 0..1024 {
            precj.push(Jacobian::default());
        }
        let mut gbase = gj;
        let mut numsbase = nums_gej;
        for j in 0..64 {
            precj[j * 16] = numsbase;
            for i in 1..16 {
                precj[j * 16 + i] = precj[j * 16 + i - 1].add_var(&gbase, None);
            }
            for _ in 0..4 {
                gbase = gbase.double_var(None);
            }
            numsbase = numsbase.double_var(None);
            if j == 62 {
                numsbase = numsbase.neg();
                numsbase = numsbase.add_var(&nums_gej, None);
            }
        }
        let prec = set_all_gej_var(&precj);

        for j in 0..64 {
            for i in 0..16 {
                let pg: AffineStorage = prec[j * 16 + i].into();
                this.prec[j][i] = pg;
            }
        }

        this
    }
}

pub fn odd_multiples_table(prej: &mut [Jacobian], zr: &mut [Field], a: &Jacobian) {
    debug_assert!(prej.len() == zr.len());
    debug_assert!(!prej.is_empty());
    debug_assert!(!a.is_infinity());

    let d = a.double_var(None);
    let d_ge = Affine {
        x: d.x,
        y: d.y,
        infinity: false,
    };

    let mut a_ge = Affine::default();
    a_ge.set_gej_zinv(a, &d.z);
    prej[0].x = a_ge.x;
    prej[0].y = a_ge.y;
    prej[0].z = a.z;
    prej[0].infinity = false;

    zr[0] = d.z;
    for i in 1..prej.len() {
        prej[i] = prej[i - 1].add_ge_var(&d_ge, Some(&mut zr[i]));
    }

    let l = prej.last().unwrap().z * d.z;
    prej.last_mut().unwrap().z = l;
}

fn odd_multiples_table_globalz_windowa(
    pre: &mut [Affine; ECMULT_TABLE_SIZE_A],
    globalz: &mut Field,
    a: &Jacobian,
) {
    let mut prej: [Jacobian; ECMULT_TABLE_SIZE_A] = Default::default();
    let mut zr: [Field; ECMULT_TABLE_SIZE_A] = Default::default();

    odd_multiples_table(&mut prej, &mut zr, a);
    globalz_set_table_gej(pre, globalz, &prej, &zr);
}

fn table_get_ge(r: &mut Affine, pre: &[Affine], n: i32, w: usize) {
    debug_assert!(n & 1 == 1);
    debug_assert!(n >= -((1 << (w - 1)) - 1));
    debug_assert!(n <= ((1 << (w - 1)) - 1));
    if n > 0 {
        *r = pre[((n - 1) / 2) as usize];
    } else {
        *r = pre[((-n - 1) / 2) as usize].neg();
    }
}

fn table_get_ge_const(r: &mut Affine, pre: &[Affine], n: i32, w: usize) {
    let abs_n = n * (if n > 0 { 1 } else { 0 } * 2 - 1);
    let idx_n = abs_n / 2;
    debug_assert!(n & 1 == 1);
    debug_assert!(n >= -((1 << (w - 1)) - 1));
    debug_assert!(n <= ((1 << (w - 1)) - 1));
    for m in 0..pre.len() {
        let flag = m == idx_n as usize;
        r.x.cmov(&pre[m].x, flag);
        r.y.cmov(&pre[m].y, flag);
    }
    r.infinity = false;
    let neg_y = r.y.neg(1);
    r.y.cmov(&neg_y, n != abs_n);
}

fn table_get_ge_storage(r: &mut Affine, pre: &[AffineStorage], n: i32, w: usize) {
    debug_assert!(n & 1 == 1);
    debug_assert!(n >= -((1 << (w - 1)) - 1));
    debug_assert!(n <= ((1 << (w - 1)) - 1));
    if n > 0 {
        *r = pre[((n - 1) / 2) as usize].into();
    } else {
        *r = pre[((-n - 1) / 2) as usize].into();
        *r = r.neg();
    }
}

pub fn ecmult_wnaf(wnaf: &mut [i32], a: &Scalar, w: usize) -> i32 {
    let mut s = *a;
    let mut last_set_bit: i32 = -1;
    let mut bit = 0;
    let mut sign = 1;
    let mut carry = 0;

    debug_assert!(wnaf.len() <= 256);
    debug_assert!(w >= 2 && w <= 31);

    for i in 0..wnaf.len() {
        wnaf[i] = 0;
    }

    if s.bits(255, 1) > 0 {
        s = -s;
        sign = -1;
    }

    while bit < wnaf.len() {
        let mut now;
        let mut word;
        if s.bits(bit, 1) == carry as u32 {
            bit += 1;
            continue;
        }

        now = w;
        if now > wnaf.len() - bit {
            now = wnaf.len() - bit;
        }

        word = (s.bits_var(bit, now) as i32) + carry;

        carry = (word >> (w - 1)) & 1;
        word -= carry << w;

        wnaf[bit] = sign * word;
        last_set_bit = bit as i32;

        bit += now;
    }
    debug_assert!(carry == 0);
    debug_assert!({
        let mut t = true;
        while bit < 256 {
            t = t && (s.bits(bit, 1) == 0);
            bit += 1;
        }
        t
    });
    last_set_bit + 1
}

pub fn ecmult_wnaf_const(wnaf: &mut [i32], a: &Scalar, w: usize) -> i32 {
    let mut s = *a;
    let mut word = 0;

    /* Note that we cannot handle even numbers by negating them to be
     * odd, as is done in other implementations, since if our scalars
     * were specified to have width < 256 for performance reasons,
     * their negations would have width 256 and we'd lose any
     * performance benefit. Instead, we use a technique from Section
     * 4.2 of the Okeya/Tagaki paper, which is to add either 1 (for
     * even) or 2 (for odd) to the number we are encoding, returning a
     * skew value indicating this, and having the caller compensate
     * after doing the multiplication. */

    /* Negative numbers will be negated to keep their bit
     * representation below the maximum width */
    let flip = s.is_high();
    /* We add 1 to even numbers, 2 to odd ones, noting that negation
     * flips parity */
    let bit = flip ^ !s.is_even();
    /* We add 1 to even numbers, 2 to odd ones, noting that negation
     * flips parity */
    let neg_s = -s;
    let not_neg_one = !neg_s.is_one();
    s.cadd_bit(if bit { 1 } else { 0 }, not_neg_one);
    /* If we had negative one, flip == 1, s.d[0] == 0, bit == 1, so
     * caller expects that we added two to it and flipped it. In fact
     * for -1 these operations are identical. We only flipped, but
     * since skewing is required (in the sense that the skew must be 1
     * or 2, never zero) and flipping is not, we need to change our
     * flags to claim that we only skewed. */
    let mut global_sign = if flip { -1 } else { 1 };
    s.cond_neg_assign(Choice::from(flip as u8));
    global_sign *= if not_neg_one { 1 } else { 0 } * 2 - 1;
    let skew = 1 << (if bit { 1 } else { 0 });

    let mut u_last: i32 = s.shr_int(w) as i32;
    let mut u: i32 = 0;
    while word * w < WNAF_BITS {
        u = s.shr_int(w) as i32;
        let even = (u & 1) == 0;
        let sign = 2 * (if u_last > 0 { 1 } else { 0 }) - 1;
        u += sign * if even { 1 } else { 0 };
        u_last -= sign * if even { 1 } else { 0 } * (1 << w);

        wnaf[word] = (u_last as i32 * global_sign as i32) as i32;
        word += 1;

        u_last = u;
    }
    wnaf[word] = u * global_sign as i32;

    debug_assert!(s.is_zero());
    let wnaf_size = (WNAF_BITS + w - 1) / w;
    debug_assert!(word == wnaf_size);

    skew
}

impl ECMultContext {
    pub fn ecmult(&self, r: &mut Jacobian, a: &Jacobian, na: &Scalar, ng: &Scalar) {
        let mut tmpa = Affine::default();
        let mut pre_a: [Affine; ECMULT_TABLE_SIZE_A] = Default::default();
        let mut z = Field::default();
        let mut wnaf_na = [0i32; 256];
        let mut wnaf_ng = [0i32; 256];
        let bits_na = ecmult_wnaf(&mut wnaf_na, na, WINDOW_A);
        let mut bits = bits_na;
        odd_multiples_table_globalz_windowa(&mut pre_a, &mut z, a);

        let bits_ng = ecmult_wnaf(&mut wnaf_ng, &ng, WINDOW_G);
        if bits_ng > bits {
            bits = bits_ng;
        }

        r.set_infinity();
        for i in (0..bits).rev() {
            let mut n;
            *r = r.double_var(None);

            n = wnaf_na[i as usize];
            if i < bits_na && n != 0 {
                table_get_ge(&mut tmpa, &pre_a, n, WINDOW_A);
                *r = r.add_ge_var(&tmpa, None);
            }
            n = wnaf_ng[i as usize];
            if i < bits_ng && n != 0 {
                table_get_ge_storage(&mut tmpa, &self.pre_g, n, WINDOW_G);
                *r = r.add_zinv_var(&tmpa, &z);
            }
        }

        if !r.is_infinity() {
            r.z *= &z;
        }
    }

    pub fn ecmult_const(&self, r: &mut Jacobian, a: &Affine, scalar: &Scalar) {
        const WNAF_SIZE: usize = (WNAF_BITS + (WINDOW_A - 1) - 1) / (WINDOW_A - 1);

        let mut tmpa = Affine::default();
        let mut pre_a: [Affine; ECMULT_TABLE_SIZE_A] = Default::default();
        let mut z = Field::default();

        let mut wnaf_1 = [0i32; 1 + WNAF_SIZE];

        let sc = *scalar;
        let skew_1 = ecmult_wnaf_const(&mut wnaf_1, &sc, WINDOW_A - 1);

        /* Calculate odd multiples of a.  All multiples are brought to
         * the same Z 'denominator', which is stored in Z. Due to
         * secp256k1' isomorphism we can do all operations pretending
         * that the Z coordinate was 1, use affine addition formulae,
         * and correct the Z coordinate of the result once at the end.
         */
        r.set_ge(a);
        odd_multiples_table_globalz_windowa(&mut pre_a, &mut z, r);
        for i in 0..ECMULT_TABLE_SIZE_A {
            pre_a[i].y.normalize_weak();
        }

        /* first loop iteration (separated out so we can directly set
         * r, rather than having it start at infinity, get doubled
         * several times, then have its new value added to it) */
        let i = wnaf_1[WNAF_SIZE];
        debug_assert!(i != 0);
        table_get_ge_const(&mut tmpa, &pre_a, i, WINDOW_A);
        r.set_ge(&tmpa);

        /* remaining loop iterations */
        for i in (0..WNAF_SIZE).rev() {
            for _ in 0..(WINDOW_A - 1) {
                let r2 = *r;
                r.double_nonzero_in_place(&r2, None);
            }

            let n = wnaf_1[i];
            table_get_ge_const(&mut tmpa, &pre_a, n, WINDOW_A);
            debug_assert!(n != 0);
            *r = r.add_ge(&tmpa);
        }

        r.z *= &z;

        /* Correct for wNAF skew */
        let mut correction = *a;
        let mut correction_1_stor: AffineStorage;
        let a2_stor: AffineStorage;
        let mut tmpj = Jacobian::default();
        tmpj.set_ge(&correction);
        tmpj = tmpj.double_var(None);
        correction.set_gej(&tmpj);
        correction_1_stor = (*a).into();
        a2_stor = correction.into();

        /* For odd numbers this is 2a (so replace it), for even ones a (so no-op) */
        correction_1_stor.cmov(&a2_stor, skew_1 == 2);

        /* Apply the correction */
        correction = correction_1_stor.into();
        correction = correction.neg();
        *r = r.add_ge(&correction)
    }
}

impl ECMultGenContext {
    pub fn ecmult_gen(&self, r: &mut Jacobian, gn: &Scalar) {
        let mut adds = AffineStorage::default();
        *r = self.initial;

        let mut gnb = gn + &self.blind;
        let mut add = Affine::default();
        add.infinity = false;

        for j in 0..64 {
            let mut bits = gnb.bits(j * 4, 4);
            for i in 0..16 {
                adds.cmov(&self.prec[j][i], i as u32 == bits);
            }
            add = adds.into();
            *r = r.add_ge(&add);
            #[allow(unused_assignments)]
            {
                bits = 0;
            }
        }
        add.clear();
        gnb.clear();
    }
}
