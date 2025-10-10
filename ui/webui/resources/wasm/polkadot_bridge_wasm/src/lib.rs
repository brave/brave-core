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

// 52707850D9298F5DFB0A3E5B23FCCA39EA286C6DEF2DB5716C996FB39DB6477C cxx

// 525afbbe4d6a301452db827c08b5be942acc0206aa7fd3044bd5535cb7cb160b4336d9b0bac89c52ae81f3a5250e5873d53370fa458da0750fd54a025181591c52707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c

const POLKADOT_TESTNET: &'static [u8] = b"\x1cwestend";
const POLKADOT_MAINNET: &'static [u8] = b"\x20polkadot";

// equivalent to the chaincode len
const JUNCTION_ID_LEN: usize = 32;

#[wasm_bindgen]
pub fn make_keypair(seed: Vec<u8>, is_testnet: bool) -> Vec<u8> {
    let key = schnorrkel::MiniSecretKey::from_bytes(&seed[..32]);
    let Ok(key) = key else {
        return Vec::new();
    };

    let chain_id = if is_testnet { POLKADOT_TESTNET } else { POLKADOT_MAINNET };

    let mut cc = [0_u8; JUNCTION_ID_LEN];
    cc[..chain_id.len()].copy_from_slice(chain_id);

    let keypair = key
        .expand_to_keypair(schnorrkel::ExpansionMode::Ed25519)
        .hard_derive_mini_secret_key(Some(schnorrkel::derive::ChainCode(cc)), b"")
        .0
        .expand_to_keypair(schnorrkel::ExpansionMode::Ed25519);

    keypair.to_bytes().iter().copied().collect()
}

#[wasm_bindgen]
pub fn derive_account_keypair(raw_keypair: Vec<u8>, account_index: u32) -> Vec<u8> {
    let mut cc = [0_u8; JUNCTION_ID_LEN];

    let bytes = &account_index.to_le_bytes();
    cc[..bytes.len()].copy_from_slice(bytes);

    let keypair = schnorrkel::keys::Keypair::from_bytes(&raw_keypair);
    let Ok(keypair) = keypair else {
        return Vec::new();
    };

    let keypair = keypair
        .hard_derive_mini_secret_key(Some(schnorrkel::derive::ChainCode(cc)), b"")
        .0
        .expand_to_keypair(schnorrkel::ExpansionMode::Ed25519);

    keypair.to_bytes().iter().copied().collect()
}
