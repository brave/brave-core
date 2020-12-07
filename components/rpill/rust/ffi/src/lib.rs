#[no_mangle]
pub unsafe extern "C" fn exec_ffi() -> bool {
    rpill_lib::exec()
}
