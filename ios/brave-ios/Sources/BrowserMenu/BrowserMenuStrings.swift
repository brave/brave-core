// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
@_exported import Strings

extension Strings {
  enum BrowserMenu {
    public static let allSettingsButtonTitle = NSLocalizedString(
      "BrowserMenu.allSettingsButtonTitle",
      bundle: .module,
      value: "All Settings",
      comment: "The title of a button that presents the settings screen"
    )
    public static let myActions = NSLocalizedString(
      "BrowserMenu.myActions",
      bundle: .module,
      value: "My Actions",
      comment:
        "The title above a list of the users selected browser actions such as bookmarks, history, etc."
    )
    public static let editButtonTitle = NSLocalizedString(
      "BrowserMenu.editButtonTitle",
      bundle: .module,
      value: "Edit",
      comment:
        "The title of a button that shows a screen that lets the user customize which actions are visible on the menu"
    )
    public static let vpnButtonTitle = NSLocalizedString(
      "BrowserMenu.vpnButtonTitle",
      bundle: .module,
      value: "VPN",
      comment: "A title on a button that lets the user switch the active VPN's selected region."
    )
    public static let customizeTitle = NSLocalizedString(
      "BrowserMenu.customizeTitle",
      bundle: .module,
      value: "Customize",
      comment: "A title shown at the top of the edit/cutsomize actions screen."
    )
    public static let doneButtonTitle = NSLocalizedString(
      "BrowserMenu.doneButtonTitle",
      bundle: .module,
      value: "Done",
      comment: "A button title that when tapped dismisses the customize screen."
    )
    public static let visibleActionsTitle = NSLocalizedString(
      "BrowserMenu.visibleActionsTitle",
      bundle: .module,
      value: "All Actions",
      comment: "A title shown above the list of actions that are visible on the menu."
    )
    public static let hiddenActionsTitle = NSLocalizedString(
      "BrowserMenu.hiddenActionsTitle",
      bundle: .module,
      value: "Available Actions",
      comment:
        "A title shown above the list of actions that are hidden on the menu but available for the user to make visible."
    )
    public static let showAllButtonTitle = NSLocalizedString(
      "BrowserMenu.showAllButtonTitle",
      bundle: .module,
      value: "Show All…",
      comment: "A button title that when taps shows all actions that are hidden by default"
    )
  }

  /// Action titles that are different when shown in the new menu design
  enum ActionTitles {
    public static let playlist = NSLocalizedString(
      "ActionTitles.playlist",
      bundle: .module,
      value: "Playlist",
      comment: "A button title shown on the menu that presents the Brave Playlist feature"
    )
    public static let rewards = NSLocalizedString(
      "ActionTitles.rewards",
      bundle: .module,
      value: "Rewards",
      comment: "A button title shown on the menu that presents the Brave Rewards feature."
    )
    public static let leoAIChat = NSLocalizedString(
      "ActionTitles.leoAIChat",
      bundle: .module,
      value: "AI Chat",
      comment: "A button title shown on the menu that presents the Brave Leo AI feature."
    )
    public static let braveTalk = NSLocalizedString(
      "ActionTitles.braveTalk",
      bundle: .module,
      value: "Brave Talk",
      comment: "A button title shown on the menu that opens a link to the Brave Talk feature."
    )
    public static let braveNews = NSLocalizedString(
      "ActionTitles.braveNews",
      bundle: .module,
      value: "Brave News",
      comment: "A button title shown on the menu that presents the Brave News feature."
    )
    public static let share = NSLocalizedString(
      "ActionTitles.share",
      bundle: .module,
      value: "Share…",
      comment:
        "A button title shown on the menu that when tapped lets the user share the current URL or document they're viewing"
    )
    public static let print = NSLocalizedString(
      "ActionTitles.print",
      bundle: .module,
      value: "Print",
      comment:
        "A button title shown on the menu that when tapped lets the user print the page they're viewing"
    )
    public static let vpn = NSLocalizedString(
      "ActionTitles.vpn",
      bundle: .module,
      value: "VPN",
      comment:
        "A button title shown on the menu that when tapped presents a Brave VPN paywall"
    )
  }
}
