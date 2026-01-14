// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
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
class OriginSettingsViewModel {
  let service: any BraveOriginService
  init(service: any BraveOriginService) {
    self.service = service
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
}
