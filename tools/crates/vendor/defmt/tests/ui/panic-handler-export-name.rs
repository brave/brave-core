#![no_main]
#![no_std]

#[defmt::panic_handler]
#[export_name = "hello"]
fn foo() -> ! {
    loop {}
}
