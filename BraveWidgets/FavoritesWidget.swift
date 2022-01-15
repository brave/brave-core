// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import WidgetKit
import SwiftUI
import Shared
import BraveShared

struct FavoritesWidget: Widget {
    var body: some WidgetConfiguration {
        StaticConfiguration(kind: "FavoritesWidget", provider: FavoritesProvider()) { entry in
            FavoritesView(entry: entry)
        }
        .configurationDisplayName(Strings.Widgets.favoritesWidgetTitle)
        .description(Strings.Widgets.favoritesWidgetDescription)
        .supportedFamilies([.systemMedium, .systemLarge])
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
        let favorites = FavoritesWidgetData.loadWidgetData() ?? []
        completion(Entry(date: Date(), favorites: favorites))
    }
    func getTimeline(in context: Context, completion: @escaping (Timeline<Entry>) -> Void) {
        let favorites = FavoritesWidgetData.loadWidgetData() ?? []
        completion(Timeline(entries: [Entry(date: Date(), favorites: favorites)], policy: .never))
    }
}

private struct FaviconImage: View {
    var image: UIImage
    var contentMode: UIView.ContentMode
    
    var usePadding: Bool {
        switch contentMode {
        case .scaleToFill, .scaleAspectFit, .scaleAspectFill:
            return false
        default:
            return true
        }
    }
    
    var body: some View {
        Image(uiImage: image)
            .resizable()
            .aspectRatio(1, contentMode: .fit)
            .frame(maxWidth: .infinity, maxHeight: .infinity)
            .clipped()
            .padding(usePadding ? 8 : 0)
    }
}

private struct NoFavoritesFoundView: View {
    var body: some View {
        VStack {
            Image("brave-icon")
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
        .background(Color(UIColor.secondaryBraveBackground))
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
        case .systemSmall, .systemExtraLarge:
            assertionFailure("widget family isn't supported")
            return 0
        @unknown default:
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
        if #available(iOS 15, *) {
            return redactionReasons.contains(.placeholder) || redactionReasons.contains(.privacy)
        } else {
            return redactionReasons.contains(.placeholder)
        }
    }
    
    var body: some View {
        LazyVGrid(columns: Array(repeating: .init(.flexible()), count: 4), spacing: 8) {
            ForEach((0..<itemsCount)) {
                if let favorite = entry.favorites[safe: $0], !placeholderOrPrivacyRedaction {
                    Link(destination: favorite.url, label: {
                        Group {
                            if let attributes = favorite.favicon, let image = attributes.image {
                                FaviconImage(image: image, contentMode: attributes.contentMode)
                                    .background(Color(attributes.backgroundColor ?? .clear))
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
                                .strokeBorder(Color(UIColor.braveSeparator).opacity(0.1), lineWidth: pixelLength)
                        )
                        .padding(widgetFamily == .systemMedium ? 4 : 0)
                    })
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
        FavoritesView(entry: .init(date: Date(), favorites: [
            // TODO: Fill with favorites.
        ]))
            .previewContext(WidgetPreviewContext(family: .systemMedium))
        FavoritesView(entry: .init(date: Date(), favorites: []))
            .previewContext(WidgetPreviewContext(family: .systemLarge))
    }
}
#endif
