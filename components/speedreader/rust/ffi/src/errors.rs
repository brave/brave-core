use super::*;
use std::error::Error;

thread_local! {
    pub static LAST_ERROR: RefCell<Option<Box<dyn Error>>> = RefCell::new(None);
}

#[no_mangle]
pub extern "C" fn speedreader_take_last_error() -> *const CharBuf {
    let err = LAST_ERROR.with(|cell| cell.borrow_mut().take());

    CharBuf::opt_ptr(err.map(|e| e.to_string()))
}
