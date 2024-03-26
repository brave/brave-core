//! Block validation
use crate::cid::Cid;
use crate::codec::{Codec, Decode, Encode, References};
use crate::error::{BlockTooLarge, InvalidMultihash, Result, UnsupportedMultihash};
use crate::ipld::Ipld;
use crate::multihash::MultihashDigest;
use crate::store::StoreParams;
use core::borrow::Borrow;
use core::convert::TryFrom;
use core::marker::PhantomData;
use core::ops::Deref;

/// Block
#[derive(Clone)]
pub struct Block<S> {
    _marker: PhantomData<S>,
    /// Content identifier.
    cid: Cid,
    /// Binary data.
    data: Vec<u8>,
}

impl<S> core::fmt::Debug for Block<S> {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        f.debug_struct("Block")
            .field("cid", &self.cid)
            .field("data", &self.data)
            .finish()
    }
}

impl<S> Deref for Block<S> {
    type Target = Cid;

    fn deref(&self) -> &Self::Target {
        &self.cid
    }
}

impl<S> core::hash::Hash for Block<S> {
    fn hash<SH: core::hash::Hasher>(&self, hasher: &mut SH) {
        core::hash::Hash::hash(&self.cid, hasher)
    }
}

impl<S> PartialEq for Block<S> {
    fn eq(&self, other: &Self) -> bool {
        self.cid == other.cid
    }
}

impl<S> Eq for Block<S> {}

impl<S> Borrow<Cid> for Block<S> {
    fn borrow(&self) -> &Cid {
        &self.cid
    }
}

impl<S> AsRef<Cid> for Block<S> {
    fn as_ref(&self) -> &Cid {
        &self.cid
    }
}

impl<S> AsRef<[u8]> for Block<S> {
    fn as_ref(&self) -> &[u8] {
        &self.data
    }
}

// TODO: move to tiny_cid
fn verify_cid<M: MultihashDigest<S>, const S: usize>(cid: &Cid, payload: &[u8]) -> Result<()> {
    let mh = M::try_from(cid.hash().code())
        .map_err(|_| UnsupportedMultihash(cid.hash().code()))?
        .digest(payload);

    if mh.digest() != cid.hash().digest() {
        return Err(InvalidMultihash(mh.to_bytes()).into());
    }
    Ok(())
}

impl<S: StoreParams> Block<S> {
    /// Creates a new block. Returns an error if the hash doesn't match
    /// the data.
    pub fn new(cid: Cid, data: Vec<u8>) -> Result<Self> {
        verify_cid::<S::Hashes, 64>(&cid, &data)?;
        Ok(Self::new_unchecked(cid, data))
    }

    /// Creates a new block without verifying the cid.
    pub fn new_unchecked(cid: Cid, data: Vec<u8>) -> Self {
        Self {
            _marker: PhantomData,
            cid,
            data,
        }
    }

    /// Returns the cid.
    pub fn cid(&self) -> &Cid {
        &self.cid
    }

    /// Returns the payload.
    pub fn data(&self) -> &[u8] {
        &self.data
    }

    /// Returns the inner cid and data.
    pub fn into_inner(self) -> (Cid, Vec<u8>) {
        (self.cid, self.data)
    }

    /// Encode a block.`
    pub fn encode<CE: Codec, T: Encode<CE> + ?Sized>(
        codec: CE,
        hcode: S::Hashes,
        payload: &T,
    ) -> Result<Self>
    where
        CE: Into<S::Codecs>,
    {
        debug_assert_eq!(
            Into::<u64>::into(codec),
            Into::<u64>::into(Into::<S::Codecs>::into(codec))
        );
        let data = codec.encode(payload)?;
        if data.len() > S::MAX_BLOCK_SIZE {
            return Err(BlockTooLarge(data.len()).into());
        }
        let mh = hcode.digest(&data);
        let cid = Cid::new_v1(codec.into(), mh);
        Ok(Self {
            _marker: PhantomData,
            cid,
            data,
        })
    }

    /// Decodes a block.
    ///
    /// # Example
    ///
    /// Decoding to [`Ipld`]:
    ///
    /// ```
    /// use libipld::block::Block;
    /// use libipld::cbor::DagCborCodec;
    /// use libipld::ipld::Ipld;
    /// use libipld::multihash::Code;
    /// use libipld::store::DefaultParams;
    ///
    /// let block =
    ///     Block::<DefaultParams>::encode(DagCborCodec, Code::Blake3_256, "Hello World!").unwrap();
    /// let ipld = block.decode::<DagCborCodec, Ipld>().unwrap();
    ///
    /// assert_eq!(ipld, Ipld::String("Hello World!".to_string()));
    /// ```
    pub fn decode<CD: Codec, T: Decode<CD>>(&self) -> Result<T>
    where
        S::Codecs: Into<CD>,
    {
        debug_assert_eq!(
            Into::<u64>::into(CD::try_from(self.cid.codec()).unwrap()),
            Into::<u64>::into(S::Codecs::try_from(self.cid.codec()).unwrap()),
        );
        CD::try_from(self.cid.codec())?.decode(&self.data)
    }

    /// Returns the decoded ipld.
    pub fn ipld(&self) -> Result<Ipld>
    where
        Ipld: Decode<S::Codecs>,
    {
        self.decode::<S::Codecs, Ipld>()
    }

    /// Returns the references.
    pub fn references<E: Extend<Cid>>(&self, set: &mut E) -> Result<()>
    where
        Ipld: References<S::Codecs>,
    {
        S::Codecs::try_from(self.cid.codec())?.references::<Ipld, E>(&self.data, set)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::cbor::DagCborCodec;
    use crate::codec_impl::IpldCodec;
    use crate::ipld;
    use crate::ipld::Ipld;
    use crate::multihash::Code;
    use crate::store::DefaultParams;
    use fnv::FnvHashSet;

    type IpldBlock = Block<DefaultParams>;

    #[test]
    fn test_references() {
        let b1 = IpldBlock::encode(IpldCodec::Raw, Code::Blake3_256, &ipld!(&b"cid1"[..])).unwrap();
        let b2 = IpldBlock::encode(IpldCodec::DagJson, Code::Blake3_256, &ipld!("cid2")).unwrap();
        let b3 = IpldBlock::encode(
            IpldCodec::DagPb,
            Code::Blake3_256,
            &ipld!({
                "Data": &b"data"[..],
                "Links": Ipld::List(vec![]),
            }),
        )
        .unwrap();

        let payload = ipld!({
            "cid1": &b1.cid,
            "cid2": { "other": true, "cid2": { "cid2": &b2.cid }},
            "cid3": [[ &b3.cid, &b1.cid ]],
        });
        let block = IpldBlock::encode(IpldCodec::DagCbor, Code::Blake3_256, &payload).unwrap();
        let payload2 = block.decode::<IpldCodec, _>().unwrap();
        assert_eq!(payload, payload2);

        let mut refs = FnvHashSet::default();
        payload2.references(&mut refs);
        assert_eq!(refs.len(), 3);
        assert!(refs.contains(&b1.cid));
        assert!(refs.contains(&b2.cid));
        assert!(refs.contains(&b3.cid));
    }

    #[test]
    fn test_transmute() {
        let b1 = IpldBlock::encode(DagCborCodec, Code::Blake3_256, &42).unwrap();
        assert_eq!(b1.cid.codec(), 0x71);
    }
}
