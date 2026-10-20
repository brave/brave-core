// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

/// User settings. On iOS they live in the app-group `UserDefaults`, so the
/// share extension reads the same values as the app.
public struct TransferSettings: Equatable, Sendable {
  public var receiveMode: ReceiveMode = .trustedDevices
  /// Off by default (design plan D5).
  public var autoAcceptTrusted = false
  /// Only effective while Sync is on.
  public var autoTrustSyncChain = true
  public var notifyOnReceive = true

  public init() {}
}

/// `UserDefaults` keys shared by the engine, the app's `Preferences` options
/// and the share extension.
public enum TransferDefaultsKey {
  public static let receiveMode = "braveTransfer.receiveMode"
  public static let autoAcceptTrusted = "braveTransfer.autoAcceptTrusted"
  public static let autoTrustSyncChain = "braveTransfer.autoTrustSyncChain"
  public static let notifyOnReceive = "braveTransfer.notifyOnReceive"
  /// Set once the user has opened brave://transfer, after which the app
  /// listens while in the foreground (design plan §5.2).
  public static let setupDone = "braveTransfer.setupDone"
}

public protocol TransferSettingsStore: AnyObject {
  var settings: TransferSettings { get set }
}

public final class UserDefaultsSettingsStore: TransferSettingsStore {
  private let defaults: UserDefaults

  public init(defaults: UserDefaults) {
    self.defaults = defaults
  }

  public var settings: TransferSettings {
    get {
      var settings = TransferSettings()
      if let raw = defaults.object(forKey: TransferDefaultsKey.receiveMode) as? Int,
        let mode = ReceiveMode(rawValue: raw)
      {
        settings.receiveMode = mode
      }
      if let value = defaults.object(forKey: TransferDefaultsKey.autoAcceptTrusted) as? Bool {
        settings.autoAcceptTrusted = value
      }
      if let value = defaults.object(forKey: TransferDefaultsKey.autoTrustSyncChain) as? Bool {
        settings.autoTrustSyncChain = value
      }
      if let value = defaults.object(forKey: TransferDefaultsKey.notifyOnReceive) as? Bool {
        settings.notifyOnReceive = value
      }
      return settings
    }
    set {
      defaults.set(newValue.receiveMode.rawValue, forKey: TransferDefaultsKey.receiveMode)
      defaults.set(newValue.autoAcceptTrusted, forKey: TransferDefaultsKey.autoAcceptTrusted)
      defaults.set(newValue.autoTrustSyncChain, forKey: TransferDefaultsKey.autoTrustSyncChain)
      defaults.set(newValue.notifyOnReceive, forKey: TransferDefaultsKey.notifyOnReceive)
    }
  }
}

public final class InMemorySettingsStore: TransferSettingsStore {
  public var settings: TransferSettings

  public init(_ settings: TransferSettings = TransferSettings()) {
    self.settings = settings
  }
}
