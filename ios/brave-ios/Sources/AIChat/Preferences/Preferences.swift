// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Preferences

extension Preferences {
  public struct AIChat {
    /// A boolean indicating whether or not to show Leo button inside the Quick Search Engines Bar
    public static let leoInQuickSearchBarEnabled = Option<Bool>(
      key: "aichat.leo-in-quick-search-bar-enabled",
      default: true
    )
  }
}
