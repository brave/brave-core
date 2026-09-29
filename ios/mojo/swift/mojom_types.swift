// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// The protocols generated bindings conform to, and their conformances for
// built in types.
//
// Every value occupies a fixed size slot in its enclosing struct, union or
// array. Primitives, enums, handles and inlined unions are stored in their
// slot, strings, arrays, maps and structs are stored out of line with a
// pointer to them in the slot.

/// A value that can be encoded into a slot of a struct, union or array.
protocol MojomValue: Sendable {
  /// Size of the slot holding the value within a struct, union or array.
  static var slotSize: Int { get }

  /// Encodes the value into the slot at `offset`, allocating any out of line
  /// data it needs.
  func encode(to encoder: inout MojoEncoder, at offset: Int)

  /// Decodes a value from the slot at `offset`.
  init(from decoder: inout MojoDecoder, at offset: Int) throws

  /// Encodes `values` as the elements of an array, returning the array's
  /// offset.
  static func encodeArrayData(_ values: [Self], to encoder: inout MojoEncoder) -> Int

  /// Decodes the elements of the array at `offset`.
  static func decodeArrayData(
    from decoder: inout MojoDecoder,
    at offset: Int,
    expectedCount: Int
  ) throws -> [Self]
}

extension MojomValue {
  static func encodeArrayData(_ values: [Self], to encoder: inout MojoEncoder) -> Int {
    let arrayOffset = encoder.allocateArray(
      count: values.count,
      storageSize: values.count * slotSize
    )
    for (index, value) in values.enumerated() {
      value.encode(to: &encoder, at: arrayOffset + mojoHeaderSize + index * slotSize)
    }
    return arrayOffset
  }

  static func decodeArrayData(
    from decoder: inout MojoDecoder,
    at offset: Int,
    expectedCount: Int
  ) throws -> [Self] {
    let count = try decoder.claimArray(at: offset, expectedCount: expectedCount) { $0 * slotSize }
    var values: [Self] = []
    values.reserveCapacity(count)
    for index in 0..<count {
      values.append(try Self(from: &decoder, at: offset + mojoHeaderSize + index * slotSize))
    }
    return values
  }
}

/// A value that may be null when made optional.
protocol MojomNullable: MojomValue {
  /// Encodes `value` into the slot at `offset`, or its null representation.
  static func encodeOptional(_ value: Self?, to encoder: inout MojoEncoder, at offset: Int)

  /// Decodes an optional value from the slot at `offset`.
  static func decodeOptional(
    from decoder: inout MojoDecoder,
    at offset: Int
  ) throws -> Self?

  /// Encodes `values` as the elements of an array of optionals, returning the
  /// array's offset.
  static func encodeOptionalArrayData(_ values: [Self?], to encoder: inout MojoEncoder) -> Int

  /// Decodes the elements of the array of optionals at `offset`.
  static func decodeOptionalArrayData(
    from decoder: inout MojoDecoder,
    at offset: Int,
    expectedCount: Int
  ) throws -> [Self?]
}

extension MojomNullable {
  // Null values are represented within their slot, which is zeroed unless
  // something is written to it.
  static func encodeOptionalArrayData(_ values: [Self?], to encoder: inout MojoEncoder) -> Int {
    let arrayOffset = encoder.allocateArray(
      count: values.count,
      storageSize: values.count * slotSize
    )
    for (index, value) in values.enumerated() {
      encodeOptional(value, to: &encoder, at: arrayOffset + mojoHeaderSize + index * slotSize)
    }
    return arrayOffset
  }

  static func decodeOptionalArrayData(
    from decoder: inout MojoDecoder,
    at offset: Int,
    expectedCount: Int
  ) throws -> [Self?] {
    let count = try decoder.claimArray(at: offset, expectedCount: expectedCount) { $0 * slotSize }
    var values: [Self?] = []
    values.reserveCapacity(count)
    for index in 0..<count {
      values.append(
        try decodeOptional(from: &decoder, at: offset + mojoHeaderSize + index * slotSize)
      )
    }
    return values
  }
}

extension Optional: MojomValue where Wrapped: MojomNullable {
  static var slotSize: Int { Wrapped.slotSize }

  func encode(to encoder: inout MojoEncoder, at offset: Int) {
    Wrapped.encodeOptional(self, to: &encoder, at: offset)
  }

  init(from decoder: inout MojoDecoder, at offset: Int) throws {
    self = try Wrapped.decodeOptional(from: &decoder, at: offset)
  }

  static func encodeArrayData(_ values: [Self], to encoder: inout MojoEncoder) -> Int {
    Wrapped.encodeOptionalArrayData(values, to: &encoder)
  }

  static func decodeArrayData(
    from decoder: inout MojoDecoder,
    at offset: Int,
    expectedCount: Int
  ) throws -> [Self] {
    try Wrapped.decodeOptionalArrayData(from: &decoder, at: offset, expectedCount: expectedCount)
  }
}

// MARK: - Out of line values

/// A value stored out of line, referenced by a pointer in its slot.
protocol MojomPointerValue: MojomNullable {
  /// Allocates and encodes the value's data, returning its offset.
  func encodeData(to encoder: inout MojoEncoder) -> Int

  /// Decodes the value's data at `offset`.
  static func decodeData(
    from decoder: inout MojoDecoder,
    at offset: Int
  ) throws -> Self
}

extension MojomPointerValue {
  static var slotSize: Int { 8 }

  func encode(to encoder: inout MojoEncoder, at offset: Int) {
    let target = encodeData(to: &encoder)
    encoder.writePointer(to: target, at: offset)
  }

  init(from decoder: inout MojoDecoder, at offset: Int) throws {
    self = try Self.decodeData(from: &decoder, at: try decoder.readNonNullPointer(at: offset))
  }

  static func encodeOptional(_ value: Self?, to encoder: inout MojoEncoder, at offset: Int) {
    // A null pointer is all zeroes, which the slot already is.
    value?.encode(to: &encoder, at: offset)
  }

  static func decodeOptional(
    from decoder: inout MojoDecoder,
    at offset: Int
  ) throws -> Self? {
    guard let target = try decoder.readPointer(at: offset) else {
      return nil
    }
    return try decodeData(from: &decoder, at: target)
  }
}

/// A version of a struct and its encoded size.
struct MojomStructVersion: Equatable, Sendable {
  var version: UInt32
  var numBytes: UInt32
}

/// A mojom struct.
///
/// Implementations only encode and decode their fields, at the offsets
/// computed by the generator from the mojom's packed layout. Out of line
/// fields must be encoded in ordinal order.
protocol MojomStruct: MojomPointerValue {
  /// Every version of the struct, in increasing order.
  static var versions: [MojomStructVersion] { get }

  /// Encodes the struct's fields relative to the struct at `offset`.
  func encodeFields(to encoder: inout MojoEncoder, at offset: Int)

  /// Decodes the struct's fields relative to the struct at `offset`. Fields
  /// newer than `version` are not present and must take their default value.
  init(
    fields decoder: inout MojoDecoder,
    at offset: Int,
    version: UInt32
  )
    throws
}

extension MojomStruct {
  func encodeData(to encoder: inout MojoEncoder) -> Int {
    let latest = Self.versions.last!
    let offset = encoder.allocate(Int(latest.numBytes))
    encoder.writeStructHeader(at: offset, numBytes: latest.numBytes, version: latest.version)
    encodeFields(to: &encoder, at: offset)
    return offset
  }

  static func decodeData(
    from decoder: inout MojoDecoder,
    at offset: Int
  ) throws -> Self {
    try decoder.nested { decoder in
      let header = try decoder.claimStruct(at: offset, versions: versions)
      return try Self(fields: &decoder, at: offset, version: header.version)
    }
  }
}

extension Array: MojomValue, MojomNullable, MojomPointerValue where Element: MojomValue {
  func encodeData(to encoder: inout MojoEncoder) -> Int {
    Element.encodeArrayData(self, to: &encoder)
  }

  static func decodeData(
    from decoder: inout MojoDecoder,
    at offset: Int
  ) throws -> [Element] {
    try decoder.nested { decoder in
      try Element.decodeArrayData(from: &decoder, at: offset, expectedCount: 0)
    }
  }
}

extension String: MojomPointerValue {
  func encodeData(to encoder: inout MojoEncoder) -> Int {
    UInt8.encodeArrayData(Array(utf8), to: &encoder)
  }

  static func decodeData(
    from decoder: inout MojoDecoder,
    at offset: Int
  ) throws -> String {
    let bytes = try decoder.nested { decoder in
      try UInt8.decodeArrayData(from: &decoder, at: offset, expectedCount: 0)
    }
    // Like the other bindings, invalid UTF-8 is not a validation error.
    return String(decoding: bytes, as: UTF8.self)
  }
}

/// Size of a map's struct: a header and pointers to its keys and values.
private let mojoMapStructSize: UInt32 = 24

extension Dictionary: MojomValue, MojomNullable, MojomPointerValue
where Key: MojomValue, Value: MojomValue {
  func encodeData(to encoder: inout MojoEncoder) -> Int {
    let offset = encoder.allocate(Int(mojoMapStructSize))
    encoder.writeStructHeader(at: offset, numBytes: mojoMapStructSize, version: 0)
    // Dictionary order is random, sort entries by their encoded keys so that
    // encoding is deterministic.
    let entries = map { entry in
      var keyEncoder = MojoEncoder()
      let keyOffset = keyEncoder.allocate(Key.slotSize)
      entry.key.encode(to: &keyEncoder, at: keyOffset)
      return (sortKey: keyEncoder.bytes, entry: entry)
    }
    .sorted { $0.sortKey.lexicographicallyPrecedes($1.sortKey) }
    let keys = Key.encodeArrayData(entries.map(\.entry.key), to: &encoder)
    encoder.writePointer(to: keys, at: offset + 8)
    let values = Value.encodeArrayData(entries.map(\.entry.value), to: &encoder)
    encoder.writePointer(to: values, at: offset + 16)
    return offset
  }

  static func decodeData(
    from decoder: inout MojoDecoder,
    at offset: Int
  ) throws -> [Key: Value] {
    try decoder.nested { decoder in
      let header = try decoder.claimStruct(
        at: offset,
        versions: [MojomStructVersion(version: 0, numBytes: mojoMapStructSize)]
      )
      guard header.version == 0, header.numBytes == mojoMapStructSize else {
        throw MojoDecodingError.unexpectedStructHeader
      }
      let keys = try [Key].decodeData(
        from: &decoder,
        at: try decoder.readNonNullPointer(at: offset + 8)
      )
      let values = try [Value].decodeData(
        from: &decoder,
        at: try decoder.readNonNullPointer(at: offset + 16)
      )
      guard keys.count == values.count else {
        throw MojoDecodingError.differentMapKeysAndValuesSizes
      }
      var map: [Key: Value] = [:]
      map.reserveCapacity(keys.count)
      for (key, value) in zip(keys, values) {
        map[key] = value
      }
      return map
    }
  }
}

// MARK: - Unions

/// A mojom union.
///
/// Unions are 16 bytes: a 4 byte size (16, or 0 for null), a 4 byte tag
/// identifying the field and 8 bytes holding the field. They're stored inline
/// in structs and arrays, but out of line when a union's field is a union.
protocol MojomUnion: MojomNullable {
  /// The tag of the current field.
  var tag: UInt32 { get }

  /// Encodes the current field into the union's 8 byte data slot at `offset`.
  func encodeField(to encoder: inout MojoEncoder, at offset: Int)

  /// Decodes the field identified by `tag` from the data slot at `offset`.
  init(tag: UInt32, from decoder: inout MojoDecoder, at offset: Int) throws
}

extension MojomUnion {
  static var slotSize: Int { mojoUnionSize }

  func encode(to encoder: inout MojoEncoder, at offset: Int) {
    encoder.write(UInt32(mojoUnionSize), at: offset)
    encoder.write(tag, at: offset + 4)
    encodeField(to: &encoder, at: offset + 8)
  }

  init(from decoder: inout MojoDecoder, at offset: Int) throws {
    guard let value = try Self.decodeOptional(from: &decoder, at: offset) else {
      throw MojoDecodingError.unexpectedNullUnion
    }
    self = value
  }

  static func encodeOptional(_ value: Self?, to encoder: inout MojoEncoder, at offset: Int) {
    // A null union has a size and tag of 0, which the slot already has.
    value?.encode(to: &encoder, at: offset)
  }

  static func decodeOptional(
    from decoder: inout MojoDecoder,
    at offset: Int
  ) throws -> Self? {
    let size: UInt32 = try decoder.read(at: offset)
    if size == 0 {
      return nil
    }
    let tag: UInt32 = try decoder.read(at: offset + 4)
    return try Self(tag: tag, from: &decoder, at: offset + 8)
  }

  /// Encodes `value` out of line, with a pointer to it in the slot at `offset`.
  /// Used when a union is the field of another union.
  static func encodeNested(_ value: Self?, to encoder: inout MojoEncoder, at offset: Int) {
    guard let value else {
      return
    }
    let target = encoder.allocate(mojoUnionSize)
    value.encode(to: &encoder, at: target)
    encoder.writePointer(to: target, at: offset)
  }

  /// Decodes a union stored out of line by ``encodeNested(_:to:at:)``.
  static func decodeNested(
    from decoder: inout MojoDecoder,
    at offset: Int
  ) throws -> Self? {
    guard let target = try decoder.readPointer(at: offset) else {
      return nil
    }
    return try decoder.nested { decoder in
      try decoder.claimMemory(at: target, count: mojoUnionSize)
      let size: UInt32 = try decoder.read(at: target)
      guard size == mojoUnionSize else {
        throw MojoDecodingError.illegalMemoryRange
      }
      return try decodeOptional(from: &decoder, at: target)
    }
  }
}

// MARK: - Primitives

/// A fixed size value stored in its slot. Nullable primitives are stored in
/// structs as a separate "has value" bit, and in arrays with a bitfield of the
/// elements that have values preceding the values.
protocol MojomPrimitive: MojomNullable {}

extension MojomPrimitive {
  static func encodeOptional(_ value: Self?, to encoder: inout MojoEncoder, at offset: Int) {
    preconditionFailure("Nullable primitives are only supported in structs and arrays")
  }

  static func decodeOptional(
    from decoder: inout MojoDecoder,
    at offset: Int
  ) throws -> Self? {
    preconditionFailure("Nullable primitives are only supported in structs and arrays")
  }

  static func encodeOptionalArrayData(_ values: [Self?], to encoder: inout MojoEncoder) -> Int {
    let flagsSize = optionalArrayFlagsSize(count: values.count)
    let arrayOffset = encoder.allocateArray(
      count: values.count,
      storageSize: flagsSize + values.count * slotSize
    )
    let flagsOffset = arrayOffset + mojoHeaderSize
    let valuesOffset = flagsOffset + flagsSize
    for (index, value) in values.enumerated() {
      guard let value else {
        continue
      }
      encoder.write(true, at: flagsOffset + index / 8, bit: index % 8)
      value.encode(to: &encoder, at: valuesOffset + index * slotSize)
    }
    return arrayOffset
  }

  static func decodeOptionalArrayData(
    from decoder: inout MojoDecoder,
    at offset: Int,
    expectedCount: Int
  ) throws -> [Self?] {
    let count = try decoder.claimArray(at: offset, expectedCount: expectedCount) {
      optionalArrayFlagsSize(count: $0) + $0 * slotSize
    }
    let flagsOffset = offset + mojoHeaderSize
    let valuesOffset = flagsOffset + optionalArrayFlagsSize(count: count)
    var values: [Self?] = []
    values.reserveCapacity(count)
    for index in 0..<count {
      if try decoder.read(at: flagsOffset + index / 8, bit: index % 8) {
        values.append(try Self(from: &decoder, at: valuesOffset + index * slotSize))
      } else {
        values.append(nil)
      }
    }
    return values
  }

  /// Size of the bitfield preceding the values of an array of nullable
  /// primitives, padded so that the values are aligned.
  private static func optionalArrayFlagsSize(count: Int) -> Int {
    let size = (count + 7) / 8
    return (size + slotSize - 1) / slotSize * slotSize
  }
}

extension MojomPrimitive where Self: FixedWidthInteger {
  static var slotSize: Int { MemoryLayout<Self>.size }

  func encode(to encoder: inout MojoEncoder, at offset: Int) {
    encoder.write(self, at: offset)
  }

  init(from decoder: inout MojoDecoder, at offset: Int) throws {
    self = try decoder.read(at: offset)
  }
}

extension Int8: MojomPrimitive {}
extension Int16: MojomPrimitive {}
extension Int32: MojomPrimitive {}
extension Int64: MojomPrimitive {}
extension UInt8: MojomPrimitive {}
extension UInt16: MojomPrimitive {}
extension UInt32: MojomPrimitive {}
extension UInt64: MojomPrimitive {}

extension Float: MojomPrimitive {
  static var slotSize: Int { 4 }

  func encode(to encoder: inout MojoEncoder, at offset: Int) {
    encoder.write(self, at: offset)
  }

  init(from decoder: inout MojoDecoder, at offset: Int) throws {
    self = try decoder.read(at: offset)
  }
}

extension Double: MojomPrimitive {
  static var slotSize: Int { 8 }

  func encode(to encoder: inout MojoEncoder, at offset: Int) {
    encoder.write(self, at: offset)
  }

  init(from decoder: inout MojoDecoder, at offset: Int) throws {
    self = try decoder.read(at: offset)
  }
}

/// Bools take a byte in unions, are packed into bits in structs (which the
/// generator handles) and in arrays.
extension Bool: MojomPrimitive {
  static var slotSize: Int { 1 }

  func encode(to encoder: inout MojoEncoder, at offset: Int) {
    encoder.write(self, at: offset, bit: 0)
  }

  init(from decoder: inout MojoDecoder, at offset: Int) throws {
    self = try decoder.read(at: offset, bit: 0)
  }

  static func encodeArrayData(_ values: [Bool], to encoder: inout MojoEncoder) -> Int {
    let arrayOffset = encoder.allocateArray(
      count: values.count,
      storageSize: (values.count + 7) / 8
    )
    for (index, value) in values.enumerated() where value {
      encoder.write(true, at: arrayOffset + mojoHeaderSize + index / 8, bit: index % 8)
    }
    return arrayOffset
  }

  static func decodeArrayData(
    from decoder: inout MojoDecoder,
    at offset: Int,
    expectedCount: Int
  ) throws -> [Bool] {
    let count = try decoder.claimArray(at: offset, expectedCount: expectedCount) { ($0 + 7) / 8 }
    var values: [Bool] = []
    values.reserveCapacity(count)
    for index in 0..<count {
      values.append(try decoder.read(at: offset + mojoHeaderSize + index / 8, bit: index % 8))
    }
    return values
  }

  // Arrays of nullable bools are a bitfield of which elements have values
  // followed by a bitfield of the values.
  static func encodeOptionalArrayData(_ values: [Bool?], to encoder: inout MojoEncoder) -> Int {
    let bitfieldSize = (values.count + 7) / 8
    let arrayOffset = encoder.allocateArray(count: values.count, storageSize: bitfieldSize * 2)
    let flagsOffset = arrayOffset + mojoHeaderSize
    for (index, value) in values.enumerated() {
      guard let value else {
        continue
      }
      encoder.write(true, at: flagsOffset + index / 8, bit: index % 8)
      encoder.write(value, at: flagsOffset + bitfieldSize + index / 8, bit: index % 8)
    }
    return arrayOffset
  }

  static func decodeOptionalArrayData(
    from decoder: inout MojoDecoder,
    at offset: Int,
    expectedCount: Int
  ) throws -> [Bool?] {
    let count = try decoder.claimArray(at: offset, expectedCount: expectedCount) {
      ($0 + 7) / 8 * 2
    }
    let bitfieldSize = (count + 7) / 8
    let flagsOffset = offset + mojoHeaderSize
    var values: [Bool?] = []
    values.reserveCapacity(count)
    for index in 0..<count {
      if try decoder.read(at: flagsOffset + index / 8, bit: index % 8) {
        values.append(
          try decoder.read(at: flagsOffset + bitfieldSize + index / 8, bit: index % 8)
        )
      } else {
        values.append(nil)
      }
    }
    return values
  }
}

// MARK: - Enums

/// A mojom enum, encoded as its 32 bit value.
protocol MojomEnum: MojomPrimitive, RawRepresentable where RawValue == Int32 {
  /// The value unknown values decode as, for `[Extensible]` enums. Decoding an
  /// unknown value of a non-extensible enum fails.
  static var defaultForUnknownValue: Self? { get }
}

extension MojomEnum {
  static var slotSize: Int { 4 }

  func encode(to encoder: inout MojoEncoder, at offset: Int) {
    encoder.write(rawValue, at: offset)
  }

  init(from decoder: inout MojoDecoder, at offset: Int) throws {
    let rawValue: Int32 = try decoder.read(at: offset)
    if let value = Self(rawValue: rawValue) {
      self = value
    } else if let value = Self.defaultForUnknownValue {
      self = value
    } else {
      throw MojoDecodingError.unknownEnumValue
    }
  }
}
