// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import BraveWidgetsModels
import DesignSystem
import Foundation
import OrderedCollections
import Strings
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
    }
    return options
  }

  var displayString: String {
    switch self {
    case .unknown:
      return ""
    case .bookmarks:
      return Strings.bookmarksMenuItem
    case .history:
      return Strings.historyMenuItem
    case .downloads:
      return Strings.downloadsMenuItem
    case .playlist:
      return Strings.bravePlaylistItemTitle
    case .wallet:
      return Strings.Wallet.wallet
    case .braveNews:
      return Strings.braveNewsItemTitle
    case .braveLeo:
      return Strings.leoMenuItem
    case .askBrave:
      return Strings.askBraveMenuItem
    default:
      return ""
    }
  }

  var braveSystemImageName: String? {
    switch self {
    case .unknown:
      return nil
    case .newTab:
      return "leo.browser.mobile-tab-new"
    case .newPrivateTab:
      return "leo.product.private-window"
    case .bookmarks:
      return "leo.product.bookmarks"
    case .history:
      return "leo.history"
    case .downloads:
      return "leo.download"
    case .playlist:
      return "leo.product.playlist"
    case .search:
      return "leo.search"
    case .wallet:
      return "leo.product.brave-wallet"
    case .scanQRCode:
      return "leo.qr.code"
    case .braveNews:
      return "leo.product.brave-news"
    case .braveLeo, .askBrave:
      return "leo.product.brave-leo"
    @unknown default:
      return nil
    }
  }

  var image: UIImage? {
    return braveSystemImageName.flatMap { UIImage(braveSystemNamed: $0) }
  }
}
