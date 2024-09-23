// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveWidgetsModels
import Shared
import SwiftUI
import WidgetKit

struct StatsWidget: Widget {
  var body: some WidgetConfiguration {
    StaticConfiguration(kind: "StatsWidget", provider: StatsProvider()) { entry in
      StatsView(entry: entry)
    }
    .supportedFamilies([.systemMedium])
    .configurationDisplayName(Strings.Widgets.shieldStatsTitle)
    .description(Strings.Widgets.shieldStatsDescription)
    .contentMarginsDisabled()
  }
}

private struct StatsEntry: TimelineEntry {
  var date: Date
  var statData: [StatData]
}

private struct StatsProvider: TimelineProvider {
  typealias Entry = StatsEntry

  var stats: [StatData] {
    let kinds: [StatKind] = [.adsBlocked, .dataSaved, .timeSaved]
    return kinds.map { StatData(name: $0.name, value: $0.displayString, color: $0.valueColor) }
  }

  func placeholder(in context: Context) -> Entry {
    Entry(date: Date(), statData: [])
  }
  func getSnapshot(in context: Context, completion: @escaping (Entry) -> Void) {
    let entry = Entry(date: Date(), statData: stats)
    completion(entry)
  }
  func getTimeline(in context: Context, completion: @escaping (Timeline<Entry>) -> Void) {
    let entry = Entry(date: Date(), statData: stats)
    let timeline = Timeline(entries: [entry], policy: .never)
    completion(timeline)
  }
}

private struct StatsView: View {
  var entry: StatsEntry
  @Environment(\.redactionReasons) var redactionReasons

  private var placeholderOrPrivacyRedaction: Bool {
    redactionReasons.contains(.placeholder) || redactionReasons.contains(.privacy)
  }

  var body: some View {
    VStack {
      HStack {
        Image("brave.shields.done")
        Text(Strings.Widgets.shieldStatsWidgetTitle)
          .foregroundColor(Color(UIColor.braveLabel))
          .font(.system(size: 13, weight: .semibold))
        Spacer()
        Image("brave-icon-no-bg")
          .resizable()
          .widgetAccentedRenderingModeFullColor()
          .aspectRatio(contentMode: .fit)
          .frame(height: 24)
      }
      .unredacted()
      Spacer()
      HStack(alignment: .top, spacing: 8) {
        ForEach(entry.statData, id: \.name) { data in
          Color.clear
            .overlay(
              VStack(spacing: 4) {
                Text(verbatim: placeholderOrPrivacyRedaction ? "-" : data.value)
                  .font(.system(size: 32.0))
                  .foregroundColor(Color(data.color))
                  .widgetAccentable()
                  .minimumScaleFactor(0.5)
                  .lineLimit(1)
                  .unredacted()
                Text(verbatim: data.name)
                  .font(.system(size: 10, weight: .semibold))
                  .multilineTextAlignment(.center)
                  .unredacted()
              }
            )
            .frame(maxWidth: .infinity, maxHeight: .infinity)
            .padding(4)
        }
      }
      Spacer()
    }
    .frame(maxWidth: .infinity, maxHeight: .infinity)
    .padding(.vertical, 12)
    .padding(.horizontal, 16)
    .widgetBackground { Color(UIColor.secondaryBraveBackground) }
    .foregroundColor(Color(UIColor.braveLabel))
  }
}

// MARK: - Previews

#if DEBUG
struct StatsWidget_Previews: PreviewProvider {
  static var stats: [StatData] {
    let kinds: [StatKind] = [.adsBlocked, .dataSaved, .timeSaved]
    return kinds.map { StatData(name: $0.name, value: $0.displayString, color: $0.valueColor) }
  }

  static var fullStats: [StatData] {
    let ads = StatData(
      name: StatKind.adsBlocked.name,
      value: "113K",
      color: StatKind.adsBlocked.valueColor
    )
    let data = StatData(
      name: StatKind.dataSaved.name,
      value: "3,47GB",
      color: StatKind.dataSaved.valueColor
    )
    let time = StatData(
      name: StatKind.timeSaved.name,
      value: "1h",
      color: StatKind.timeSaved.valueColor
    )
    return [ads, data, time]
  }

  static var previews: some View {
    StatsView(entry: StatsEntry(date: Date(), statData: fullStats))
      .previewContext(WidgetPreviewContext(family: .systemMedium))
    StatsView(entry: StatsEntry(date: Date(), statData: stats))
      .previewContext(WidgetPreviewContext(family: .systemMedium))
    StatsView(entry: StatsEntry(date: Date(), statData: stats))
      .redacted(reason: .placeholder)
      .previewContext(WidgetPreviewContext(family: .systemMedium))
  }
}
#endif
