// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/// Why a message failed to decode. A message that fails to decode is treated
/// as a bad message, and the pipe it was received on is closed.
enum MojoDecodingError: Error, Equatable, Sendable {
  /// A read went past the end of the message.
  case outOfBounds
  /// An object didn't start on an 8-byte boundary.
  case misalignedObject
  /// An object overlapped the previous one, or appeared out of order.
  case illegalMemoryRange
  /// A pointer's offset wasn't representable.
  case illegalPointer
  /// A non-nullable pointer was null.
  case unexpectedNullPointer
  /// A handle index was out of range or appeared out of order.
  case illegalHandle
  /// A non-nullable handle or interface was invalid.
  case unexpectedInvalidHandle
  /// A struct header didn't match any known version of the struct.
  case unexpectedStructHeader
  /// An array header's size didn't fit its elements.
  case unexpectedArrayHeader
  /// A fixed size array had the wrong number of elements.
  case unexpectedArrayLength
  /// A map's keys and values arrays had different lengths.
  case differentMapKeysAndValuesSizes
  /// A non-nullable union was null.
  case unexpectedNullUnion
  /// A union had a tag that isn't known and it isn't extensible.
  case unknownUnionTag
  /// An enum had a value that isn't known and it isn't extensible.
  case unknownEnumValue
  /// Objects were nested deeper than allowed.
  case maxRecursionDepthExceeded
  /// The message header's flags were contradictory.
  case invalidMessageFlags
  /// The message header was missing a request ID its flags require.
  case missingRequestID
}

/// Bounds checked little-endian reads from a byte buffer.
struct ByteReader: Sendable {
  let bytes: [UInt8]

  func read<T: FixedWidthInteger>(at offset: Int) throws -> T {
    guard offset >= 0, offset <= bytes.count - MemoryLayout<T>.size else {
      throw MojoDecodingError.outOfBounds
    }
    return bytes.withUnsafeBytes {
      T(littleEndian: $0.loadUnaligned(fromByteOffset: offset, as: T.self))
    }
  }

  /// Reads the pointer at `offset`, returning the absolute offset it points to
  /// or `nil` if it's null. The target is not checked to be within the buffer.
  func readPointer(at offset: Int) throws -> Int? {
    let pointer: UInt64 = try read(at: offset)
    if pointer == 0 {
      return nil
    }
    guard pointer <= UInt64(UInt32.max) else {
      throw MojoDecodingError.illegalPointer
    }
    return offset + Int(pointer)
  }
}

/// Deserializes and validates values in the mojo wire format.
///
/// Validation follows the C++ bindings: every object must be aligned, lie
/// within the message and start after the previous object ends, and handles
/// must be claimed in increasing index order, each at most once.
struct MojoDecoder: Sendable {
  /// Matches the C++ bindings' limit.
  static let maxRecursionDepth = 200

  private let reader: ByteReader
  private var handles: [MojoHandle?]
  /// Objects may only be claimed within this range.
  private let end: Int
  /// Start of the unclaimed part of the message.
  private var nextClaimableOffset: Int
  /// Lowest handle index that may still be claimed.
  private var nextClaimableHandle = 0
  private var depth = 0

  /// Creates a decoder for the objects within `range` of `bytes`.
  init(bytes: [UInt8], handles: [MojoHandle], range: Range<Int>) {
    reader = ByteReader(bytes: bytes)
    self.handles = handles
    precondition(range.lowerBound >= 0 && range.upperBound <= bytes.count)
    nextClaimableOffset = range.lowerBound
    end = range.upperBound
  }

  /// The handles that haven't been claimed by anything decoded.
  var remainingHandles: [MojoHandle] {
    handles.compactMap { $0 }
  }

  // MARK: Reading

  /// Reads a value within the claimed part of the message.
  func read<T: FixedWidthInteger>(at offset: Int) throws -> T {
    guard offset + MemoryLayout<T>.size <= end else {
      throw MojoDecodingError.outOfBounds
    }
    return try reader.read(at: offset)
  }

  func read(at offset: Int) throws -> Float {
    Float(bitPattern: try read(at: offset))
  }

  func read(at offset: Int) throws -> Double {
    Double(bitPattern: try read(at: offset))
  }

  func read(at offset: Int, bit: Int) throws -> Bool {
    let byte: UInt8 = try read(at: offset)
    return byte & (1 << bit) != 0
  }

  /// Reads the pointer at `offset`, returning the absolute offset it points to
  /// or `nil` if it's null.
  func readPointer(at offset: Int) throws -> Int? {
    guard offset + 8 <= end else {
      throw MojoDecodingError.outOfBounds
    }
    return try reader.readPointer(at: offset)
  }

  /// Reads the pointer at `offset`, throwing if it's null.
  func readNonNullPointer(at offset: Int) throws -> Int {
    guard let target = try readPointer(at: offset) else {
      throw MojoDecodingError.unexpectedNullPointer
    }
    return target
  }

  // MARK: Claiming

  /// Claims `count` bytes at `offset` for an object.
  mutating func claimMemory(at offset: Int, count: Int) throws {
    guard offset % mojoAlignment == 0 else {
      throw MojoDecodingError.misalignedObject
    }
    guard count > 0, offset >= nextClaimableOffset, offset <= end - count else {
      throw MojoDecodingError.illegalMemoryRange
    }
    nextClaimableOffset = offset + count
  }

  /// Claims the struct at `offset`, validating its header against `versions`
  /// and returning its header.
  mutating func claimStruct(
    at offset: Int,
    versions: [MojomStructVersion]
  ) throws -> (numBytes: UInt32, version: UInt32) {
    guard offset % mojoAlignment == 0 else {
      throw MojoDecodingError.misalignedObject
    }
    let numBytes: UInt32 = try reader.read(at: offset)
    let version: UInt32 = try reader.read(at: offset + 4)
    guard numBytes >= mojoHeaderSize else {
      throw MojoDecodingError.unexpectedStructHeader
    }
    try claimMemory(at: offset, count: Int(numBytes))

    guard let latest = versions.last else {
      preconditionFailure("A struct must have at least one version")
    }
    if version <= latest.version {
      // The size must match the newest version not newer than the header's.
      guard let known = versions.last(where: { $0.version <= version }),
        known.numBytes == numBytes
      else {
        throw MojoDecodingError.unexpectedStructHeader
      }
    } else if numBytes < latest.numBytes {
      throw MojoDecodingError.unexpectedStructHeader
    }
    return (numBytes, version)
  }

  /// Claims the array at `offset`, validating that its storage is large enough
  /// for its elements, and returns its number of elements.
  mutating func claimArray(
    at offset: Int,
    expectedCount: Int = 0,
    storageSize: (Int) -> Int
  ) throws -> Int {
    guard offset % mojoAlignment == 0 else {
      throw MojoDecodingError.misalignedObject
    }
    let numBytes: UInt32 = try reader.read(at: offset)
    let count: UInt32 = try reader.read(at: offset + 4)
    try claimMemory(at: offset, count: Int(numBytes))
    guard Int(numBytes) >= mojoHeaderSize + storageSize(Int(count)) else {
      throw MojoDecodingError.unexpectedArrayHeader
    }
    if expectedCount != 0, Int(count) != expectedCount {
      throw MojoDecodingError.unexpectedArrayLength
    }
    return Int(count)
  }

  /// Claims the handle encoded as `index`, returning `nil` for
  /// ``mojoInvalidHandleIndex``.
  mutating func claimHandle(_ index: UInt32) throws -> MojoHandle? {
    if index == mojoInvalidHandleIndex {
      return nil
    }
    let index = Int(index)
    guard index >= nextClaimableHandle, index < handles.count,
      let handle = handles[index]
    else {
      throw MojoDecodingError.illegalHandle
    }
    handles[index] = nil
    nextClaimableHandle = index + 1
    return handle
  }

  /// Runs `body` one nesting level deeper, failing past
  /// ``maxRecursionDepth``.
  mutating func nested<T>(
    _ body: (inout MojoDecoder) throws -> T
  ) throws -> T {
    depth += 1
    defer { depth -= 1 }
    guard depth < Self.maxRecursionDepth else {
      throw MojoDecodingError.maxRecursionDepthExceeded
    }
    return try body(&self)
  }

  /// Decodes the struct starting at the beginning of the decoder's range.
  mutating func decodeRootStruct<T: MojomStruct>(
    _ type: T.Type
  ) throws -> T {
    try T.decodeData(from: &self, at: nextClaimableOffset)
  }
}
