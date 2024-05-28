//! Structs representing the TZE components within Zcash transactions.

use byteorder::{LittleEndian, ReadBytesExt, WriteBytesExt};
use std::convert::TryFrom;
use std::fmt::Debug;
use std::io::{self, Read, Write};

use zcash_encoding::{CompactSize, Vector};

use super::amount::Amount;
use crate::{extensions::transparent as tze, transaction::TxId};

pub mod builder;

fn to_io_error(_: std::num::TryFromIntError) -> io::Error {
    io::Error::new(io::ErrorKind::InvalidData, "value out of range")
}

pub trait Authorization: Debug {
    type Witness: Debug + Clone + PartialEq;
}

#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub struct Authorized;

impl Authorization for Authorized {
    type Witness = tze::AuthData;
}

pub trait MapAuth<A: Authorization, B: Authorization> {
    fn map_witness(&self, s: A::Witness) -> B::Witness;
    fn map_authorization(&self, s: A) -> B;
}

#[derive(Debug, Clone, PartialEq)]
pub struct Bundle<A: Authorization> {
    pub vin: Vec<TzeIn<A::Witness>>,
    pub vout: Vec<TzeOut>,
    pub authorization: A,
}

impl<A: Authorization> Bundle<A> {
    pub fn map_authorization<B: Authorization, F: MapAuth<A, B>>(self, f: F) -> Bundle<B> {
        Bundle {
            vin: self
                .vin
                .into_iter()
                .map(|tzein| TzeIn {
                    prevout: tzein.prevout,
                    witness: tzein.witness.map_payload(|p| f.map_witness(p)),
                })
                .collect(),
            vout: self.vout,
            authorization: f.map_authorization(self.authorization),
        }
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct OutPoint {
    txid: TxId,
    n: u32,
}

impl OutPoint {
    pub fn new(txid: TxId, n: u32) -> Self {
        OutPoint { txid, n }
    }

    pub fn read<R: Read>(mut reader: R) -> io::Result<Self> {
        let txid = TxId::read(&mut reader)?;
        let n = reader.read_u32::<LittleEndian>()?;
        Ok(OutPoint { txid, n })
    }

    pub fn write<W: Write>(&self, mut writer: W) -> io::Result<()> {
        self.txid.write(&mut writer)?;
        writer.write_u32::<LittleEndian>(self.n)
    }

    pub fn n(&self) -> u32 {
        self.n
    }

    pub fn txid(&self) -> &TxId {
        &self.txid
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct TzeIn<Payload> {
    pub prevout: OutPoint,
    pub witness: tze::Witness<Payload>,
}

impl<Payload> TzeIn<Payload> {
    /// Write without witness data (for signature hashing)
    ///
    /// This is also used as the prefix for the encoded form used
    /// within a serialized transaction.
    pub fn write_without_witness<W: Write>(&self, mut writer: W) -> io::Result<()> {
        self.prevout.write(&mut writer)?;

        CompactSize::write(
            &mut writer,
            usize::try_from(self.witness.extension_id).map_err(to_io_error)?,
        )?;

        CompactSize::write(
            &mut writer,
            usize::try_from(self.witness.mode).map_err(to_io_error)?,
        )
    }
}

/// Transaction encoding and decoding functions conforming to [ZIP 222].
///
/// [ZIP 222]: https://zips.z.cash/zip-0222#encoding-in-transactions
impl TzeIn<()> {
    /// Convenience constructor
    pub fn new(prevout: OutPoint, extension_id: u32, mode: u32) -> Self {
        TzeIn {
            prevout,
            witness: tze::Witness {
                extension_id,
                mode,
                payload: (),
            },
        }
    }
}

impl TzeIn<<Authorized as Authorization>::Witness> {
    /// Read witness metadata & payload
    ///
    /// Used to decode the encoded form used within a serialized
    /// transaction.
    pub fn read<R: Read>(mut reader: &mut R) -> io::Result<Self> {
        let prevout = OutPoint::read(&mut reader)?;

        let extension_id = CompactSize::read_t(&mut reader)?;
        let mode = CompactSize::read_t(&mut reader)?;
        let payload = Vector::read(&mut reader, |r| r.read_u8())?;

        Ok(TzeIn {
            prevout,
            witness: tze::Witness {
                extension_id,
                mode,
                payload: tze::AuthData(payload),
            },
        })
    }

    /// Write prevout, extension, and mode followed by witness data.
    ///
    /// This calls [`write_without_witness`] to serialize witness metadata,
    /// then appends the witness bytes themselves. This is the encoded
    /// form that is used in a serialized transaction.
    ///
    /// [`write_without_witness`]: TzeIn::write_without_witness
    pub fn write<W: Write>(&self, mut writer: W) -> io::Result<()> {
        self.write_without_witness(&mut writer)?;
        Vector::write(&mut writer, &self.witness.payload.0, |w, b| w.write_u8(*b))
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct TzeOut {
    pub value: Amount,
    pub precondition: tze::Precondition,
}

impl TzeOut {
    pub fn read<R: Read>(mut reader: &mut R) -> io::Result<Self> {
        let value = {
            let mut tmp = [0; 8];
            reader.read_exact(&mut tmp)?;
            Amount::from_nonnegative_i64_le_bytes(tmp)
        }
        .map_err(|_| io::Error::new(io::ErrorKind::InvalidData, "value out of range"))?;

        let extension_id = CompactSize::read_t(&mut reader)?;
        let mode = CompactSize::read_t(&mut reader)?;
        let payload = Vector::read(&mut reader, |r| r.read_u8())?;

        Ok(TzeOut {
            value,
            precondition: tze::Precondition {
                extension_id,
                mode,
                payload,
            },
        })
    }

    pub fn write<W: Write>(&self, mut writer: W) -> io::Result<()> {
        writer.write_all(&self.value.to_i64_le_bytes())?;

        CompactSize::write(
            &mut writer,
            usize::try_from(self.precondition.extension_id).map_err(to_io_error)?,
        )?;
        CompactSize::write(
            &mut writer,
            usize::try_from(self.precondition.mode).map_err(to_io_error)?,
        )?;
        Vector::write(&mut writer, &self.precondition.payload, |w, b| {
            w.write_u8(*b)
        })
    }
}

#[cfg(any(test, feature = "test-dependencies"))]
pub mod testing {
    use proptest::collection::vec;
    use proptest::prelude::*;

    use crate::{
        consensus::BranchId,
        extensions::transparent::{AuthData, Precondition, Witness},
        transaction::components::amount::testing::arb_nonnegative_amount,
        transaction::testing::arb_txid,
    };

    use super::{Authorized, Bundle, OutPoint, TzeIn, TzeOut};

    prop_compose! {
        pub fn arb_outpoint()(txid in arb_txid(), n in 0..100u32) -> OutPoint {
            OutPoint::new(txid, n)
        }
    }

    prop_compose! {
        pub fn arb_witness()(extension_id in 0..100u32, mode in 0..100u32, payload in vec(any::<u8>(), 32..256).prop_map(AuthData))  -> Witness<AuthData> {
            Witness { extension_id, mode, payload }
        }
    }

    prop_compose! {
        pub fn arb_tzein()(prevout in arb_outpoint(), witness in arb_witness()) -> TzeIn<AuthData> {
            TzeIn { prevout, witness }
        }
    }

    prop_compose! {
        pub fn arb_precondition()(extension_id in 0..100u32, mode in 0..100u32, payload in vec(any::<u8>(), 32..256))  -> Precondition {
            Precondition { extension_id, mode, payload }
        }
    }

    prop_compose! {
        pub fn arb_tzeout()(value in arb_nonnegative_amount(), precondition in arb_precondition()) -> TzeOut {
            TzeOut { value: value.into(), precondition }
        }
    }

    prop_compose! {
        pub fn arb_bundle(branch_id: BranchId)(
            vin in vec(arb_tzein(), 0..10),
            vout in vec(arb_tzeout(), 0..10),
        ) -> Option<Bundle<Authorized>> {
            if branch_id != BranchId::ZFuture || (vin.is_empty() && vout.is_empty()) {
                None
            } else {
                Some(Bundle { vin, vout, authorization: Authorized })
            }
        }
    }
}
