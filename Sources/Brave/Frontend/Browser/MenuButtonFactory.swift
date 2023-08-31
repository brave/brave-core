// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveStrings
import DesignSystem
import SwiftUI

class MenuItemFactory {
  enum MenuItemType {
    case bookmarks
    case downloads
    case history
    case news
    case playlist(subtitle: String? = nil)
    case settings
    case talk
    case wallet(subtitle: String? = nil)
    
    var icon: Image {
      switch self {
        case .bookmarks:
          return Image(braveSystemName: "leo.product.bookmarks")
        case .downloads:
          return Image(braveSystemName: "leo.download")
        case .history:
          return Image(braveSystemName: "leo.history")
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
      }
    }
    
    var subtitle: String? {
      switch self {
      case .news:
        return Strings.OptionsMenu.braveNewsItemDescription
      case let .playlist(subtitle):
        return subtitle
      case .talk:
        return Strings.OptionsMenu.braveTalkItemDescription
      case let .wallet(subtitle):
        return subtitle
      default:
        return nil
      }
    }
  }
  
  static func button(for buttonType: MenuItemType, completion: @escaping () -> Void) -> MenuItemButton {
    MenuItemButton(icon: buttonType.icon, title: buttonType.title, subtitle: buttonType.subtitle) {
      completion()
    }
  }
}
