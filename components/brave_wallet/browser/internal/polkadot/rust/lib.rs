extern crate cxx;
extern crate schnorrkel;

#[allow(unused)]
#[allow(unsafe_op_in_unsafe_fn)]
#[cxx::bridge(namespace = brave_wallet::schnorrkel)]
mod ffi {
    extern "Rust" {
        type CxxSchnorrkelKeyPair;
        type CxxSchnorrkelKeyPairResult;

        fn generate_sr25519_keypair_from_seed(bytes: &[u8]) -> Box<CxxSchnorrkelKeyPairResult>;
        fn get_public_key(self: &CxxSchnorrkelKeyPair) -> [u8; 32];
        fn is_ok(self: &CxxSchnorrkelKeyPairResult) -> bool;
        fn unwrap(self: &mut CxxSchnorrkelKeyPairResult) -> Box<CxxSchnorrkelKeyPair>;
    }
}

#[derive(Clone, Debug)]
pub enum Error {
    Schnorrkel(schnorrkel::SignatureError),
}

#[derive(Clone)]
struct CxxSchnorrkelKeyPair(SchnorrkelKeyPair);

#[derive(Clone)]
struct CxxSchnorrkelKeyPairResult(Result<Option<CxxSchnorrkelKeyPair>, Error>);

#[derive(Clone)]
struct SchnorrkelKeyPair {
    kp: schnorrkel::Keypair,
}

fn generate_sr25519_keypair_from_seed(bytes: &[u8]) -> Box<CxxSchnorrkelKeyPairResult> {
    let kp = schnorrkel::MiniSecretKey::from_bytes(bytes)
        .map(|kp| {
            Some(CxxSchnorrkelKeyPair(SchnorrkelKeyPair {
                kp: kp.expand_to_keypair(schnorrkel::ExpansionMode::Ed25519),
            }))
        })
        .map_err(|e| Error::Schnorrkel(e));

    Box::new(CxxSchnorrkelKeyPairResult(kp))
}

// fn use_keypair(p: &Box<CxxSchnorrkelKeyPairResult>) {
//     assert!(
//         <std::result::Result<Option<CxxSchnorrkelKeyPair>, Error> as
// Clone>::clone(&(&p.0))             .unwrap()
//             .unwrap()
//             .0
//             .kp
//             .secret
//             .to_bytes()
//             .len()
//             > 0
//     );
// }

impl CxxSchnorrkelKeyPair {
    fn get_public_key(self: &CxxSchnorrkelKeyPair) -> [u8; 32] {
        self.0.kp.public.to_bytes()
    }
}

impl CxxSchnorrkelKeyPairResult {
    fn is_ok(self: &CxxSchnorrkelKeyPairResult) -> bool {
        match &self.0 {
            Ok(_) => true,
            _ => false,
        }
    }

    fn unwrap(self: &mut CxxSchnorrkelKeyPairResult) -> Box<CxxSchnorrkelKeyPair> {
        Box::new(self.0.as_mut().unwrap().take().unwrap())
    }
}
