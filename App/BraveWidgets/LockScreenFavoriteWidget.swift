// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WidgetKit
import SwiftUI
import BraveWidgetsModels
import FaviconModels
import Strings

struct LockScreenFavoriteWidget: Widget {
  var body: some WidgetConfiguration {
    if #available(iOSApplicationExtension 16.0, *) {
      return IntentConfiguration(kind: "LockScreenFavoriteWidget", intent: LockScreenFavoriteConfigurationIntent.self, provider: LockScreenFavoriteProvider()) { entry in
        LockScreenFavoriteView(entry: entry)
      }
      .configurationDisplayName(Strings.Widgets.favoritesWidgetTitle)
      .description(Strings.Widgets.favoritesWidgetDescription)
      .supportedFamilies([.accessoryCircular])
    } else {
      return EmptyWidgetConfiguration()
    }
  }
}

private struct LockScreenFavoriteEntry: TimelineEntry {
  var date: Date
  var favorite: WidgetFavorite?
}

private struct LockScreenFavoriteProvider: IntentTimelineProvider {
  typealias Intent = LockScreenFavoriteConfigurationIntent
  typealias Entry = LockScreenFavoriteEntry
  
  func placeholder(in context: Context) -> Entry {
    Entry(date: Date(), favorite: nil)
  }
  
  func widgetFavorite(for url: URL?, completion: @escaping (WidgetFavorite?) -> Void) {
    let favorites = FavoritesWidgetData.loadWidgetData() ?? []
    var selectedFavorite = favorites.first(where: { $0.url == url }) ?? favorites.first
    if let favicon = selectedFavorite?.favicon, let image = favicon.image {
      image.prepareThumbnail(of: .init(width: 64, height: 64)) { image in
        selectedFavorite?.favicon = .init(
          image: image,
          isMonogramImage: favicon.isMonogramImage,
          backgroundColor: favicon.backgroundColor
        )
        completion(selectedFavorite)
      }
    } else {
      completion(selectedFavorite)
    }
  }
  func getSnapshot(for configuration: LockScreenFavoriteConfigurationIntent, in context: Context, completion: @escaping (LockScreenFavoriteEntry) -> Void) {
    widgetFavorite(for: configuration.favorite?.url) { selectedFavorite in
      completion(Entry(date: Date(), favorite: selectedFavorite))
    }
  }
  func getTimeline(for configuration: LockScreenFavoriteConfigurationIntent, in context: Context, completion: @escaping (Timeline<LockScreenFavoriteEntry>) -> Void) {
    widgetFavorite(for: configuration.favorite?.url) { selectedFavorite in
      completion(Timeline(entries: [Entry(date: Date(), favorite: selectedFavorite)], policy: .never))
    }
  }
}

@available(iOS 16.0, *)
private struct LockScreenFavoriteView: View {
  var entry: LockScreenFavoriteEntry
  
  var body: some View {
    ZStack {
      AccessoryWidgetBackground()
        .widgetBackground { EmptyView() }
      if let fav = entry.favorite {
        Group {
          if let attributes = fav.favicon, let image = attributes.image {
            FaviconImage(image: image, contentMode: .scaleAspectFit, includePadding: false) // includePadding forced to false here since we are providing our own padding below
              .background(attributes.backgroundColor.cgColor.alpha == 0 ? .white :  Color(attributes.backgroundColor))
              .clipShape(RoundedRectangle(cornerRadius: 8, style: .continuous))
              .padding(12)
          } else {
            Text(verbatim: fav.url.baseDomain?.first?.uppercased() ?? "")
              .frame(maxWidth: .infinity, maxHeight: .infinity)
              .font(.system(size: 28))
              .clipped()
              .padding(4)
              .background(
                Color.white.aspectRatio(1, contentMode: .fit)
                  .clipShape(RoundedRectangle(cornerRadius: 8, style: .continuous))
              )
              .padding(12)
              .foregroundColor(Color.black)
          }
        }
        .accessibilityLabel(fav.title ?? fav.url.absoluteString)
        .widgetLabel(fav.title ?? "")
        .widgetURL(fav.url)
      } else {
        Image(braveSystemName: "leo.brave.icon-monochrome")
          .imageScale(.large)
          .font(.system(size: 24))
          .foregroundColor(Color.black)
          .frame(maxWidth: .infinity, maxHeight: .infinity)
          .background(
            Color(white: 0.9)
              .clipShape(RoundedRectangle(cornerRadius: 8, style: .continuous))
              .padding(12)
          )
      }
    }
  }
}

#if DEBUG
@available(iOS 16.0, *)
// swiftlint:disable:next swiftui_previews_guard
struct LockScreenFavoriteViewWidget_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      LockScreenFavoriteView(entry: .init(date: .now, favorite: nil))
        .previewDisplayName("No Favs")
      LockScreenFavoriteView(
        entry: .init(
          date: .now,
          favorite: .init(
            url: URL(string: "https://brave.com")!,
            title: "Brave",
            favicon: .init(
              image: nil,
              isMonogramImage: false,
              backgroundColor: .white
            )
          )
        )
      )
      .previewDisplayName("Monogram")
      LockScreenFavoriteView(
        entry: .init(
          date: .now,
          favorite: .init(
            url: URL(string: "https://wikipedia.com")!,
            title: "Wikipedia",
            favicon: .init(
              image: mockImage,
              isMonogramImage: false,
              backgroundColor: .white
            )
          )
        )
      )
      .previewDisplayName("Favicon")
    }
    .previewContext(WidgetPreviewContext(family: .accessoryCircular))
  }
}

private let mockImage = UIImage(data: Data(base64Encoded: "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".data(using: .utf8)!)!) // swiftlint:disable:this line_length
#endif
