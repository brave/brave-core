// Copyright (c) 2016 The Rouille developers
// Licensed under the Apache License, Version 2.0
// <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT
// license <LICENSE-MIT or http://opensource.org/licenses/MIT>,
// at your option. All files in the project carrying such
// notice may not be copied, modified, or distributed except
// according to those terms.

//! Low-level parsing of websocket frames.
//!
//! Usage:
//!
//! - Create a `StateMachine` with `StateMachine::new`.
//! - Whenever data is received on the socket, call `StateMachine::feed`.
//! - The returned iterator produces zero, one or multiple `Element` objects containing what was
//!   received.
//! - For `Element::Data`, the `Data` object is an iterator over the decoded bytes.
//! - If `Element::Error` is produced, immediately end the connection.
//!
//! Glossary:
//!
//! - A websocket stream is made of multiple *messages*.
//! - Each message is made of one or more *frames*. See https://tools.ietf.org/html/rfc6455#section-5.4.
//! - Each frame can be received progressively, where each packet is an `Element` object (below).

/// A websocket element decoded from the data given to `StateMachine::feed`.
#[derive(Debug, PartialEq, Eq)]
pub enum Element<'a> {
    /// A new frame has started.
    FrameStart {
        /// If true, this is the last frame of the message.
        fin: bool,
        /// Length of the frame in bytes.
        length: u64,
        /// Opcode. See https://tools.ietf.org/html/rfc6455#section-5.2.
        opcode: u8,
    },

    /// Data was received as part of the current frame.
    Data {
        /// The decoded data. An iterator that produces `u8`s.
        data: Data<'a>,
        /// If true, this is the last packet in the frame.
        last_in_frame: bool,
    },

    /// An error in the stream. The connection must be dropped ASAP.
    Error {
        /// A description of the error. Can or cannot be be returned to the client.
        desc: &'static str,
    },
}

/// Decoded data. Implements `Iterator<Item = u8>`.
#[derive(Debug, PartialEq, Eq)]
pub struct Data<'a> {
    // Source data. Undecoded.
    data: &'a [u8],
    // Copy of the mask of the current frame.
    mask: u32,
    // Same as `StateMachineInner::InData::offset`. Updated at each iteration.
    offset: u8,
}

/// A websocket state machine. Contains partial data.
pub struct StateMachine {
    // Actual state.
    inner: StateMachineInner,
    // Contains the start of the header. Must be empty if `inner` is equal to `InData`.
    buffer: Vec<u8>, // TODO: use SmallVec?
}

enum StateMachineInner {
    // If `StateMachine::inner` is `InHeader`, then `buffer` contains the start of the header.
    InHeader,
    // If `StateMachine::inner` is `InData`, then `buffer` must be empty.
    InData {
        // Mask to decode the message.
        mask: u32,
        // Value between 0 and 3 that indicates the number of bytes between the start of the data
        // and the next expected byte.
        offset: u8,
        // Number of bytes remaining in the frame.
        remaining_len: u64,
    },
}

impl StateMachine {
    /// Initializes a new state machine for a new stream. Expects to see a new frame as the first
    /// packet.
    pub fn new() -> StateMachine {
        StateMachine {
            inner: StateMachineInner::InHeader,
            buffer: Vec::with_capacity(14),
        }
    }

    /// Feeds data to the state machine. Returns an iterator to the list of elements that were
    /// received.
    #[inline]
    pub fn feed<'a>(&'a mut self, data: &'a [u8]) -> ElementsIter<'a> {
        ElementsIter { state: self, data }
    }
}

// Helpers for decoding masked big-endian byte sequences
// These could probably be replaced with something more robust like `nom` if we want to
// take the hit of adding another dependency.
fn read_u16_be<'a, T: Iterator<Item = &'a u8>>(input: &mut T) -> u16 {
    let buf: [u8; 2] = [*input.next().unwrap(), *input.next().unwrap()];
    u16::from_be_bytes(buf)
}

fn read_u32_be<'a, T: Iterator<Item = &'a u8>>(input: &mut T) -> u32 {
    let buf: [u8; 4] = [
        *input.next().unwrap(),
        *input.next().unwrap(),
        *input.next().unwrap(),
        *input.next().unwrap(),
    ];
    u32::from_be_bytes(buf)
}

fn read_u64_be<'a, T: Iterator<Item = &'a u8>>(input: &mut T) -> u64 {
    let buf: [u8; 8] = [
        *input.next().unwrap(),
        *input.next().unwrap(),
        *input.next().unwrap(),
        *input.next().unwrap(),
        *input.next().unwrap(),
        *input.next().unwrap(),
        *input.next().unwrap(),
        *input.next().unwrap(),
    ];
    u64::from_be_bytes(buf)
}

/// Iterator to the list of elements that were received.
pub struct ElementsIter<'a> {
    state: &'a mut StateMachine,
    data: &'a [u8],
}

impl<'a> Iterator for ElementsIter<'a> {
    type Item = Element<'a>;

    fn next(&mut self) -> Option<Element<'a>> {
        if self.data.is_empty() {
            return None;
        }

        match self.state.inner {
            // First situation, we are in the header.
            StateMachineInner::InHeader => {
                // We need at least 6 bytes for a successful header. Otherwise we just return.
                let total_buffered = self.state.buffer.len() + self.data.len();
                if total_buffered < 6 {
                    self.state.buffer.extend_from_slice(self.data);
                    self.data = &[];
                    return None;
                }

                // Retrieve the first two bytes of the header.
                let (first_byte, second_byte) = {
                    let mut mask_iter = self.state.buffer.iter().chain(self.data.iter());
                    let first_byte = *mask_iter.next().unwrap();
                    let second_byte = *mask_iter.next().unwrap();
                    (first_byte, second_byte)
                };

                // Reserved bits must be zero, otherwise error.
                if (first_byte & 0x70) != 0 {
                    return Some(Element::Error {
                        desc: "Reserved bits must be zero",
                    });
                }

                // Client-to-server messages **must** be encoded.
                if (second_byte & 0x80) == 0 {
                    return Some(Element::Error {
                        desc: "Client-to-server messages must be masked",
                    });
                }

                // Find the length of the frame and the mask.
                let (length, mask) = match second_byte & 0x7f {
                    126 => {
                        if total_buffered < 8 {
                            self.state.buffer.extend_from_slice(self.data);
                            self.data = &[];
                            return None;
                        }

                        let mut mask_iter =
                            self.state.buffer.iter().chain(self.data.iter()).skip(2);

                        let length = read_u16_be(&mut mask_iter) as u64;
                        let mask = read_u32_be(&mut mask_iter);
                        (length, mask)
                    }
                    127 => {
                        if total_buffered < 14 {
                            self.state.buffer.extend_from_slice(self.data);
                            self.data = &[];
                            return None;
                        }

                        let mut mask_iter =
                            self.state.buffer.iter().chain(self.data.iter()).skip(2);

                        let length = {
                            let length = read_u64_be(&mut mask_iter);
                            // The most significant bit must be zero according to the specs.
                            if (length & 0x8000000000000000) != 0 {
                                return Some(Element::Error {
                                    desc: "Most-significant bit of the length must be zero",
                                });
                            }
                            length
                        };

                        let mask = read_u32_be(&mut mask_iter);

                        (length, mask)
                    }
                    n => {
                        let mut mask_iter =
                            self.state.buffer.iter().chain(self.data.iter()).skip(2);

                        let mask = read_u32_be(&mut mask_iter);
                        (u64::from(n), mask)
                    }
                };

                // Builds a slice containing the start of the data.
                let data_start = {
                    let data_start_off = match second_byte & 0x7f {
                        126 => 8,
                        127 => 14,
                        _ => 6,
                    };

                    assert!(self.state.buffer.len() < data_start_off);
                    &self.data[(data_start_off - self.state.buffer.len())..]
                };

                // Update ourselves for the next loop and return a FrameStart message.
                self.data = data_start;
                self.state.buffer.clear();
                self.state.inner = StateMachineInner::InData {
                    mask,
                    remaining_len: length,
                    offset: 0,
                };
                Some(Element::FrameStart {
                    fin: (first_byte & 0x80) != 0,
                    length,
                    opcode: first_byte & 0xf,
                })
            }

            // Second situation, we are in the message and we don't have enough data to finish the
            // current frame.
            StateMachineInner::InData {
                mask,
                ref mut remaining_len,
                ref mut offset,
            } if *remaining_len > self.data.len() as u64 => {
                let data = Data {
                    data: self.data,
                    mask,
                    offset: *offset,
                };

                *offset += (self.data.len() % 4) as u8;
                *offset %= 4;
                *remaining_len -= self.data.len() as u64;

                self.data = &[];

                Some(Element::Data {
                    data,
                    last_in_frame: false,
                })
            }

            // Third situation, we have enough data to finish the frame.
            StateMachineInner::InData {
                mask,
                remaining_len,
                offset,
            } => {
                debug_assert!(self.data.len() as u64 >= remaining_len);

                let data = Data {
                    data: &self.data[0..remaining_len as usize],
                    mask,
                    offset,
                };

                self.data = &self.data[remaining_len as usize..];
                self.state.inner = StateMachineInner::InHeader;
                debug_assert!(self.state.buffer.is_empty());

                Some(Element::Data {
                    data,
                    last_in_frame: true,
                })
            }
        }
    }
}

impl<'a> Iterator for Data<'a> {
    type Item = u8;

    #[inline]
    fn next(&mut self) -> Option<u8> {
        if self.data.is_empty() {
            return None;
        }

        let byte = self.data[0];
        let mask = ((self.mask >> ((3 - self.offset) * 8)) & 0xff) as u8;
        let decoded = byte ^ mask;

        self.data = &self.data[1..];
        self.offset = (self.offset + 1) % 4;

        Some(decoded)
    }

    #[inline]
    fn size_hint(&self) -> (usize, Option<usize>) {
        let l = self.data.len();
        (l, Some(l))
    }
}

impl<'a> ExactSizeIterator for Data<'a> {}

#[cfg(test)]
mod tests {
    use super::Element;
    use super::StateMachine;

    #[test]
    fn basic() {
        let mut machine = StateMachine::new();

        let data = &[
            0x81, 0x85, 0x37, 0xfa, 0x21, 0x3d, 0x7f, 0x9f, 0x4d, 0x51, 0x58,
        ];
        let mut iter = machine.feed(data);

        assert_eq!(
            iter.next().unwrap(),
            Element::FrameStart {
                fin: true,
                length: 5,
                opcode: 1
            }
        );

        match iter.next().unwrap() {
            Element::Data {
                data,
                last_in_frame,
            } => {
                assert!(last_in_frame);
                assert_eq!(data.collect::<Vec<_>>(), b"Hello");
            }
            _ => panic!(),
        }
    }
}
