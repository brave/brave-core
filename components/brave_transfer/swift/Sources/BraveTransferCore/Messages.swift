// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

// Swift mirror of components/brave_transfer/core/brave_transfer.proto.
// Enums are open structs so that unknown values survive decoding and can be
// rejected by validation rather than by the codec.

public protocol ProtoMessage: Equatable {
  init()
  init(serialized: Bytes) throws
  func serialized() -> Bytes
}

public struct DeviceType: RawRepresentable, Hashable {
  public var rawValue: UInt32
  public init(rawValue: UInt32) { self.rawValue = rawValue }
  public static let unspecified = DeviceType(rawValue: 0)
  public static let desktop = DeviceType(rawValue: 1)
  public static let phone = DeviceType(rawValue: 2)
  public static let tablet = DeviceType(rawValue: 3)
}

public struct ItemKind: RawRepresentable, Hashable {
  public var rawValue: UInt32
  public init(rawValue: UInt32) { self.rawValue = rawValue }
  public static let unspecified = ItemKind(rawValue: 0)
  public static let file = ItemKind(rawValue: 1)
  public static let image = ItemKind(rawValue: 2)
  public static let text = ItemKind(rawValue: 3)
  public static let url = ItemKind(rawValue: 4)
}

public struct Decision: RawRepresentable, Hashable {
  public var rawValue: UInt32
  public init(rawValue: UInt32) { self.rawValue = rawValue }
  public static let unspecified = Decision(rawValue: 0)
  public static let acceptAll = Decision(rawValue: 1)
  public static let acceptSome = Decision(rawValue: 2)
  public static let decline = Decision(rawValue: 3)
  public static let busy = Decision(rawValue: 4)
  public static let noSpace = Decision(rawValue: 5)
}

public struct CancelReason: RawRepresentable, Hashable {
  public var rawValue: UInt32
  public init(rawValue: UInt32) { self.rawValue = rawValue }
  public static let unspecified = CancelReason(rawValue: 0)
  public static let user = CancelReason(rawValue: 1)
  public static let timeout = CancelReason(rawValue: 2)
  public static let error = CancelReason(rawValue: 3)
}

public struct DoneStatus: RawRepresentable, Hashable {
  public var rawValue: UInt32
  public init(rawValue: UInt32) { self.rawValue = rawValue }
  public static let unspecified = DoneStatus(rawValue: 0)
  public static let ok = DoneStatus(rawValue: 1)
  public static let hashMismatch = DoneStatus(rawValue: 2)
  public static let writeFailed = DoneStatus(rawValue: 3)
  public static let interrupted = DoneStatus(rawValue: 4)
}

public struct Capabilities: OptionSet, Hashable {
  public var rawValue: UInt64
  public init(rawValue: UInt64) { self.rawValue = rawValue }
  public static let receive = Capabilities(rawValue: 1 << 0)
  public static let send = Capabilities(rawValue: 1 << 1)
}

public struct ClientHello: ProtoMessage {
  public var protocolVersion: UInt32 = 0
  public var trustTag = Bytes()

  public init() {}

  public init(protocolVersion: UInt32, trustTag: Bytes) {
    self.protocolVersion = protocolVersion
    self.trustTag = trustTag
  }

  public init(serialized data: Bytes) throws {
    try ProtoReader.forEachField(data) { field, value in
      switch field {
      case 1: protocolVersion = value.uint32 ?? protocolVersion
      case 2: trustTag = value.bytes ?? trustTag
      default: break
      }
    }
  }

  public func serialized() -> Bytes {
    var w = ProtoWriter()
    w.writeUInt(1, protocolVersion)
    w.writeBytes(2, trustTag)
    return w.bytes
  }
}

public struct PeerInfo: ProtoMessage {
  public var protocolVersion: UInt32 = 0
  public var deviceName = ""
  public var deviceType = DeviceType.unspecified
  public var syncDeviceGUID = ""
  public var capabilities = Capabilities()

  public init() {}

  public init(
    protocolVersion: UInt32 = TransferProtocol.version,
    deviceName: String,
    deviceType: DeviceType,
    syncDeviceGUID: String,
    capabilities: Capabilities = [.receive, .send]
  ) {
    self.protocolVersion = protocolVersion
    self.deviceName = deviceName
    self.deviceType = deviceType
    self.syncDeviceGUID = syncDeviceGUID
    self.capabilities = capabilities
  }

  public init(serialized data: Bytes) throws {
    try ProtoReader.forEachField(data) { field, value in
      switch field {
      case 1: protocolVersion = value.uint32 ?? protocolVersion
      case 2: deviceName = try value.string() ?? deviceName
      case 3: deviceType = value.uint32.map(DeviceType.init) ?? deviceType
      case 4: syncDeviceGUID = try value.string() ?? syncDeviceGUID
      case 5: capabilities = value.uint64.map(Capabilities.init) ?? capabilities
      default: break
      }
    }
  }

  public func serialized() -> Bytes {
    var w = ProtoWriter()
    w.writeUInt(1, protocolVersion)
    w.writeString(2, deviceName)
    w.writeUInt(3, deviceType.rawValue)
    w.writeString(4, syncDeviceGUID)
    w.writeUInt(5, capabilities.rawValue)
    return w.bytes
  }
}

public struct OfferItem: ProtoMessage {
  public var itemID: UInt32 = 0
  public var kind = ItemKind.unspecified
  public var name = ""
  public var mimeType = ""
  public var size: UInt64 = 0
  public var sha256 = Bytes()
  public var inlineData = Bytes()

  public init() {}

  public init(
    itemID: UInt32, kind: ItemKind, name: String, mimeType: String, size: UInt64,
    sha256: Bytes, inlineData: Bytes = []
  ) {
    self.itemID = itemID
    self.kind = kind
    self.name = name
    self.mimeType = mimeType
    self.size = size
    self.sha256 = sha256
    self.inlineData = inlineData
  }

  public init(serialized data: Bytes) throws {
    try ProtoReader.forEachField(data) { field, value in
      switch field {
      case 1: itemID = value.uint32 ?? itemID
      case 2: kind = value.uint32.map(ItemKind.init) ?? kind
      case 3: name = try value.string() ?? name
      case 4: mimeType = try value.string() ?? mimeType
      case 5: size = value.uint64 ?? size
      case 6: sha256 = value.bytes ?? sha256
      case 7: inlineData = value.bytes ?? inlineData
      default: break
      }
    }
  }

  public func serialized() -> Bytes {
    var w = ProtoWriter()
    w.writeUInt(1, itemID)
    w.writeUInt(2, kind.rawValue)
    w.writeString(3, name)
    w.writeString(4, mimeType)
    w.writeUInt(5, size)
    w.writeBytes(6, sha256)
    w.writeBytes(7, inlineData)
    return w.bytes
  }
}

public struct Offer: ProtoMessage {
  public var transferID = Bytes()
  public var items: [OfferItem] = []

  public init() {}

  public init(transferID: Bytes, items: [OfferItem]) {
    self.transferID = transferID
    self.items = items
  }

  public init(serialized data: Bytes) throws {
    try ProtoReader.forEachField(data) { field, value in
      switch field {
      case 1: transferID = value.bytes ?? transferID
      case 2:
        if let b = value.bytes { items.append(try OfferItem(serialized: b)) }
      default: break
      }
    }
  }

  public func serialized() -> Bytes {
    var w = ProtoWriter()
    w.writeBytes(1, transferID)
    for item in items {
      w.writeLengthDelimited(2, item.serialized())
    }
    return w.bytes
  }
}

public struct Response: ProtoMessage {
  public var transferID = Bytes()
  public var decision = Decision.unspecified
  public var acceptedItemIDs: [UInt32] = []

  public init() {}

  public init(transferID: Bytes, decision: Decision, acceptedItemIDs: [UInt32] = []) {
    self.transferID = transferID
    self.decision = decision
    self.acceptedItemIDs = acceptedItemIDs
  }

  public init(serialized data: Bytes) throws {
    try ProtoReader.forEachField(data) { field, value in
      switch field {
      case 1: transferID = value.bytes ?? transferID
      case 2: decision = value.uint32.map(Decision.init) ?? decision
      case 3:
        switch value {
        case .varint(let v): acceptedItemIDs.append(UInt32(truncatingIfNeeded: v))
        case .lengthDelimited(let packed):
          var reader = ProtoReader(packed)
          while !reader.isAtEnd {
            acceptedItemIDs.append(UInt32(truncatingIfNeeded: try reader.readVarint()))
          }
        case .fixed: break
        }
      default: break
      }
    }
  }

  public func serialized() -> Bytes {
    var w = ProtoWriter()
    w.writeBytes(1, transferID)
    w.writeUInt(2, decision.rawValue)
    w.writePacked(3, acceptedItemIDs)
    return w.bytes
  }
}

public struct Cancel: ProtoMessage {
  public var transferID = Bytes()
  public var reason = CancelReason.unspecified

  public init() {}

  public init(transferID: Bytes, reason: CancelReason) {
    self.transferID = transferID
    self.reason = reason
  }

  public init(serialized data: Bytes) throws {
    try ProtoReader.forEachField(data) { field, value in
      switch field {
      case 1: transferID = value.bytes ?? transferID
      case 2: reason = value.uint32.map(CancelReason.init) ?? reason
      default: break
      }
    }
  }

  public func serialized() -> Bytes {
    var w = ProtoWriter()
    w.writeBytes(1, transferID)
    w.writeUInt(2, reason.rawValue)
    return w.bytes
  }
}

public struct Done: ProtoMessage {
  public var transferID = Bytes()
  public var status = DoneStatus.unspecified

  public init() {}

  public init(transferID: Bytes, status: DoneStatus) {
    self.transferID = transferID
    self.status = status
  }

  public init(serialized data: Bytes) throws {
    try ProtoReader.forEachField(data) { field, value in
      switch field {
      case 1: transferID = value.bytes ?? transferID
      case 2: status = value.uint32.map(DoneStatus.init) ?? status
      default: break
      }
    }
  }

  public func serialized() -> Bytes {
    var w = ProtoWriter()
    w.writeBytes(1, transferID)
    w.writeUInt(2, status.rawValue)
    return w.bytes
  }
}

public struct Ping: ProtoMessage {
  public var sequence: UInt64 = 0

  public init() {}

  public init(sequence: UInt64) {
    self.sequence = sequence
  }

  public init(serialized data: Bytes) throws {
    try ProtoReader.forEachField(data) { field, value in
      if field == 1 { sequence = value.uint64 ?? sequence }
    }
  }

  public func serialized() -> Bytes {
    var w = ProtoWriter()
    w.writeUInt(1, sequence)
    return w.bytes
  }
}

public struct Chunk: ProtoMessage {
  public var transferID = Bytes()
  public var itemID: UInt32 = 0
  public var offset: UInt64 = 0
  public var data = Bytes()

  public init() {}

  public init(transferID: Bytes, itemID: UInt32, offset: UInt64, data: Bytes) {
    self.transferID = transferID
    self.itemID = itemID
    self.offset = offset
    self.data = data
  }

  public init(serialized bytes: Bytes) throws {
    try ProtoReader.forEachField(bytes) { field, value in
      switch field {
      case 1: transferID = value.bytes ?? transferID
      case 2: itemID = value.uint32 ?? itemID
      case 3: offset = value.uint64 ?? offset
      case 4: data = value.bytes ?? data
      default: break
      }
    }
  }

  public func serialized() -> Bytes {
    var w = ProtoWriter()
    w.writeBytes(1, transferID)
    w.writeUInt(2, itemID)
    w.writeUInt(3, offset)
    w.writeBytes(4, data)
    return w.bytes
  }
}

/// The plaintext of every transport message. Exactly one body must be
/// present; unlike protobuf, two bodies are rejected (PROTOCOL.md §5.2).
public enum Frame: Equatable {
  case offer(Offer)
  case response(Response)
  case cancel(Cancel)
  case done(Done)
  case ping(Ping)
  case chunk(Chunk)

  public init(serialized data: Bytes) throws {
    var body: Frame?
    try ProtoReader.forEachField(data) { field, value in
      guard (1...6).contains(field), let b = value.bytes else { return }
      guard body == nil else { throw DecodeError("frame has more than one body") }
      switch field {
      case 1: body = .offer(try Offer(serialized: b))
      case 2: body = .response(try Response(serialized: b))
      case 3: body = .cancel(try Cancel(serialized: b))
      case 4: body = .done(try Done(serialized: b))
      case 5: body = .ping(try Ping(serialized: b))
      default: body = .chunk(try Chunk(serialized: b))
      }
    }
    guard let body else { throw DecodeError("frame has no body") }
    self = body
  }

  public func serialized() -> Bytes {
    var w = ProtoWriter()
    switch self {
    case .offer(let m): w.writeLengthDelimited(1, m.serialized())
    case .response(let m): w.writeLengthDelimited(2, m.serialized())
    case .cancel(let m): w.writeLengthDelimited(3, m.serialized())
    case .done(let m): w.writeLengthDelimited(4, m.serialized())
    case .ping(let m): w.writeLengthDelimited(5, m.serialized())
    case .chunk(let m): w.writeLengthDelimited(6, m.serialized())
    }
    return w.bytes
  }

  public var isChunk: Bool {
    if case .chunk = self { return true }
    return false
  }
}
