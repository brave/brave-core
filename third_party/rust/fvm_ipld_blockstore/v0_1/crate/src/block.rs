use cid::multihash::{self, MultihashDigest};
use cid::Cid;

/// Block represents a typed (i.e., with codec) IPLD block.
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub struct Block<D>
where
    D: AsRef<[u8]> + ?Sized,
{
    pub codec: u64,
    pub data: D,
}

impl<D> Block<D>
where
    D: AsRef<[u8]> + ?Sized,
{
    pub fn new(codec: u64, data: D) -> Self
    where
        Self: Sized,
        D: Sized,
    {
        Self { codec, data }
    }

    pub fn cid(&self, mh_code: multihash::Code) -> Cid {
        Cid::new_v1(self.codec, mh_code.digest(self.data.as_ref()))
    }

    #[allow(clippy::len_without_is_empty)]
    pub fn len(&self) -> usize {
        self.data.as_ref().len()
    }
}

impl<D> AsRef<[u8]> for Block<D>
where
    D: AsRef<[u8]>,
{
    fn as_ref(&self) -> &[u8] {
        self.data.as_ref()
    }
}

impl<'a, D> From<&'a Block<D>> for Block<&'a [u8]>
where
    D: AsRef<[u8]>,
{
    fn from(b: &'a Block<D>) -> Self {
        Block {
            codec: b.codec,
            data: b.data.as_ref(),
        }
    }
}
