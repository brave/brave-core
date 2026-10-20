// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferCore
import Foundation

// Swift mirrors of the types in brave_transfer.mojom. The iOS page handler
// bridge converts between these and the generated ObjC mojom wrappers, so
// the engine (shared with the share extension) never depends on BraveCore.

public enum TransferDeviceType: Int, Codable, Sendable {
  case unknown, desktop, laptop, phone, tablet

  public init(_ type: DeviceType) {
    switch type {
    case .desktop: self = .desktop
    case .phone: self = .phone
    case .tablet: self = .tablet
    default: self = .unknown
    }
  }
}

public enum TrustSourceKind: Int, Codable, Sendable {
  case syncChain, paired
}

public enum DevicePolicy: Int, Codable, Sendable, CaseIterable {
  case `default`, alwaysAccept, alwaysAsk, block
}

public struct TransferDevice: Equatable, Sendable {
  public var id: String
  public var name: String
  public var type: TransferDeviceType
  public var source: TrustSourceKind
  public var isConnected: Bool
  /// Milliseconds since the Unix epoch, or 0 if never.
  public var lastSeenTime: Double
  public var keyFingerprint: String
  public var policy: DevicePolicy

  public init(
    id: String, name: String, type: TransferDeviceType, source: TrustSourceKind = .syncChain,
    isConnected: Bool = false, lastSeenTime: Double = 0, keyFingerprint: String = "",
    policy: DevicePolicy = .default
  ) {
    self.id = id
    self.name = name
    self.type = type
    self.source = source
    self.isConnected = isConnected
    self.lastSeenTime = lastSeenTime
    self.keyFingerprint = keyFingerprint
    self.policy = policy
  }
}

public struct TransferServiceState: Equatable, Sendable {
  public var syncEnabled: Bool
  public var discoveryActive: Bool

  public init(syncEnabled: Bool, discoveryActive: Bool) {
    self.syncEnabled = syncEnabled
    self.discoveryActive = discoveryActive
  }
}

public enum TransferItemKind: Int, Codable, Sendable {
  case text, link, image, file

  public init(_ kind: ItemKind) {
    switch kind {
    case .text: self = .text
    case .url: self = .link
    case .image: self = .image
    default: self = .file
    }
  }
}

public struct TransferItemInfo: Equatable, Sendable {
  public var id: String
  public var kind: TransferItemKind
  /// File name for images and files; empty for text and links.
  public var name: String
  public var mimeType: String
  public var size: UInt64
  /// Text or URL for text and link items; empty otherwise.
  public var text: String

  public init(
    id: String, kind: TransferItemKind, name: String, mimeType: String, size: UInt64,
    text: String
  ) {
    self.id = id
    self.kind = kind
    self.name = name
    self.mimeType = mimeType
    self.size = size
    self.text = text
  }

  init(itemID: UInt32, kind: ItemKind, name: String, mimeType: String, size: UInt64,
       inlineData: Bytes) {
    let textual = kind == .text || kind == .url
    self.init(
      id: String(itemID), kind: TransferItemKind(kind), name: textual ? "" : name,
      mimeType: mimeType, size: size,
      text: textual ? String(decoding: inlineData, as: UTF8.self) : "")
  }

  init(_ item: OfferItem) {
    self.init(
      itemID: item.itemID, kind: item.kind, name: item.name, mimeType: item.mimeType,
      size: item.size, inlineData: item.inlineData)
  }
}

public struct IncomingOfferInfo: Equatable, Sendable {
  public var id: String
  public var deviceID: String
  public var deviceName: String
  public var items: [TransferItemInfo]
  /// Milliseconds since the Unix epoch.
  public var expiryTime: Double

  public init(
    id: String, deviceID: String, deviceName: String, items: [TransferItemInfo],
    expiryTime: Double
  ) {
    self.id = id
    self.deviceID = deviceID
    self.deviceName = deviceName
    self.items = items
    self.expiryTime = expiryTime
  }
}

public enum TransferDirection: Int, Sendable {
  case send, receive
}

public enum TransferState: Int, Sendable {
  case waitingForResponse, inProgress, completed, declined, expired, cancelled, failed

  public var isFinal: Bool { self != .waitingForResponse && self != .inProgress }
}

public struct TransferInfo: Equatable, Sendable {
  public var id: String
  public var deviceID: String
  public var deviceName: String
  public var direction: TransferDirection
  public var state: TransferState
  public var items: [TransferItemInfo]
  public var bytesTransferred: UInt64
  public var bytesTotal: UInt64

  public init(
    id: String, deviceID: String, deviceName: String, direction: TransferDirection,
    state: TransferState, items: [TransferItemInfo], bytesTransferred: UInt64,
    bytesTotal: UInt64
  ) {
    self.id = id
    self.deviceID = deviceID
    self.deviceName = deviceName
    self.direction = direction
    self.state = state
    self.items = items
    self.bytesTransferred = bytesTransferred
    self.bytesTotal = bytesTotal
  }
}

public enum SendResult: Int, Sendable {
  case success, deviceUnavailable, tooLarge, cancelled, notSupported, failed
}

public enum ReceiveMode: Int, Codable, Sendable {
  case off, trustedDevices
}

public enum PermissionStatus: Int, Sendable {
  case notDetermined, granted, limited, denied
}

public struct TransferPermissions: Equatable, Sendable {
  public var localNetwork: PermissionStatus
  public var photos: PermissionStatus

  public init(localNetwork: PermissionStatus, photos: PermissionStatus) {
    self.localNetwork = localNetwork
    self.photos = photos
  }
}
