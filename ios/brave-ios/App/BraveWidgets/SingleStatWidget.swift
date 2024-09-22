// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveShields
import BraveWidgetsModels
import DesignSystem
import Intents
import Strings
import SwiftUI
import WidgetKit

struct SingleStatWidget: Widget {

  var body: some WidgetConfiguration {
    IntentConfiguration(
      kind: "SingleStatWidget",
      intent: StatsConfigurationIntent.self,
      provider: StatProvider()
    ) { entry in
      StatView(entry: entry)
    }
    .supportedFamilies([.systemSmall])
    .configurationDisplayName(Strings.Widgets.singleStatTitle)
    .description(Strings.Widgets.singleStatDescription)
  }
}

private struct StatEntry: TimelineEntry {
  var date: Date
  var statData: StatData
}

private struct StatProvider: IntentTimelineProvider {
  typealias Intent = StatsConfigurationIntent
  typealias Entry = StatEntry

  func placeholder(in context: Context) -> Entry {
    StatEntry(
      date: Date(),
      statData: .init(name: Strings.Shields.shieldsAdAndTrackerStats, value: "100k")
    )
  }
  func getSnapshot(
    for configuration: Intent,
    in context: Context,
    completion: @escaping (Entry) -> Void
  ) {
    let stat = configuration.statKind
    let entry = StatEntry(
      date: Date(),
      statData: .init(name: stat.name, value: stat.displayString, color: stat.valueColor)
    )
    completion(entry)
  }
  func getTimeline(
    for configuration: Intent,
    in context: Context,
    completion: @escaping (Timeline<Entry>) -> Void
  ) {
    let stat = configuration.statKind
    let entry = StatEntry(
      date: Date(),
      statData: .init(name: stat.name, value: stat.displayString, color: stat.valueColor)
    )
    let timeline = Timeline(entries: [entry], policy: .never)
    completion(timeline)
  }
}

private struct StatView: View {
  var entry: StatEntry

  var body: some View {
    VStack(alignment: .leading) {
      HStack {
        Spacer()
        Image("brave-icon-no-bg")
          .resizable()
          .widgetAccentedRenderingModeFullColor()
          .aspectRatio(contentMode: .fit)
          .frame(height: 24)
          .unredacted()
      }
      Spacer()
      Text(verbatim: entry.statData.value)
        .font(.system(size: 40))
        .foregroundColor(Color(entry.statData.color))
        .widgetAccentable()
      Text(verbatim: entry.statData.name)
        .font(.system(size: 17))
        .bold()
        .fixedSize(horizontal: false, vertical: true)
        .unredacted()
    }
    .frame(maxWidth: .infinity, alignment: .leading)
    .widgetBackground { Color(UIColor.secondaryBraveBackground) }
    .foregroundColor(Color(UIColor.braveLabel))
  }
}

// MARK: - Previews

#if DEBUG
struct SingleStatWidget_Previews: PreviewProvider {
  static var previews: some View {
    StatView(
      entry: StatEntry(
        date: Date(),
        statData: .init(
          name: "Ads & Trackers Blocked",
          value: "100k",
          color: UIColor.braveBlurpleTint
        )
      )
    )
    .previewContext(WidgetPreviewContext(family: .systemSmall))
    StatView(
      entry: StatEntry(
        date: Date(),
        statData: .init(name: "Placeholder Count", value: "100k", color: UIColor.braveBlurpleTint)
      )
    )
    .redacted(reason: .placeholder)
    .previewContext(WidgetPreviewContext(family: .systemSmall))
  }
}
#endif
