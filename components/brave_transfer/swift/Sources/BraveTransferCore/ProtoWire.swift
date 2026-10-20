// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

/// Minimal protobuf wire format for brave_transfer.proto (PROTOCOL.md §5).
/// SwiftProtobuf is not available to brave-ios, so the codec is hand-written.

public struct DecodeError: Error, CustomStringConvertible {
  public let description: String
  init(_ description: String) { self.description = description }
}

enum WireType: UInt64 {
  case varint = 0
  case i64 = 1
  case len = 2
  case i32 = 5
}

enum FieldValue {
  case varint(UInt64)
  case lengthDelimited(Bytes)
  case fixed
}

struct ProtoWriter {
  private(set) var bytes = Bytes()

  mutating func writeVarint(_ value: UInt64) {
    var v = value
    while v >= 0x80 {
      bytes.append(UInt8(truncatingIfNeeded: v) | 0x80)
      v >>= 7
    }
    bytes.append(UInt8(v))
  }

  private mutating func writeKey(_ field: UInt32, _ wireType: WireType) {
    writeVarint(UInt64(field) << 3 | wireType.rawValue)
  }

  /// Omitted when zero (proto3 default).
  mutating func writeUInt(_ field: UInt32, _ value: UInt64) {
    guard value != 0 else { return }
    writeKey(field, .varint)
    writeVarint(value)
  }

  mutating func writeUInt(_ field: UInt32, _ value: UInt32) {
    writeUInt(field, UInt64(value))
  }

  /// Omitted when empty (proto3 default).
  mutating func writeBytes(_ field: UInt32, _ value: Bytes) {
    guard !value.isEmpty else { return }
    writeLengthDelimited(field, value)
  }

  mutating func writeString(_ field: UInt32, _ value: String) {
    writeBytes(field, Bytes(value.utf8))
  }

  /// Always written, even when empty: used for sub-messages.
  mutating func writeLengthDelimited(_ field: UInt32, _ value: Bytes) {
    writeKey(field, .len)
    writeVarint(UInt64(value.count))
    bytes += value
  }

  mutating func writePacked(_ field: UInt32, _ values: [UInt32]) {
    guard !values.isEmpty else { return }
    var inner = ProtoWriter()
    for value in values {
      inner.writeVarint(UInt64(value))
    }
    writeLengthDelimited(field, inner.bytes)
  }
}

struct ProtoReader {
  private let data: Bytes
  private var pos = 0

  init(_ data: Bytes) {
    self.data = data
  }

  var isAtEnd: Bool { pos >= data.count }

  /// Like protobuf, bits beyond 64 in a 10-byte varint are dropped.
  mutating func readVarint() throws -> UInt64 {
    var result: UInt64 = 0
    for i in 0..<10 {
      guard pos < data.count else { throw DecodeError("truncated varint") }
      let byte = data[pos]
      pos += 1
      result |= UInt64(byte & 0x7f) << UInt64(7 * i)
      if byte & 0x80 == 0 {
        return result
      }
    }
    throw DecodeError("varint too long")
  }

  private mutating func take(_ count: UInt64) throws -> Bytes {
    guard count <= UInt64(data.count - pos) else { throw DecodeError("truncated field") }
    let end = pos + Int(count)
    defer { pos = end }
    return Bytes(data[pos..<end])
  }

  mutating func nextField() throws -> (field: UInt64, value: FieldValue)? {
    guard !isAtEnd else { return nil }
    let key = try readVarint()
    let field = key >> 3
    guard field != 0, field < 1 << 29 else { throw DecodeError("bad field number") }
    switch WireType(rawValue: key & 7) {
    case .varint: return (field, .varint(try readVarint()))
    case .i64:
      _ = try take(8)
      return (field, .fixed)
    case .len: return (field, .lengthDelimited(try take(try readVarint())))
    case .i32:
      _ = try take(4)
      return (field, .fixed)
    case nil: throw DecodeError("unsupported wire type")
    }
  }

  /// Calls `body` for every field. Known fields with the wrong wire type
  /// are the caller's to skip, like protobuf does.
  static func forEachField(
    _ data: Bytes, _ body: (UInt64, FieldValue) throws -> Void
  ) throws {
    var reader = ProtoReader(data)
    while let (field, value) = try reader.nextField() {
      try body(field, value)
    }
  }
}

extension FieldValue {
  var uint64: UInt64? {
    if case .varint(let v) = self { return v }
    return nil
  }

  var uint32: UInt32? {
    uint64.map { UInt32(truncatingIfNeeded: $0) }
  }

  var bytes: Bytes? {
    if case .lengthDelimited(let b) = self { return b }
    return nil
  }

  func string() throws -> String? {
    guard let b = bytes else { return nil }
    guard let s = decodeUTF8Strictly(b) else { throw DecodeError("invalid UTF-8") }
    return s
  }
}
