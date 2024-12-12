// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
@preconcurrency import Preferences

extension Preferences {
  struct BrowserMenu {
    // maps `Action.Identifier.id` to whether or not the item is explicitly visible in the menu
    static let actionVisibility: Option<[String: Bool]> = .init(
      key: "menu.action-visibility-overrides",
      default: [:]
    )

    // maps `Action.Identifier.id` to the user defined rankings
    static let actionRanks: Option<[String: Double]> = .init(key: "menu.action-ranks", default: [:])
  }
}
