use core::{
    convert::{AsMut, AsRef},
    mem,
};

use crate::{error::Error, scalar::Scalar};

pub struct SignatureArray([u8; 6 + 33 + 33], usize);

impl SignatureArray {
    pub fn new(size: usize) -> Self {
        SignatureArray([0u8; 6 + 33 + 33], size)
    }

    pub fn len(&self) -> usize {
        self.1
    }

    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }
}

impl AsRef<[u8]> for SignatureArray {
    fn as_ref(&self) -> &[u8] {
        &self.0[..self.1]
    }
}

impl AsMut<[u8]> for SignatureArray {
    fn as_mut(&mut self) -> &mut [u8] {
        &mut self.0[..self.1]
    }
}

pub struct Decoder<'a>(&'a [u8], usize);

impl<'a> Decoder<'a> {
    pub fn new(arr: &'a [u8]) -> Self {
        Decoder(arr, 0)
    }

    pub fn remaining_len(&self) -> usize {
        self.0.len() - self.1
    }

    pub fn read(&mut self) -> Result<u8, Error> {
        if self.1 >= self.0.len() {
            Err(Error::InvalidSignature)
        } else {
            let v = self.0[self.1];
            self.1 += 1;
            Ok(v)
        }
    }

    pub fn peek(&self, forward: usize) -> Result<u8, Error> {
        if self.1 + forward >= self.0.len() {
            Err(Error::InvalidSignature)
        } else {
            let v = self.0[self.1 + forward];
            Ok(v)
        }
    }

    pub fn peek_slice(&self, len: usize) -> Result<&[u8], Error> {
        if (len == 0 && self.1 >= self.0.len()) || self.1 + len > self.0.len() {
            Err(Error::InvalidSignature)
        } else {
            let v = &self.0[self.1..(self.1 + len)];
            Ok(v)
        }
    }

    pub fn skip(&mut self, len: usize) -> Result<(), Error> {
        if (len == 0 && self.1 >= self.0.len()) || self.1 + len > self.0.len() {
            Err(Error::InvalidSignature)
        } else {
            self.1 += len;
            Ok(())
        }
    }

    pub fn read_constructed_sequence(&mut self) -> Result<(), Error> {
        let v = self.read()?;
        if v == 0x30 {
            Ok(())
        } else {
            Err(Error::InvalidSignature)
        }
    }

    pub fn read_len(&mut self) -> Result<usize, Error> {
        let b1 = self.read()?;
        if b1 == 0xff {
            return Err(Error::InvalidSignature);
        }

        // Short form
        if b1 & 0x80 == 0 {
            return Ok(b1 as usize);
        }

        // Infinite length is not allowed
        if b1 == 0x80 {
            return Err(Error::InvalidSignature);
        }

        let mut lenleft = (b1 & 0x7f) as usize;
        if lenleft > self.remaining_len() {
            return Err(Error::InvalidSignature);
        }

        if self.peek(0)? == 0 {
            // Not the shortest possible length encoding
            return Err(Error::InvalidSignature);
        }

        if lenleft > mem::size_of::<usize>() {
            return Err(Error::InvalidSignature);
        }

        let mut ret = 0;
        while lenleft > 0 {
            ret = (ret << 8) | (self.read()? as usize);
            if ret + lenleft > self.remaining_len() {
                return Err(Error::InvalidSignature);
            }
            lenleft -= 1;
        }

        if ret < 128 {
            // Not the shortest possible length encoding
            return Err(Error::InvalidSignature);
        }

        Ok(ret)
    }

    pub fn read_integer(&mut self) -> Result<Scalar, Error> {
        if self.read()? != 0x02 {
            return Err(Error::InvalidSignature);
        }

        let mut rlen = self.read_len()?;
        if rlen == 0 || rlen > self.remaining_len() {
            return Err(Error::InvalidSignature);
        }

        if self.peek(0)? == 0x00 && rlen > 1 && (self.peek(1)? & 0x80) == 0x00 {
            return Err(Error::InvalidSignature);
        }

        if self.peek(0)? == 0xff && rlen > 1 && (self.peek(1)? & 0x80) == 0x00 {
            return Err(Error::InvalidSignature);
        }

        let mut overflow = false;
        if self.peek(0)? & 0x80 == 0x80 {
            overflow |= true;
        }

        // Skip leading zero bytes
        while rlen > 0 && self.peek(0)? == 0 {
            rlen -= 1;
            self.read()?;
        }

        if rlen > 32 {
            overflow |= true;
        }

        let mut int = Scalar::default();

        if !overflow {
            let mut b32 = [0u8; 32];
            b32[32 - rlen..].copy_from_slice(self.peek_slice(rlen)?);
            self.skip(rlen)?;

            overflow |= bool::from(int.set_b32(&b32));
        }

        if overflow {
            int = Scalar::default();
        }

        Ok(int)
    }

    pub fn read_seq_len_lax(&mut self) -> Result<usize, Error> {
        let mut len = self.read()?;
        if len & 0x80 != 0x00 {
            len -= 0x80;
            if len as usize > self.remaining_len() {
                return Err(Error::InvalidSignature);
            }
            self.skip(len as usize)?;
        }

        Ok(len as usize)
    }

    pub fn read_len_lax(&mut self) -> Result<usize, Error> {
        let mut ret = 0usize;
        let mut len = self.read()?;
        if len & 0x80 != 0x00 {
            len -= 0x80;
            if len as usize > self.remaining_len() {
                return Err(Error::InvalidSignature);
            }
            while len > 0 && self.peek(0)? == 0 {
                self.skip(1)?;
                len -= 1;
            }
            if (len as usize) >= mem::size_of::<usize>() {
                return Err(Error::InvalidSignature);
            }
            while len > 0 {
                ret = (ret << 8) + (self.read()? as usize);
                len -= 1;
            }
        } else {
            ret = len as usize;
        }
        if ret > self.remaining_len() {
            return Err(Error::InvalidSignature);
        }

        Ok(ret)
    }

    pub fn read_integer_lax(&mut self) -> Result<Scalar, Error> {
        // Integer tag byte.
        if self.read()? != 0x02 {
            return Err(Error::InvalidSignature);
        }

        let mut len = self.read_len_lax()?;

        // Ignore leading zeroes.
        while len > 0 && self.peek(0)? == 0 {
            len -= 1;
            self.skip(1)?;
        }

        let mut overflow = false;
        // Copy value
        if len > 32 {
            overflow |= true;
        }

        let mut int = Scalar::default();

        if !overflow {
            let mut b32 = [0u8; 32];
            b32[32 - len..].copy_from_slice(&self.peek_slice(len)?);
            self.skip(len)?;

            overflow |= bool::from(int.set_b32(&b32));
        }

        if overflow {
            int = Scalar::default();
        }

        Ok(int)
    }
}
