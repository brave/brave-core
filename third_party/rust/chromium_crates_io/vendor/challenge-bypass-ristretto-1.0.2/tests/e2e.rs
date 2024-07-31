extern crate challenge_bypass_ristretto;
extern crate hmac;
extern crate rand;
extern crate serde;
extern crate sha2;

use hmac::Hmac;
use rand::rngs::OsRng;
use sha2::Sha512;

#[cfg(feature = "serde_base64")]
use serde::{Deserialize, Serialize};

use challenge_bypass_ristretto::errors::*;
use challenge_bypass_ristretto::voprf::*;

type HmacSha512 = Hmac<Sha512>;

#[cfg_attr(feature = "serde_base64", derive(Serialize, Deserialize))]
struct SigningRequest {
    blinded_tokens: Vec<BlindedToken>,
}

#[cfg_attr(feature = "serde_base64", derive(Serialize, Deserialize))]
struct SigningResponse {
    signed_tokens: Vec<SignedToken>,
    public_key: PublicKey,
    batch_proof: BatchDLEQProof,
}

#[cfg_attr(feature = "serde_base64", derive(Serialize, Deserialize))]
struct RedeemRequest {
    preimages: Vec<TokenPreimage>,
    verification_signatures: Vec<VerificationSignature>,
    payload: Vec<u8>,
}

struct Client {
    tokens: Vec<Token>,
    blinded_tokens: Vec<BlindedToken>,
    unblinded_tokens: Vec<UnblindedToken>,
}

#[cfg(not(feature = "merlin"))]
impl Client {
    fn create_tokens(&mut self, n: u8) -> SigningRequest {
        let mut rng = OsRng;

        for _i in 0..n {
            // client prepares a random token and blinding scalar
            let token = Token::random::<Sha512, OsRng>(&mut rng);

            // client blinds the token
            let blinded_token = token.blind();

            // stores the token in it's local state
            self.tokens.push(token);
            self.blinded_tokens.push(blinded_token);
        }

        // and sends the blinded token to the server in a signing request
        SigningRequest {
            blinded_tokens: self.blinded_tokens.clone(),
        }
    }

    fn store_signed_tokens(&mut self, resp: SigningResponse) -> Result<(), TokenError> {
        self.unblinded_tokens
            .append(&mut resp.batch_proof.verify_and_unblind::<Sha512, _>(
                &self.tokens,
                &self.blinded_tokens,
                &resp.signed_tokens,
                &resp.public_key,
            )?);

        assert_eq!(self.tokens.len(), self.unblinded_tokens.len());
        Ok(())
    }

    fn redeem_tokens(&self) -> RedeemRequest {
        let payload = b"test message".to_vec();

        let mut preimages: Vec<TokenPreimage> = vec![];
        let mut verification_signatures: Vec<VerificationSignature> = vec![];

        for unblinded_token in self.unblinded_tokens.iter() {
            preimages.push(unblinded_token.t);

            // client derives the shared key from the unblinded token
            let verification_key = unblinded_token.derive_verification_key::<Sha512>();

            // client signs a message using the shared key
            verification_signatures.push(verification_key.sign::<HmacSha512>(&payload));
        }

        RedeemRequest {
            preimages,
            verification_signatures,
            payload,
        }
    }
}

struct Server {
    signing_key: SigningKey,
    spent_tokens: Vec<TokenPreimage>,
}

#[cfg(not(feature = "merlin"))]
impl Server {
    fn sign_tokens(&self, req: SigningRequest) -> SigningResponse {
        let mut rng = OsRng;

        let public_key = self.signing_key.public_key;

        let signed_tokens: Vec<SignedToken> = req
            .blinded_tokens
            .iter()
            .filter_map(|t| self.signing_key.sign(t).ok())
            .collect();

        let batch_proof = BatchDLEQProof::new::<Sha512, OsRng>(
            &mut rng,
            &req.blinded_tokens,
            &signed_tokens,
            &self.signing_key,
        )
        .unwrap();

        SigningResponse {
            signed_tokens,
            public_key,
            batch_proof,
        }
    }

    fn redeem_tokens(&mut self, req: &RedeemRequest) {
        for (preimage, client_sig) in req.preimages.iter().zip(req.verification_signatures.iter()) {
            // the server checks that the preimage has not previously been speant
            assert!(!self.spent_tokens.contains(preimage));

            // server derives the unblinded token using it's key and the clients token preimage
            let unblinded_token = self.signing_key.rederive_unblinded_token(preimage);

            // server derives the shared key from the unblinded token
            let verification_key = unblinded_token.derive_verification_key::<Sha512>();

            // server signs the same message using the shared key
            let sig = verification_key.sign::<HmacSha512>(&req.payload);

            // the server compares the client signature to it's own
            assert!(*client_sig == sig);

            // the server marks the token as spent
            self.spent_tokens.push(*preimage);
        }
    }
}

#[test]
#[cfg(not(feature = "merlin"))]
fn e2e_works() {
    let mut rng = OsRng;
    let signing_key = SigningKey::random(&mut rng);

    let mut client = Client {
        tokens: Vec::new(),
        blinded_tokens: Vec::new(),
        unblinded_tokens: Vec::new(),
    };
    let mut server = Server {
        signing_key,
        spent_tokens: Vec::new(),
    };

    let signing_req = client.create_tokens(10);

    let signing_resp = server.sign_tokens(signing_req);
    client.store_signed_tokens(signing_resp).unwrap();

    let redeem_request = client.redeem_tokens();
    server.redeem_tokens(&redeem_request);
}

#[cfg(feature = "serde_base64")]
#[test]
fn e2e_serde_works() {
    let mut rng = OsRng;
    let signing_key = SigningKey::random(&mut rng);

    let mut client = Client {
        tokens: Vec::new(),
        blinded_tokens: Vec::new(),
        unblinded_tokens: Vec::new(),
    };
    let mut server = Server {
        signing_key,
        spent_tokens: Vec::new(),
    };

    let signing_req = client.create_tokens(10);

    // serde roundtrip
    let signing_req = serde_json::to_string(&signing_req).unwrap();
    let signing_req: SigningRequest = serde_json::from_str(&signing_req).unwrap();

    let signing_resp = server.sign_tokens(signing_req);

    // serde roundtrip
    let signing_resp = serde_json::to_string(&signing_resp).unwrap();
    let signing_resp: SigningResponse = serde_json::from_str(&signing_resp).unwrap();

    client.store_signed_tokens(signing_resp).unwrap();

    let redeem_request = client.redeem_tokens();

    // serde roundtrip
    let redeem_request = serde_json::to_string(&redeem_request).unwrap();
    let redeem_request: RedeemRequest = serde_json::from_str(&redeem_request).unwrap();

    server.redeem_tokens(&redeem_request);
}
