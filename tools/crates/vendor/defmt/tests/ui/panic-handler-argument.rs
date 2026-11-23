#![no_main]
#![no_std]

#[defmt::panic_handler]
fn foo(x: bool) -> ! { info!("{:?}", x); loop {} }
