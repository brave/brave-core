// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import AppIntents
import BraveShared
import BraveWidgetsModels
import Foundation
import Strings
import SwiftUI
import WidgetKit

struct LockScreenShortcutWidget: Widget {
  var body: some WidgetConfiguration {
    if #available(iOSApplicationExtension 16.0, *) {
      return AppIntentConfiguration(
        kind: "LockScreenShortcutWidget",
        intent: LockScreenShortcutConfigurationIntent.self,
        provider: LockScreenShortcutProvider()
      ) { entry in
        LockScreenShortcutView(entry: entry)
      }
      .configurationDisplayName(Strings.Widgets.shortcutsWidgetTitle)
      .description(Strings.Widgets.shortcutsWidgetDescription)
      .supportedFamilies([.accessoryCircular])
    } else {
      return EmptyWidgetConfiguration()
    }
  }
}

struct LockScreenShortcutEntry: TimelineEntry {
  var date: Date
  var widgetShortcut: WidgetShortcut?
}

struct LockScreenShortcutProvider: AppIntentTimelineProvider {
  typealias Intent = LockScreenShortcutConfigurationIntent
  typealias Entry = LockScreenShortcutEntry

  private func shortcut(for configuration: Intent) async -> WidgetShortcut? {
    let disabledShortcuts = await DisabledShortcutsWidgetData.loadDisabledShortcuts()
    if disabledShortcuts.contains(configuration.shortcut) {
      return nil
    }
    return configuration.shortcut
  }

  func placeholder(in context: Context) -> Entry {
    .init(date: Date(), widgetShortcut: .bookmarks)
  }
  func snapshot(for configuration: Intent, in context: Context) async -> Entry {
    let shortcut = await shortcut(for: configuration)
    return LockScreenShortcutEntry(date: Date(), widgetShortcut: shortcut)
  }
  func timeline(for configuration: Intent, in context: Context) async -> Timeline<Entry> {
    let entry = LockScreenShortcutEntry(date: Date(), widgetShortcut: await shortcut(for: configuration))
    return .init(entries: [entry], policy: .never)
  }
}

struct LockScreenShortcutView: View {
  var entry: LockScreenShortcutEntry

  var body: some View {
    ZStack {
      AccessoryWidgetBackground()
        .widgetBackground { EmptyView() }
      Group {
        if let widgetShortcut = entry.widgetShortcut {
          widgetShortcut.image
            .widgetLabel(widgetShortcut.displayString)
            .accessibilityLabel(Text(widgetShortcut.displayString))
            .widgetURL(
              URL(
                string: "\(AppURLScheme.appURLScheme)://shortcut?path=\(widgetShortcut.rawValue)"
              )
            )
        } else {
          Image(systemName: "xmark.octagon")
            .accessibilityLabel(Text(Strings.Widgets.shortcutsEmptyState))
        }
      }
      .imageScale(.large)
      .frame(maxWidth: .infinity, maxHeight: .infinity)
      .font(.system(size: 20))
      .unredacted()
    }
  }
}

#if DEBUG

#Preview(
  as: .accessoryCircular,
  widget: {
    LockScreenShortcutWidget()
  },
  timeline: {
    LockScreenShortcutEntry(date: .now, widgetShortcut: .newTab)
  }
)

#Preview(
  as: .accessoryCircular,
  widget: {
    LockScreenShortcutWidget()
  },
  timeline: {
    LockScreenShortcutEntry(date: .now, widgetShortcut: nil)
  }
)

#endif
