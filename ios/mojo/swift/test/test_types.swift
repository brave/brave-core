// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Hand written equivalents of generated bindings, used to test the runtime.
// Field offsets follow mojom's packing rules (see mojom/generate/pack.py).

@testable import SwiftMojo

/// Builds expected wire bytes.
struct WireBytes {
  private(set) var bytes: [UInt8] = []

  mutating func append<T: FixedWidthInteger>(_ value: T) {
    withUnsafeBytes(of: value.littleEndian) { bytes.append(contentsOf: $0) }
  }

  mutating func append(_ value: Double) {
    append(value.bitPattern)
  }

  mutating func append(bytes other: [UInt8]) {
    bytes.append(contentsOf: other)
  }

  /// Appends a struct or array header.
  mutating func appendHeader(numBytes: UInt32, versionOrCount: UInt32) {
    append(numBytes)
    append(versionOrCount)
  }

  /// Pads with zeroes to the next 8 byte boundary.
  mutating func pad() {
    while bytes.count % 8 != 0 {
      bytes.append(0)
    }
  }
}

extension MojomStruct {
  /// Encodes the struct on its own, as a message payload would be.
  func encodedBytes() -> [UInt8] {
    var encoder = MojoEncoder()
    encoder.encodeStruct(self)
    return encoder.finish().bytes
  }

  /// Decodes a struct encoded on its own.
  static func decode(_ bytes: [UInt8]) throws -> Self {
    var decoder = MojoDecoder(bytes: bytes, handles: [], range: 0..<bytes.count)
    return try decoder.decodeRootStruct(Self.self)
  }
}

// struct Primitives {
//   bool a;
//   int32 b;
//   bool c;
//   int8 d;
//   uint16 e;
//   int64 f;
// };
//
// Packed as: a@8.0 c@8.1 d@9 e@10 b@12 f@16, 24 bytes.
struct Primitives: MojomStruct, Equatable {
  var a = false
  var b: Int32 = 0
  var c = false
  var d: Int8 = 0
  var e: UInt16 = 0
  var f: Int64 = 0

  static let versions = [MojomStructVersion(version: 0, numBytes: 24)]

  init(a: Bool, b: Int32, c: Bool, d: Int8, e: UInt16, f: Int64) {
    self.a = a
    self.b = b
    self.c = c
    self.d = d
    self.e = e
    self.f = f
  }

  func encodeFields(to encoder: inout MojoEncoder, at offset: Int) {
    encoder.write(a, at: offset + 8, bit: 0)
    encoder.write(b, at: offset + 12)
    encoder.write(c, at: offset + 8, bit: 1)
    encoder.write(d, at: offset + 9)
    encoder.write(e, at: offset + 10)
    encoder.write(f, at: offset + 16)
  }

  init(
    fields decoder: inout MojoDecoder,
    at offset: Int,
    version: UInt32
  )
    throws
  {
    a = try decoder.read(at: offset + 8, bit: 0)
    b = try decoder.read(at: offset + 12)
    c = try decoder.read(at: offset + 8, bit: 1)
    d = try decoder.read(at: offset + 9)
    e = try decoder.read(at: offset + 10)
    f = try decoder.read(at: offset + 16)
  }
}

enum Color: Int32, MojomEnum {
  case red = 0
  case green = 1
  case blue = 2

  static let defaultForUnknownValue: Color? = nil
}

// [Extensible]
enum Shape: Int32, MojomEnum {
  // [Default]
  case unknown = 0
  case circle = 1

  static let defaultForUnknownValue: Shape? = .unknown
}

// union Inner {
//   int8 small;
//   bool flag;
// };
enum Inner: MojomUnion, Equatable {
  case small(Int8)
  case flag(Bool)

  var tag: UInt32 {
    switch self {
    case .small: 0
    case .flag: 1
    }
  }

  func encodeField(to encoder: inout MojoEncoder, at offset: Int) {
    switch self {
    case .small(let value):
      value.encode(to: &encoder, at: offset)
    case .flag(let value):
      value.encode(to: &encoder, at: offset)
    }
  }

  init(
    tag: UInt32,
    from decoder: inout MojoDecoder,
    at offset: Int
  )
    throws
  {
    switch tag {
    case 0:
      self = .small(try Int8(from: &decoder, at: offset))
    case 1:
      self = .flag(try Bool(from: &decoder, at: offset))
    default:
      throw MojoDecodingError.unknownUnionTag
    }
  }
}

// union Value {
//   int32 number;
//   string text;
//   Inner? nested;
// };
enum Value: MojomUnion, Equatable {
  case number(Int32)
  case text(String)
  case nested(Inner?)

  var tag: UInt32 {
    switch self {
    case .number: 0
    case .text: 1
    case .nested: 2
    }
  }

  func encodeField(to encoder: inout MojoEncoder, at offset: Int) {
    switch self {
    case .number(let value):
      value.encode(to: &encoder, at: offset)
    case .text(let value):
      value.encode(to: &encoder, at: offset)
    case .nested(let value):
      Inner.encodeNested(value, to: &encoder, at: offset)
    }
  }

  init(
    tag: UInt32,
    from decoder: inout MojoDecoder,
    at offset: Int
  )
    throws
  {
    switch tag {
    case 0:
      self = .number(try Int32(from: &decoder, at: offset))
    case 1:
      self = .text(try String(from: &decoder, at: offset))
    case 2:
      self = .nested(try Inner.decodeNested(from: &decoder, at: offset))
    default:
      throw MojoDecodingError.unknownUnionTag
    }
  }
}

// struct Containers {
//   string name;
//   array<int32> numbers;
//   double ratio;
// };
//
// Packed as: name@8 numbers@16 ratio@24, 32 bytes.
struct Containers: MojomStruct, Equatable {
  var name = ""
  var numbers: [Int32] = []
  var ratio = 0.0

  static let versions = [MojomStructVersion(version: 0, numBytes: 32)]

  init(name: String, numbers: [Int32], ratio: Double) {
    self.name = name
    self.numbers = numbers
    self.ratio = ratio
  }

  func encodeFields(to encoder: inout MojoEncoder, at offset: Int) {
    name.encode(to: &encoder, at: offset + 8)
    numbers.encode(to: &encoder, at: offset + 16)
    encoder.write(ratio, at: offset + 24)
  }

  init(
    fields decoder: inout MojoDecoder,
    at offset: Int,
    version: UInt32
  )
    throws
  {
    name = try String(from: &decoder, at: offset + 8)
    numbers = try [Int32](from: &decoder, at: offset + 16)
    ratio = try decoder.read(at: offset + 24)
  }
}

// struct Everything {
//   string? optional_name;
//   Color color;
//   int32? maybe;
//   Value value;
//   array<string?> names;
//   map<string, int32> counts;
//   array<bool> flags;
//   array<int16?> maybe_numbers;
//   array<bool?> maybe_flags;
//   Containers? child;
//   Shape shape;
// };
//
// Packed as: optional_name@8 color@16 maybe's flag@20.0 maybe@24 shape@28
// value@32 names@48 counts@56 flags@64 maybe_numbers@72 maybe_flags@80
// child@88, 96 bytes.
struct Everything: MojomStruct, Equatable {
  var optionalName: String?
  var color = Color.red
  var maybe: Int32?
  var value = Value.number(0)
  var names: [String?] = []
  var counts: [String: Int32] = [:]
  var flags: [Bool] = []
  var maybeNumbers: [Int16?] = []
  var maybeFlags: [Bool?] = []
  var child: Containers?
  var shape = Shape.unknown

  static let versions = [MojomStructVersion(version: 0, numBytes: 96)]

  init() {}

  func encodeFields(to encoder: inout MojoEncoder, at offset: Int) {
    optionalName.encode(to: &encoder, at: offset + 8)
    color.encode(to: &encoder, at: offset + 16)
    encoder.write(maybe != nil, at: offset + 20, bit: 0)
    if let maybe {
      encoder.write(maybe, at: offset + 24)
    }
    value.encode(to: &encoder, at: offset + 32)
    names.encode(to: &encoder, at: offset + 48)
    counts.encode(to: &encoder, at: offset + 56)
    flags.encode(to: &encoder, at: offset + 64)
    maybeNumbers.encode(to: &encoder, at: offset + 72)
    maybeFlags.encode(to: &encoder, at: offset + 80)
    child.encode(to: &encoder, at: offset + 88)
    shape.encode(to: &encoder, at: offset + 28)
  }

  init(
    fields decoder: inout MojoDecoder,
    at offset: Int,
    version: UInt32
  )
    throws
  {
    optionalName = try String?(from: &decoder, at: offset + 8)
    color = try Color(from: &decoder, at: offset + 16)
    if try decoder.read(at: offset + 20, bit: 0) {
      maybe = try decoder.read(at: offset + 24)
    }
    value = try Value(from: &decoder, at: offset + 32)
    names = try [String?](from: &decoder, at: offset + 48)
    counts = try [String: Int32](from: &decoder, at: offset + 56)
    flags = try [Bool](from: &decoder, at: offset + 64)
    maybeNumbers = try [Int16?](from: &decoder, at: offset + 72)
    maybeFlags = try [Bool?](from: &decoder, at: offset + 80)
    child = try Containers?(from: &decoder, at: offset + 88)
    shape = try Shape(from: &decoder, at: offset + 28)
  }
}

// struct Versioned {
//   int32 a;
//   [MinVersion=1] int32 b;
//   [MinVersion=2] string? c;
// };
struct Versioned: MojomStruct, Equatable {
  var a: Int32 = 0
  var b: Int32 = 0
  var c: String?

  static let versions = [
    MojomStructVersion(version: 0, numBytes: 16),
    MojomStructVersion(version: 1, numBytes: 16),
    MojomStructVersion(version: 2, numBytes: 24),
  ]

  init(a: Int32, b: Int32, c: String?) {
    self.a = a
    self.b = b
    self.c = c
  }

  func encodeFields(to encoder: inout MojoEncoder, at offset: Int) {
    encoder.write(a, at: offset + 8)
    encoder.write(b, at: offset + 12)
    c.encode(to: &encoder, at: offset + 16)
  }

  init(
    fields decoder: inout MojoDecoder,
    at offset: Int,
    version: UInt32
  )
    throws
  {
    a = try decoder.read(at: offset + 8)
    if version >= 1 {
      b = try decoder.read(at: offset + 12)
    }
    if version >= 2 {
      c = try String?(from: &decoder, at: offset + 16)
    }
  }
}

// struct Tree {
//   array<Tree> children;
// };
struct Tree: MojomStruct, Equatable {
  var children: [Tree] = []

  static let versions = [MojomStructVersion(version: 0, numBytes: 16)]

  init(children: [Tree]) {
    self.children = children
  }

  /// A tree nested `depth` levels deep.
  static func nested(depth: Int) -> Tree {
    var tree = Tree(children: [])
    for _ in 0..<depth {
      tree = Tree(children: [tree])
    }
    return tree
  }

  func encodeFields(to encoder: inout MojoEncoder, at offset: Int) {
    children.encode(to: &encoder, at: offset + 8)
  }

  init(
    fields decoder: inout MojoDecoder,
    at offset: Int,
    version: UInt32
  )
    throws
  {
    children = try [Tree](from: &decoder, at: offset + 8)
  }
}
