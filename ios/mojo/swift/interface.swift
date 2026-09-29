// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/// A mojom interface.
public protocol MojomInterface {
  /// The interface's fully qualified mojom name, e.g. `skus.mojom.SkusService`.
  static var name: String { get }
  /// The interface's version, the highest `[MinVersion]` of its methods.
  static var version: UInt32 { get }
}

/// The remote end of an interface's message pipe that has not been bound yet.
///
/// Copies share the same pipe, sending or binding one invalidates the others.
public struct PendingRemote<Interface: MojomInterface>: Sendable {
  let handle: MojoHandle
  /// The version of the interface implemented by the receiver.
  public let version: UInt32

  init(handle: MojoHandle, version: UInt32) {
    self.handle = handle
    self.version = version
  }

  /// Whether this still owns its pipe.
  public var isValid: Bool {
    handle.isValid
  }
}

/// The receiving end of an interface's message pipe that has not been bound
/// yet.
///
/// Copies share the same pipe, sending or binding one invalidates the others.
public struct PendingReceiver<Interface: MojomInterface>: Sendable {
  let handle: MojoHandle

  init(handle: MojoHandle) {
    self.handle = handle
  }

  /// Whether this still owns its pipe.
  public var isValid: Bool {
    handle.isValid
  }
}

extension PendingRemote {
  /// Creates a message pipe for `Interface`, returning its two ends.
  static func makePipe() throws -> (
    remote: PendingRemote<Interface>, receiver: PendingReceiver<Interface>
  ) {
    let pipe = try MessagePipe()
    return (
      PendingRemote(handle: pipe.handle0, version: Interface.version),
      PendingReceiver(handle: pipe.handle1)
    )
  }
}

// An interface is encoded as its pipe's handle index followed by its version.
extension PendingRemote: MojomNullable {
  static var slotSize: Int { 8 }

  func encode(to encoder: inout MojoEncoder, at offset: Int) {
    encoder.write(encoder.append(handle), at: offset)
    encoder.write(version, at: offset + 4)
  }

  init(from decoder: inout MojoDecoder, at offset: Int) throws {
    guard let value = try Self.decodeOptional(from: &decoder, at: offset) else {
      throw MojoDecodingError.unexpectedInvalidHandle
    }
    self = value
  }

  static func encodeOptional(_ value: Self?, to encoder: inout MojoEncoder, at offset: Int) {
    guard let value else {
      encoder.write(mojoInvalidHandleIndex, at: offset)
      return
    }
    value.encode(to: &encoder, at: offset)
  }

  static func decodeOptional(
    from decoder: inout MojoDecoder,
    at offset: Int
  ) throws -> Self? {
    let index: UInt32 = try decoder.read(at: offset)
    let version: UInt32 = try decoder.read(at: offset + 4)
    guard let handle = try decoder.claimHandle(index) else {
      return nil
    }
    return PendingRemote(handle: handle, version: version)
  }
}

// A pending receiver is encoded as its pipe's handle index.
extension PendingReceiver: MojomNullable {
  static var slotSize: Int { 4 }

  func encode(to encoder: inout MojoEncoder, at offset: Int) {
    encoder.write(encoder.append(handle), at: offset)
  }

  init(from decoder: inout MojoDecoder, at offset: Int) throws {
    guard let value = try Self.decodeOptional(from: &decoder, at: offset) else {
      throw MojoDecodingError.unexpectedInvalidHandle
    }
    self = value
  }

  static func encodeOptional(_ value: Self?, to encoder: inout MojoEncoder, at offset: Int) {
    guard let value else {
      encoder.write(mojoInvalidHandleIndex, at: offset)
      return
    }
    value.encode(to: &encoder, at: offset)
  }

  static func decodeOptional(
    from decoder: inout MojoDecoder,
    at offset: Int
  ) throws -> Self? {
    let index: UInt32 = try decoder.read(at: offset)
    guard let handle = try decoder.claimHandle(index) else {
      return nil
    }
    return PendingReceiver(handle: handle)
  }
}
