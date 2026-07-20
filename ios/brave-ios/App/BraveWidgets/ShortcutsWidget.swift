// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import AppIntents
import BraveShared
import BraveWidgetsModels
import Intents
import Strings
import SwiftUI
import WidgetKit

struct ShortcutsWidget: Widget {
  var body: some WidgetConfiguration {
    AppIntentConfiguration(
      kind: "ShortcutsWidget",
      intent: ShortcutsConfigurationIntent.self,
      provider: ShortcutProvider()
    ) { entry in
      ShortcutsView(slots: entry.shortcutSlots)
        .unredacted()
    }
    .configurationDisplayName(Strings.Widgets.shortcutsWidgetTitle)
    .description(Strings.Widgets.shortcutsWidgetDescription)
    .supportedFamilies([.systemMedium])
    .contentMarginsDisabled()
  }
}

private struct ShortcutEntry: TimelineEntry {
  var date: Date
  var shortcutSlots: [WidgetShortcut]
}

private struct ShortcutProvider: AppIntentTimelineProvider {
  typealias Intent = ShortcutsConfigurationIntent
  typealias Entry = ShortcutEntry

  private func shortcuts(for configuration: Intent) async -> [WidgetShortcut] {
    let disabledShortcuts = await DisabledShortcutsWidgetData.loadDisabledShortcuts()
    return [
      configuration.slot1,
      configuration.slot2,
      configuration.slot3,
      configuration.slot4,
    ].filter { !disabledShortcuts.contains($0) }
  }

  func snapshot(for configuration: Intent, in context: Context) async -> ShortcutEntry {
    ShortcutEntry(date: Date(), shortcutSlots: await shortcuts(for: configuration))
  }

  func placeholder(in context: Context) -> ShortcutEntry {
    .init(
      date: Date(),
      shortcutSlots: context.isPreview ? [] : [.playlist, .newPrivateTab, .bookmarks]
    )
  }

  func timeline(for configuration: Intent, in context: Context) async -> Timeline<ShortcutEntry> {
    let entry = ShortcutEntry(date: Date(), shortcutSlots: await shortcuts(for: configuration))
    return .init(entries: [entry], policy: .never)
  }
}

private struct ShortcutLink<Content: View>: View {
  @Environment(\.widgetRenderingMode) private var renderingMode

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
      Link(
        destination: url,
        label: {
          VStack(spacing: 8) {
            image
              .imageScale(.large)
              .font(.system(size: 20))
              .frame(height: 24)
            Text(verbatim: text)
              .font(.system(size: 10, weight: .medium))
              .multilineTextAlignment(.center)
          }
          .padding(8)
          .foregroundColor(Color(braveSystemName: .textPrimary))
          .frame(maxWidth: .infinity, maxHeight: .infinity)
          .background(
            renderingMode == .accented
              ? Color.white.opacity(0.1) : Color(UIColor(braveSystemName: .containerBackground)),
            in: .containerRelative
          )
        }
      )
    } else {
      EmptyView()
    }
  }
}

extension WidgetShortcut {
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
      return Strings.Widgets.bookmarksMenuItem
    case .history:
      return Strings.Widgets.historyMenuItem
    case .downloads:
      return Strings.Widgets.downloadsMenuItem
    case .playlist:
      // We usually use `Brave Playlist` to describe this feature.
      // Here we try to be more concise and use 'Playlist' word only.
      return Strings.Widgets.shortcutsPlaylistButton
    case .search:
      return Strings.Widgets.searchShortcutTitle
    case .wallet:
      return Strings.Widgets.walletShortcutTitle
    case .scanQRCode:
      return Strings.Widgets.QRCode
    case .braveNews:
      return Strings.Widgets.braveNews
    case .braveLeo:
      return Strings.Widgets.braveLeo
    case .askBrave:
      return Strings.Widgets.askBrave
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
      return Image(braveSystemName: "leo.browser.mobile-tab-new")
    case .newPrivateTab:
      return Image(braveSystemName: "leo.product.private-window")
    case .bookmarks:
      return Image(braveSystemName: "leo.product.bookmarks")
    case .history:
      return Image(braveSystemName: "leo.history")
    case .downloads:
      return Image(braveSystemName: "leo.download")
    case .playlist:
      return Image(braveSystemName: "leo.product.playlist")
    case .search:
      return Image(braveSystemName: "leo.search")
    case .wallet:
      return Image(braveSystemName: "leo.product.brave-wallet")
    case .scanQRCode:
      return Image(braveSystemName: "leo.qr.code")
    case .braveNews:
      return Image(braveSystemName: "leo.product.brave-news")
    case .braveLeo, .askBrave:
      return Image(braveSystemName: "leo.product.brave-leo")
    @unknown default:
      assertionFailure()
      return Image(systemName: "xmark.octagon")
    }
  }
}

private struct ShortcutsView: View {
  @Environment(\.widgetRenderingMode) private var renderingMode

  var slots: [WidgetShortcut]

  var body: some View {
    VStack(spacing: 8) {
      // TODO: Would be nice to export handling this url to `BraveShared`.
      // Now it's hardcoded here and in `NavigationRouter`.
      if let url = URL(string: "\(AppURLScheme.appURLScheme)://shortcut?path=0") {
        Link(
          destination: url,
          label: {
            Label {
              Text(Strings.Widgets.shortcutsEnterURLButton)
            } icon: {
              Image("brave-logo-no-bg-small")
                .widgetAccentedRenderingModeFullColor()
            }
            .foregroundColor(Color(braveSystemName: .textPrimary))
            .frame(maxWidth: .infinity)
            .frame(height: 44)
            .background(
              renderingMode == .accented
                ? Color.white.opacity(0.1) : Color(UIColor(braveSystemName: .containerBackground)),
              in: .containerRelative
            )
          }
        )
      }
      HStack(spacing: 8) {
        if slots.isEmpty {
          Text(Strings.Widgets.shortcutsEmptyState)
            .padding(8)
            .foregroundColor(
              renderingMode == .accented
                ? Color.white.opacity(0.7) : Color(braveSystemName: .textDisabled)
            )
            .frame(maxWidth: .infinity, maxHeight: .infinity)
            .overlay {
              ContainerRelativeShape()
                .stroke(
                  renderingMode == .accented
                    ? Color.white.opacity(0.2) : Color(braveSystemName: .dividerSubtle),
                  style: .init(lineWidth: 1)
                )
            }
        } else {
          ForEach(slots, id: \.self) { shortcut in
            ShortcutLink(
              url: "\(AppURLScheme.appURLScheme)://shortcut?path=\(shortcut.rawValue)",
              text: shortcut.displayString,
              image: {
                shortcut.image
              }
            )
          }
        }
      }
      .frame(maxHeight: .infinity)
    }
    .padding(8)
    .widgetBackground { Color(UIColor(braveSystemName: .containerHighlight)) }
  }
}

// MARK: - Previews

#if DEBUG

#Preview(
  as: .systemMedium,
  widget: {
    ShortcutsWidget()
  },
  timeline: {
    ShortcutEntry(date: .now, shortcutSlots: [.newTab, .newPrivateTab, .bookmarks])
  }
)

#Preview(
  as: .systemMedium,
  widget: {
    ShortcutsWidget()
  },
  timeline: {
    ShortcutEntry(date: .now, shortcutSlots: [])
  }
)

#endif
