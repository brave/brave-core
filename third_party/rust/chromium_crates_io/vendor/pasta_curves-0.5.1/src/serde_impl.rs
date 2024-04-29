use ff::PrimeField;
use group::GroupEncoding;
use serde_crate::{
    de::Error as DeserializeError, Deserialize, Deserializer, Serialize, Serializer,
};

use crate::{
    curves::{Ep, EpAffine, Eq, EqAffine},
    fields::{Fp, Fq},
    group::Curve,
};

/// Serializes bytes to human readable or compact representation.
///
/// Depending on whether the serializer is a human readable one or not, the bytes are either
/// encoded as a hex string or a list of bytes.
fn serialize_bytes<S: Serializer>(bytes: [u8; 32], s: S) -> Result<S::Ok, S::Error> {
    if s.is_human_readable() {
        hex::serde::serialize(bytes, s)
    } else {
        bytes.serialize(s)
    }
}

/// Deserialize bytes from human readable or compact representation.
///
/// Depending on whether the deserializer is a human readable one or not, the bytes are either
/// decoded from a hex string or a list of bytes.
fn deserialize_bytes<'de, D: Deserializer<'de>>(d: D) -> Result<[u8; 32], D::Error> {
    if d.is_human_readable() {
        hex::serde::deserialize(d)
    } else {
        <[u8; 32]>::deserialize(d)
    }
}

impl Serialize for Fp {
    fn serialize<S: Serializer>(&self, s: S) -> Result<S::Ok, S::Error> {
        serialize_bytes(self.to_repr(), s)
    }
}

impl<'de> Deserialize<'de> for Fp {
    fn deserialize<D: Deserializer<'de>>(d: D) -> Result<Self, D::Error> {
        let bytes = deserialize_bytes(d)?;
        match Fp::from_repr(bytes).into() {
            Some(fq) => Ok(fq),
            None => Err(D::Error::custom(
                "deserialized bytes don't encode a Pallas field element",
            )),
        }
    }
}

impl Serialize for Fq {
    fn serialize<S: Serializer>(&self, s: S) -> Result<S::Ok, S::Error> {
        serialize_bytes(self.to_repr(), s)
    }
}

impl<'de> Deserialize<'de> for Fq {
    fn deserialize<D: Deserializer<'de>>(d: D) -> Result<Self, D::Error> {
        let bytes = deserialize_bytes(d)?;
        match Fq::from_repr(bytes).into() {
            Some(fq) => Ok(fq),
            None => Err(D::Error::custom(
                "deserialized bytes don't encode a Vesta field element",
            )),
        }
    }
}

impl Serialize for EpAffine {
    fn serialize<S: Serializer>(&self, s: S) -> Result<S::Ok, S::Error> {
        serialize_bytes(self.to_bytes(), s)
    }
}

impl<'de> Deserialize<'de> for EpAffine {
    fn deserialize<D: Deserializer<'de>>(d: D) -> Result<Self, D::Error> {
        let bytes = deserialize_bytes(d)?;
        match EpAffine::from_bytes(&bytes).into() {
            Some(ep_affine) => Ok(ep_affine),
            None => Err(D::Error::custom(
                "deserialized bytes don't encode a Pallas curve point",
            )),
        }
    }
}

impl Serialize for EqAffine {
    fn serialize<S: Serializer>(&self, s: S) -> Result<S::Ok, S::Error> {
        serialize_bytes(self.to_bytes(), s)
    }
}

impl<'de> Deserialize<'de> for EqAffine {
    fn deserialize<D: Deserializer<'de>>(d: D) -> Result<Self, D::Error> {
        let bytes = deserialize_bytes(d)?;
        match EqAffine::from_bytes(&bytes).into() {
            Some(eq_affine) => Ok(eq_affine),
            None => Err(D::Error::custom(
                "deserialized bytes don't encode a Vesta curve point",
            )),
        }
    }
}

impl Serialize for Ep {
    fn serialize<S: Serializer>(&self, s: S) -> Result<S::Ok, S::Error> {
        EpAffine::serialize(&self.to_affine(), s)
    }
}

impl<'de> Deserialize<'de> for Ep {
    fn deserialize<D: Deserializer<'de>>(d: D) -> Result<Self, D::Error> {
        Ok(Self::from(EpAffine::deserialize(d)?))
    }
}

impl Serialize for Eq {
    fn serialize<S: Serializer>(&self, s: S) -> Result<S::Ok, S::Error> {
        EqAffine::serialize(&self.to_affine(), s)
    }
}

impl<'de> Deserialize<'de> for Eq {
    fn deserialize<D: Deserializer<'de>>(d: D) -> Result<Self, D::Error> {
        Ok(Self::from(EqAffine::deserialize(d)?))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    use core::fmt::Debug;

    use ff::Field;
    use group::{prime::PrimeCurveAffine, Curve, Group};
    use rand::SeedableRng;
    use rand_xorshift::XorShiftRng;

    use crate::curves::{Ep, Eq};

    fn test_roundtrip<T: Serialize + for<'a> Deserialize<'a> + Debug + PartialEq>(t: &T) {
        let serialized_json = serde_json::to_vec(t).unwrap();
        assert_eq!(*t, serde_json::from_slice(&serialized_json).unwrap());

        let serialized_bincode = bincode::serialize(t).unwrap();
        assert_eq!(*t, bincode::deserialize(&serialized_bincode).unwrap());
    }

    #[test]
    fn serde_fp() {
        let mut rng = XorShiftRng::from_seed([
            0x59, 0x62, 0xbe, 0x5d, 0x76, 0x3d, 0x31, 0x8d, 0x17, 0xdb, 0x37, 0x32, 0x54, 0x06,
            0xbc, 0xe5,
        ]);

        for _ in 0..100 {
            let f = Fp::random(&mut rng);
            test_roundtrip(&f);
        }

        let f = Fp::zero();
        test_roundtrip(&f);
        assert_eq!(
            serde_json::from_slice::<Fp>(
                br#""0000000000000000000000000000000000000000000000000000000000000000""#
            )
            .unwrap(),
            f
        );
        assert_eq!(
            bincode::deserialize::<Fp>(&[
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0
            ])
            .unwrap(),
            f
        );

        let f = Fp::one();
        test_roundtrip(&f);
        assert_eq!(
            serde_json::from_slice::<Fp>(
                br#""0100000000000000000000000000000000000000000000000000000000000000""#
            )
            .unwrap(),
            f
        );
        assert_eq!(
            bincode::deserialize::<Fp>(&[
                1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0
            ])
            .unwrap(),
            f
        );
    }

    #[test]
    fn serde_fq() {
        let mut rng = XorShiftRng::from_seed([
            0x59, 0x62, 0xbe, 0x5d, 0x76, 0x3d, 0x31, 0x8d, 0x17, 0xdb, 0x37, 0x32, 0x54, 0x06,
            0xbc, 0xe5,
        ]);

        for _ in 0..100 {
            let f = Fq::random(&mut rng);
            test_roundtrip(&f);
        }

        let f = Fq::zero();
        test_roundtrip(&f);
        assert_eq!(
            serde_json::from_slice::<Fq>(
                br#""0000000000000000000000000000000000000000000000000000000000000000""#
            )
            .unwrap(),
            f
        );
        assert_eq!(
            bincode::deserialize::<Fq>(&[
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0
            ])
            .unwrap(),
            f
        );

        let f = Fq::one();
        test_roundtrip(&f);
        assert_eq!(
            serde_json::from_slice::<Fq>(
                br#""0100000000000000000000000000000000000000000000000000000000000000""#
            )
            .unwrap(),
            f
        );
        assert_eq!(
            bincode::deserialize::<Fq>(&[
                1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0
            ])
            .unwrap(),
            f
        );
    }

    #[test]
    fn serde_ep_affine() {
        let mut rng = XorShiftRng::from_seed([
            0x59, 0x62, 0xbe, 0x5d, 0x76, 0x3d, 0x31, 0x8d, 0x17, 0xdb, 0x37, 0x32, 0x54, 0x06,
            0xbc, 0xe5,
        ]);

        for _ in 0..100 {
            let f = Ep::random(&mut rng);
            test_roundtrip(&f.to_affine());
        }

        let f = EpAffine::identity();
        test_roundtrip(&f);
        assert_eq!(
            serde_json::from_slice::<EpAffine>(
                br#""0000000000000000000000000000000000000000000000000000000000000000""#
            )
            .unwrap(),
            f
        );
        assert_eq!(
            bincode::deserialize::<EpAffine>(&[
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0
            ])
            .unwrap(),
            f
        );

        let f = EpAffine::generator();
        test_roundtrip(&f);
        assert_eq!(
            serde_json::from_slice::<EpAffine>(
                br#""00000000ed302d991bf94c09fc98462200000000000000000000000000000040""#
            )
            .unwrap(),
            f
        );
        assert_eq!(
            bincode::deserialize::<EpAffine>(&[
                0, 0, 0, 0, 237, 48, 45, 153, 27, 249, 76, 9, 252, 152, 70, 34, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 64
            ])
            .unwrap(),
            f
        );
    }

    #[test]
    fn serde_eq_affine() {
        let mut rng = XorShiftRng::from_seed([
            0x59, 0x62, 0xbe, 0x5d, 0x76, 0x3d, 0x31, 0x8d, 0x17, 0xdb, 0x37, 0x32, 0x54, 0x06,
            0xbc, 0xe5,
        ]);

        for _ in 0..100 {
            let f = Eq::random(&mut rng);
            test_roundtrip(&f.to_affine());
        }

        let f = EqAffine::identity();
        test_roundtrip(&f);
        assert_eq!(
            serde_json::from_slice::<EqAffine>(
                br#""0000000000000000000000000000000000000000000000000000000000000000""#
            )
            .unwrap(),
            f
        );
        assert_eq!(
            bincode::deserialize::<EqAffine>(&[
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0
            ])
            .unwrap(),
            f
        );

        let f = EqAffine::generator();
        test_roundtrip(&f);
        assert_eq!(
            serde_json::from_slice::<EqAffine>(
                br#""0000000021eb468cdda89409fc98462200000000000000000000000000000040""#
            )
            .unwrap(),
            f
        );
        assert_eq!(
            bincode::deserialize::<EqAffine>(&[
                0, 0, 0, 0, 33, 235, 70, 140, 221, 168, 148, 9, 252, 152, 70, 34, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 64
            ])
            .unwrap(),
            f
        );
    }

    #[test]
    fn serde_ep() {
        let mut rng = XorShiftRng::from_seed([
            0x59, 0x62, 0xbe, 0x5d, 0x76, 0x3d, 0x31, 0x8d, 0x17, 0xdb, 0x37, 0x32, 0x54, 0x06,
            0xbc, 0xe5,
        ]);

        for _ in 0..100 {
            let f = Ep::random(&mut rng);
            test_roundtrip(&f);
        }

        let f = Ep::identity();
        test_roundtrip(&f);
        assert_eq!(
            serde_json::from_slice::<Ep>(
                br#""0000000000000000000000000000000000000000000000000000000000000000""#
            )
            .unwrap(),
            f
        );
        assert_eq!(
            bincode::deserialize::<Ep>(&[
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0
            ])
            .unwrap(),
            f
        );

        let f = Ep::generator();
        test_roundtrip(&f);
        assert_eq!(
            serde_json::from_slice::<Ep>(
                br#""00000000ed302d991bf94c09fc98462200000000000000000000000000000040""#
            )
            .unwrap(),
            f
        );
        assert_eq!(
            bincode::deserialize::<Ep>(&[
                0, 0, 0, 0, 237, 48, 45, 153, 27, 249, 76, 9, 252, 152, 70, 34, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 64
            ])
            .unwrap(),
            f
        );
    }

    #[test]
    fn serde_eq() {
        let mut rng = XorShiftRng::from_seed([
            0x59, 0x62, 0xbe, 0x5d, 0x76, 0x3d, 0x31, 0x8d, 0x17, 0xdb, 0x37, 0x32, 0x54, 0x06,
            0xbc, 0xe5,
        ]);

        for _ in 0..100 {
            let f = Eq::random(&mut rng);
            test_roundtrip(&f);
        }

        let f = Eq::identity();
        test_roundtrip(&f);
        assert_eq!(
            serde_json::from_slice::<Eq>(
                br#""0000000000000000000000000000000000000000000000000000000000000000""#
            )
            .unwrap(),
            f
        );
        assert_eq!(
            bincode::deserialize::<Eq>(&[
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0
            ])
            .unwrap(),
            f
        );

        let f = Eq::generator();
        test_roundtrip(&f);
        assert_eq!(
            serde_json::from_slice::<Eq>(
                br#""0000000021eb468cdda89409fc98462200000000000000000000000000000040""#
            )
            .unwrap(),
            f
        );
        assert_eq!(
            bincode::deserialize::<Eq>(&[
                0, 0, 0, 0, 33, 235, 70, 140, 221, 168, 148, 9, 252, 152, 70, 34, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 64
            ])
            .unwrap(),
            f
        );
    }
}
