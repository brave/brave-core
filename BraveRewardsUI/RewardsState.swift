/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import BraveRewards

/// Holds the state of this BraveRewards panel
struct RewardsState {
  /// The controlling ledger
  var ledger: BraveLedger
  /// The controlling ads
  var ads: BraveAds
  /// The tab id for this panel
  var tabId: UInt64
  /// The url currently viewing
  var url: URL
  /// The favicon URL for `url`
  var faviconURL: URL?
  /// The Rewards delegate
  weak var delegate: RewardsUIDelegate?
  /// The Rewards data source
  weak var dataSource: RewardsDataSource?
}
