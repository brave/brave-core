use super::*;
use std::error::Error;

thread_local! {
    pub static SPEEDREADER_LAST_ERROR: RefCell<Option<Box<dyn Error>>> = RefCell::new(None);
}

#[no_mangle]
pub extern "C" fn take_last_speedreader_error() -> *const CharBuf {
    let err = SPEEDREADER_LAST_ERROR.with(|cell| cell.borrow_mut().take());

    CharBuf::opt_ptr(err.map(|e| e.to_string()))
}
