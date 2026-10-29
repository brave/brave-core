// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import AppIntents

/// Picker value for the configurable shortcut control.
/// Generated `WidgetShortcut` cannot conform to `AppEnum`.
@available(iOS 26.0, *)
public enum ShortcutControlOption: Int, AppEnum {
  case newTab = 1
  case newPrivateTab = 2
  case bookmarks = 3
  case history = 4
  case downloads = 5
  case playlist = 6
  case search = 7
  case wallet = 8
  case scanQRCode = 9
  case braveNews = 10
  case braveLeo = 11
  case askBrave = 12
  case braveLeoVoiceInput = 13

  public static let typeDisplayRepresentation: TypeDisplayRepresentation = "Shortcut"

  // AppEnum requires a dictionary literal. The control button still uses `displayString`.
  public static let caseDisplayRepresentations: [ShortcutControlOption: DisplayRepresentation] = [
    .newTab: "New Tab",
    .newPrivateTab: "Private Tab",
    .bookmarks: "Bookmarks",
    .history: "History",
    .downloads: "Downloads",
    .playlist: "Playlist",
    .search: "Search",
    .wallet: "Brave Wallet",
    .scanQRCode: "QR Code",
    .braveNews: "Brave News",
    .braveLeo: "Leo AI",
    .askBrave: "Ask Brave",
    .braveLeoVoiceInput: "Leo Voice Input",
  ]

  public var widgetShortcut: WidgetShortcut {
    WidgetShortcut(rawValue: rawValue) ?? .unknown
  }
}
