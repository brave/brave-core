extern crate schnorrkel;
extern crate cxx;

#[allow(unused)]
#[allow(unsafe_op_in_unsafe_fn)]
#[cxx::bridge(namespace = brave_wallet::polkadot)]
mod ffi {
    extern "Rust" {
        type CxxSr25519KeyPair;
        type CxxSr25519KeyPairResult;

        fn generate_sr25519_keypair_from_seed(bytes: &[u8],) -> Box<CxxSr25519KeyPairResult>;
        fn use_keypair(p: &Box<CxxSr25519KeyPairResult>);
        fn is_ok(self: &CxxSr25519KeyPairResult) -> bool;
        fn unwrap(self: &mut CxxSr25519KeyPairResult) -> Box<CxxSr25519KeyPair>;
    }
}

#[derive(Clone, Debug)]
pub enum Error {
    Schnorrkel(schnorrkel::SignatureError)
}

#[derive(Clone)]
struct CxxSr25519KeyPair(Sr25519KeyPair);

#[derive(Clone)]
struct CxxSr25519KeyPairResult(Result<Option<CxxSr25519KeyPair>, Error>);

#[derive(Clone)]
struct Sr25519KeyPair {
    kp: schnorrkel::Keypair,
}

fn generate_sr25519_keypair_from_seed(bytes: &[u8],) -> Box<CxxSr25519KeyPairResult> {
    let kp = schnorrkel::MiniSecretKey::from_bytes(bytes)
        .map(|kp| Some(CxxSr25519KeyPair(Sr25519KeyPair {kp: kp.expand_to_keypair(schnorrkel::ExpansionMode::Ed25519)})))
        .map_err(|e| Error::Schnorrkel(e));

    Box::new(CxxSr25519KeyPairResult(kp))
}

fn use_keypair(p: &Box<CxxSr25519KeyPairResult>) {
    assert!(<std::result::Result<Option<CxxSr25519KeyPair>, Error> as Clone>::clone(&(&p.0)).unwrap().unwrap().0.kp.secret.to_bytes().len() > 0);
}

impl CxxSr25519KeyPairResult {
    fn is_ok(self: &CxxSr25519KeyPairResult) -> bool {
        match &self.0 {
            Ok(_) => true,
            _ => false,
        }
    }

    fn unwrap(self: &mut CxxSr25519KeyPairResult) -> Box<CxxSr25519KeyPair>
    {
        Box::new(self.0.as_mut().unwrap().take().unwrap())
    }
}
