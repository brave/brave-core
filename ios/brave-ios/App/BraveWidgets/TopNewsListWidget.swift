// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import AVFoundation
import BraveShared
import BraveWidgetsModels
import CodableHelpers
import DesignSystem
import Shared
import SwiftUI
import UIKit
import WidgetKit
import os

struct TopNewsListWidget: Widget {
  var body: some WidgetConfiguration {
    StaticConfiguration(kind: "TopNewsListWidget", provider: TopNewsListWidgetProvider()) { entry in
      TopNewsListView(entry: entry)
    }
    .supportedFamilies([.systemMedium, .systemLarge])
    .configurationDisplayName(Strings.Widgets.newsClusteringWidgetTitle)
    .description(Strings.Widgets.newsClusteringWidgetDescription)
    .contentMarginsDisabled()
  }
}

private struct TopNewsListEntry: TimelineEntry {
  var date: Date = .now
  var topics: [NewsTopic]?
  var images: [NewsTopic.ID: UIImage] = [:]
}

private struct TopNewsListWidgetProvider: TimelineProvider {
  private let model: NewsTopicsModel = .live
  private let thumbnailSize: CGSize = .init(width: 192, height: 192)

  func getSnapshot(in context: Context, completion: @escaping (TopNewsListEntry) -> Void) {
    Task {
      let grouping = context.family == .systemMedium ? 2 : 5
      // No need to load more than the grouping in the snapshot
      let topics = Array(await model.fetchNewsTopics(Locale.autoupdatingCurrent).prefix(grouping))
      let images = await model.fetchImageThumbnailsForTopics(topics, thumbnailSize)
      completion(.init(date: Date(), topics: topics, images: images))
    }
  }
  func getTimeline(in context: Context, completion: @escaping (Timeline<TopNewsListEntry>) -> Void)
  {
    Task {
      let grouping = context.family == .systemMedium ? 2 : 5
      let interval = context.family == .systemMedium ? 15 : 20
      let allTopics = Array(
        await model.fetchNewsTopics(Locale.autoupdatingCurrent).prefix(grouping * 3)
      )
      let topics = allTopics.splitEvery(grouping)
      let images = await model.fetchImageThumbnailsForTopics(allTopics, thumbnailSize)
      let entries: [TopNewsListEntry] = zip(topics, topics.indices).map({ topics, index in
        .init(
          date: Date().addingTimeInterval(interval.minutes * TimeInterval(index)),
          topics: topics,
          images: images
        )
      })
      completion(.init(entries: entries, policy: .after(Date().addingTimeInterval(60.minutes))))
    }
  }
  func placeholder(in context: Context) -> TopNewsListEntry {
    .init(date: Date(), topics: nil)
  }
}

private struct TopNewsListView: View {
  @Environment(\.pixelLength) var pixelLength
  @Environment(\.widgetFamily) var widgetFamily
  var entry: TopNewsListEntry

  private var headerView: some View {
    HStack {
      HStack(spacing: 4) {
        Image("brave-icon-no-bg")
          .resizable()
          .aspectRatio(contentMode: .fit)
          .frame(width: 16, height: 16)
        Text(Strings.Widgets.braveNews)
          .foregroundColor(Color(.braveLabel))
          .font(.system(size: 14, weight: .bold, design: .rounded))
      }
      Spacer()
      Link(
        destination: URL(
          string:
            "\(AppURLScheme.appURLScheme)://shortcut?path=\(WidgetShortcut.braveNews.rawValue)"
        )!
      ) {
        Text(Strings.Widgets.newsClusteringReadMoreButtonTitle)
          .foregroundColor(Color(.braveBlurpleTint))
          .font(.system(size: 13, weight: .semibold, design: .rounded))
      }
    }
    .padding(.horizontal)
    .padding(.vertical, widgetFamily == .systemLarge ? 12 : 8)
    .unredacted()
  }

  var body: some View {
    Group {
      if let topics = entry.topics, !topics.isEmpty {
        VStack(alignment: .leading, spacing: widgetFamily == .systemLarge ? 12 : 8) {
          headerView
            .background(Color(.braveGroupedBackground))
          VStack(alignment: .leading, spacing: 8) {
            ForEach(topics.prefix(widgetFamily == .systemLarge ? 5 : 2)) { topic in
              HStack {
                Link(destination: topic.url) {
                  VStack(alignment: .leading, spacing: 2) {
                    Text(topic.title)
                      .lineLimit(widgetFamily == .systemLarge ? 3 : 2)
                      .font(
                        .system(
                          size: widgetFamily == .systemLarge ? 14 : 13,
                          weight: .semibold,
                          design: .rounded
                        )
                      )
                      .foregroundColor(.primary)
                    Text(topic.publisherName)
                      .lineLimit(1)
                      .font(.system(size: 10, weight: .medium, design: .rounded))
                      .foregroundColor(.secondary)
                  }
                }
                if let image = entry.images[topic.id] {
                  Spacer()
                  Color.clear
                    .aspectRatio(1, contentMode: .fit)
                    .frame(maxHeight: 50)
                    .overlay(
                      Image(uiImage: image)
                        .resizable()
                        .aspectRatio(contentMode: .fill)
                    )
                    .clipShape(RoundedRectangle(cornerRadius: 8, style: .continuous))
                    .overlay(
                      RoundedRectangle(cornerRadius: 8, style: .continuous).strokeBorder(
                        Color.primary.opacity(0.3),
                        lineWidth: pixelLength
                      )
                    )
                }
              }
            }
          }
          .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
          .padding(.horizontal)
        }
        .padding(.bottom)
      } else {
        ZStack(alignment: .top) {
          Text(Strings.Widgets.newsClusteringErrorLabel)
            .font(
              .system(
                size: widgetFamily == .systemLarge ? 26 : 18,
                weight: .semibold,
                design: .rounded
              )
            )
            .padding()
            .frame(maxWidth: .infinity, maxHeight: .infinity)
            .background {
              LinearGradient(braveGradient: .darkGradient01)
                .mask {
                  Image("brave-today-error")
                    .renderingMode(.template)
                    .resizable(resizingMode: .tile)
                    .opacity(0.1)
                    .rotationEffect(.degrees(-45))
                    .scaleEffect(x: 2.1, y: 2.1)
                }
            }
          headerView
            .background {
              LinearGradient(
                colors: [Color(.braveBackground), Color(.braveBackground).opacity(0.0)],
                startPoint: .top,
                endPoint: .bottom
              )
            }
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .top)
      }
    }
    .widgetBackground { Color(.braveBackground) }
  }
}

#if DEBUG
struct TopNewsListView_PreviewProvider: PreviewProvider {
  struct PreviewView: View {
    let model = NewsTopicsModel.mock
    @State private var topics: [NewsTopic] = []

    var body: some View {
      TopNewsListView(entry: .init(date: .now, topics: topics, images: [:]))
        .task {
          topics = await model.fetchNewsTopics(Locale(identifier: "en_US"))
        }
    }
  }
  static var previews: some View {
    PreviewView()
      .previewContext(WidgetPreviewContext(family: .systemLarge))
    PreviewView()
      .previewContext(WidgetPreviewContext(family: .systemMedium))
  }
}
#endif
