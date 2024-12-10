// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import DesignSystem
import SwiftUI
import UIKit

class MenuItemFactory {
  enum MenuItemType {
    case bookmarks
    case downloads
    case history
    case leo
    case news
    case playlist(subtitle: String? = nil)
    case settings
    case talk
    case wallet(subtitle: String? = nil)
    case dataImport

    var icon: Image {
      switch self {
      case .bookmarks:
        return Image(braveSystemName: "leo.product.bookmarks")
      case .downloads:
        return Image(braveSystemName: "leo.download")
      case .history:
        return Image(braveSystemName: "leo.history")
      case .leo:
        return Image(braveSystemName: "leo.product.brave-leo")
      case .news:
        return Image(braveSystemName: "leo.product.brave-news")
      case .playlist:
        return Image(braveSystemName: "leo.product.playlist")
      case .settings:
        return Image(braveSystemName: "leo.settings")
      case .talk:
        return Image(braveSystemName: "leo.product.brave-talk")
      case .wallet(_):
        return Image(braveSystemName: "leo.product.brave-wallet")
      case .dataImport:
        return Image(systemName: "zipper.page")
      }
    }

    var title: String {
      switch self {
      case .bookmarks:
        return Strings.bookmarksMenuItem
      case .downloads:
        return Strings.downloadsMenuItem
      case .history:
        return Strings.historyMenuItem
      case .leo:
        return Strings.leoMenuItem
      case .news:
        return Strings.OptionsMenu.braveNewsItemTitle
      case .playlist:
        return Strings.OptionsMenu.bravePlaylistItemTitle
      case .settings:
        return Strings.settingsMenuItem
      case .talk:
        return Strings.OptionsMenu.braveTalkItemTitle
      case .wallet:
        return Strings.Wallet.wallet
      case .dataImport:
        return "Data Importer"
      }
    }

    var subtitle: String? {
      switch self {
      case .news:
        return Strings.OptionsMenu.braveNewsItemDescription
      case .playlist(let subtitle):
        return subtitle
      case .talk:
        return Strings.OptionsMenu.braveTalkItemDescription
      case .wallet(let subtitle):
        return subtitle
      case .leo:
        return Strings.OptionsMenu.braveLeoItemDescription
      default:
        return nil
      }
    }
  }

  static func button(
    for buttonType: MenuItemType,
    completion: @escaping () -> Void
  ) -> MenuItemButton {
    MenuItemButton(icon: buttonType.icon, title: buttonType.title, subtitle: buttonType.subtitle) {
      completion()
    }
  }
}
