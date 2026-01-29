// consts.rs

// CTL* constants belong to libc crate but have not been added there yet.
// They will be removed from here once in the libc crate.
pub const CTL_MAXNAME: libc::c_uint = 24;

pub const CTLTYPE: libc::c_uint = 0xf; /* mask for the type */

pub const CTLTYPE_NODE: libc::c_uint = 1;
pub const CTLTYPE_INT: libc::c_uint = 2;
pub const CTLTYPE_STRING: libc::c_uint = 3;
pub const CTLTYPE_S64: libc::c_uint = 4;
pub const CTLTYPE_OPAQUE: libc::c_uint = 5;
pub const CTLTYPE_STRUCT: libc::c_uint = 5;
pub const CTLTYPE_UINT: libc::c_uint = 6;
pub const CTLTYPE_LONG: libc::c_uint = 7;
pub const CTLTYPE_ULONG: libc::c_uint = 8;
pub const CTLTYPE_U64: libc::c_uint = 9;
pub const CTLTYPE_U8: libc::c_uint = 10;
pub const CTLTYPE_U16: libc::c_uint = 11;
pub const CTLTYPE_S8: libc::c_uint = 12;
pub const CTLTYPE_S16: libc::c_uint = 13;
pub const CTLTYPE_S32: libc::c_uint = 14;
pub const CTLTYPE_U32: libc::c_uint = 15;

pub const CTLFLAG_RD: libc::c_uint = 0x80000000;
pub const CTLFLAG_WR: libc::c_uint = 0x40000000;
pub const CTLFLAG_RW: libc::c_uint = 0x80000000 | 0x40000000;
pub const CTLFLAG_DORMANT: libc::c_uint = 0x20000000;
pub const CTLFLAG_ANYBODY: libc::c_uint = 0x10000000;
pub const CTLFLAG_SECURE: libc::c_uint = 0x08000000;
pub const CTLFLAG_PRISON: libc::c_uint = 0x04000000;
pub const CTLFLAG_DYN: libc::c_uint = 0x02000000;
pub const CTLFLAG_SKIP: libc::c_uint = 0x01000000;
pub const CTLFLAG_TUN: libc::c_uint = 0x00080000;
pub const CTLFLAG_RDTUN: libc::c_uint = CTLFLAG_RD | CTLFLAG_TUN;
pub const CTLFLAG_RWTUN: libc::c_uint = CTLFLAG_RW | CTLFLAG_TUN;
pub const CTLFLAG_MPSAFE: libc::c_uint = 0x00040000;
pub const CTLFLAG_VNET: libc::c_uint = 0x00020000;
pub const CTLFLAG_DYING: libc::c_uint = 0x00010000;
pub const CTLFLAG_CAPRD: libc::c_uint = 0x00008000;
pub const CTLFLAG_CAPWR: libc::c_uint = 0x00004000;
pub const CTLFLAG_STATS: libc::c_uint = 0x00002000;
pub const CTLFLAG_NOFETCH: libc::c_uint = 0x00001000;
pub const CTLFLAG_CAPRW: libc::c_uint = CTLFLAG_CAPRD | CTLFLAG_CAPWR;
pub const CTLFLAG_SECURE1: libc::c_uint = 134217728;
pub const CTLFLAG_SECURE2: libc::c_uint = 135266304;
pub const CTLFLAG_SECURE3: libc::c_uint = 136314880;

pub const CTLMASK_SECURE: libc::c_uint = 15728640;
pub const CTLSHIFT_SECURE: libc::c_uint = 20;
