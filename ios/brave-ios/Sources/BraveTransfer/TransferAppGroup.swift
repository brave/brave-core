// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferEngine
import Foundation

/// The state the app and the "Brave Transfer" share extension share through
/// the app group: settings, the Sync chain snapshot, pinned keys and the
/// outbox. Keys live in the app group's keychain access group
/// (`TransferKeychain`).
public struct TransferAppGroup {
  public enum Error: Swift.Error {
    case missingGroupIdentifier
    case containerUnavailable
  }

  /// The app group, also used as the keychain access group.
  public let identifier: String
  public let containerURL: URL

  /// Reads the group from `KSApplicationGroup` in the bundle's Info.plist,
  /// which the app and every extension set to `$(BRAVE_GROUP_ID)`.
  public static func current(bundle: Bundle = .main) throws -> TransferAppGroup {
    guard let identifier = bundle.object(forInfoDictionaryKey: "KSApplicationGroup") as? String,
      !identifier.isEmpty
    else {
      throw Error.missingGroupIdentifier
    }
    guard
      let containerURL = FileManager.default.containerURL(
        forSecurityApplicationGroupIdentifier: identifier
      )
    else {
      throw Error.containerUnavailable
    }
    return TransferAppGroup(identifier: identifier, containerURL: containerURL)
  }

  public init(identifier: String, containerURL: URL) {
    self.identifier = identifier
    self.containerURL = containerURL
  }

  /// `<container>/BraveTransfer`, created on first use.
  public var directory: URL {
    let url = containerURL.appendingPathComponent("BraveTransfer", isDirectory: true)
    try? FileManager.default.createDirectory(at: url, withIntermediateDirectories: true)
    return url
  }

  /// The Sync chain as last exported by the app (`ChainSnapshot`).
  public var chainSnapshotURL: URL { directory.appendingPathComponent("chain.json") }

  /// Pinned static keys and device policies (`FileTrustStore`).
  public var trustStoreURL: URL { directory.appendingPathComponent("trust.json") }

  public var outbox: Outbox { Outbox(appGroupContainer: containerURL) }

  public var defaults: UserDefaults? { UserDefaults(suiteName: identifier) }

  /// Posted (Darwin notification) by either process after it changes
  /// settings or the chain snapshot, so the other reloads.
  public static let didChangeNotification = "com.brave.transfer.didChange"

  public static func postDidChange() {
    CFNotificationCenterPostNotification(
      CFNotificationCenterGetDarwinNotifyCenter(),
      CFNotificationName(didChangeNotification as CFString),
      nil,
      nil,
      true
    )
  }
}
