// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Data

/// Rperesents a blocked ad or tracker.
struct PrivacyReportsTracker: Identifiable {
  let name: String
  let count: Int
  let source: Source?
  
  /// Blocked tracker may come in two types:
  /// Items blocked by Brave Shields, handled by `BlockedResource` model.
  /// Items blocked by the Brave VPN feature, handled by `BraveVPNAlert` model.
  /// This data is used to show correct labels on the all-time blocked items list.
  enum Source {
    case shields
    case vpn
    // The tracker was found by both Brave Shields and the Brave VPN alerts.
    case both
  }
  
  var id: String {
    name
  }
  
  /// Blocked trackers are detected by Brave shields and Brave VPN.
  /// This method merges those two sources of data and sets proper `Source` for them.
  static func merge(
    shieldItems: Set<CountableEntity>,
    vpnItems: Set<CountableEntity>
  ) -> [PrivacyReportsTracker] {

    let shieldsAndVPNBlockedItems = shieldItems.intersection(vpnItems).compactMap { item -> PrivacyReportsTracker? in
      guard let secondValue = vpnItems.first(where: { item.name == $0.name })?.count else { return nil }

      return .init(name: item.name, count: item.count + secondValue, source: .both)
    }

    let shieldsOnlyBlockedItems = shieldItems.subtracting(vpnItems)
      .map { PrivacyReportsTracker(name: $0.name, count: $0.count, source: .shields) }
    let vpnOnlyBlockedItems = vpnItems.subtracting(shieldItems)
      .map { PrivacyReportsTracker(name: $0.name, count: $0.count, source: .vpn) }

    let allBlockedItems = shieldsAndVPNBlockedItems + shieldsOnlyBlockedItems + vpnOnlyBlockedItems

    return allBlockedItems.sorted(by: { $0.count > $1.count })

  }
}
