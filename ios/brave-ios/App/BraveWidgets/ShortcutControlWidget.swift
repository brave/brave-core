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
  static let kind = "ShortcutControlWidget"

  var body: some ControlWidgetConfiguration {
    if #available(iOS 26.0, *) {
      return AppIntentControlConfiguration(
        kind: Self.kind,
        provider: ShortcutControlValueProvider()
      ) { shortcut in
        ControlWidgetButton(action: OpenControlWidgetShortcutIntent(shortcut: shortcut)) {
          Label(
            shortcut?.widgetShortcut.displayString ?? Strings.Widgets.shortcutControlWidgetTitle,
            braveSystemImage: shortcut?.widgetShortcut.braveSystemImageName
              ?? "leo.brave.icon-monochrome"
          )
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

/// Picker for the configurable control. Kept separate from the scene action so Control Center
/// shows the shortcut list instead of a text field.
@available(iOS 26.0, *)
struct ShortcutControlConfigurationIntent: ControlConfigurationIntent {
  static let title: LocalizedStringResource = "Shortcut"
  static let isDiscoverable: Bool = false

  @Parameter(title: "Shortcut", optionsProvider: WidgetShortcutControlOptionsProvider())
  var shortcut: ShortcutControlOption?

  init() {}
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
struct ShortcutControlValueProvider: AppIntentControlValueProvider {
  func previewValue(configuration: ShortcutControlConfigurationIntent) -> ShortcutControlOption? {
    shortcut(for: configuration.shortcut, disabledShortcuts: [])
  }

  func currentValue(
    configuration: ShortcutControlConfigurationIntent
  ) async throws -> ShortcutControlOption? {
    let disabledShortcuts = await DisabledShortcutsWidgetData.loadDisabledShortcuts()
    return shortcut(for: configuration.shortcut, disabledShortcuts: disabledShortcuts)
  }

  /// `nil` is the unconfigured control. A disabled choice opens Search.
  private func shortcut(
    for shortcut: ShortcutControlOption?,
    disabledShortcuts: Set<WidgetShortcut>
  ) -> ShortcutControlOption? {
    guard let shortcut else { return nil }
    if disabledShortcuts.contains(shortcut.widgetShortcut) {
      return .search
    }
    return shortcut
  }
}
