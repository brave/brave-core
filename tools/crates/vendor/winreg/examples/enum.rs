// Copyright 2023, Igor Shaula
// Licensed under the MIT License <LICENSE or
// http://opensource.org/licenses/MIT>. This file
// may not be copied, modified, or distributed
// except according to those terms.
use std::io;
use winreg::{HKCR, HKLM};

fn main() -> io::Result<()> {
    println!("File extensions, registered in system:");
    for i in HKCR
        .enum_keys()
        .map(|x| x.unwrap())
        .filter(|x| x.starts_with('.'))
    {
        println!("{}", i);
    }

    let system = HKLM.open_subkey("HARDWARE\\DESCRIPTION\\System")?;
    for (name, value) in system.enum_values().map(|x| x.unwrap()) {
        println!("{} = {:?}", name, value);
    }

    Ok(())
}
