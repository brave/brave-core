// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import AVFoundation
import CodableHelpers
import DesignSystem
import OrderedCollections
import Shared
import SwiftUI
import WidgetKit
import os

struct TopNewsWidget: Widget {
  var supportedFamilies: [WidgetFamily] {
    return [.systemSmall, .accessoryRectangular]
  }

  var body: some WidgetConfiguration {
    StaticConfiguration(kind: "TopNewsWidget", provider: TopNewsWidgetProvider()) { entry in
      TopNewsView(entry: entry)
    }
    .supportedFamilies(supportedFamilies)
    .configurationDisplayName(Strings.Widgets.newsClusteringWidgetTitle)
    .description(Strings.Widgets.newsClusteringWidgetDescription)
    .contentMarginsDisabled()
    .containerBackgroundRemovable(false)
  }
}

private struct TopNewsEntry: TimelineEntry {
  var date: Date
  var topic: NewsTopic?
  var image: UIImage?

  init(date: Date = .now, topic: NewsTopic?, image: UIImage? = nil) {
    self.date = date
    self.topic = topic
    self.image = image
  }
}

extension WidgetFamily {
  var isLockScreen: Bool {
    switch self {
    case .accessoryCircular, .accessoryInline, .accessoryRectangular:
      return true
    case .systemSmall, .systemMedium, .systemLarge, .systemExtraLarge:
      return false
    @unknown default:
      return false
    }
  }
}

private struct TopNewsWidgetProvider: TimelineProvider {
  private let model: NewsTopicsModel = .live

  private func thumbnailSize(for context: Context) -> CGSize {
    let size = context.displaySize
    let scale = context.environmentVariants.displayScale?.max() ?? 1.0
    return .init(width: size.width * scale, height: size.height * scale)
  }

  func getSnapshot(in context: Context, completion: @escaping (TopNewsEntry) -> Void) {
    Task {
      let topics = await model.fetchNewsTopics(Locale.autoupdatingCurrent)
      var entry: TopNewsEntry = .init(topic: topics.first)
      if let topic = entry.topic, !context.family.isLockScreen {
        if let (_, image) = await model.fetchImageThumbnailsForTopics(
          [topic],
          thumbnailSize(for: context)
        ).first {
          entry.image = image
        }
      }
      completion(entry)
    }
  }

  func getTimeline(in context: Context, completion: @escaping (Timeline<TopNewsEntry>) -> Void) {
    Task {
      let topics = Array(await model.fetchNewsTopics(Locale.autoupdatingCurrent).prefix(6))
      var images: [NewsTopic.ID: UIImage] = [:]
      if !context.family.isLockScreen {
        images = await model.fetchImageThumbnailsForTopics(topics, thumbnailSize(for: context))
      }
      let entries: [TopNewsEntry] = zip(topics, topics.indices).map {
        .init(
          date: Date.now.addingTimeInterval(15.minutes * TimeInterval($1)),
          topic: $0,
          image: images[$0.id]
        )
      }
      completion(.init(entries: entries, policy: .after(Date.now.addingTimeInterval(60.minutes))))
    }
  }

  func placeholder(in context: Context) -> TopNewsEntry {
    .init(date: Date(), topic: nil)
  }
}

private struct TopNewsView: View {
  @Environment(\.widgetFamily) private var widgetFamily
  var entry: TopNewsEntry

  var body: some View {
    if widgetFamily == .accessoryRectangular {
      LockScreenTopNewsView(entry: entry)
    } else {
      WidgetTopNewsView(entry: entry)
    }
  }
}

private struct LockScreenTopNewsView: View {
  var entry: TopNewsEntry

  var body: some View {
    if let topic = entry.topic {
      VStack(alignment: .leading, spacing: 2) {
        Text(topic.title)
          .font(.system(size: 12, weight: .bold, design: .rounded))
          .lineSpacing(0)
          .layoutPriority(2)
          .multilineTextAlignment(.leading)
        HStack(spacing: 3) {
          Image(braveSystemName: "leo.brave.icon-outline")
            .foregroundColor(.orange)
            .font(.system(size: 12))
            .padding(.trailing, -1)
            .unredacted()
          Divider()
            .frame(height: 11)
          Text("\(topic.publisherName)")
            .lineLimit(1)
            .font(.system(size: 11, weight: .semibold, design: .rounded))
            .foregroundColor(.secondary)
            .minimumScaleFactor(0.75)
        }
        .foregroundColor(.secondary)
      }
      .allowsTightening(true)
      .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
      .widgetURL(topic.url)
      .widgetBackground { Color.clear }
    } else {
      VStack(spacing: 4) {
        Image("brave-today-error")
          .resizable()
          .renderingMode(.template)
          .aspectRatio(contentMode: .fit)
          .frame(height: 32)
          .foregroundColor(.black)
        HStack(spacing: 3) {
          Text(Strings.Widgets.newsClusteringErrorLabel)
            .lineLimit(1)
            .font(.system(size: 11, weight: .semibold, design: .rounded))
            .foregroundColor(.secondary)
            .minimumScaleFactor(0.75)
        }
        .frame(maxWidth: .infinity)
      }
      .allowsTightening(true)
      .frame(maxWidth: .infinity, maxHeight: .infinity)
      .widgetBackground { Color.clear }
    }
  }
}

private struct WidgetTopNewsView: View {
  var entry: TopNewsEntry

  var body: some View {
    if let topic = entry.topic {
      VStack(alignment: .leading) {
        HStack {
          Image(braveSystemName: "leo.brave.icon-monochrome")
            .font(.footnote)
            .imageScale(.large)
            .foregroundColor(Color(.braveOrange))
            .padding(4)
            .background(
              Color(.white).clipShape(Circle()).shadow(color: .black.opacity(0.2), radius: 2, y: 1)
            )
        }
        Spacer()
        VStack(alignment: .leading, spacing: 4) {
          Text(topic.title)
            .shadow(radius: 2, y: 1)
            .lineLimit(3)
            .font(.system(size: 16, weight: .semibold, design: .rounded))
            .foregroundColor(.white)
            .allowsTightening(true)
          Text(topic.publisherName)
            .shadow(radius: 2, y: 1)
            .lineLimit(1)
            .minimumScaleFactor(0.7)
            .font(.system(size: 11, weight: .medium, design: .rounded))
            .foregroundColor(.white.opacity(0.95))
        }
      }
      .padding()
      .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .leading)
      .widgetBackground {
        if let image = entry.image {
          Image(uiImage: image)
            .resizable()
            .aspectRatio(contentMode: .fill)
            .overlay(
              LinearGradient(
                colors: [.black.opacity(0.0), .black.opacity(0.6)],
                startPoint: .top,
                endPoint: .bottom
              )
            )
        } else {
          LinearGradient(braveGradient: .darkGradient01)
        }
      }
      .widgetURL(topic.url)
    } else {
      VStack(alignment: .leading) {
        Image(braveSystemName: "leo.brave.icon-monochrome")
          .font(.footnote)
          .imageScale(.large)
          .foregroundColor(Color(.braveOrange))
          .padding(4)
          .background(
            Color(.white).clipShape(Circle()).shadow(color: .black.opacity(0.2), radius: 2, y: 1)
          )
        Spacer()
        Text(Strings.Widgets.newsClusteringErrorLabel)
          .font(.system(size: 14, weight: .medium, design: .rounded))
      }
      .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .leading)
      .padding()
      .widgetBackground {
        LinearGradient(braveGradient: .darkGradient01)
          .mask {
            Image("brave-today-error")
              .renderingMode(.template)
              .resizable(resizingMode: .tile)
              .opacity(0.1)
              .rotationEffect(.degrees(-45))
              .scaleEffect(x: 1.5, y: 1.5)
          }
      }
      //      .background(Color(.braveBackground))
    }
  }
}

#if DEBUG
struct TopNewsView_PreviewProvider: PreviewProvider {
  private static var entry: TopNewsEntry {
    .init(
      date: Date(),
      topic: .init(
        topicIndex: 0,
        title:
          "Jacinda Ardern resignation – live: Shock as New Zealand prime minister announces decision",
        description:
          "Ardern became the world’s youngest female head of government when she was elected prime minister at age 37 in 2017",
        url: URL(
          string:
            "https://www.independent.co.uk/news/world/australasia/jacinda-ardern-resignation-new-zealand-polls-prime-minister-b2265025.html"
        )!,
        imageURL: URL(
          string:
            "https://static.independent.co.uk/2023/01/19/01/NUEVA_ZELANDA_ELECCIONES_98401.jpg?width=1200&auto=webp"
        )!,
        publisherName: "The Independent World News",
        date: {
          let df = DateFormatter()
          df.dateFormat = "yyyy-MM-dd HH:mm:ss"
          return df.date(from: "2023-01-19 15:48:54")!
        }(),
        score: 3.94501334,
        category: "World News"
      )
    )
  }
  static var previews: some View {
    TopNewsView(
      entry: entry
    )
    .previewContext(WidgetPreviewContext(family: .systemSmall))
    TopNewsView(entry: entry)
      .previewContext(WidgetPreviewContext(family: .accessoryRectangular))
    TopNewsView(entry: .init(topic: nil))
      .previewContext(WidgetPreviewContext(family: .accessoryRectangular))
    TopNewsView(entry: .init(topic: nil))
      .previewContext(WidgetPreviewContext(family: .systemSmall))
  }
}
#endif
