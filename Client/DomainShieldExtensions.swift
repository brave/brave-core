// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveShared
import Data

extension Domain {
    /// Whether or not a given shield should be enabled based on domain exceptions and the users global preference
    func isShieldExpected(_ shield: BraveShieldState.Shield) -> Bool {
        switch shield {
        case .AllOff:
            return self.shield_allOff?.boolValue ?? false
        case .AdblockAndTp:
            return self.shield_adblockAndTp?.boolValue ?? Preferences.Shields.blockAdsAndTracking.value
        case .HTTPSE:
            return self.shield_httpse?.boolValue ?? Preferences.Shields.httpsEverywhere.value
        case .SafeBrowsing:
            return self.shield_safeBrowsing?.boolValue ?? Preferences.Shields.blockPhishingAndMalware.value
        case .FpProtection:
            return self.shield_fpProtection?.boolValue ?? Preferences.Shields.fingerprintingProtection.value
        case .NoScript:
            return self.shield_noScript?.boolValue ?? Preferences.Shields.blockScripts.value
        }
    }
}
