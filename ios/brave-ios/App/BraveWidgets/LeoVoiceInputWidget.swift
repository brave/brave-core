// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveShared
import BraveWidgetsModels
import Strings
import SwiftUI
import WidgetKit

struct LeoVoiceInputWidget: Widget {
  var body: some WidgetConfiguration {
    StaticConfiguration(kind: "LeoVoiceInputWidget", provider: LeoVoiceInputProvider()) { _ in
      LeoVoiceInputView()
        .unredacted()
    }
    .configurationDisplayName(Strings.Widgets.leoVoiceInputWidgetTitle)
    .description(Strings.Widgets.leoVoiceInputWidgetDescription)
    .supportedFamilies([.systemSmall])
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

private struct LeoVoiceInputView: View {
  var body: some View {
    Link(
      destination: URL(
        string:
          "\(AppURLScheme.appURLScheme)://shortcut?path=\(WidgetShortcut.braveLeoVoiceInput.rawValue)"
      )!,
      label: {
        VStack(spacing: 8) {
          WidgetShortcut.braveLeoVoiceInput.image
            .imageScale(.large)
            .font(.system(size: 28))
          Text(Strings.Widgets.braveLeoVoiceInput)
            .font(.system(size: 13, weight: .semibold))
            .multilineTextAlignment(.center)
        }
        .foregroundColor(Color(braveSystemName: .textPrimary))
        .frame(maxWidth: .infinity, maxHeight: .infinity)
      }
    )
    .widgetBackground { Color(UIColor(braveSystemName: .containerHighlight)) }
  }
}

// MARK: - Previews

#if DEBUG

#Preview(
  as: .systemSmall,
  widget: {
    LeoVoiceInputWidget()
  },
  timeline: {
    LeoVoiceInputEntry(date: .now)
  }
)

#endif
