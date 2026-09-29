// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/// Mojo objects (structs, arrays, maps and non-inlined unions) are aligned to
/// and padded out to this many bytes.
let mojoAlignment = 8

/// Size of struct and array headers.
let mojoHeaderSize = 8

/// Size of an inlined union.
let mojoUnionSize = 16

/// Encoded handle index representing no handle.
let mojoInvalidHandleIndex = UInt32.max

func mojoAlign(_ size: Int) -> Int {
  (size + mojoAlignment - 1) & ~(mojoAlignment - 1)
}

/// Serializes values into the mojo wire format.
///
/// Objects are laid out in the order they are allocated, and every object's
/// out-of-line children must be allocated after it, in field order. Decoding
/// validates that objects appear in exactly that order.
struct MojoEncoder: Sendable {
  private(set) var bytes: [UInt8] = []
  /// Handles referenced from `bytes` by index. They're owned by the encoder
  /// until the message is sent, so they're closed if it never is.
  private(set) var handles: [MojoHandle] = []

  /// Allocates `size` zeroed bytes, padded to ``mojoAlignment``, returning
  /// their offset.
  mutating func allocate(_ size: Int) -> Int {
    let offset = bytes.count
    bytes.append(contentsOf: repeatElement(0, count: mojoAlign(size)))
    return offset
  }

  mutating func write<T: FixedWidthInteger>(_ value: T, at offset: Int) {
    precondition(offset >= 0 && offset + MemoryLayout<T>.size <= bytes.count)
    bytes.withUnsafeMutableBytes {
      $0.storeBytes(of: value.littleEndian, toByteOffset: offset, as: T.self)
    }
  }

  mutating func write(_ value: Float, at offset: Int) {
    write(value.bitPattern, at: offset)
  }

  mutating func write(_ value: Double, at offset: Int) {
    write(value.bitPattern, at: offset)
  }

  /// Sets or clears bit `bit` of the byte at `offset`.
  mutating func write(_ value: Bool, at offset: Int, bit: Int) {
    precondition((0..<8).contains(bit))
    if value {
      bytes[offset] |= 1 << bit
    } else {
      bytes[offset] &= ~(1 << bit)
    }
  }

  /// Writes a pointer at `offset` to the object at `target`. Pointers are
  /// encoded relative to their own position.
  mutating func writePointer(to target: Int, at offset: Int) {
    precondition(target > offset)
    write(UInt64(target - offset), at: offset)
  }

  mutating func writeStructHeader(at offset: Int, numBytes: UInt32, version: UInt32) {
    write(numBytes, at: offset)
    write(version, at: offset + 4)
  }

  /// Allocates an array with `count` elements whose storage (excluding the
  /// header) takes `storageSize` bytes, returning the offset of the header.
  mutating func allocateArray(count: Int, storageSize: Int) -> Int {
    let numBytes = mojoHeaderSize + storageSize
    let offset = allocate(numBytes)
    write(UInt32(numBytes), at: offset)
    write(UInt32(count), at: offset + 4)
    return offset
  }

  /// Adds `handle` to the message, returning the index to encode for it, or
  /// ``mojoInvalidHandleIndex`` if it was already transferred elsewhere.
  mutating func append(_ handle: MojoHandle) -> UInt32 {
    let rawValue = handle.take()
    guard rawValue != 0 else {
      assertionFailure("Encoding a handle that was already transferred")
      return mojoInvalidHandleIndex
    }
    handles.append(MojoHandle(adopting: rawValue))
    return UInt32(handles.count - 1)
  }

  /// Encodes `value` as the root object of the message.
  mutating func encodeStruct<T: MojomStruct>(_ value: T) {
    _ = value.encodeData(to: &self)
  }

  consuming func finish() -> Message {
    Message(bytes: bytes, handles: handles)
  }
}
