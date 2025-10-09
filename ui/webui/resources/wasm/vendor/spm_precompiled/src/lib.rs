//! This crate aims to emulate https://github.com/google/sentencepiece Dart::DoubleArray
//! struct and it's Normalizer. It's main intent is to be used with tokenizers
//! that is a Rust library that aims to provide facilities to tokenize string
//! for use with HuggingFace's transformers library
//!
//! This crate is highly specialized and not intended for general use.
//!
//! The core of the algorithm is to read spm's binary `precompiled_charsmap`.
use nom::{number::complete::le_u32, IResult, ToUsize};
use serde::{de::Error, Deserialize, Deserializer, Serialize, Serializer};
use std::convert::TryFrom;
use unicode_segmentation::UnicodeSegmentation;

/// This struct is specifically done to be compatible with SentencePiece
/// SentencePiece models embed their Normalizer within a `precompiled_charsmap`
/// that both represents a Trie, and embedded rewrite rules.
/// In order to be 100% compliant we need to interpret that binary format too.
/// The format is [u32 (length of trie), trie: [u32], normalized: String]
/// The trie has u8 as entries, and u32 as values, those u32 values
/// point to offsets withing the String that correspond to the real replace value
/// The normalized string contains '\0' that should indicate the end of an entry.
///
/// Hence, normalized could be "abc\0", some entry in the trie could be 0 meaning
/// the value is "abc" and another one be 1 meaning the actual entry was "bc".
#[derive(Default, Clone, Debug, PartialEq, Serialize, Deserialize)]
#[serde(tag = "type", try_from = "PrecompiledDeserializer")]
pub struct Precompiled {
    #[serde(serialize_with = "as_base64", deserialize_with = "from_base64")]
    precompiled_charsmap: Vec<u8>,
    #[serde(skip)]
    normalized: String,
    #[serde(skip)]
    trie: DoubleArray,
}

#[doc(hidden)]
#[derive(Deserialize)]
#[serde(tag = "type")]
struct PrecompiledDeserializer {
    #[serde(deserialize_with = "from_base64")]
    precompiled_charsmap: Vec<u8>,
}

fn as_base64<T, S>(key: &T, serializer: S) -> Result<S::Ok, S::Error>
where
    T: AsRef<[u8]>,
    S: Serializer,
{
    serializer.serialize_str(&base64::encode(key.as_ref()))
}

fn from_base64<'de, D>(deserializer: D) -> Result<Vec<u8>, D::Error>
where
    D: Deserializer<'de>,
{
    let s: &str = Deserialize::deserialize(deserializer)?;
    let precompiled_charsmap = base64::decode(s).map_err(|err| Error::custom(err.to_string()))?;
    Ok(precompiled_charsmap)
}

impl TryFrom<PrecompiledDeserializer> for Precompiled {
    type Error = PrecompiledError;

    fn try_from(t: PrecompiledDeserializer) -> Result<Self, Self::Error> {
        Self::from(&t.precompiled_charsmap)
    }
}

pub type ArrayUnit = usize;

trait ArrayUnitTrait {
    fn has_leaf(&self) -> bool;
    fn value(&self) -> isize;
    fn label(&self) -> usize;
    fn offset(&self) -> usize;
}

impl ArrayUnitTrait for ArrayUnit {
    fn has_leaf(&self) -> bool {
        (self >> 8) & 1 == 1
    }

    fn value(&self) -> isize {
        (self & ((1usize << 31) - 1)) as isize
    }

    fn label(&self) -> usize {
        self & ((1usize << 31) | 0xFF)
    }

    fn offset(&self) -> usize {
        (self >> 10) << ((self & (1usize << 9)) >> 6)
    }
}

type Array = Vec<ArrayUnit>;

#[derive(Default, Clone, Debug, Serialize, Deserialize, PartialEq)]
pub struct DoubleArray {
    array: Array,
}

impl DoubleArray {
    fn from(array: Array) -> Self {
        Self { array }
    }

    pub fn common_prefix_search(&self, key: &[u8]) -> Vec<isize> {
        let mut node_pos = 0;
        let mut results = vec![];

        let mut unit = self.array[node_pos];
        node_pos ^= unit.offset();
        for c in key {
            if *c == 0u8 {
                break;
            }
            node_pos ^= *c as usize;
            unit = self.array[node_pos];
            if unit.label() != *c as usize {
                return results;
            }
            node_pos ^= unit.offset();
            if unit.has_leaf() {
                results.push(self.array[node_pos].value());
            }
        }
        results
    }
}

fn parse(precompiled_charsmap: &[u8]) -> IResult<&[u8], Array> {
    let (mut rest, trie_size) = le_u32(precompiled_charsmap)?;
    // u8 to u32.
    let trie_char_size = trie_size / 4;
    let mut trie_blob = Vec::with_capacity(trie_char_size as usize);
    for _ in 0..trie_char_size {
        let (rest2, n) = le_u32(rest)?;
        rest = rest2;
        trie_blob.push(n.to_usize());
    }
    let normalized_blob = rest;
    Ok((normalized_blob, trie_blob))
}

#[derive(Debug)]
pub enum PrecompiledError {
    ParseError,
    NormalizedInvalidUtf8,
}

impl std::fmt::Display for PrecompiledError {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        write!(f, "Cannot parse precompiled_charsmap")
    }
}

impl std::error::Error for PrecompiledError {}

impl Precompiled {
    pub fn from(precompiled_charsmap: &[u8]) -> Result<Precompiled, PrecompiledError> {
        let (normalized_blob, trie_blob) =
            parse(precompiled_charsmap).map_err(|_| PrecompiledError::ParseError)?;
        let normalized = String::from_utf8(normalized_blob.to_vec())
            .map_err(|_| PrecompiledError::NormalizedInvalidUtf8)?;
        let trie = DoubleArray::from(trie_blob);
        let precompiled = Precompiled {
            precompiled_charsmap: precompiled_charsmap.to_vec(),
            normalized,
            trie,
        };
        Ok(precompiled)
    }

    pub fn transform(&self, chunk: &str) -> Option<&str> {
        let results = self.trie.common_prefix_search(chunk.as_bytes());
        if results.is_empty() {
            None
        } else {
            let index = results[0] as usize;
            let mut index2 = index;
            while index2 < self.normalized.len() {
                if *self.normalized.as_bytes().get(index2)? == 0u8 {
                    break;
                }
                index2 += 1;
            }
            let normalized = &self.normalized[index..index2];
            Some(normalized)
        }
    }

    pub fn normalize_string(&self, original: &str) -> String {
        let mut string = String::with_capacity(original.len());
        // Future reader. From @Narsil.
        // Yes, this is weird,
        // Yes, this seems broken
        // No, I don't know why Google did this.
        // If you question this code, check this normalizer against
        // XNLI database (all languages) with Unigram model against
        // Mbart, XLMRoberta *AND* Marian. If you don't get 100% or
        // break a single test.
        // You don't pass.
        original.graphemes(true).for_each(|grapheme| {
            if grapheme.len() < 6 {
                if let Some(norm) = self.transform(grapheme) {
                    for c in norm.chars() {
                        string.push(c);
                    }
                    return;
                }
            }
            for (char_index, c) in grapheme.char_indices() {
                let part = &grapheme[char_index..char_index + c.len_utf8()];
                if let Some(norm) = self.transform(part) {
                    for c in norm.chars() {
                        string.push(c);
                    }
                } else {
                    string.push(c);
                }
            }
        });
        string
    }
}

#[cfg(test)]
mod tests;
