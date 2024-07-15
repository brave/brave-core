// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Preferences

// These override the setting in the prefs
public enum BraveShield {
  case allOff
  case fpProtection
  case noScript

  public var globalPreference: Bool {
    switch self {
    case .allOff:
      return false
    case .fpProtection:
      return Preferences.Shields.fingerprintingProtection.value
    case .noScript:
      return Preferences.Shields.blockScripts.value
    }
  }
}
