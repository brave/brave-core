use super::Io;

/// MMIO using pointer instead of wrapped type
pub struct MmioPtr<T> {
    ptr: *mut T,
}

impl<T> MmioPtr<T> {
    //TODO: reads and writes are unsafe, not new.
    /// Creates a `MmioPtr`.
    pub unsafe fn new(ptr: *mut T) -> Self {
        Self { ptr }
    }

    /// Creates a const pointer from a `MmioPtr`.
    pub const fn as_ptr(&self) -> *const T {
        self.ptr
    }

    /// Creates a mutable pointer from a `MmioPtr`.
    pub const fn as_mut_ptr(&mut self) -> *mut T {
        self.ptr
    }
}

// Generic implementation (WARNING: requires aligned pointers!)
#[cfg(not(any(target_arch = "x86", target_arch = "x86_64")))]
impl<T> Io for MmioPtr<T>
where
    T: Copy
        + PartialEq
        + core::ops::BitAnd<Output = T>
        + core::ops::BitOr<Output = T>
        + core::ops::Not<Output = T>,
{
    type Value = T;

    fn read(&self) -> T {
        unsafe { core::ptr::read_volatile(self.ptr) }
    }

    fn write(&mut self, value: T) {
        unsafe { core::ptr::write_volatile(self.ptr, value) };
    }
}

// x86 u8 implementation
#[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
impl Io for MmioPtr<u8> {
    type Value = u8;

    fn read(&self) -> Self::Value {
        unsafe {
            let value: Self::Value;
            core::arch::asm!(
                "mov {}, [{}]",
                out(reg_byte) value,
                in(reg) self.ptr
            );
            value
        }
    }

    fn write(&mut self, value: Self::Value) {
        unsafe {
            core::arch::asm!(
                "mov [{}], {}",
                in(reg) self.ptr,
                in(reg_byte) value,
            );
        }
    }
}

// x86 u16 implementation
#[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
impl Io for MmioPtr<u16> {
    type Value = u16;

    fn read(&self) -> Self::Value {
        unsafe {
            let value: Self::Value;
            core::arch::asm!(
                "mov {:x}, [{}]",
                out(reg) value,
                in(reg) self.ptr
            );
            value
        }
    }

    fn write(&mut self, value: Self::Value) {
        unsafe {
            core::arch::asm!(
                "mov [{}], {:x}",
                in(reg) self.ptr,
                in(reg) value,
            );
        }
    }
}

// x86 u32 implementation
#[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
impl Io for MmioPtr<u32> {
    type Value = u32;

    fn read(&self) -> Self::Value {
        unsafe {
            let value: Self::Value;
            core::arch::asm!(
                "mov {:e}, [{}]",
                out(reg) value,
                in(reg) self.ptr
            );
            value
        }
    }

    fn write(&mut self, value: Self::Value) {
        unsafe {
            core::arch::asm!(
                "mov [{}], {:e}",
                in(reg) self.ptr,
                in(reg) value,
            );
        }
    }
}

// x86 u64 implementation (x86_64 only)
#[cfg(target_arch = "x86_64")]
impl Io for MmioPtr<u64> {
    type Value = u64;

    fn read(&self) -> Self::Value {
        unsafe {
            let value: Self::Value;
            core::arch::asm!(
                "mov {:r}, [{}]",
                out(reg) value,
                in(reg) self.ptr
            );
            value
        }
    }

    fn write(&mut self, value: Self::Value) {
        unsafe {
            core::arch::asm!(
                "mov [{}], {:r}",
                in(reg) self.ptr,
                in(reg) value,
            );
        }
    }
}
