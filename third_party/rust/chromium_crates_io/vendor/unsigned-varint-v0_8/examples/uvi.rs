extern crate hex;
extern crate unsigned_varint;

use std::{env, process};
use unsigned_varint::{decode, encode};

pub fn main() {
    let mut args = env::args().skip(1);

    let mode =
        if let Some(s) = args.next() {
            s
        } else {
            println!("usage: -d <hex-encoded-string> | -e <number>");
            process::exit(1)
        };

    match (mode.as_ref(), args.next()) {
        ("-d", Some(xs)) => {
            let v =
                if let Ok(b) = hex::decode(&xs) {
                    b
                } else {
                    println!("failed to decode hex string");
                    process::exit(1)
                };
            match decode::u128(&v) {
                Ok((n, _)) => println!("{}", n),
                Err(e) => {
                    println!("{}", e);
                    process::exit(2)
                }
            }
        }
        ("-e", Some(xs)) => {
            match xs.parse() {
                Ok(n) => {
                    let mut buf = encode::u128_buffer();
                    let bytes = encode::u128(n, &mut buf);
                    println!("{}", hex::encode(&bytes))
                }
                Err(e) => {
                    println!("{}", e);
                    process::exit(3)
                }
            }
        }
        _ => {
            println!("usage: -d <hex-encoded-string> | -e <number>");
            process::exit(1)
        }
    }
}
