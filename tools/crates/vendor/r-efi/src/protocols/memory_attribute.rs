//! Memory Attribute Protocol
//!
//! Provides an interface to abstract setting or getting of memory attributes in the UEFI environment.

pub const PROTOCOL_GUID: crate::base::Guid = crate::base::Guid::from_fields(
    0xf4560cf6,
    0x40ec,
    0x4b4a,
    0xa1,
    0x92,
    &[0xbf, 0x1d, 0x57, 0xd0, 0xb1, 0x89],
);

pub type GetMemoryAttributes = unsafe extern "efiapi" fn(
    *mut Protocol,
    crate::base::PhysicalAddress,
    u64,
    *mut u64,
) -> crate::base::Status;

pub type SetMemoryAttributes = unsafe extern "efiapi" fn(
    *mut Protocol,
    crate::base::PhysicalAddress,
    u64,
    u64,
) -> crate::base::Status;

pub type ClearMemoryAttributes = unsafe extern "efiapi" fn(
    *mut Protocol,
    crate::base::PhysicalAddress,
    u64,
    u64,
) -> crate::base::Status;

#[repr(C)]
pub struct Protocol {
    pub get_memory_attributes: GetMemoryAttributes,
    pub set_memory_attributes: SetMemoryAttributes,
    pub clear_memory_attributes: ClearMemoryAttributes,
}
