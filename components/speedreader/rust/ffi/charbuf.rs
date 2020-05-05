use super::*;

// NOTE: we don't use CStr and CString as the transfer type because UTF8
// strings comming from both sides can contain interior NULLs.
#[repr(C)]
pub struct CharBuf {
    data: *const c_char,
    len: size_t,
}

impl CharBuf {
    pub fn new(string: String) -> Self {
        CharBuf {
            len: string.len(),
            data: Box::into_raw(string.into_boxed_str()) as *const c_char,
        }
    }

    #[inline]
    pub fn opt_ptr(string: Option<String>) -> *const Self {
        match string {
            Some(string) => to_ptr(Self::new(string)),
            None => ptr::null(),
        }
    }
}

impl Drop for CharBuf {
    fn drop(&mut self) {
        let bytes = unsafe { slice::from_raw_parts_mut(self.data as *mut c_char, self.len) };

        drop(unsafe { Box::from_raw(bytes) });
    }
}

#[no_mangle]
pub extern "C" fn speedreader_str_free(string: CharBuf) {
    drop(string);
}
