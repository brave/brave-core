use brave_miracl::bn254::{
    big::{self, BIG},
    ecp::ECP,
    ecp2::ECP2,
    fp2::FP2,
};

use super::CredentialError;

pub const BIG_SIZE: usize = big::MODBYTES;
pub const ECP_SIZE: usize = BIG_SIZE * 2 + 1;
pub const ECP2_COMPAT_SIZE: usize = BIG_SIZE * 4;

pub const ECP_PROOF_SIZE: usize = BIG_SIZE * 2;
pub const JOIN_REQUEST_SIZE: usize = ECP_SIZE + ECP_PROOF_SIZE;
pub const USER_CREDENTIALS_SIZE: usize = ECP_SIZE * 4;
pub const JOIN_RESPONSE_SIZE: usize = USER_CREDENTIALS_SIZE + ECP_PROOF_SIZE;
pub const GROUP_PUBLIC_KEY_SIZE: usize = ECP2_COMPAT_SIZE * 2 + BIG_SIZE * 4;
pub const SIGNATURE_SIZE: usize = ECP_SIZE * 5 + ECP_PROOF_SIZE;

pub struct JoinRequest {
    pub(crate) q: ECP, // G1 ** gsk

    pub(crate) proof: ECPProof,
}

pub struct JoinResponse {
    pub(crate) cred: UserCredentials,
    pub(crate) proof: ECPProof,
}

pub struct UserCredentials {
    pub(crate) a: ECP,
    pub(crate) b: ECP,
    pub(crate) c: ECP,
    pub(crate) d: ECP,
}

pub struct Signature {
    pub(crate) a: ECP,
    pub(crate) b: ECP,
    pub(crate) c: ECP,
    pub(crate) d: ECP,
    pub(crate) nym: ECP,

    pub(crate) proof: ECPProof,
}

pub struct GroupPublicKey {
    pub(crate) x: ECP2, // G2 ** x
    pub(crate) y: ECP2, // G2 ** y

    // ZK of discrete-log knowledge for X and Y
    pub(crate) cx: BIG,
    pub(crate) sx: BIG,
    pub(crate) cy: BIG,
    pub(crate) sy: BIG,
}

pub struct ECPProof {
    pub(crate) c: BIG,
    pub(crate) s: BIG,
}

pub struct CredentialBIG(pub(crate) BIG);

pub struct StartJoinResult {
    pub gsk: CredentialBIG,
    pub join_msg: JoinRequest,
}

pub(crate) fn ecp_from_bytes(bytes: &[u8]) -> Result<ECP, CredentialError> {
    if bytes.len() != ECP_SIZE {
        return Err(CredentialError::BadECP);
    }
    Ok(ECP::frombytes(bytes))
}

pub(crate) fn ecp2_from_compat_bytes(bytes: &[u8]) -> Result<ECP2, CredentialError> {
    if bytes.len() != ECP2_COMPAT_SIZE {
        return Err(CredentialError::BadECP2);
    }
    let x = FP2::new_bigs(
        &big_from_bytes(&bytes[..BIG_SIZE])?,
        &big_from_bytes(&bytes[BIG_SIZE..BIG_SIZE * 2])?,
    );
    let y = FP2::new_bigs(
        &big_from_bytes(&bytes[BIG_SIZE * 2..BIG_SIZE * 3])?,
        &big_from_bytes(&bytes[BIG_SIZE * 3..BIG_SIZE * 4])?,
    );
    Ok(ECP2::new_fp2s(&x, &y))
}

pub(crate) fn ecp2_to_compat_bytes(point: &ECP2) -> [u8; ECP2_COMPAT_SIZE] {
    let mut result = [0u8; ECP2_COMPAT_SIZE];

    let mut x = point.getx();
    let mut y = point.gety();

    x.geta().tobytes(&mut result[..BIG_SIZE]);
    x.getb().tobytes(&mut result[BIG_SIZE..BIG_SIZE * 2]);
    y.geta().tobytes(&mut result[BIG_SIZE * 2..BIG_SIZE * 3]);
    y.getb().tobytes(&mut result[BIG_SIZE * 3..]);

    result
}

pub(crate) fn big_from_bytes(bytes: &[u8]) -> Result<BIG, CredentialError> {
    if bytes.len() != BIG_SIZE {
        return Err(CredentialError::BadBIG);
    }
    Ok(BIG::frombytes(bytes))
}

impl TryFrom<&[u8]> for JoinResponse {
    type Error = CredentialError;

    fn try_from(bytes: &[u8]) -> Result<Self, Self::Error> {
        if bytes.len() != JOIN_RESPONSE_SIZE {
            return Err(CredentialError::BadJoinResponse);
        }

        Ok(JoinResponse {
            cred: bytes[..USER_CREDENTIALS_SIZE].try_into()?,
            proof: bytes[USER_CREDENTIALS_SIZE..].try_into()?,
        })
    }
}

impl TryFrom<&[u8]> for UserCredentials {
    type Error = CredentialError;

    fn try_from(bytes: &[u8]) -> Result<Self, Self::Error> {
        if bytes.len() != USER_CREDENTIALS_SIZE {
            return Err(CredentialError::BadUserCredentials);
        }
        Ok(UserCredentials {
            a: ecp_from_bytes(&bytes[..ECP_SIZE])?,
            b: ecp_from_bytes(&bytes[ECP_SIZE..ECP_SIZE * 2])?,
            c: ecp_from_bytes(&bytes[ECP_SIZE * 2..ECP_SIZE * 3])?,
            d: ecp_from_bytes(&bytes[ECP_SIZE * 3..ECP_SIZE * 4])?,
        })
    }
}

impl TryFrom<&[u8]> for GroupPublicKey {
    type Error = CredentialError;

    fn try_from(bytes: &[u8]) -> Result<Self, Self::Error> {
        if bytes.len() != GROUP_PUBLIC_KEY_SIZE {
            return Err(CredentialError::GroupPublicKeyLength);
        }

        let big_start = ECP2_COMPAT_SIZE * 2;

        Ok(GroupPublicKey {
            x: ecp2_from_compat_bytes(&bytes[..ECP2_COMPAT_SIZE])?,
            y: ecp2_from_compat_bytes(&bytes[ECP2_COMPAT_SIZE..ECP2_COMPAT_SIZE * 2])?,
            cx: big_from_bytes(&bytes[big_start..big_start + BIG_SIZE])?,
            sx: big_from_bytes(&bytes[big_start + BIG_SIZE..big_start + BIG_SIZE * 2])?,
            cy: big_from_bytes(&bytes[big_start + BIG_SIZE * 2..big_start + BIG_SIZE * 3])?,
            sy: big_from_bytes(&bytes[big_start + BIG_SIZE * 3..big_start + BIG_SIZE * 4])?,
        })
    }
}

impl CredentialBIG {
    pub fn to_bytes(&self) -> [u8; BIG_SIZE] {
        let mut bytes = [0u8; BIG_SIZE];
        self.0.tobytes(&mut bytes);
        bytes
    }
}

impl TryFrom<&[u8]> for CredentialBIG {
    type Error = CredentialError;

    fn try_from(bytes: &[u8]) -> Result<Self, Self::Error> {
        Ok(Self(big_from_bytes(bytes)?))
    }
}

impl JoinRequest {
    pub fn to_bytes(&self) -> [u8; JOIN_REQUEST_SIZE] {
        let mut result = [0u8; JOIN_REQUEST_SIZE];
        self.q.tobytes(&mut result, false);
        result[ECP_SIZE..].copy_from_slice(&self.proof.to_bytes());
        result
    }
}

impl UserCredentials {
    pub fn to_bytes(&self) -> [u8; USER_CREDENTIALS_SIZE] {
        let mut result = [0u8; USER_CREDENTIALS_SIZE];
        self.a.tobytes(&mut result[..ECP_SIZE], false);
        self.b.tobytes(&mut result[ECP_SIZE..ECP_SIZE * 2], false);
        self.c
            .tobytes(&mut result[ECP_SIZE * 2..ECP_SIZE * 3], false);
        self.d
            .tobytes(&mut result[ECP_SIZE * 3..ECP_SIZE * 4], false);
        result
    }
}

impl ECPProof {
    pub fn to_bytes(&self) -> [u8; ECP_PROOF_SIZE] {
        let mut result = [0u8; ECP_PROOF_SIZE];
        self.c.tobytes(&mut result[..BIG_SIZE]);
        self.s.tobytes(&mut result[BIG_SIZE..]);
        result
    }
}

impl TryFrom<&[u8]> for ECPProof {
    type Error = CredentialError;

    fn try_from(bytes: &[u8]) -> Result<Self, Self::Error> {
        if bytes.len() != ECP_PROOF_SIZE {
            return Err(CredentialError::BadECPProof);
        }

        Ok(ECPProof {
            c: big_from_bytes(&bytes[0..BIG_SIZE])?,
            s: big_from_bytes(&bytes[BIG_SIZE..])?,
        })
    }
}

impl Signature {
    pub fn to_bytes(&self) -> [u8; SIGNATURE_SIZE] {
        let mut result = [0u8; SIGNATURE_SIZE];
        self.a.tobytes(&mut result[..ECP_SIZE], false);
        self.b.tobytes(&mut result[ECP_SIZE..ECP_SIZE * 2], false);
        self.c
            .tobytes(&mut result[ECP_SIZE * 2..ECP_SIZE * 3], false);
        self.d
            .tobytes(&mut result[ECP_SIZE * 3..ECP_SIZE * 4], false);
        self.nym
            .tobytes(&mut result[ECP_SIZE * 4..ECP_SIZE * 5], false);
        result[ECP_SIZE * 5..].copy_from_slice(&self.proof.to_bytes());
        result
    }
}
