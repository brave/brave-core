// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Preferences

enum DefaultBrowserHelper {
  /// Whether or not its likely that Brave is the users default browser based on a 14 day usage period
  static func isBraveLikelyDefaultBrowser() -> Bool {
    guard let date = Preferences.General.lastHTTPURLOpenedDate.value else { return false }
    return date >= Date.now.addingTimeInterval(-14.days)
  }
}
