#![no_main]
#![no_std]

#[defmt::panic_handler]
#[no_mangle]
fn panic() -> ! {
    loop {}
}
