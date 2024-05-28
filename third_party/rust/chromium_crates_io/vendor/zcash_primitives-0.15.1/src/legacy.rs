//! Support for legacy transparent addresses and scripts.

use byteorder::{ReadBytesExt, WriteBytesExt};

use std::fmt;
use std::io::{self, Read, Write};
use std::ops::Shl;

use zcash_encoding::Vector;

#[cfg(feature = "transparent-inputs")]
pub mod keys;

/// Defined script opcodes.
///
/// Most of the opcodes are unused by this crate, but we define them so that the alternate
/// `Debug` impl for [`Script`] renders correctly for unexpected scripts.
#[derive(Debug)]
enum OpCode {
    // push value
    Op0 = 0x00, // False
    PushData1 = 0x4c,
    PushData2 = 0x4d,
    PushData4 = 0x4e,
    Negative1 = 0x4f,
    Reserved = 0x50,
    Op1 = 0x51, // True
    Op2 = 0x52,
    Op3 = 0x53,
    Op4 = 0x54,
    Op5 = 0x55,
    Op6 = 0x56,
    Op7 = 0x57,
    Op8 = 0x58,
    Op9 = 0x59,
    Op10 = 0x5a,
    Op11 = 0x5b,
    Op12 = 0x5c,
    Op13 = 0x5d,
    Op14 = 0x5e,
    Op15 = 0x5f,
    Op16 = 0x60,

    // control
    Nop = 0x61,
    Ver = 0x62,
    If = 0x63,
    NotIf = 0x64,
    VerIf = 0x65,
    VerNotIf = 0x66,
    Else = 0x67,
    EndIf = 0x68,
    Verify = 0x69,
    Return = 0x6a,

    // stack ops
    ToAltStack = 0x6b,
    FromAltStack = 0x6c,
    Drop2 = 0x6d,
    Dup2 = 0x6e,
    Dup3 = 0x6f,
    Over2 = 0x70,
    Rot2 = 0x71,
    Swap2 = 0x72,
    IfDup = 0x73,
    Depth = 0x74,
    Drop = 0x75,
    Dup = 0x76,
    Nip = 0x77,
    Over = 0x78,
    Pick = 0x79,
    Roll = 0x7a,
    Rot = 0x7b,
    Swap = 0x7c,
    Tuck = 0x7d,

    // splice ops
    Cat = 0x7e,    // Disabled
    Substr = 0x7f, // Disabled
    Left = 0x80,   // Disabled
    Right = 0x81,  // Disabled
    Size = 0x82,

    // bit logic
    Invert = 0x83, // Disabled
    And = 0x84,    // Disabled
    Or = 0x85,     // Disabled
    Xor = 0x86,    // Disabled
    Equal = 0x87,
    EqualVerify = 0x88,
    Reserved1 = 0x89,
    Reserved2 = 0x8a,

    // numeric
    Add1 = 0x8b,
    Sub1 = 0x8c,
    Mul2 = 0x8d, // Disabled
    Div2 = 0x8e, // Disabled
    Negate = 0x8f,
    Abs = 0x90,
    Not = 0x91,
    NotEqual0 = 0x92,

    Add = 0x93,
    Sub = 0x94,
    Mul = 0x95,    // Disabled
    Div = 0x96,    // Disabled
    Mod = 0x97,    // Disabled
    LShift = 0x98, // Disabled
    RShift = 0x99, // Disabled

    BoolAnd = 0x9a,
    BoolOr = 0x9b,
    NumEqual = 0x9c,
    NumEqualVerify = 0x9d,
    NumNotEqual = 0x9e,
    LessThan = 0x9f,
    GreaterThan = 0xa0,
    LessThanOrEqual = 0xa1,
    GreaterThanOrEqual = 0xa2,
    Min = 0xa3,
    Max = 0xa4,

    Within = 0xa5,

    // crypto
    Ripemd160 = 0xa6,
    Sha1 = 0xa7,
    Sha256 = 0xa8,
    Hash160 = 0xa9,
    Hash256 = 0xaa,
    CodeSeparator = 0xab, // Disabled
    CheckSig = 0xac,
    CheckSigVerify = 0xad,
    CheckMultisig = 0xae,
    CheckMultisigVerify = 0xaf,

    // expansion
    Nop1 = 0xb0,
    CheckLockTimeVerify = 0xb1,
    Nop3 = 0xb2,
    Nop4 = 0xb3,
    Nop5 = 0xb4,
    Nop6 = 0xb5,
    Nop7 = 0xb6,
    Nop8 = 0xb7,
    Nop9 = 0xb8,
    Nop10 = 0xb9,

    InvalidOpCode = 0xff,
}

impl OpCode {
    fn parse(b: u8) -> Option<Self> {
        match b {
            0x00 => Some(OpCode::Op0),
            0x4c => Some(OpCode::PushData1),
            0x4d => Some(OpCode::PushData2),
            0x4e => Some(OpCode::PushData4),
            0x4f => Some(OpCode::Negative1),
            0x50 => Some(OpCode::Reserved),
            0x51 => Some(OpCode::Op1),
            0x52 => Some(OpCode::Op2),
            0x53 => Some(OpCode::Op3),
            0x54 => Some(OpCode::Op4),
            0x55 => Some(OpCode::Op5),
            0x56 => Some(OpCode::Op6),
            0x57 => Some(OpCode::Op7),
            0x58 => Some(OpCode::Op8),
            0x59 => Some(OpCode::Op9),
            0x5a => Some(OpCode::Op10),
            0x5b => Some(OpCode::Op11),
            0x5c => Some(OpCode::Op12),
            0x5d => Some(OpCode::Op13),
            0x5e => Some(OpCode::Op14),
            0x5f => Some(OpCode::Op15),
            0x60 => Some(OpCode::Op16),
            0x61 => Some(OpCode::Nop),
            0x62 => Some(OpCode::Ver),
            0x63 => Some(OpCode::If),
            0x64 => Some(OpCode::NotIf),
            0x65 => Some(OpCode::VerIf),
            0x66 => Some(OpCode::VerNotIf),
            0x67 => Some(OpCode::Else),
            0x68 => Some(OpCode::EndIf),
            0x69 => Some(OpCode::Verify),
            0x6a => Some(OpCode::Return),
            0x6b => Some(OpCode::ToAltStack),
            0x6c => Some(OpCode::FromAltStack),
            0x6d => Some(OpCode::Drop2),
            0x6e => Some(OpCode::Dup2),
            0x6f => Some(OpCode::Dup3),
            0x70 => Some(OpCode::Over2),
            0x71 => Some(OpCode::Rot2),
            0x72 => Some(OpCode::Swap2),
            0x73 => Some(OpCode::IfDup),
            0x74 => Some(OpCode::Depth),
            0x75 => Some(OpCode::Drop),
            0x76 => Some(OpCode::Dup),
            0x77 => Some(OpCode::Nip),
            0x78 => Some(OpCode::Over),
            0x79 => Some(OpCode::Pick),
            0x7a => Some(OpCode::Roll),
            0x7b => Some(OpCode::Rot),
            0x7c => Some(OpCode::Swap),
            0x7d => Some(OpCode::Tuck),
            0x7e => Some(OpCode::Cat),
            0x7f => Some(OpCode::Substr),
            0x80 => Some(OpCode::Left),
            0x81 => Some(OpCode::Right),
            0x82 => Some(OpCode::Size),
            0x83 => Some(OpCode::Invert),
            0x84 => Some(OpCode::And),
            0x85 => Some(OpCode::Or),
            0x86 => Some(OpCode::Xor),
            0x87 => Some(OpCode::Equal),
            0x88 => Some(OpCode::EqualVerify),
            0x89 => Some(OpCode::Reserved1),
            0x8a => Some(OpCode::Reserved2),
            0x8b => Some(OpCode::Add1),
            0x8c => Some(OpCode::Sub1),
            0x8d => Some(OpCode::Mul2),
            0x8e => Some(OpCode::Div2),
            0x8f => Some(OpCode::Negate),
            0x90 => Some(OpCode::Abs),
            0x91 => Some(OpCode::Not),
            0x92 => Some(OpCode::NotEqual0),
            0x93 => Some(OpCode::Add),
            0x94 => Some(OpCode::Sub),
            0x95 => Some(OpCode::Mul),
            0x96 => Some(OpCode::Div),
            0x97 => Some(OpCode::Mod),
            0x98 => Some(OpCode::LShift),
            0x99 => Some(OpCode::RShift),
            0x9a => Some(OpCode::BoolAnd),
            0x9b => Some(OpCode::BoolOr),
            0x9c => Some(OpCode::NumEqual),
            0x9d => Some(OpCode::NumEqualVerify),
            0x9e => Some(OpCode::NumNotEqual),
            0x9f => Some(OpCode::LessThan),
            0xa0 => Some(OpCode::GreaterThan),
            0xa1 => Some(OpCode::LessThanOrEqual),
            0xa2 => Some(OpCode::GreaterThanOrEqual),
            0xa3 => Some(OpCode::Min),
            0xa4 => Some(OpCode::Max),
            0xa5 => Some(OpCode::Within),
            0xa6 => Some(OpCode::Ripemd160),
            0xa7 => Some(OpCode::Sha1),
            0xa8 => Some(OpCode::Sha256),
            0xa9 => Some(OpCode::Hash160),
            0xaa => Some(OpCode::Hash256),
            0xab => Some(OpCode::CodeSeparator),
            0xac => Some(OpCode::CheckSig),
            0xad => Some(OpCode::CheckSigVerify),
            0xae => Some(OpCode::CheckMultisig),
            0xaf => Some(OpCode::CheckMultisigVerify),
            0xb0 => Some(OpCode::Nop1),
            0xb1 => Some(OpCode::CheckLockTimeVerify),
            0xb2 => Some(OpCode::Nop3),
            0xb3 => Some(OpCode::Nop4),
            0xb4 => Some(OpCode::Nop5),
            0xb5 => Some(OpCode::Nop6),
            0xb6 => Some(OpCode::Nop7),
            0xb7 => Some(OpCode::Nop8),
            0xb8 => Some(OpCode::Nop9),
            0xb9 => Some(OpCode::Nop10),
            0xff => Some(OpCode::InvalidOpCode),
            _ => None,
        }
    }
}

/// A serialized script, used inside transparent inputs and outputs of a transaction.
#[derive(Clone, Default, PartialEq, Eq)]
pub struct Script(pub Vec<u8>);

impl fmt::Debug for Script {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        struct ScriptPrinter<'s>(&'s [u8]);
        impl<'s> fmt::Debug for ScriptPrinter<'s> {
            fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
                let mut l = f.debug_list();
                let mut unknown: Option<String> = None;
                for b in self.0 {
                    if let Some(opcode) = OpCode::parse(*b) {
                        if let Some(s) = unknown.take() {
                            l.entry(&s);
                        }
                        l.entry(&opcode);
                    } else {
                        let encoded = format!("{:02x}", b);
                        if let Some(s) = &mut unknown {
                            s.push_str(&encoded);
                        } else {
                            unknown = Some(encoded);
                        }
                    }
                }
                l.finish()
            }
        }

        if f.alternate() {
            f.debug_tuple("Script")
                .field(&ScriptPrinter(&self.0))
                .finish()
        } else {
            f.debug_tuple("Script")
                .field(&hex::encode(&self.0))
                .finish()
        }
    }
}

impl Script {
    pub fn read<R: Read>(mut reader: R) -> io::Result<Self> {
        let script = Vector::read(&mut reader, |r| r.read_u8())?;
        Ok(Script(script))
    }

    pub fn write<W: Write>(&self, mut writer: W) -> io::Result<()> {
        Vector::write(&mut writer, &self.0, |w, e| w.write_u8(*e))
    }

    /// Returns the address that this Script contains, if any.
    pub(crate) fn address(&self) -> Option<TransparentAddress> {
        if self.0.len() == 25
            && self.0[0..3] == [OpCode::Dup as u8, OpCode::Hash160 as u8, 0x14]
            && self.0[23..25] == [OpCode::EqualVerify as u8, OpCode::CheckSig as u8]
        {
            let mut hash = [0; 20];
            hash.copy_from_slice(&self.0[3..23]);
            Some(TransparentAddress::PublicKeyHash(hash))
        } else if self.0.len() == 23
            && self.0[0..2] == [OpCode::Hash160 as u8, 0x14]
            && self.0[22] == OpCode::Equal as u8
        {
            let mut hash = [0; 20];
            hash.copy_from_slice(&self.0[2..22]);
            Some(TransparentAddress::ScriptHash(hash))
        } else {
            None
        }
    }
}

impl Shl<OpCode> for Script {
    type Output = Self;

    fn shl(mut self, rhs: OpCode) -> Self {
        self.0.push(rhs as u8);
        self
    }
}

impl Shl<&[u8]> for Script {
    type Output = Self;

    fn shl(mut self, data: &[u8]) -> Self {
        if data.len() < OpCode::PushData1 as usize {
            self.0.push(data.len() as u8);
        } else if data.len() <= 0xff {
            self.0.push(OpCode::PushData1 as u8);
            self.0.push(data.len() as u8);
        } else if data.len() <= 0xffff {
            self.0.push(OpCode::PushData2 as u8);
            self.0.extend((data.len() as u16).to_le_bytes());
        } else {
            self.0.push(OpCode::PushData4 as u8);
            self.0.extend((data.len() as u32).to_le_bytes());
        }
        self.0.extend(data);
        self
    }
}

/// A transparent address corresponding to either a public key or a `Script`.
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub enum TransparentAddress {
    PublicKeyHash([u8; 20]),
    ScriptHash([u8; 20]),
}

impl TransparentAddress {
    /// Generate the `scriptPubKey` corresponding to this address.
    pub fn script(&self) -> Script {
        match self {
            TransparentAddress::PublicKeyHash(key_id) => {
                // P2PKH script
                Script::default()
                    << OpCode::Dup
                    << OpCode::Hash160
                    << &key_id[..]
                    << OpCode::EqualVerify
                    << OpCode::CheckSig
            }
            TransparentAddress::ScriptHash(script_id) => {
                // P2SH script
                Script::default() << OpCode::Hash160 << &script_id[..] << OpCode::Equal
            }
        }
    }
}

#[cfg(any(test, feature = "test-dependencies"))]
pub mod testing {
    use proptest::prelude::{any, prop_compose};

    use super::TransparentAddress;

    prop_compose! {
        pub fn arb_transparent_addr()(v in proptest::array::uniform20(any::<u8>())) -> TransparentAddress {
            TransparentAddress::PublicKeyHash(v)
        }
    }
}

#[cfg(test)]
mod tests {
    use super::{OpCode, Script, TransparentAddress};

    #[test]
    fn script_opcode() {
        {
            let script = Script::default() << OpCode::PushData1;
            assert_eq!(&script.0, &[OpCode::PushData1 as u8]);
        }
    }

    #[test]
    fn script_pushdata() {
        {
            let script = Script::default() << &[1, 2, 3, 4][..];
            assert_eq!(&script.0, &[4, 1, 2, 3, 4]);
        }

        {
            let short_data = vec![2; 100];
            let script = Script::default() << &short_data[..];
            assert_eq!(script.0[0], OpCode::PushData1 as u8);
            assert_eq!(script.0[1] as usize, 100);
            assert_eq!(&script.0[2..], &short_data[..]);
        }

        {
            let medium_data = vec![7; 1024];
            let script = Script::default() << &medium_data[..];
            assert_eq!(script.0[0], OpCode::PushData2 as u8);
            assert_eq!(&script.0[1..3], &[0x00, 0x04][..]);
            assert_eq!(&script.0[3..], &medium_data[..]);
        }

        {
            let long_data = vec![42; 1_000_000];
            let script = Script::default() << &long_data[..];
            assert_eq!(script.0[0], OpCode::PushData4 as u8);
            assert_eq!(&script.0[1..5], &[0x40, 0x42, 0x0f, 0x00][..]);
            assert_eq!(&script.0[5..], &long_data[..]);
        }
    }

    #[test]
    fn p2pkh() {
        let addr = TransparentAddress::PublicKeyHash([4; 20]);
        assert_eq!(
            &addr.script().0,
            &[
                0x76, 0xa9, 0x14, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
                0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x88, 0xac,
            ]
        );
        assert_eq!(addr.script().address(), Some(addr));
    }

    #[test]
    fn p2sh() {
        let addr = TransparentAddress::ScriptHash([7; 20]);
        assert_eq!(
            &addr.script().0,
            &[
                0xa9, 0x14, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
                0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x87,
            ]
        );
        assert_eq!(addr.script().address(), Some(addr));
    }
}
