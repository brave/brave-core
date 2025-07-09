// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Preferences

extension BraveShieldsUtilsIOS {

  /// Returns the current `AdBlockMode` - optionally considering if BraveShields is enabled - for the given URL.
  @MainActor public func adBlockMode(
    for url: URL,
    isPrivate: Bool,
    considerAllShieldsOption: Bool
  ) -> BraveShields.AdBlockMode {
    if considerAllShieldsOption && !isBraveShieldsEnabled(for: url, isPrivate: isPrivate) {
      // Shields is disabled for this url
      return .allow
    }
    return adBlockMode(for: url, isPrivate: isPrivate)
  }

  /// Determines if the given `BraveShield` - optionally considering if BraveShields is enabled - is enabled for the given URL.
  @MainActor public func isShieldExpected(
    url: URL,
    isPrivate: Bool,
    shield: BraveShield,
    considerAllShieldsOption: Bool
  ) -> Bool {
    if considerAllShieldsOption && !isBraveShieldsEnabled(for: url, isPrivate: isPrivate) {
      // Shields is disabled for this url
      return false
    }
    switch shield {
    case .allOff:
      return isBraveShieldsEnabled(for: url, isPrivate: isPrivate)
    case .fpProtection:
      return isBlockFingerprintingEnabled(for: url, isPrivate: isPrivate)
    case .noScript:
      return isBlockScriptsEnabled(for: url, isPrivate: isPrivate)
    }
  }
}
