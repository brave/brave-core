// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveWidgetsModels
import FaviconModels
import Strings
import SwiftUI
import WidgetKit

struct FavoritesWidget: Widget {
  var body: some WidgetConfiguration {
    StaticConfiguration(kind: "FavoritesWidget", provider: FavoritesProvider()) { entry in
      FavoritesView(entry: entry)
    }
    .configurationDisplayName(Strings.Widgets.favoritesWidgetTitle)
    .description(Strings.Widgets.favoritesWidgetDescription)
    .supportedFamilies([.systemMedium, .systemLarge])
    .contentMarginsDisabled()
  }
}

private struct FavoriteEntry: TimelineEntry {
  var date: Date
  var favorites: [WidgetFavorite]
}

private struct FavoritesProvider: TimelineProvider {
  typealias Entry = FavoriteEntry

  func placeholder(in context: Context) -> Entry {
    Entry(date: Date(), favorites: [])
  }
  func getSnapshot(in context: Context, completion: @escaping (Entry) -> Void) {
    Task {
      let favorites = await FavoritesWidgetData.loadWidgetData() ?? []
      completion(Entry(date: Date(), favorites: favorites))
    }
  }
  func getTimeline(in context: Context, completion: @escaping (Timeline<Entry>) -> Void) {
    Task {
      let favorites = await FavoritesWidgetData.loadWidgetData() ?? []
      completion(Timeline(entries: [Entry(date: Date(), favorites: favorites)], policy: .never))
    }
  }
}

struct FaviconImage: View {
  var image: UIImage
  var contentMode: UIView.ContentMode
  var includePadding: Bool

  var body: some View {
    Image(uiImage: image)
      .resizable()
      .widgetAccentedRenderingModeFullColor()
      .aspectRatio(1, contentMode: .fit)
      .frame(maxWidth: .infinity, maxHeight: .infinity)
      .clipped()
      .padding(includePadding ? 8 : 0)
  }
}

private struct NoFavoritesFoundView: View {
  var body: some View {
    VStack {
      Image("brave-icon")
        .widgetAccentedRenderingModeFullColor()
        .clipShape(RoundedRectangle(cornerRadius: 8, style: .continuous))
      Text(Strings.Widgets.noFavoritesFound)
        .multilineTextAlignment(.center)
        .foregroundColor(Color(UIColor.braveLabel))
    }
    .padding()
    .unredacted()
  }
}

private struct FavoritesView: View {
  var entry: FavoriteEntry

  var body: some View {
    Group {
      if entry.favorites.isEmpty {
        NoFavoritesFoundView()
      } else {
        FavoritesGridView(entry: entry)
      }
    }
    .frame(maxWidth: .infinity, maxHeight: .infinity)
    .widgetBackground { Color(UIColor.secondaryBraveBackground) }
  }
}

private struct FavoritesGridView: View {
  var entry: FavoriteEntry
  @Environment(\.widgetFamily) var widgetFamily
  @Environment(\.colorScheme) var colorScheme
  @Environment(\.pixelLength) var pixelLength
  @Environment(\.redactionReasons) var redactionReasons

  var itemsCount: Int {
    switch widgetFamily {
    case .systemMedium:
      return 8
    case .systemLarge:
      return 16
    default:
      assertionFailure("widget family isn't supported")
      return 0
    }
  }

  var itemShape: RoundedRectangle {
    RoundedRectangle(cornerRadius: widgetFamily == .systemMedium ? 16 : 12, style: .continuous)
  }

  var emptyField: some View {
    itemShape
      .fill(Color.black.opacity(0.05))
      .frame(maxWidth: .infinity, maxHeight: .infinity)
      .overlay(
        itemShape
          .strokeBorder(Color(UIColor.braveLabel).opacity(0.2), lineWidth: pixelLength)
      )
      .aspectRatio(1.0, contentMode: .fit)
  }

  private var placeholderOrPrivacyRedaction: Bool {
    redactionReasons.contains(.placeholder) || redactionReasons.contains(.privacy)
  }

  func image(for favicon: Favicon) -> UIImage? {
    guard let image = favicon.image else { return nil }
    return image.preparingThumbnail(of: CGSize(width: 128, height: 128))
  }

  var body: some View {
    LazyVGrid(columns: Array(repeating: .init(.flexible()), count: 4), spacing: 8) {
      ForEach((0..<itemsCount), id: \.self) {
        if let favorite = entry.favorites[safe: $0], !placeholderOrPrivacyRedaction {
          Link(
            destination: favorite.url,
            label: {
              Group {
                if let attributes = favorite.favicon, let image = image(for: attributes) {
                  FaviconImage(image: image, contentMode: .scaleAspectFit, includePadding: false)
                    .background(Color(attributes.backgroundColor))
                } else {
                  Text(verbatim: favorite.url.baseDomain?.first?.uppercased() ?? "")
                    .frame(maxWidth: .infinity, maxHeight: .infinity)
                    .font(.system(size: 36))
                    .aspectRatio(1.0, contentMode: .fit)
                    .clipped()
                    .background(Color(UIColor.braveBackground))
                    .foregroundColor(Color(UIColor.braveLabel))
                }
              }
              .clipShape(itemShape)
              .background(Color(UIColor.braveBackground).opacity(0.05).clipShape(itemShape))
              .overlay(
                itemShape
                  .strokeBorder(Color(UIColor.braveLabel).opacity(0.2), lineWidth: pixelLength)
              )
              .padding(widgetFamily == .systemMedium ? 4 : 0)
            }
          )
        } else {
          emptyField
            .padding(widgetFamily == .systemMedium ? 4 : 0)
        }
      }
    }
    .padding()
  }
}

// MARK: - Preview

#if DEBUG
struct FavoritesView_Previews: PreviewProvider {
  static var previews: some View {
    FavoritesView(entry: .init(date: Date(), favorites: []))
      .previewContext(WidgetPreviewContext(family: .systemMedium))
    FavoritesView(
      entry: .init(
        date: Date(),
        favorites: [
          // TODO: Fill with favorites.
        ]
      )
    )
    .previewContext(WidgetPreviewContext(family: .systemMedium))
    FavoritesView(entry: .init(date: Date(), favorites: []))
      .previewContext(WidgetPreviewContext(family: .systemLarge))
  }
}
#endif
