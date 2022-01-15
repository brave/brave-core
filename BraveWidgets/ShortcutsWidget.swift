// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import WidgetKit
import Shared
import BraveShared
import Intents

struct ShortcutsWidget: Widget {
    var body: some WidgetConfiguration {
        IntentConfiguration(kind: "ShortcutsWidget", intent: ShortcutsConfigurationIntent.self,
                            provider: ShortcutProvider()) { entry in
            ShortcutsView(slots: entry.shortcutSlots)
                .unredacted()
        }
        .configurationDisplayName(Strings.Widgets.shortcutsWidgetTitle)
        .description(Strings.Widgets.shortcutsWidgetDescription)
        .supportedFamilies([.systemMedium])
    }
}

private struct ShortcutEntry: TimelineEntry {
    var date: Date
    var shortcutSlots: [WidgetShortcut]
}

private struct ShortcutProvider: IntentTimelineProvider {
    typealias Intent = ShortcutsConfigurationIntent
    typealias Entry = ShortcutEntry
    func getSnapshot(for configuration: Intent, in context: Context,
                     completion: @escaping (ShortcutEntry) -> Void) {
        let entry = ShortcutEntry(
            date: Date(),
            shortcutSlots: [
                configuration.slot1,
                configuration.slot2,
                configuration.slot3
            ]
        )
        completion(entry)
    }
    
    func placeholder(in context: Context) -> ShortcutEntry {
        .init(date: Date(),
              shortcutSlots: context.isPreview ? [] : [.playlist, .newPrivateTab, .bookmarks])
    }
    
    func getTimeline(for configuration: Intent, in context: Context,
                     completion: @escaping (Timeline<ShortcutEntry>) -> Void) {
        let entry = ShortcutEntry(
            date: Date(),
            shortcutSlots: [
                configuration.slot1,
                configuration.slot2,
                configuration.slot3
            ]
        )
        completion(.init(entries: [entry], policy: .never))
    }
}

private struct ShortcutLink<Content: View>: View {
    var url: String
    var text: String
    var image: Content
    
    init(url: String, text: String, @ViewBuilder image: () -> Content) {
        self.url = url
        self.text = text
        self.image = image()
    }
    
    var body: some View {
        if let url = URL(string: url) {
            Link(destination: url, label: {
                VStack(spacing: 8) {
                    image
                        .imageScale(.large)
                        .font(Font.system(.body).bold())
                        .frame(height: 24)
                    Text(verbatim: text)
                        .font(.system(size: 13, weight: .medium))
                        .multilineTextAlignment(.center)
                }
                .padding(8)
                .foregroundColor(Color(UIColor.braveLabel))
                .frame(maxWidth: .infinity, maxHeight: .infinity)
                .background(
                    Color(UIColor.braveBackground)
                        .clipShape(ContainerRelativeShape())
                )
            })
        } else {
            EmptyView()
        }
    }
}

private extension WidgetShortcut {
    var displayString: String {
        switch self {
        case .unknown:
            assertionFailure()
            return ""
        case .newTab:
            return Strings.Widgets.shortcutsNewTabButton
        case .newPrivateTab:
            return Strings.Widgets.shortcutsPrivateTabButton
        // Reusing localized strings for few items here.
        case .bookmarks:
            return Strings.bookmarksMenuItem
        case .history:
            return Strings.historyMenuItem
        case .downloads:
            return Strings.downloadsMenuItem
        case .playlist:
            // We usually use `Brave Playlist` to describe this feature.
            // Here we try to be more concise and use 'Playlist' word only.
            return Strings.Widgets.shortcutsPlaylistButton
        @unknown default:
            assertionFailure()
            return ""
        }
    }
    
    var image: Image {
        switch self {
        case .unknown:
            assertionFailure()
            return Image(systemName: "xmark.octagon")
        case .newTab:
            return shortcutsImage(with: "brave.plus")
        case .newPrivateTab:
            return shortcutsImage(with: "brave.shades")
        case .bookmarks:
            return shortcutsImage(with: "menu_bookmarks")
        case .history:
            return shortcutsImage(with: "brave.history")
        case .downloads:
            return shortcutsImage(with: "brave.downloads")
        case .playlist:
            return shortcutsImage(with: "brave.playlist")
        @unknown default:
            assertionFailure()
            return Image(systemName: "xmark.octagon")
        }
    }
    
    private func shortcutsImage(with name: String) -> Image {
        let fallbackImage = Image(systemName: "xmark.octagon")
        
        guard let image = UIImage(named: name)?
                .applyingSymbolConfiguration(.init(font: .systemFont(ofSize: 20)))?
                .template else {
            return fallbackImage
        }
        
        return Image(uiImage: image)
    }
}

private struct ShortcutsView: View {
    var slots: [WidgetShortcut]
    
    var body: some View {
        VStack(spacing: 8) {
            // TODO: Would be nice to export handling this url to `BraveShared`.
            // Now it's hardcoded here and in `NavigationRouter`.
            if let url = URL(string: "brave://shortcut?path=0"),
                let image = UIImage(named: "brave-logo-no-bg-small") {
                Link(destination: url, label: {
                    Label(Strings.Widgets.shortcutsEnterURLButton, uiImage: image)
                        .foregroundColor(Color(UIColor.braveLabel))
                        .frame(maxWidth: .infinity)
                        .frame(height: 44)
                        .background(
                            Color(UIColor.braveBackground)
                                .clipShape(ContainerRelativeShape())
                        )
                })
            }
            HStack(spacing: 8) {
                ForEach(slots, id: \.self) { shortcut in
                    ShortcutLink(
                        url: "brave://shortcut?path=\(shortcut.rawValue)",
                        text: shortcut.displayString,
                        image: {
                            shortcut.image
                        })
                }
            }
            .frame(maxHeight: .infinity)
        }
        .padding(8)
        .background(Color(UIColor.secondaryBraveBackground))
    }
}

// MARK: - Previews

#if DEBUG
struct ShortcutsWidget_Previews: PreviewProvider {
    static var previews: some View {
        ShortcutsView(slots: [.newTab, .newPrivateTab, .bookmarks])
            .previewContext(WidgetPreviewContext(family: .systemMedium))
        ShortcutsView(slots: [.downloads, .history, .newPrivateTab])
            .previewContext(WidgetPreviewContext(family: .systemMedium))
        ShortcutsView(slots: [.newTab])
            .previewContext(WidgetPreviewContext(family: .systemMedium))
    }
}
#endif
