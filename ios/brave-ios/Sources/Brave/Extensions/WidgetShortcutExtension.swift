// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveWidgetsModels
import DesignSystem
import Foundation
import OrderedCollections
import UIKit

extension WidgetShortcut {
  static func eligibleButtonShortcuts(
    prefs: any PrefService,
    isWalletAvailable: Bool
  ) -> OrderedSet<WidgetShortcut> {
    var options = OrderedSet<WidgetShortcut>([
      .bookmarks,
      .history,
      .downloads,
      .playlist,
      .wallet,
      .braveNews,
      .braveLeo,
      .askBrave,
      .braveLeoVoiceInput,
    ])
    if !prefs.isPlaylistAvailable {
      options.remove(.playlist)
    }
    if !prefs.isBraveNewsAvailable {
      options.remove(.braveNews)
    }
    if !isWalletAvailable {
      options.remove(.wallet)
    }
    if !AIChatUtils.isAIChatEnabled(for: prefs) {
      options.remove(.braveLeo)
      options.remove(.braveLeoVoiceInput)
    }
    return options
  }

  /// The set of shortcuts that are currently unavailable and should be hidden from the Shortcuts
  /// widgets (for example, disabled by a Brave Origin or enterprise policy).
  static func disabledWidgetShortcuts(
    prefs: any PrefService,
    isWalletAvailable: Bool
  ) -> Set<WidgetShortcut> {
    var disabled: Set<WidgetShortcut> = []
    if !prefs.isPlaylistAvailable {
      disabled.insert(.playlist)
    }
    if !prefs.isBraveNewsAvailable {
      disabled.insert(.braveNews)
    }
    if !isWalletAvailable {
      disabled.insert(.wallet)
    }
    if !AIChatUtils.isAIChatEnabled(for: prefs) {
      disabled.insert(.braveLeo)
      disabled.insert(.braveLeoVoiceInput)
    }
    return disabled
  }

  var image: UIImage? {
    return braveSystemImageName.flatMap { UIImage(braveSystemNamed: $0) }
  }
}
