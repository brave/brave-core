use crate::prelude::*;

pub(crate) const _ALIGNBYTES: usize = mem::size_of::<c_int>() - 1;

pub const _MAX_PAGE_SHIFT: u32 = 12;
