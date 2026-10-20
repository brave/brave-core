// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferCore
import Foundation
import XCTest

typealias JSONObject = [String: Any]

/// Loads the shared test vectors from components/brave_transfer/core/test/data,
/// which tools/brave_transfer/gen_vectors.py writes.
enum Vectors {
  static let directory = URL(fileURLWithPath: #filePath)
    .deletingLastPathComponent()  // BraveTransferCoreTests
    .deletingLastPathComponent()  // Tests
    .deletingLastPathComponent()  // swift
    .deletingLastPathComponent()  // brave_transfer
    .appendingPathComponent("core/test/data")

  static func load(_ name: String) throws -> JSONObject {
    let data = try Data(contentsOf: directory.appendingPathComponent(name))
    return try XCTUnwrap(JSONSerialization.jsonObject(with: data) as? JSONObject)
  }
}

func hex(_ value: Any?) -> Bytes {
  Bytes(hexString: value as? String ?? "")!
}

func uint(_ value: Any?) -> UInt64 {
  switch value {
  case let n as UInt64: return n
  case let n as Int: return UInt64(n)
  case let n as Int64: return UInt64(n)
  case let n as Double: return UInt64(n)
  case let n as NSNumber: return n.uint64Value
  default: return 0
  }
}

func uint32(_ value: Any?) -> UInt32 {
  UInt32(uint(value))
}

func objects(_ value: Any?) -> [JSONObject] {
  value as? [JSONObject] ?? []
}

extension OfferItem {
  init(json j: JSONObject) {
    self.init(
      itemID: uint32(j["item_id"]), kind: ItemKind(rawValue: uint32(j["kind"])),
      name: j["name"] as? String ?? "", mimeType: j["mime_type"] as? String ?? "",
      size: uint(j["size"]), sha256: hex(j["sha256"]), inlineData: hex(j["inline_data"]))
  }
}

extension Offer {
  init(json j: JSONObject) {
    self.init(transferID: hex(j["transfer_id"]), items: objects(j["items"]).map(OfferItem.init))
  }
}

extension PeerInfo {
  init(json j: JSONObject) {
    self.init(
      protocolVersion: uint32(j["protocol_version"]), deviceName: j["device_name"] as! String,
      deviceType: DeviceType(rawValue: uint32(j["device_type"])),
      syncDeviceGUID: j["sync_device_guid"] as! String,
      capabilities: Capabilities(rawValue: uint(j["capabilities"])))
  }
}

extension Response {
  init(json j: JSONObject) {
    self.init(
      transferID: hex(j["transfer_id"]), decision: Decision(rawValue: uint32(j["decision"])),
      acceptedItemIDs: (j["accepted_item_ids"] as? [Any] ?? []).map(uint32))
  }
}

extension Cancel {
  init(json j: JSONObject) {
    self.init(transferID: hex(j["transfer_id"]), reason: CancelReason(rawValue: uint32(j["reason"])))
  }
}

extension Done {
  init(json j: JSONObject) {
    self.init(transferID: hex(j["transfer_id"]), status: DoneStatus(rawValue: uint32(j["status"])))
  }
}

extension Chunk {
  init(json j: JSONObject) {
    self.init(
      transferID: hex(j["transfer_id"]), itemID: uint32(j["item_id"]), offset: uint(j["offset"]),
      data: hex(j["data"]))
  }
}

extension Frame {
  init(json j: JSONObject) {
    let (name, value) = j.first!
    let body = value as! JSONObject
    switch name {
    case "offer": self = .offer(Offer(json: body))
    case "response": self = .response(Response(json: body))
    case "cancel": self = .cancel(Cancel(json: body))
    case "done": self = .done(Done(json: body))
    case "ping": self = .ping(Ping(sequence: uint(body["sequence"])))
    case "chunk": self = .chunk(Chunk(json: body))
    default: fatalError("unknown frame body \(name)")
    }
  }
}

/// Builds the named message from its test-vector JSON and serializes it.
func encode(type: String, json j: JSONObject) -> Bytes {
  switch type {
  case "ClientHello":
    return ClientHello(protocolVersion: uint32(j["protocol_version"]), trustTag: hex(j["trust_tag"]))
      .serialized()
  case "PeerInfo": return PeerInfo(json: j).serialized()
  case "OfferItem": return OfferItem(json: j).serialized()
  case "Offer": return Offer(json: j).serialized()
  case "Response": return Response(json: j).serialized()
  case "Cancel": return Cancel(json: j).serialized()
  case "Done": return Done(json: j).serialized()
  case "Ping": return Ping(sequence: uint(j["sequence"])).serialized()
  case "Chunk": return Chunk(json: j).serialized()
  case "Frame": return Frame(json: j).serialized()
  default: fatalError("unknown type \(type)")
  }
}

/// Decodes `data` as the named message and re-serializes it canonically.
func decodeAndReencode(type: String, _ data: Bytes) throws -> Bytes {
  switch type {
  case "ClientHello": return try ClientHello(serialized: data).serialized()
  case "PeerInfo": return try PeerInfo(serialized: data).serialized()
  case "OfferItem": return try OfferItem(serialized: data).serialized()
  case "Offer": return try Offer(serialized: data).serialized()
  case "Response": return try Response(serialized: data).serialized()
  case "Cancel": return try Cancel(serialized: data).serialized()
  case "Done": return try Done(serialized: data).serialized()
  case "Ping": return try Ping(serialized: data).serialized()
  case "Chunk": return try Chunk(serialized: data).serialized()
  case "Frame": return try Frame(serialized: data).serialized()
  default: fatalError("unknown type \(type)")
  }
}

/// Item contents used by the vectors: byte i is i % 251.
func pattern(_ count: Int) -> Bytes {
  (0..<count).map { UInt8($0 % 251) }
}
