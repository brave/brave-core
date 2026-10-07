// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveStrings
import Foundation

extension WidgetShortcut {
  /// Symbol name shared by widgets and the in-app shortcut button.
  public var braveSystemImageName: String? {
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
    case .braveLeo:
      return "leo.product.brave-leo"
    case .askBrave:
      return "leo.brave.ask"
    case .braveLeoVoiceInput:
      return "leo.leo.voice-input"
    @unknown default:
      return nil
    }
  }

  /// Title shared by widgets and the in-app shortcut button.
  public var displayString: String {
    switch self {
    case .unknown:
      return ""
    case .newTab:
      return Strings.quickActionNewTab
    case .newPrivateTab:
      return Strings.shortcutPrivateTab
    case .bookmarks:
      return Strings.bookmarksMenuItem
    case .history:
      return Strings.historyMenuItem
    case .downloads:
      return Strings.downloadsMenuItem
    case .playlist:
      return Strings.bravePlaylistItemTitle
    case .search:
      return Strings.shortcutSearch
    case .wallet:
      return Strings.shortcutBraveWallet
    case .scanQRCode:
      return Strings.shortcutQRCode
    case .braveNews:
      return Strings.braveNewsItemTitle
    case .braveLeo:
      return Strings.leoMenuItem
    case .askBrave:
      return Strings.askBraveMenuItem
    case .braveLeoVoiceInput:
      return Strings.leoVoiceInputMenuItem
    @unknown default:
      return ""
    }
  }
}
