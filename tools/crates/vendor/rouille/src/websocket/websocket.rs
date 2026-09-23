// Copyright (c) 2016 The Rouille developers
// Licensed under the Apache License, Version 2.0
// <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT
// license <LICENSE-MIT or http://opensource.org/licenses/MIT>,
// at your option. All files in the project carrying such
// notice may not be copied, modified, or distributed except
// according to those terms.

use std::io;
use std::io::Write;
use std::mem;
use std::sync::mpsc::Sender;
use ReadWrite;
use Upgrade;

use websocket::low_level;

/// A successful websocket. An open channel of communication. Implements `Read` and `Write`.
pub struct Websocket {
    // The socket. `None` if closed.
    socket: Option<Box<dyn ReadWrite + Send>>,
    // The websocket state machine.
    state_machine: low_level::StateMachine,
    // True if the fragmented message currently being processed is binary. False if string. Pings
    // are excluded.
    current_message_binary: bool,
    // Buffer for the fragmented message currently being processed. Pings are excluded.
    current_message_payload: Vec<u8>,
    // Opcode of the fragment currently being processed.
    current_frame_opcode: u8,
    // Fin flag of the fragment currently being processed.
    current_frame_fin: bool,
    // Data of the fragment currently being processed.
    current_frame_payload: Vec<u8>,
    // Queue of the messages that are going to be returned by `next()`.
    messages_in_queue: Vec<Message>,
}

/// A message produced by a websocket connection.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Message {
    /// Text data. If the client is in Javascript, this happens when the client called `send()`
    /// with a string.
    Text(String),

    /// Binary data. If the client is in Javascript, this happens when the client called `send()`
    /// with a blob or an arraybuffer.
    Binary(Vec<u8>),
}

/// Error that can happen when sending a message to the client.
#[derive(Debug)]
pub enum SendError {
    /// Failed to transfer the message on the socket.
    IoError(io::Error),

    /// The websocket connection is closed.
    Closed,
}

impl From<io::Error> for SendError {
    #[inline]
    fn from(err: io::Error) -> SendError {
        SendError::IoError(err)
    }
}

impl Websocket {
    /// Sends text data over the websocket.
    ///
    /// Returns an error if the message didn't send correctly or if the connection is closed.
    ///
    /// If the client is in javascript, the message will contain a string.
    #[inline]
    pub fn send_text(&mut self, data: &str) -> Result<(), SendError> {
        let socket = match self.socket {
            Some(ref mut s) => s,
            None => return Err(SendError::Closed),
        };

        send(data.as_bytes(), Write::by_ref(socket), 0x1)?;
        Ok(())
    }

    /// Sends binary data over the websocket.
    ///
    /// Returns an error if the message didn't send correctly or if the connection is closed.
    ///
    /// If the client is in javascript, the message will contain a blob or an arraybuffer.
    #[inline]
    pub fn send_binary(&mut self, data: &[u8]) -> Result<(), SendError> {
        let socket = match self.socket {
            Some(ref mut s) => s,
            None => return Err(SendError::Closed),
        };

        send(data, Write::by_ref(socket), 0x2)?;
        Ok(())
    }

    /// Returns `true` if the websocket has been closed by either the client (voluntarily or not)
    /// or by the server (if the websocket protocol was violated).
    #[inline]
    pub fn is_closed(&self) -> bool {
        self.socket.is_none()
    }

    // TODO: give access to close reason
}

impl Upgrade for Sender<Websocket> {
    fn build(&mut self, socket: Box<dyn ReadWrite + Send>) {
        let websocket = Websocket {
            socket: Some(socket),
            state_machine: low_level::StateMachine::new(),
            current_message_binary: false,
            current_message_payload: Vec::new(),
            current_frame_opcode: 0,
            current_frame_fin: false,
            current_frame_payload: Vec::new(),
            messages_in_queue: Vec::new(),
        };

        let _ = self.send(websocket);
    }
}

impl Iterator for Websocket {
    type Item = Message;

    fn next(&mut self) -> Option<Message> {
        loop {
            // If the socket is `None`, the connection has been closed.
            self.socket.as_ref()?;

            // There may be some messages waiting to be processed.
            if !self.messages_in_queue.is_empty() {
                return Some(self.messages_in_queue.remove(0));
            }

            // Read `n` bytes in `buf`.
            let mut buf = [0; 256];
            let n = match self.socket.as_mut().unwrap().read(&mut buf) {
                Ok(n) if n == 0 => {
                    // Read returning zero means EOF
                    self.socket = None;
                    return None;
                }
                Ok(n) => n,
                Err(ref err) if err.kind() == io::ErrorKind::Interrupted => 0,
                Err(_) => {
                    self.socket = None;
                    return None;
                }
            };

            // Fill `messages_in_queue` by analyzing the packets.
            for element in self.state_machine.feed(&buf[0..n]) {
                match element {
                    low_level::Element::FrameStart { fin, opcode, .. } => {
                        debug_assert!(self.current_frame_payload.is_empty());
                        self.current_frame_fin = fin;
                        self.current_frame_opcode = opcode;
                    }

                    low_level::Element::Data {
                        data,
                        last_in_frame,
                    } => {
                        // Under normal circumstances we just handle data by pushing it to
                        // `current_frame_payload`.
                        self.current_frame_payload.extend(data);

                        // But if the frame is finished we additionally need to dispatch it.
                        if last_in_frame {
                            match self.current_frame_opcode {
                                // Frame is a continuation of the current message.
                                0x0 => {
                                    self.current_message_payload
                                        .append(&mut self.current_frame_payload);

                                    // If the message is finished, dispatch it.
                                    if self.current_frame_fin {
                                        let binary = mem::take(&mut self.current_message_payload);

                                        if self.current_message_binary {
                                            self.messages_in_queue.push(Message::Binary(binary));
                                        } else {
                                            let string = match String::from_utf8(binary) {
                                                Ok(s) => s,
                                                Err(_) => {
                                                    // Closing connection because text wasn't UTF-8
                                                    let _ = send(
                                                        b"1007 Invalid UTF-8 encoding",
                                                        Write::by_ref(
                                                            self.socket.as_mut().unwrap(),
                                                        ),
                                                        0x8,
                                                    );
                                                    self.socket = None;
                                                    return None;
                                                }
                                            };

                                            self.messages_in_queue.push(Message::Text(string));
                                        }
                                    }
                                }

                                // Frame is an individual text frame.
                                0x1 => {
                                    // If we're in the middle of a message, this frame is invalid
                                    // and we need to close.
                                    if !self.current_message_payload.is_empty() {
                                        let _ = send(
                                            b"1002 Expected continuation frame",
                                            Write::by_ref(self.socket.as_mut().unwrap()),
                                            0x8,
                                        );
                                        self.socket = None;
                                        return None;
                                    }

                                    if self.current_frame_fin {
                                        // There's only one frame in this message.
                                        let binary = mem::take(&mut self.current_frame_payload);
                                        let string = match String::from_utf8(binary) {
                                            Ok(s) => s,
                                            Err(_err) => {
                                                // Closing connection because text wasn't UTF-8
                                                let _ = send(
                                                    b"1007 Invalid UTF-8 encoding",
                                                    Write::by_ref(self.socket.as_mut().unwrap()),
                                                    0x8,
                                                );
                                                self.socket = None;
                                                return None;
                                            }
                                        };

                                        self.messages_in_queue.push(Message::Text(string));
                                    } else {
                                        // Start of a fragmented message.
                                        self.current_message_binary = false;
                                        self.current_message_payload
                                            .append(&mut self.current_frame_payload);
                                    }
                                }

                                // Frame is an individual binary frame.
                                0x2 => {
                                    // If we're in the middle of a message, this frame is invalid
                                    // and we need to close.
                                    if !self.current_message_payload.is_empty() {
                                        let _ = send(
                                            b"1002 Expected continuation frame",
                                            Write::by_ref(self.socket.as_mut().unwrap()),
                                            0x8,
                                        );
                                        self.socket = None;
                                        return None;
                                    }

                                    if self.current_frame_fin {
                                        let binary = mem::take(&mut self.current_frame_payload);
                                        self.messages_in_queue.push(Message::Binary(binary));
                                    } else {
                                        // Start of a fragmented message.
                                        self.current_message_binary = true;
                                        self.current_message_payload
                                            .append(&mut self.current_frame_payload);
                                    }
                                }

                                // Close request.
                                0x8 => {
                                    // We need to send a confirmation.
                                    let _ = send(
                                        &self.current_frame_payload,
                                        Write::by_ref(self.socket.as_mut().unwrap()),
                                        0x8,
                                    );
                                    // Since the packets are always received in order, and since
                                    // the server is considered dead as soon as it sends the
                                    // confirmation, we have no risk of losing packets.
                                    self.socket = None;
                                    return None;
                                }

                                // Ping.
                                0x9 => {
                                    // Send the pong.
                                    let _ = send(
                                        &self.current_frame_payload,
                                        Write::by_ref(self.socket.as_mut().unwrap()),
                                        0xA,
                                    );
                                }

                                // Pong. We ignore this as there's nothing to do.
                                0xA => {}

                                // Unknown opcode means error and close.
                                _ => {
                                    let _ = send(
                                        b"Unknown opcode",
                                        Write::by_ref(self.socket.as_mut().unwrap()),
                                        0x8,
                                    );
                                    self.socket = None;
                                    return None;
                                }
                            }

                            self.current_frame_payload.clear();
                        }
                    }

                    low_level::Element::Error { desc } => {
                        // The low level layer signaled an error. Sending it to client and closing.
                        let _ = send(
                            desc.as_bytes(),
                            Write::by_ref(self.socket.as_mut().unwrap()),
                            0x8,
                        );
                        self.socket = None;
                        return None;
                    }
                }
            }
        }
    }
}

// Sends a message to a websocket.
// TODO: message fragmentation?
fn send<W: Write>(data: &[u8], mut dest: W, opcode: u8) -> io::Result<()> {
    // Write the opcode
    assert!(opcode <= 0xf);
    let first_byte = 0x80 | opcode;
    dest.write_all(&[first_byte])?;

    // Write the length
    if data.len() >= 65536 {
        dest.write_all(&[127u8])?;
        let len = data.len() as u64;
        assert!(len < 0x8000_0000_0000_0000);
        dest.write_all(&len.to_be_bytes())?;
    } else if data.len() >= 126 {
        dest.write_all(&[126u8])?;
        let len = data.len() as u16;
        dest.write_all(&len.to_be_bytes())?;
    } else {
        dest.write_all(&[data.len() as u8])?;
    }

    // Write the data
    dest.write_all(data)?;
    dest.flush()?;
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_ws_framing_short() {
        let data: &[u8] = &[0xAB, 0xAB, 0xAB, 0xAB];
        let mut buf = Vec::new();

        send(data, &mut buf, 0x2).unwrap();

        // Expected
        // 0x82 (FIN = 1 | RSV1/2/3 = 0 | OPCODE = 2)
        // 0x04 (len = 4 bytes)
        // 0xABABABAB (payload = 4 bytes)
        assert_eq!(&buf, &[0x82, 0x04, 0xAB, 0xAB, 0xAB, 0xAB]);
    }

    #[test]
    fn test_ws_framing_medium() {
        let data: [u8; 125] = [0xAB; 125];
        let mut buf = Vec::new();

        send(&data, &mut buf, 0x2).unwrap();

        // Expected
        // 0x82 (FIN = 1 | RSV1/2/3 = 0 | OPCODE = 2)
        // 0x7D (len = 125 bytes)
        // 0xABABABAB... (payload = 125 bytes)
        assert_eq!(&buf[0..2], &[0x82, 0x7D]);
        assert_eq!(&buf[2..], &data[..]);
    }

    #[test]
    fn test_ws_framing_long() {
        let data: [u8; 65534] = [0xAB; 65534];
        let mut buf = Vec::new();

        send(&data, &mut buf, 0x2).unwrap();

        // Expected
        // 0x82 (FIN = 1 | RSV1/2/3 = 0 | OPCODE = 2)
        // 0x7E (len = 126 = extended 7+16)
        // 0xFFFE (extended_len = 65534 - Network Byte Order)
        // 0xABABABAB... (payload = 65534 bytes)
        assert_eq!(&buf[0..4], &[0x82, 0x7E, 0xFF, 0xFE]);
        assert_eq!(&buf[4..], &data[..]);
    }

    #[test]
    fn test_ws_framing_very_long() {
        let data: [u8; 0x100FF] = [0xAB; 0x100FF]; // 65791
        let mut buf = Vec::new();

        send(&data, &mut buf, 0x2).unwrap();

        // Expected
        // 0x82 (FIN = 1 | RSV1/2/3 = 0 | OPCODE = 2)
        // 0x7F (len = 127 = extended 7+64)
        // 0x00000000000100FF (extended_len = 65791 - Network Byte Order)
        // 0xABABABAB... (payload = 65791 bytes)
        assert_eq!(
            &buf[0..10],
            &[0x82, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0xFF]
        );
        assert_eq!(&buf[10..], &data[..]);
    }
}
