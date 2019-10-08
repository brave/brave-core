/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation

/// Protocol defining a few things BraveRewards wants the client to do
public protocol RewardsUIDelegate: AnyObject {
  /// Tells the client to present some controller onto its top view controller
  func presentBraveRewardsController(_ controller: UIViewController)
  /// Tells the client to load a tab to a specific URL
  func loadNewTabWithURL(_ url: URL)
}
