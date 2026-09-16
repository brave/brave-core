// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import AppIntents
import UIKit

@available(iOS 26.0, *)
public struct OpenControlWidgetShortcutIntent: UISceneAppIntent {
  public static var title: LocalizedStringResource = "Open Brave"
  public static var openAppWhenRun: Bool = true

  @Parameter(title: "Shortcut")
  public var shortcut: WidgetShortcut

  public init() {
    self.shortcut = .unknown
  }

  public init(shortcut: WidgetShortcut) {
    self.shortcut = shortcut
  }
}

extension WidgetShortcut: AppEnum, CaseIterable {
  public static var typeDisplayRepresentation: TypeDisplayRepresentation = "Shortcut"

  public static var caseDisplayRepresentations: [WidgetShortcut: DisplayRepresentation] = [
    .unknown: "Unknown",
    .newTab: "New Tab",
    .newPrivateTab: "New Private Tab",
    .bookmarks: "Bookmarks",
    .history: "History",
    .downloads: "Downloads",
    .playlist: "Brave Playlist",
    .search: "Search",
    .wallet: "Brave Wallet",
    .scanQRCode: "Scan QR Code",
    .braveNews: "Brave News",
    .braveLeo: "Leo AI",
    .askBrave: "Ask Brave",
    .braveLeoVoiceInput: "Leo AI Voice Input",
  ]

  public static var allCases: [WidgetShortcut] {
    [
      .unknown,
      .newTab,
      .newPrivateTab,
      .bookmarks,
      .history,
      .downloads,
      .playlist,
      .search,
      .wallet,
      .scanQRCode,
      .braveNews,
      .braveLeo,
      .askBrave,
      .braveLeoVoiceInput,
    ]
  }
}
