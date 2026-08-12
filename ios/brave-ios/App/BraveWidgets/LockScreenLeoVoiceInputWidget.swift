// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveShared
import BraveWidgetsModels
import Strings
import SwiftUI
import WidgetKit

struct LockScreenLeoVoiceInputWidget: Widget {
  var body: some WidgetConfiguration {
    StaticConfiguration(
      kind: "LockScreenLeoVoiceInputWidget",
      provider: LeoVoiceInputProvider()
    ) { _ in
      LockScreenLeoVoiceInputView()
    }
    .configurationDisplayName(Strings.Widgets.leoVoiceInputWidgetTitle)
    .description(Strings.Widgets.leoVoiceInputWidgetDescription)
    .supportedFamilies([.accessoryCircular])
    .contentMarginsDisabled()
  }
}

private struct LeoVoiceInputEntry: TimelineEntry {
  var date: Date
}

private struct LeoVoiceInputProvider: TimelineProvider {
  typealias Entry = LeoVoiceInputEntry

  func placeholder(in context: Context) -> Entry {
    .init(date: Date())
  }
  func getSnapshot(in context: Context, completion: @escaping (Entry) -> Void) {
    completion(.init(date: Date()))
  }
  func getTimeline(in context: Context, completion: @escaping (Timeline<Entry>) -> Void) {
    completion(.init(entries: [.init(date: Date())], policy: .never))
  }
}

private struct LockScreenLeoVoiceInputView: View {
  var body: some View {
    ZStack {
      AccessoryWidgetBackground()
        .widgetBackground { EmptyView() }
      WidgetShortcut.braveLeoVoiceInput.image
        .imageScale(.large)
        .font(.system(size: 20))
        .widgetLabel(Strings.Widgets.braveLeoVoiceInput)
        .accessibilityLabel(Text(Strings.Widgets.braveLeoVoiceInput))
    }
    .widgetURL(
      URL(
        string:
          "\(AppURLScheme.appURLScheme)://shortcut?path=\(WidgetShortcut.braveLeoVoiceInput.rawValue)"
      )
    )
  }
}

// MARK: - Previews

#if DEBUG

#Preview(
  as: .accessoryCircular,
  widget: {
    LockScreenLeoVoiceInputWidget()
  },
  timeline: {
    LeoVoiceInputEntry(date: .now)
  }
)

#endif
