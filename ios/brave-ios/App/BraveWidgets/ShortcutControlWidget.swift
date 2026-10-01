// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import AppIntents
import BraveWidgetsModels
import DesignSystem
import Strings
import SwiftUI
import WidgetKit

struct ShortcutControlWidget: ControlWidget {
  var body: some ControlWidgetConfiguration {
    if #available(iOS 26.0, *) {
      return AppIntentControlConfiguration(
        kind: "ShortcutControlWidget",
        provider: ShortcutControlValueProvider()
      ) { state in
        ControlWidgetButton(action: OpenControlWidgetShortcutIntent(shortcut: state.actionShortcut))
        {
          Label(state.title, braveSystemImage: state.imageName)
        }
      }
      .displayName(
        LocalizedStringResource(stringLiteral: Strings.Widgets.shortcutControlWidgetTitle)
      )
      .description(
        LocalizedStringResource(stringLiteral: Strings.Widgets.shortcutControlWidgetDescription)
      )
      .promptsForUserConfiguration()
    } else {
      return EmptyControlWidgetConfiguration()
    }
  }
}

/// Picker value for the control. Generated `WidgetShortcut` cannot conform to `AppEnum`.
@available(iOS 26.0, *)
enum ShortcutControlOption: String, AppEnum {
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

  static let typeDisplayRepresentation: TypeDisplayRepresentation = "Shortcut"

  // AppEnum requires a dictionary literal. The control button still uses `displayString`.
  static let caseDisplayRepresentations: [ShortcutControlOption: DisplayRepresentation] = [
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

  var widgetShortcut: WidgetShortcut {
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

@available(iOS 26.0, *)
struct ShortcutControlConfigurationIntent: ControlConfigurationIntent {
  static let title: LocalizedStringResource = "Shortcut"
  static let isDiscoverable: Bool = false

  @Parameter(title: "Shortcut", optionsProvider: WidgetShortcutControlOptionsProvider())
  var shortcut: ShortcutControlOption?
}

@available(iOS 26.0, *)
struct WidgetShortcutControlOptionsProvider: DynamicOptionsProvider {
  func results() async throws -> [ShortcutControlOption] {
    let disabledShortcuts = await DisabledShortcutsWidgetData.loadDisabledShortcuts()
    return ShortcutControlOption.allCases.filter {
      !disabledShortcuts.contains($0.widgetShortcut)
    }
  }
}

@available(iOS 26.0, *)
struct ShortcutControlState {
  /// `nil` is the unconfigured control. It shows the lion and still opens Search.
  var shortcut: WidgetShortcut?

  var title: String {
    shortcut?.displayString ?? Strings.Widgets.shortcutControlWidgetTitle
  }

  var imageName: String {
    shortcut?.braveSystemImageName ?? "leo.brave.icon-monochrome"
  }

  var actionShortcut: WidgetShortcut {
    shortcut ?? .search
  }
}

@available(iOS 26.0, *)
struct ShortcutControlValueProvider: AppIntentControlValueProvider {
  func previewValue(configuration: ShortcutControlConfigurationIntent) -> ShortcutControlState {
    state(for: configuration.shortcut?.widgetShortcut, disabledShortcuts: [])
  }

  func currentValue(
    configuration: ShortcutControlConfigurationIntent
  ) async throws -> ShortcutControlState {
    let disabledShortcuts = await DisabledShortcutsWidgetData.loadDisabledShortcuts()
    return state(for: configuration.shortcut?.widgetShortcut, disabledShortcuts: disabledShortcuts)
  }

  private func state(
    for shortcut: WidgetShortcut?,
    disabledShortcuts: Set<WidgetShortcut>
  ) -> ShortcutControlState {
    guard let shortcut else {
      return ShortcutControlState(shortcut: nil)
    }
    if disabledShortcuts.contains(shortcut) {
      return ShortcutControlState(shortcut: .search)
    }
    return ShortcutControlState(shortcut: shortcut)
  }
}
