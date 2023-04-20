// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveStrings

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
    
    var icon: UIImage {
      switch self {
        case .bookmarks:
          return UIImage(named: "menu_bookmarks", in: .module, compatibleWith: nil)!.template
        case .downloads:
          return UIImage(named: "menu-downloads", in: .module, compatibleWith: nil)!.template
        case .history:
          return UIImage(named: "menu-history", in: .module, compatibleWith: nil)!.template
        case .news:
          return UIImage(named: "menu_brave_news", in: .module, compatibleWith: nil)!.template
        case .playlist:
          return UIImage(named: "playlist_menu", in: .module, compatibleWith: nil)!.template
        case .settings:
          return UIImage(named: "menu-settings", in: .module, compatibleWith: nil)!.template
        case .talk:
          return UIImage(named: "menu-brave-talk", in: .module, compatibleWith: nil)!.template
        case .wallet(_):
          return UIImage(named: "menu-crypto", in: .module, compatibleWith: nil)!.template
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
