extern crate schnorrkel;

use wasm_bindgen::prelude::*;

#[wasm_bindgen]
pub fn get_signature() -> String {
    let seed = [
        157, 97, 177, 157, 239, 253, 90, 96, 186, 132, 74, 244, 146, 236, 44, 196, 68, 73, 197,
        105, 123, 50, 105, 25, 112, 59, 172, 3, 28, 174, 127, 96,
    ];

    let mini_key = schnorrkel::MiniSecretKey::from_bytes(&seed).unwrap();
    let keypair = mini_key.expand_to_keypair(schnorrkel::ExpansionMode::Ed25519);
    let _bytes = keypair.public.to_bytes();

    let message = "hello, world!";
    const SIGNING_CTX: &'static [u8] = b"substrate";

    let signature_bytes = keypair.sign_simple(SIGNING_CTX, message.as_bytes()).to_bytes();
    signature_bytes.iter().map(|byte| format!("{byte:02X}")).collect::<String>()
}

#[wasm_bindgen]
pub fn get_pubkey() -> String {
    let seed = [
        157, 97, 177, 157, 239, 253, 90, 96, 186, 132, 74, 244, 146, 236, 44, 196, 68, 73, 197,
        105, 123, 50, 105, 25, 112, 59, 172, 3, 28, 174, 127, 96,
    ];

    let mini_key = schnorrkel::MiniSecretKey::from_bytes(&seed).unwrap();
    let keypair = mini_key.expand_to_keypair(schnorrkel::ExpansionMode::Ed25519);
    let bytes = keypair.public.to_bytes();
    bytes.iter().map(|byte| format!("{byte:02X}")).collect::<String>()
}
