// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStore
import SwiftUI

@propertyWrapper
struct OriginPolicyBooleanValue {
  var key: BraveOriginPolicyKey

  var wrappedValue: Bool {
    get { fatalError("Can only be used within OriginSettingsViewModel") }
    set { fatalError("Can only be used within OriginSettingsViewModel") }
  }

  static subscript(
    _enclosingInstance instance: OriginSettingsViewModel,
    wrapped wrappedKeyPath: ReferenceWritableKeyPath<OriginSettingsViewModel, Bool>,
    storage storageKeyPath: ReferenceWritableKeyPath<OriginSettingsViewModel, Self>
  ) -> Bool {
    get {
      instance.access(keyPath: wrappedKeyPath)
      let key = instance[keyPath: storageKeyPath].key
      return instance.service.getPolicyValue(key) as? Bool ?? false
    }
    set {
      let key = instance[keyPath: storageKeyPath].key
      instance.withMutation(keyPath: wrappedKeyPath) {
        _ = instance.service.setPolicyValue(key, value: newValue)
      }
    }
  }
}

@Observable
public class OriginSettingsViewModel {
  fileprivate let service: any BraveOriginService
  private let storeSDK: BraveStoreSDK

  public init(service: any BraveOriginService, storeSDK: BraveStoreSDK) {
    self.service = service
    self.storeSDK = storeSDK

    Task {
      await updatePurchaseStatus()
    }
  }

  @ObservationIgnored
  @OriginPolicyBooleanValue(key: .rewardsDisabled)
  var isRewardsDisabled: Bool

  @ObservationIgnored
  @OriginPolicyBooleanValue(key: .aiChatEnabled)
  var isAIChatEnabled: Bool

  @ObservationIgnored
  @OriginPolicyBooleanValue(key: .newsDisabled)
  var isNewsDisabled: Bool

  @ObservationIgnored
  @OriginPolicyBooleanValue(key: .p3AEnabled)
  var isP3AEnabled: Bool

  @ObservationIgnored
  @OriginPolicyBooleanValue(key: .statsPingEnabled)
  var isStatsPingEnabled: Bool

  @ObservationIgnored
  @OriginPolicyBooleanValue(key: .talkDisabled)
  var isTalkDisabled: Bool

  @ObservationIgnored
  @OriginPolicyBooleanValue(key: .walletDisabled)
  var isWalletDisabled: Bool

  @ObservationIgnored
  @OriginPolicyBooleanValue(key: .vpnDisabled)
  var isVPNDisabled: Bool

  @ObservationIgnored
  @OriginPolicyBooleanValue(key: .playlistEnabled)
  var isPlaylistEnabled: Bool

  private(set) var isPuchaseLinkable: Bool = false

  @MainActor
  private func updatePurchaseStatus() async {
    guard let transaction = await storeSDK.originPurchaseProduct?.latestTransaction else {
      isPuchaseLinkable = false
      return
    }
    isPuchaseLinkable =
      switch transaction {
      case .verified:
        true
      case .unverified:
        false
      }
  }

  /// Reset all of the policy values back to their defaults
  func reset() {
    self.isRewardsDisabled = true
    self.isAIChatEnabled = false
    self.isNewsDisabled = true
    self.isP3AEnabled = false
    self.isStatsPingEnabled = false
    self.isTalkDisabled = true
    self.isWalletDisabled = true
    self.isVPNDisabled = true
  }
}
