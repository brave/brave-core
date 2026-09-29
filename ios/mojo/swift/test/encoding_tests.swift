// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import XCTest

@testable import SwiftMojo

/// Reads the pointer at `offset` of `bytes`, returning its absolute target.
private func pointerTarget(in bytes: [UInt8], at offset: Int) throws -> Int {
  try XCTUnwrap(try ByteReader(bytes: bytes).readPointer(at: offset))
}

/// Overwrites the little-endian `value` at `offset` of `bytes`.
private func patch<T: FixedWidthInteger>(_ bytes: inout [UInt8], _ value: T, at offset: Int) {
  withUnsafeBytes(of: value.littleEndian) {
    bytes.replaceSubrange(offset..<offset + MemoryLayout<T>.size, with: $0)
  }
}

private func assertDecodingError<T>(
  _ expression: @autoclosure () throws -> T,
  _ expected: MojoDecodingError,
  file: StaticString = #filePath,
  line: UInt = #line
) {
  do {
    _ = try expression()
    XCTFail("Expected \(expected) to be thrown", file: file, line: line)
  } catch {
    XCTAssertEqual(error as? MojoDecodingError, expected, file: file, line: line)
  }
}

private enum TestInterface: MojomInterface {
  static let name = "swift_mojo_test.mojom.TestInterface"
  static let version: UInt32 = 0
}

// struct Receivers {
//   pending_receiver<TestInterface>? receiver;
// };
private struct Receivers: MojomStruct {
  var receiver: PendingReceiver<TestInterface>?

  static let versions = [MojomStructVersion(version: 0, numBytes: 16)]

  init(receiver: PendingReceiver<TestInterface>?) {
    self.receiver = receiver
  }

  func encodeFields(to encoder: inout MojoEncoder, at offset: Int) {
    receiver.encode(to: &encoder, at: offset + 8)
  }

  init(
    fields decoder: inout MojoDecoder,
    at offset: Int,
    version: UInt32
  )
    throws
  {
    receiver = try PendingReceiver<TestInterface>?(from: &decoder, at: offset + 8)
  }
}

final class EncodingTests: XCTestCase {
  func testPrimitivesWireFormat() throws {
    let value = Primitives(a: true, b: -2, c: true, d: -1, e: 0x1234, f: 0x0102_0304_0506_0708)

    var expected = WireBytes()
    expected.appendHeader(numBytes: 24, versionOrCount: 0)
    expected.append(UInt8(0b11))
    expected.append(Int8(-1))
    expected.append(UInt16(0x1234))
    expected.append(Int32(-2))
    expected.append(Int64(0x0102_0304_0506_0708))

    XCTAssertEqual(value.encodedBytes(), expected.bytes)
    XCTAssertEqual(try Primitives.decode(expected.bytes), value)
  }

  func testContainersWireFormat() throws {
    let value = Containers(name: "hello", numbers: [1, 2, 3], ratio: 0.5)

    var expected = WireBytes()
    expected.appendHeader(numBytes: 32, versionOrCount: 0)
    // Pointers are relative to their own offset.
    expected.append(UInt64(32 - 8))
    expected.append(UInt64(48 - 16))
    expected.append(0.5)
    // "hello" at 32.
    expected.appendHeader(numBytes: 8 + 5, versionOrCount: 5)
    expected.append(bytes: Array("hello".utf8))
    expected.pad()
    // [1, 2, 3] at 48.
    expected.appendHeader(numBytes: 8 + 12, versionOrCount: 3)
    expected.append(Int32(1))
    expected.append(Int32(2))
    expected.append(Int32(3))
    expected.pad()

    XCTAssertEqual(value.encodedBytes(), expected.bytes)
    XCTAssertEqual(try Containers.decode(expected.bytes), value)
  }

  func testInlineUnionWireFormat() throws {
    var value = Everything()
    value.value = .number(42)
    let bytes = value.encodedBytes()

    var expected = WireBytes()
    expected.appendHeader(numBytes: 16, versionOrCount: 0)
    expected.append(Int32(42))
    expected.append(UInt32(0))
    XCTAssertEqual(Array(bytes[32..<48]), expected.bytes)
  }

  func testNestedUnionWireFormat() throws {
    var value = Everything()
    value.value = .nested(.flag(true))
    let bytes = value.encodedBytes()

    // The inner union is out of line, right after the struct.
    XCTAssertEqual(try pointerTarget(in: bytes, at: 40), 96)
    var expected = WireBytes()
    expected.appendHeader(numBytes: 16, versionOrCount: 1)
    expected.append(UInt64(1))
    XCTAssertEqual(Array(bytes[96..<112]), expected.bytes)
    XCTAssertEqual(try Everything.decode(bytes), value)
  }

  func testNullableArraysWireFormat() throws {
    var value = Everything()
    value.flags = [true, false, true, true, false, false, false, false, true]
    value.maybeNumbers = [1, nil, 3]
    value.maybeFlags = [true, nil, false]
    let bytes = value.encodedBytes()

    let flags = try pointerTarget(in: bytes, at: 64)
    var expectedFlags = WireBytes()
    expectedFlags.appendHeader(numBytes: 8 + 2, versionOrCount: 9)
    expectedFlags.append(UInt8(0b0000_1101))
    expectedFlags.append(UInt8(0b1))
    XCTAssertEqual(Array(bytes[flags..<flags + 10]), expectedFlags.bytes)

    // The engaged bitfield is padded to the element size.
    let maybeNumbers = try pointerTarget(in: bytes, at: 72)
    var expectedNumbers = WireBytes()
    expectedNumbers.appendHeader(numBytes: 8 + 2 + 6, versionOrCount: 3)
    expectedNumbers.append(UInt8(0b101))
    expectedNumbers.append(UInt8(0))
    expectedNumbers.append(Int16(1))
    expectedNumbers.append(Int16(0))
    expectedNumbers.append(Int16(3))
    XCTAssertEqual(Array(bytes[maybeNumbers..<maybeNumbers + 16]), expectedNumbers.bytes)

    // Nullable bools are an engaged bitfield followed by a value bitfield.
    let maybeFlags = try pointerTarget(in: bytes, at: 80)
    var expectedMaybeFlags = WireBytes()
    expectedMaybeFlags.appendHeader(numBytes: 8 + 2, versionOrCount: 3)
    expectedMaybeFlags.append(UInt8(0b101))
    expectedMaybeFlags.append(UInt8(0b001))
    XCTAssertEqual(Array(bytes[maybeFlags..<maybeFlags + 10]), expectedMaybeFlags.bytes)

    XCTAssertEqual(try Everything.decode(bytes), value)
  }

  func testNullablePrimitiveFieldWireFormat() throws {
    var value = Everything()
    XCTAssertEqual(value.encodedBytes()[20], 0)
    value.maybe = 7
    let bytes = value.encodedBytes()
    XCTAssertEqual(bytes[20], 1)
    XCTAssertEqual(try ByteReader(bytes: bytes).read(at: 24) as Int32, 7)
    XCTAssertEqual(try Everything.decode(bytes), value)
  }

  func testRoundTripEverything() throws {
    var value = Everything()
    value.optionalName = "name"
    value.color = .blue
    value.maybe = -5
    value.value = .text("union text")
    value.names = ["a", nil, "", "d"]
    value.counts = ["one": 1, "two": 2, "three": 3]
    value.flags = [true, false, true]
    value.maybeNumbers = [nil, 2, nil]
    value.maybeFlags = [nil, true, false]
    value.child = Containers(name: "child", numbers: [], ratio: -1)
    value.shape = .circle
    XCTAssertEqual(try Everything.decode(value.encodedBytes()), value)

    // Nulls and empties.
    XCTAssertEqual(try Everything.decode(Everything().encodedBytes()), Everything())
  }

  func testMapEncodingIsDeterministic() {
    var value = Everything()
    value.counts = Dictionary(uniqueKeysWithValues: (0..<32).map { ("key\($0)", Int32($0)) })
    let bytes = value.encodedBytes()
    for _ in 0..<10 {
      var copy = Everything()
      copy.counts = Dictionary(uniqueKeysWithValues: value.counts.shuffled())
      XCTAssertEqual(copy.encodedBytes(), bytes)
    }
  }

  func testUnknownEnumValue() throws {
    var bytes = Everything().encodedBytes()
    patch(&bytes, Int32(7), at: 16)
    assertDecodingError(try Everything.decode(bytes), .unknownEnumValue)
  }

  func testUnknownExtensibleEnumValueDecodesAsDefault() throws {
    var value = Everything()
    value.shape = .circle
    var bytes = value.encodedBytes()
    patch(&bytes, Int32(9), at: 28)
    XCTAssertEqual(try Everything.decode(bytes).shape, .unknown)
  }

  func testUnknownUnionTag() throws {
    var bytes = Everything().encodedBytes()
    patch(&bytes, UInt32(5), at: 36)
    assertDecodingError(try Everything.decode(bytes), .unknownUnionTag)
  }

  func testNullNonNullableUnion() throws {
    var bytes = Everything().encodedBytes()
    patch(&bytes, UInt32(0), at: 32)
    assertDecodingError(try Everything.decode(bytes), .unexpectedNullUnion)
  }

  func testNullNonNullablePointer() throws {
    var bytes = Containers(name: "a", numbers: [], ratio: 0).encodedBytes()
    patch(&bytes, UInt64(0), at: 8)
    assertDecodingError(try Containers.decode(bytes), .unexpectedNullPointer)
  }

  func testMisalignedPointer() throws {
    var bytes = Containers(name: "a", numbers: [], ratio: 0).encodedBytes()
    patch(&bytes, UInt64(28), at: 8)
    assertDecodingError(try Containers.decode(bytes), .misalignedObject)
  }

  func testPointerIntoEnclosingStruct() throws {
    var bytes = Containers(name: "a", numbers: [], ratio: 0).encodedBytes()
    patch(&bytes, UInt64(8), at: 8)
    assertDecodingError(try Containers.decode(bytes), .illegalMemoryRange)
  }

  func testObjectsOutOfOrder() throws {
    var bytes = Containers(name: "a", numbers: [], ratio: 0).encodedBytes()
    // Point `name` at `numbers`, and `numbers` at `name`.
    let name = try pointerTarget(in: bytes, at: 8)
    let numbers = try pointerTarget(in: bytes, at: 16)
    patch(&bytes, UInt64(numbers - 8), at: 8)
    patch(&bytes, UInt64(name - 16), at: 16)
    assertDecodingError(try Containers.decode(bytes), .illegalMemoryRange)
  }

  func testPointerOutOfRange() throws {
    var bytes = Containers(name: "a", numbers: [], ratio: 0).encodedBytes()
    patch(&bytes, UInt64(1) << 33, at: 8)
    assertDecodingError(try Containers.decode(bytes), .illegalPointer)
  }

  func testTruncated() throws {
    let bytes = Primitives(a: true, b: 1, c: false, d: 2, e: 3, f: 4).encodedBytes()
    assertDecodingError(try Primitives.decode(Array(bytes.prefix(16))), .illegalMemoryRange)
    assertDecodingError(try Primitives.decode(Array(bytes.prefix(4))), .outOfBounds)
  }

  func testUnexpectedStructSize() throws {
    var bytes = Primitives(a: true, b: 1, c: false, d: 2, e: 3, f: 4).encodedBytes()
    patch(&bytes, UInt32(16), at: 0)
    assertDecodingError(try Primitives.decode(bytes), .unexpectedStructHeader)
  }

  func testArrayTooSmallForElements() throws {
    var bytes = Containers(name: "abc", numbers: [], ratio: 0).encodedBytes()
    let name = try pointerTarget(in: bytes, at: 8)
    patch(&bytes, UInt32(10), at: name + 4)
    assertDecodingError(try Containers.decode(bytes), .unexpectedArrayHeader)
  }

  func testOlderVersionDecodesDefaults() throws {
    var older = WireBytes()
    older.appendHeader(numBytes: 16, versionOrCount: 0)
    older.append(Int32(1))
    older.append(Int32(99))
    // `b` is not part of version 0, so it's ignored.
    XCTAssertEqual(try Versioned.decode(older.bytes), Versioned(a: 1, b: 0, c: nil))

    var v1 = WireBytes()
    v1.appendHeader(numBytes: 16, versionOrCount: 1)
    v1.append(Int32(1))
    v1.append(Int32(2))
    XCTAssertEqual(try Versioned.decode(v1.bytes), Versioned(a: 1, b: 2, c: nil))
  }

  func testNewerVersionIsAccepted() throws {
    var newer = WireBytes()
    newer.appendHeader(numBytes: 32, versionOrCount: 5)
    newer.append(Int32(1))
    newer.append(Int32(2))
    newer.append(UInt64(0))
    // A field this code doesn't know about.
    newer.append(UInt64(0xFFFF))
    XCTAssertEqual(try Versioned.decode(newer.bytes), Versioned(a: 1, b: 2, c: nil))
  }

  func testVersionSizeMismatch() throws {
    // Version 2 must be 24 bytes.
    var wrongSize = WireBytes()
    wrongSize.appendHeader(numBytes: 16, versionOrCount: 2)
    wrongSize.append(Int32(1))
    wrongSize.append(Int32(2))
    assertDecodingError(try Versioned.decode(wrongSize.bytes), .unexpectedStructHeader)

    // Unknown versions must be at least as large as the latest known one.
    var tooSmall = WireBytes()
    tooSmall.appendHeader(numBytes: 16, versionOrCount: 3)
    tooSmall.append(Int32(1))
    tooSmall.append(Int32(2))
    assertDecodingError(try Versioned.decode(tooSmall.bytes), .unexpectedStructHeader)
  }

  func testRecursionDepth() throws {
    let shallow = Tree.nested(depth: 50)
    XCTAssertEqual(try Tree.decode(shallow.encodedBytes()), shallow)

    assertDecodingError(
      try Tree.decode(Tree.nested(depth: 150).encodedBytes()),
      .maxRecursionDepthExceeded
    )
  }

  func testNullHandle() throws {
    let bytes = Receivers(receiver: nil).encodedBytes()
    XCTAssertEqual(try ByteReader(bytes: bytes).read(at: 8) as UInt32, UInt32.max)
    XCTAssertNil(try Receivers.decode(bytes).receiver)
  }

  func testHandleIndexOutOfRange() throws {
    var bytes = Receivers(receiver: nil).encodedBytes()
    patch(&bytes, UInt32(0), at: 8)
    assertDecodingError(try Receivers.decode(bytes), .illegalHandle)
  }
}

final class MessageHeaderTests: XCTestCase {
  func testEncodesVersion1Header() throws {
    let header = MessageHeader(
      name: 3,
      flags: .expectsResponse,
      requestID: 0x0102_0304_0506_0708
    )
    let payload = Primitives(a: false, b: 1, c: false, d: 0, e: 0, f: 0)
    let message = Message(header: header, payload: payload)

    var expected = WireBytes()
    expected.appendHeader(numBytes: 32, versionOrCount: 1)
    expected.append(UInt32(0))
    expected.append(UInt32(3))
    expected.append(MessageFlags.expectsResponse.rawValue)
    expected.append(UInt32(0))
    expected.append(UInt64(0x0102_0304_0506_0708))
    expected.append(bytes: payload.encodedBytes())
    XCTAssertEqual(message.bytes, expected.bytes)

    let parsed = try message.parseHeader()
    XCTAssertEqual(parsed.header, header)
    XCTAssertEqual(parsed.version, 1)
    XCTAssertEqual(parsed.payload, 32..<message.bytes.count)

    var received = message
    XCTAssertEqual(try received.decodePayload(Primitives.self, parsedHeader: parsed), payload)
  }

  /// A header of `version` with `numBytes`, followed by `payload`, with the
  /// payload pointer set for versions 2 and later.
  private func makeMessage(
    version: UInt32,
    numBytes: UInt32,
    flags: MessageFlags = [],
    payload: [UInt8]
  ) -> Message {
    var bytes = WireBytes()
    bytes.appendHeader(numBytes: numBytes, versionOrCount: version)
    bytes.append(UInt32(0))
    bytes.append(UInt32(9))
    bytes.append(flags.rawValue)
    bytes.append(UInt32(0))
    while bytes.bytes.count < numBytes {
      bytes.append(UInt64(0))
    }
    var message = Message(bytes: bytes.bytes + payload)
    if version >= 2 {
      patch(&message.bytes, UInt64(numBytes - 32), at: 32)
    }
    return message
  }

  func testParsesAllVersions() throws {
    let payload = Primitives(a: true, b: 2, c: true, d: 3, e: 4, f: 5)
    for (version, numBytes) in [(0, 24), (1, 32), (2, 48), (3, 56), (7, 72)] as [(UInt32, UInt32)] {
      var message = makeMessage(
        version: version,
        numBytes: numBytes,
        payload: payload.encodedBytes()
      )
      let parsed = try message.parseHeader()
      XCTAssertEqual(parsed.version, version)
      XCTAssertEqual(parsed.header.name, 9)
      XCTAssertEqual(parsed.payload, Int(numBytes)..<message.bytes.count)
      XCTAssertFalse(parsed.hasPayloadInterfaceIDs)
      XCTAssertEqual(try message.decodePayload(Primitives.self, parsedHeader: parsed), payload)
    }
  }

  func testRejectsUnexpectedHeaderSize() {
    for (version, numBytes) in [(0, 32), (1, 24), (2, 56), (3, 48), (4, 48)] as [(UInt32, UInt32)] {
      let message = makeMessage(version: version, numBytes: numBytes, payload: [])
      assertDecodingError(try message.parseHeader(), .unexpectedStructHeader)
    }
  }

  func testRejectsMissingRequestID() {
    let message = makeMessage(version: 0, numBytes: 24, flags: .isResponse, payload: [])
    assertDecodingError(try message.parseHeader(), .missingRequestID)
  }

  func testRejectsContradictoryFlags() {
    let message = makeMessage(
      version: 1,
      numBytes: 32,
      flags: [.expectsResponse, .isResponse],
      payload: []
    )
    assertDecodingError(try message.parseHeader(), .invalidMessageFlags)
  }

  func testRejectsNullPayloadPointer() {
    var message = makeMessage(version: 2, numBytes: 48, payload: [0, 0, 0, 0, 0, 0, 0, 0])
    patch(&message.bytes, UInt64(0), at: 32)
    assertDecodingError(try message.parseHeader(), .unexpectedNullPointer)
  }

  func testRejectsPayloadPointerIntoHeader() {
    var message = makeMessage(version: 2, numBytes: 48, payload: [0, 0, 0, 0, 0, 0, 0, 0])
    patch(&message.bytes, UInt64(8), at: 32)
    assertDecodingError(try message.parseHeader(), .illegalMemoryRange)
  }

  func testTruncatedHeader() {
    let message = Message(bytes: [24, 0, 0, 0])
    assertDecodingError(try message.parseHeader(), .outOfBounds)
  }
}
