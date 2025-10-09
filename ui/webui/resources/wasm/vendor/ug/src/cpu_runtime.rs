use crate::{DType, Error, Layout, Result};
use half::{bf16, f16};
use std::path::PathBuf;

#[derive(Debug, Clone)]
pub enum CpuStorage {
    BF16(Vec<bf16>),
    F16(Vec<f16>),
    F32(Vec<f32>),
    I32(Vec<i32>),
    I64(Vec<i64>),
}

// Poor man's GADT...
#[derive(Debug, Copy, Clone)]
pub enum CpuStorageRef<'a> {
    BF16(&'a [bf16]),
    F16(&'a [f16]),
    F32(&'a [f32]),
    I32(&'a [i32]),
    I64(&'a [i64]),
}

#[derive(Debug)]
pub enum CpuStorageRefMut<'a> {
    BF16(&'a mut [bf16]),
    F16(&'a mut [f16]),
    F32(&'a mut [f32]),
    I32(&'a mut [i32]),
    I64(&'a mut [i64]),
}

impl From<Vec<bf16>> for CpuStorage {
    fn from(value: Vec<bf16>) -> Self {
        Self::BF16(value)
    }
}

impl From<Vec<f16>> for CpuStorage {
    fn from(value: Vec<f16>) -> Self {
        Self::F16(value)
    }
}

impl From<Vec<f32>> for CpuStorage {
    fn from(value: Vec<f32>) -> Self {
        Self::F32(value)
    }
}

impl From<Vec<i32>> for CpuStorage {
    fn from(value: Vec<i32>) -> Self {
        Self::I32(value)
    }
}

impl From<Vec<i64>> for CpuStorage {
    fn from(value: Vec<i64>) -> Self {
        Self::I64(value)
    }
}

impl CpuStorage {
    pub fn as_mut_ptr(&mut self) -> *mut std::ffi::c_void {
        match self {
            Self::BF16(s) => s.as_mut_ptr() as *mut std::ffi::c_void,
            Self::F16(s) => s.as_mut_ptr() as *mut std::ffi::c_void,
            Self::F32(s) => s.as_mut_ptr() as *mut std::ffi::c_void,
            Self::I32(s) => s.as_mut_ptr() as *mut std::ffi::c_void,
            Self::I64(s) => s.as_mut_ptr() as *mut std::ffi::c_void,
        }
    }

    pub fn as_ptr(&mut self) -> *const std::ffi::c_void {
        match self {
            Self::BF16(s) => s.as_ptr() as *const std::ffi::c_void,
            Self::F16(s) => s.as_ptr() as *const std::ffi::c_void,
            Self::F32(s) => s.as_ptr() as *const std::ffi::c_void,
            Self::I32(s) => s.as_ptr() as *const std::ffi::c_void,
            Self::I64(s) => s.as_ptr() as *const std::ffi::c_void,
        }
    }

    pub fn len(&self) -> usize {
        match self {
            Self::BF16(s) => s.len(),
            Self::F16(s) => s.len(),
            Self::F32(s) => s.len(),
            Self::I32(s) => s.len(),
            Self::I64(s) => s.len(),
        }
    }

    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }

    pub fn dtype(&self) -> DType {
        match self {
            Self::BF16(_) => DType::BF16,
            Self::F16(_) => DType::F16,
            Self::F32(_) => DType::F32,
            Self::I32(_) => DType::I32,
            Self::I64(_) => DType::I64,
        }
    }

    pub fn as_ref(&self) -> CpuStorageRef<'_> {
        match self {
            Self::BF16(v) => CpuStorageRef::BF16(v.as_slice()),
            Self::F16(v) => CpuStorageRef::F16(v.as_slice()),
            Self::F32(v) => CpuStorageRef::F32(v.as_slice()),
            Self::I32(v) => CpuStorageRef::I32(v.as_slice()),
            Self::I64(v) => CpuStorageRef::I64(v.as_slice()),
        }
    }

    pub fn as_mut_ref(&mut self) -> CpuStorageRefMut<'_> {
        match self {
            Self::BF16(v) => CpuStorageRefMut::BF16(v.as_mut_slice()),
            Self::F16(v) => CpuStorageRefMut::F16(v.as_mut_slice()),
            Self::F32(v) => CpuStorageRefMut::F32(v.as_mut_slice()),
            Self::I32(v) => CpuStorageRefMut::I32(v.as_mut_slice()),
            Self::I64(v) => CpuStorageRefMut::I64(v.as_mut_slice()),
        }
    }

    pub fn data<T: crate::WithDType>(&self) -> Result<&[T]> {
        T::from_cpu_storage(self.as_ref())
    }

    pub fn data_mut<T: crate::WithDType>(&mut self) -> Result<&mut [T]> {
        T::from_cpu_storage_mut(self.as_mut_ref())
    }
}

impl CpuStorageRef<'_> {
    pub fn dtype(&self) -> DType {
        match self {
            Self::BF16(_) => DType::BF16,
            Self::F16(_) => DType::F16,
            Self::F32(_) => DType::F32,
            Self::I32(_) => DType::I32,
            Self::I64(_) => DType::I64,
        }
    }

    pub fn len(&self) -> usize {
        match self {
            Self::BF16(s) => s.len(),
            Self::F16(s) => s.len(),
            Self::F32(s) => s.len(),
            Self::I32(s) => s.len(),
            Self::I64(s) => s.len(),
        }
    }

    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }
}

impl<'a> From<&'a [bf16]> for CpuStorageRef<'a> {
    fn from(value: &'a [bf16]) -> Self {
        Self::BF16(value)
    }
}

impl<'a> From<&'a [f16]> for CpuStorageRef<'a> {
    fn from(value: &'a [f16]) -> Self {
        Self::F16(value)
    }
}

impl<'a> From<&'a [f32]> for CpuStorageRef<'a> {
    fn from(value: &'a [f32]) -> Self {
        Self::F32(value)
    }
}

impl<'a> From<&'a [i32]> for CpuStorageRef<'a> {
    fn from(value: &'a [i32]) -> Self {
        Self::I32(value)
    }
}

impl<'a> From<&'a [i64]> for CpuStorageRef<'a> {
    fn from(value: &'a [i64]) -> Self {
        Self::I64(value)
    }
}

impl CpuStorageRefMut<'_> {
    pub fn dtype(&self) -> DType {
        match self {
            Self::BF16(_) => DType::BF16,
            Self::F16(_) => DType::F16,
            Self::F32(_) => DType::F32,
            Self::I32(_) => DType::I32,
            Self::I64(_) => DType::I64,
        }
    }
    pub fn len(&self) -> usize {
        match self {
            Self::BF16(s) => s.len(),
            Self::F16(s) => s.len(),
            Self::F32(s) => s.len(),
            Self::I32(s) => s.len(),
            Self::I64(s) => s.len(),
        }
    }

    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }
}

impl<'a> From<&'a mut [bf16]> for CpuStorageRefMut<'a> {
    fn from(value: &'a mut [bf16]) -> Self {
        Self::BF16(value)
    }
}

impl<'a> From<&'a mut [f16]> for CpuStorageRefMut<'a> {
    fn from(value: &'a mut [f16]) -> Self {
        Self::F16(value)
    }
}

impl<'a> From<&'a mut [f32]> for CpuStorageRefMut<'a> {
    fn from(value: &'a mut [f32]) -> Self {
        Self::F32(value)
    }
}

impl<'a> From<&'a mut [i32]> for CpuStorageRefMut<'a> {
    fn from(value: &'a mut [i32]) -> Self {
        Self::I32(value)
    }
}

impl<'a> From<&'a mut [i64]> for CpuStorageRefMut<'a> {
    fn from(value: &'a mut [i64]) -> Self {
        Self::I64(value)
    }
}

#[derive(Clone, Copy, Debug)]
pub struct CpuDevice;

impl crate::Device for CpuDevice {
    type Slice = CpuStorage;
    type Func = Func;

    unsafe fn allocate_uninit(&self, dtype: DType, len: usize) -> Result<Self::Slice> {
        let slice = match dtype {
            DType::BF16 => CpuStorage::BF16(vec![bf16::ZERO; len]),
            DType::F16 => CpuStorage::F16(vec![f16::ZERO; len]),
            DType::F32 => CpuStorage::F32(vec![0f32; len]),
            DType::I32 => CpuStorage::I32(vec![0i32; len]),
            DType::I64 => CpuStorage::I64(vec![0i64; len]),
        };
        Ok(slice)
    }

    fn synchronize(&self) -> Result<()> {
        Ok(())
    }

    fn use_grid() -> bool {
        false
    }

    fn compile(&self, kernel: &crate::lang::ssa::Kernel, name: Option<&str>) -> Result<Self::Func> {
        let mut c_code = Vec::with_capacity(8192);
        // Compilation for the cpu runtime uses temporary files, so we use the pid to ensure that
        // there is no name collision.
        let pid = std::process::id();
        let kernel_id = KernelId::new().as_usize();
        let func_name = match name {
            Some(name) => format!("ugc_{name}_{pid}_{kernel_id}"),
            None => format!("ugc_{pid}_{kernel_id}"),
        };
        crate::cpu_code_gen::gen(&mut c_code, &func_name, kernel)?;
        self.compile_c(&c_code, func_name)
    }

    fn run(&self, f: &Self::Func, args: &mut [&mut Self::Slice]) -> Result<()> {
        use libloading::Symbol as S;
        use std::ffi::c_void;

        let func_name = f.func_name.as_bytes();
        // TODO: For the calls below to be safe, we should store the kernel signature in Func
        // and check that args matches it.
        match args {
            [] => {
                let symbol: S<extern "C" fn()> = unsafe { f.lib.get(func_name)? };
                symbol()
            }
            [a1] => {
                let symbol: S<extern "C" fn(*mut c_void)> = unsafe { f.lib.get(func_name)? };
                symbol(a1.as_mut_ptr())
            }
            [a1, a2] => {
                let symbol: S<extern "C" fn(*mut c_void, *mut c_void)> =
                    unsafe { f.lib.get(func_name)? };
                symbol(a1.as_mut_ptr(), a2.as_mut_ptr())
            }
            [a1, a2, a3] => {
                let symbol: S<extern "C" fn(*mut c_void, *mut c_void, *mut c_void)> =
                    unsafe { f.lib.get(func_name)? };
                symbol(a1.as_mut_ptr(), a2.as_mut_ptr(), a3.as_mut_ptr())
            }
            [a1, a2, a3, a4] => {
                let symbol: S<extern "C" fn(*mut c_void, *mut c_void, *mut c_void, *mut c_void)> =
                    unsafe { f.lib.get(func_name)? };
                symbol(a1.as_mut_ptr(), a2.as_mut_ptr(), a3.as_mut_ptr(), a4.as_mut_ptr())
            }
            [a1, a2, a3, a4, a5] => {
                let symbol: S<
                    extern "C" fn(*mut c_void, *mut c_void, *mut c_void, *mut c_void, *mut c_void),
                > = unsafe { f.lib.get(func_name)? };
                symbol(
                    a1.as_mut_ptr(),
                    a2.as_mut_ptr(),
                    a3.as_mut_ptr(),
                    a4.as_mut_ptr(),
                    a5.as_mut_ptr(),
                )
            }
            _ => crate::bail!("unsupported number of args for kernel {}", args.len()),
        }
        Ok(())
    }

    fn matmul(
        &self,
        dst: &mut Self::Slice,
        lhs: &Self::Slice,
        rhs: &Self::Slice,
        bmnk: (usize, usize, usize, usize),
        lhs_l: &Layout,
        rhs_l: &Layout,
    ) -> Result<()> {
        use CpuStorage::{F16, F32};
        let mm = MatMul(bmnk);
        let (dst_dt, lhs_dt, rhs_dt) = (dst.dtype(), lhs.dtype(), rhs.dtype());
        match (dst, lhs, rhs) {
            (F16(dst), F16(lhs), F16(rhs)) => mm.gemm(dst, lhs, lhs_l, rhs, rhs_l)?,
            (F32(dst), F32(lhs), F32(rhs)) => mm.gemm(dst, lhs, lhs_l, rhs, rhs_l)?,
            _ => {
                crate::bail!(
                    "incorrect dtypes for matmul, dst: {dst_dt:?}, lhs: {lhs_dt:?}, rhs: {rhs_dt:?}"
                )
            }
        }
        Ok(())
    }
}

impl crate::Slice for CpuStorage {
    type Device = CpuDevice;

    fn len(&self) -> usize {
        CpuStorage::len(self)
    }

    fn dtype(&self) -> crate::DType {
        CpuStorage::dtype(self)
    }

    fn device(&self) -> &Self::Device {
        &CpuDevice
    }

    fn copy_host_to_device<DT: crate::WithDType>(&mut self, src: &[DT]) -> Result<()> {
        use CpuStorage as S;
        use CpuStorageRef as C;
        let dtype = self.dtype();
        if src.len() != self.len() {
            crate::bail!("dtoh len mismatch, dst {}, len {}", self.len(), src.len())
        }
        match (self, DT::to_cpu_storage(src)) {
            (S::BF16(dst), C::BF16(src)) => dst.copy_from_slice(src),
            (S::F16(dst), C::F16(src)) => dst.copy_from_slice(src),
            (S::F32(dst), C::F32(src)) => dst.copy_from_slice(src),
            (S::I32(dst), C::I32(src)) => dst.copy_from_slice(src),
            (S::I64(dst), C::I64(src)) => dst.copy_from_slice(src),
            (_, _) => {
                crate::bail!("htod dtype mismatch, dst {dtype:?}, src {:?}", DT::DTYPE)
            }
        }
        Ok(())
    }

    fn copy_device_to_host<DT: crate::WithDType>(&self, dst: &mut [DT]) -> Result<()> {
        use CpuStorage as S;
        use CpuStorageRefMut as C;
        let dtype = self.dtype();
        if dst.len() != self.len() {
            crate::bail!("dtoh len mismatch, dst {}, len {}", dst.len(), self.len())
        }
        match (self, DT::to_cpu_storage_mut(dst)) {
            (S::BF16(src), C::BF16(dst)) => dst.copy_from_slice(src),
            (S::F16(src), C::F16(dst)) => dst.copy_from_slice(src),
            (S::F32(src), C::F32(dst)) => dst.copy_from_slice(src),
            (S::I32(src), C::I32(dst)) => dst.copy_from_slice(src),
            (S::I64(src), C::I64(dst)) => dst.copy_from_slice(src),
            (_, _) => crate::bail!("dtoh dtype mismatch, dst {:?}, src {dtype:?}", DT::DTYPE),
        }
        Ok(())
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct KernelId(usize);

impl KernelId {
    pub(crate) fn new() -> Self {
        // https://users.rust-lang.org/t/idiomatic-rust-way-to-generate-unique-id/33805
        use std::sync::atomic;
        static COUNTER: atomic::AtomicUsize = atomic::AtomicUsize::new(1);
        Self(COUNTER.fetch_add(1, atomic::Ordering::Relaxed))
    }

    pub fn as_usize(&self) -> usize {
        self.0
    }
}

pub struct Func {
    func_name: String,
    lib: libloading::Library,
}

impl Func {
    pub fn name(&self) -> &str {
        self.func_name.as_str()
    }

    #[allow(clippy::missing_safety_doc)]
    pub unsafe fn run0(&self) -> Result<()> {
        let func_name = self.func_name.as_bytes();
        let symbol: libloading::Symbol<unsafe extern "C" fn()> = self.lib.get(func_name)?;
        symbol();
        Ok(())
    }

    #[allow(clippy::missing_safety_doc)]
    pub unsafe fn run3<T>(&self, v1: &mut [T], v2: &mut [T], v3: &mut [T]) -> Result<()> {
        use std::ffi::c_void;

        let func_name = self.func_name.as_bytes();
        let symbol: libloading::Symbol<
            unsafe extern "C" fn(*mut c_void, *mut c_void, *mut c_void),
        > = self.lib.get(func_name)?;
        symbol(
            v1.as_mut_ptr() as *mut c_void,
            v2.as_mut_ptr() as *mut c_void,
            v3.as_mut_ptr() as *mut c_void,
        );
        Ok(())
    }
}

impl crate::CpuDevice {
    pub fn compile_c(&self, c_code: &[u8], func_name: String) -> Result<Func> {
        fn compile_inner(
            c_code: &[u8],
            func_name: String,
            tmp_c: &PathBuf,
            tmp_so: &PathBuf,
        ) -> Result<Func> {
            std::fs::write(tmp_c, c_code)?;
            // TODO: add some environment variable or other ways to set some flags.
            let output = std::process::Command::new("gcc")
                .arg(tmp_c)
                .args([
                    "-shared",
                    "-lm",
                    "-O3",
                    "-march=native",
                    "-ffast-math",
                    "-fomit-frame-pointer",
                    "-o",
                ])
                .arg(tmp_so)
                .output()?;

            if !output.status.success() {
                crate::bail!(
                    "compilation failed\nstdout:\n{}\nstderr:{}",
                    String::from_utf8_lossy(&output.stdout),
                    String::from_utf8_lossy(&output.stderr)
                )
            }
            let lib = unsafe { libloading::Library::new(tmp_so)? };
            Ok(Func { func_name, lib })
        }

        let tmp_dir = std::env::temp_dir();
        let tmp_c = tmp_dir.join(format!("{func_name}.c"));
        let tmp_so = tmp_dir.join(format!("{func_name}.so"));
        let result = compile_inner(c_code, func_name, &tmp_c, &tmp_so);
        // Ensure that the temporary files are cleaned up, even on failures.
        if !crate::utils::KEEP_TMP.with(|b| *b) {
            let _ = std::fs::remove_file(tmp_c);
            let _ = std::fs::remove_file(tmp_so);
        }
        result
    }
}

pub struct MatMul((usize, usize, usize, usize));

impl MatMul {
    fn striding_error(&self, lhs_l: &Layout, rhs_l: &Layout, msg: &'static str) -> Error {
        Error::MatMulUnexpectedStriding(Box::new(crate::error::MatMulUnexpectedStriding {
            lhs_l: lhs_l.clone(),
            rhs_l: rhs_l.clone(),
            bmnk: self.0,
            msg,
        }))
        .bt()
    }

    fn ab_skip(&self, lhs_l: &Layout, rhs_l: &Layout) -> Result<(usize, usize)> {
        let lhs_stride = lhs_l.strides();
        let rhs_stride = rhs_l.strides();
        let rank = lhs_stride.len();
        let (_b, m, n, k) = self.0;
        let a_skip: usize = match lhs_stride[..rank - 2] {
            [s1, stride] if s1 == stride * lhs_l.dims()[1] => stride,
            [_, stride] if lhs_l.dims()[0] == 1 => stride,
            [stride, _] if lhs_l.dims()[1] == 1 => stride,
            [stride] => stride,
            [] => m * k,
            _ => Err(self.striding_error(lhs_l, rhs_l, "non-contiguous lhs"))?,
        };
        let b_skip: usize = match rhs_stride[..rank - 2] {
            [s1, stride] if s1 == stride * rhs_l.dims()[1] => stride,
            [_, stride] if rhs_l.dims()[0] == 1 => stride,
            [stride, _] if rhs_l.dims()[1] == 1 => stride,
            [stride] => stride,
            [] => n * k,
            _ => Err(self.striding_error(lhs_l, rhs_l, "non-contiguous rhs"))?,
        };
        Ok((a_skip, b_skip))
    }

    pub fn gemm<T: crate::WithDType>(
        &self,
        dst: &mut [T],
        lhs: &[T],
        lhs_l: &Layout,
        rhs: &[T],
        rhs_l: &Layout,
    ) -> Result<()> {
        use gemm::{gemm, Parallelism};

        match T::DTYPE {
            DType::F16 | DType::F32 => {}
            _ => crate::bail!("unsupported dtype for gemm"),
        }

        let (b, m, n, k) = self.0;
        let lhs = &lhs[lhs_l.offset()..];
        let rhs = &rhs[rhs_l.offset()..];

        let lhs_strides = lhs_l.strides();
        let rhs_strides = rhs_l.strides();
        let rank = lhs_strides.len();
        let lhs_cs = lhs_strides[rank - 1];
        let lhs_rs = lhs_strides[rank - 2];

        let rhs_cs = rhs_strides[rank - 1];
        let rhs_rs = rhs_strides[rank - 2];

        let (a_skip, b_skip) = self.ab_skip(lhs_l, rhs_l)?;
        let c_skip: usize = m * n;

        let dst_shape: crate::Shape = (m, n).into();
        let dst_strides = dst_shape.stride_contiguous();
        let dst_rs = dst_strides[0];
        let dst_cs = dst_strides[1];

        let num_threads = crate::utils::get_num_threads();
        let parallelism =
            if num_threads > 1 { Parallelism::Rayon(num_threads) } else { Parallelism::None };
        for step in 0..b {
            let lhs_p = &lhs[step * a_skip..];
            let rhs_p = &rhs[step * b_skip..];
            let dst_p = &mut dst[step * c_skip..];
            unsafe {
                gemm(
                    /* m: usize = */ m,
                    /* n: usize = */ n,
                    /* k: usize = */ k,
                    /* dst: *mut T = */ dst_p.as_mut_ptr(),
                    /* dst_cs: isize = */ dst_cs as isize,
                    /* dst_rs: isize = */ dst_rs as isize,
                    /* read_dst: bool = */ false,
                    /* lhs: *const T = */ lhs_p.as_ptr(),
                    /* lhs_cs: isize = */ lhs_cs as isize,
                    /* lhs_rs: isize = */ lhs_rs as isize,
                    /* rhs: *const T = */ rhs_p.as_ptr(),
                    /* rhs_cs: isize = */ rhs_cs as isize,
                    /* rhs_rs: isize = */ rhs_rs as isize,
                    /* alpha: T = */ T::zero(),
                    /* beta: T = */ T::one(),
                    /* conj_dst: bool = */ false,
                    /* conj_lhs: bool = */ false,
                    /* conj_rhs: bool = */ false,
                    parallelism,
                )
            }
        }
        Ok(())
    }
}
