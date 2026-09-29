// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/// A serialized mojo message: its wire format bytes plus the handles attached
/// to it. Handles are referenced from the bytes by their index.
struct Message: Sendable {
  var bytes: [UInt8] = []
  var handles: [MojoHandle] = []
}

/// The flags of a ``MessageHeader``.
struct MessageFlags: OptionSet, Sendable {
  let rawValue: UInt32

  static let expectsResponse = MessageFlags(rawValue: 1 << 0)
  static let isResponse = MessageFlags(rawValue: 1 << 1)
  static let isSync = MessageFlags(rawValue: 1 << 2)
  static let noInterrupt = MessageFlags(rawValue: 1 << 3)
  static let isUrgent = MessageFlags(rawValue: 1 << 4)
}

/// The header every mojo message starts with.
///
/// Headers come in versions 0 through 3 (and possibly later), each extending
/// the previous one. Incoming headers of any version are accepted, fields added
/// by versions this code doesn't know about are ignored. Outgoing headers are
/// always written as version 1, which is all that is needed without associated
/// interfaces.
struct MessageHeader: Equatable, Sendable {
  /// Identifies the interface on the pipe the message is for, 0 for the pipe's
  /// primary interface.
  var interfaceID: UInt32 = 0
  /// The ordinal of the method the message is for, scoped to the interface.
  var name: UInt32
  var flags: MessageFlags = []
  /// Matches responses to requests. Only meaningful when `flags` contains
  /// `.expectsResponse` or `.isResponse`.
  var requestID: UInt64 = 0

  /// Sizes of each known header version, indexed by version.
  static let versionSizes: [UInt32] = [24, 32, 48, 56]

  /// Size of the headers this code writes.
  static let encodedSize = versionSizes[1]
}

/// A message header along with the location of the payload it precedes.
struct ParsedMessageHeader: Equatable, Sendable {
  var header: MessageHeader
  /// Version of the header as it was on the wire.
  var version: UInt32
  /// Byte range of the payload within the message.
  var payload: Range<Int>
  /// Whether the message carries associated interface IDs, which aren't
  /// supported.
  var hasPayloadInterfaceIDs: Bool
}

extension Message {
  /// Creates a message with `header` followed by `payload` encoded as the
  /// message's payload.
  init<Payload: MojomStruct>(header: MessageHeader, payload: Payload) {
    var encoder = MojoEncoder()
    let headerOffset = encoder.allocate(Int(MessageHeader.encodedSize))
    encoder.writeStructHeader(at: headerOffset, numBytes: MessageHeader.encodedSize, version: 1)
    encoder.write(header.interfaceID, at: headerOffset + 8)
    encoder.write(header.name, at: headerOffset + 12)
    encoder.write(header.flags.rawValue, at: headerOffset + 16)
    // trace_nonce at offset 20 is only used for tracing and is left as 0.
    encoder.write(header.requestID, at: headerOffset + 24)
    encoder.encodeStruct(payload)
    self = encoder.finish()
  }

  /// Parses and validates the message's header.
  func parseHeader() throws -> ParsedMessageHeader {
    let reader = ByteReader(bytes: bytes)
    let numBytes: UInt32 = try reader.read(at: 0)
    let version: UInt32 = try reader.read(at: 4)

    let versionSizes = MessageHeader.versionSizes
    if version < versionSizes.count {
      guard numBytes == versionSizes[Int(version)] else {
        throw MojoDecodingError.unexpectedStructHeader
      }
    } else {
      guard numBytes >= versionSizes.last! else {
        throw MojoDecodingError.unexpectedStructHeader
      }
    }
    guard numBytes <= bytes.count else {
      throw MojoDecodingError.outOfBounds
    }

    var header = MessageHeader(name: try reader.read(at: 12))
    header.interfaceID = try reader.read(at: 8)
    header.flags = MessageFlags(rawValue: try reader.read(at: 16))

    let requestIDFlags: MessageFlags = [.expectsResponse, .isResponse]
    if header.flags.isSuperset(of: requestIDFlags) {
      throw MojoDecodingError.invalidMessageFlags
    }
    if version == 0 {
      guard header.flags.isDisjoint(with: requestIDFlags) else {
        throw MojoDecodingError.missingRequestID
      }
      return ParsedMessageHeader(
        header: header,
        version: version,
        payload: Int(numBytes)..<bytes.count,
        hasPayloadInterfaceIDs: false
      )
    }
    header.requestID = try reader.read(at: 24)
    if version == 1 {
      return ParsedMessageHeader(
        header: header,
        version: version,
        payload: Int(numBytes)..<bytes.count,
        hasPayloadInterfaceIDs: false
      )
    }

    // Version 2 and later locate the payload with a pointer, and may be
    // followed by an array of associated interface IDs.
    guard let payloadStart = try reader.readPointer(at: 32) else {
      throw MojoDecodingError.unexpectedNullPointer
    }
    let interfaceIDsStart = try reader.readPointer(at: 40)
    let payloadEnd = interfaceIDsStart ?? bytes.count
    guard payloadStart >= Int(numBytes), payloadStart < payloadEnd,
      payloadEnd <= bytes.count
    else {
      throw MojoDecodingError.illegalMemoryRange
    }
    return ParsedMessageHeader(
      header: header,
      version: version,
      payload: payloadStart..<payloadEnd,
      hasPayloadInterfaceIDs: interfaceIDsStart != nil
    )
  }

  /// Decodes the payload following `parsedHeader` as `Payload`, taking
  /// ownership of the handles it references.
  mutating func decodePayload<Payload: MojomStruct>(
    _ type: Payload.Type,
    parsedHeader: ParsedMessageHeader
  ) throws -> Payload {
    var decoder = MojoDecoder(
      bytes: bytes,
      handles: handles,
      range: parsedHeader.payload
    )
    defer { handles = decoder.remainingHandles }
    return try decoder.decodeRootStruct(Payload.self)
  }
}
