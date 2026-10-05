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
      return Self.widgetLocalized(
        "widgets.shortcutsPrivateTabButton",
        value: "Private Tab",
        comment: "Button to open new private browser tab."
      )
    case .bookmarks:
      return Strings.bookmarksMenuItem
    case .history:
      return Strings.historyMenuItem
    case .downloads:
      return Strings.downloadsMenuItem
    case .playlist:
      return Strings.bravePlaylistItemTitle
    case .search:
      return Self.widgetLocalized(
        "widgets.searchShortcutTitle",
        value: "Search",
        comment: "Description for the search option on the 'shortcuts' widget."
      )
    case .wallet:
      return Self.widgetLocalized(
        "widgets.walletShortcutTitle",
        value: "Brave Wallet",
        comment: "Description for the Brave Wallet option on the 'shortcuts' widget."
      )
    case .scanQRCode:
      return Self.widgetLocalized(
        "widgets.QRCode",
        value: "QR Code",
        comment: "QR Code section title"
      )
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

  /// `NSLocalizedString` reads `Bundle.main`. In the widget extension that bundle already has these keys.
  private static func widgetLocalized(
    _ key: String,
    value: String,
    comment: String
  ) -> String {
    NSLocalizedString(key, value: value, comment: comment)
  }
}
