// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Preferences

extension BraveShieldsUtilsIOS {
  @MainActor public func isShieldExpected(
    url: URL,
    shield: BraveShield,
    considerAllShieldsOption: Bool
  ) -> Bool {
    if considerAllShieldsOption && !isBraveShieldsEnabled(for: url) {
      // Shields is disabled for this url
      return false
    }
    switch shield {
    case .allOff:
      return isBraveShieldsEnabled(for: url)
    case .fpProtection:
      return isBlockFingerprintingEnabled(for: url)
    case .noScript:
      return isBlockScriptsEnabled(for: url)
    }
  }
}
