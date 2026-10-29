// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import AppIntents

/// Picker value for the configurable shortcut control.
/// Generated `WidgetShortcut` cannot conform to `AppEnum`.
@available(iOS 26.0, *)
public enum ShortcutControlOption: String, AppEnum {
  case newTab
  case newPrivateTab
  case bookmarks
  case history
  case downloads
  case playlist
  case search
  case wallet
  case scanQRCode
  case braveNews
  case braveLeo
  case askBrave
  case braveLeoVoiceInput

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
    switch self {
    case .newTab: .newTab
    case .newPrivateTab: .newPrivateTab
    case .bookmarks: .bookmarks
    case .history: .history
    case .downloads: .downloads
    case .playlist: .playlist
    case .search: .search
    case .wallet: .wallet
    case .scanQRCode: .scanQRCode
    case .braveNews: .braveNews
    case .braveLeo: .braveLeo
    case .askBrave: .askBrave
    case .braveLeoVoiceInput: .braveLeoVoiceInput
    }
  }
}
