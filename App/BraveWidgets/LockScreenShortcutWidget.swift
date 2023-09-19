// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI
import WidgetKit
import BraveWidgetsModels
import BraveShared
import Strings

struct LockScreenShortcutWidget: Widget {
  var body: some WidgetConfiguration {
    if #available(iOSApplicationExtension 16.0, *) {
      return IntentConfiguration(kind: "LockScreenShortcutWidget", intent: LockScreenShortcutConfigurationIntent.self, provider: LockScreenShortcutProvider()) { entry in
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
  var widgetShortcut: WidgetShortcut
}

struct LockScreenShortcutProvider: IntentTimelineProvider {
  typealias Intent = LockScreenShortcutConfigurationIntent
  typealias Entry = LockScreenShortcutEntry
  
  func placeholder(in context: Context) -> Entry {
    .init(date: Date(), widgetShortcut: .bookmarks)
  }
  func getSnapshot(for configuration: Intent, in context: Context, completion: @escaping (Entry) -> Void) {
    let entry = LockScreenShortcutEntry(
      date: Date(),
      widgetShortcut: configuration.shortcut
    )
    completion(entry)
  }
  func getTimeline(for configuration: Intent, in context: Context, completion: @escaping (Timeline<Entry>) -> Void) {
    let entry = LockScreenShortcutEntry(
      date: Date(),
      widgetShortcut: configuration.shortcut
    )
    completion(.init(entries: [entry], policy: .never))
  }
}

@available(iOS 16.0, *)
struct LockScreenShortcutView: View {
  var entry: LockScreenShortcutEntry
  
  var body: some View {
    ZStack {
      AccessoryWidgetBackground()
        .widgetBackground { EmptyView() }
      entry.widgetShortcut.image
        .imageScale(.large)
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .font(.system(size: 20))
        .widgetLabel(entry.widgetShortcut.displayString)
        .accessibilityLabel(Text(entry.widgetShortcut.displayString))
        .unredacted()
        .widgetURL(URL(string: "\(AppURLScheme.appURLScheme)://shortcut?path=\(entry.widgetShortcut.rawValue)"))
    }
  }
}

