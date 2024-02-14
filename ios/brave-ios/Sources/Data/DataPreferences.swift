// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Preferences

extension Preferences {
  public final class BraveVPNAlertTotals {
    public static let consolidatedTrackerCount = Option<Int>(key: "vpn-alert-consolidated-tracker", default: 0)
    public static let consolidatedLocationPingCount = Option<Int>(key: "vpn-alert-consolidated-location", default: 0)
    public static let consolidatedEmailTrackerCount = Option<Int>(key: "vpn-alert-consolidated-email", default: 0)
  }
}
