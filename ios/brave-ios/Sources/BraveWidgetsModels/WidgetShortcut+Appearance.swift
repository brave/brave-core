// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

extension WidgetShortcut {
  /// Symbol name shared by widgets and the in-app shortcut button.
  /// Titles stay in each host. Widget copy is localized in the widget bundle, and the toolbar
  /// menu uses shorter in-app strings.
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
}
