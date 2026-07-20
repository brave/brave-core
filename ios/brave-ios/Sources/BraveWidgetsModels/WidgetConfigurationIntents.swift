// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import AppIntents
import Foundation

public enum StatKind: Int, AppEnum, CaseIterable, Codable, Hashable, Sendable {
  case unknown = 0
  case adsBlocked
  case dataSaved
  case timeSaved

  public static var typeDisplayRepresentation: TypeDisplayRepresentation = "Stat Kind"

  public static var caseDisplayRepresentations: [StatKind: DisplayRepresentation] = [
    .unknown: "Unknown",
    .adsBlocked: "Ads Blocked",
    .dataSaved: "Est. Data Saved",
    .timeSaved: "Est. Time Saved",
  ]
}

public enum WidgetShortcut: Int, AppEnum, CaseIterable, Codable, Hashable, Sendable {
  case unknown = 0
  case newTab
  case newPrivateTab
  case bookmarks
  case history
  case downloads
  case playlist
  case search
  case wallet
  case scanQRCode
  case braveNews
  case braveLeo
  case askBrave

  public static var typeDisplayRepresentation: TypeDisplayRepresentation = "Widget Shortcut"

  public static var caseDisplayRepresentations: [WidgetShortcut: DisplayRepresentation] = [
    .unknown: "Unknown",
    .newTab: "New Tab",
    .newPrivateTab: "New Private Tab",
    .bookmarks: "Bookmarks",
    .history: "History",
    .downloads: "Downloads",
    .playlist: "Brave Playlist",
    .search: "Search",
    .wallet: "Brave Wallet",
    .scanQRCode: "Scan QR Code",
    .braveNews: "Brave News",
    .braveLeo: "Leo AI",
    .askBrave: "Ask Brave",
  ]
}

public struct StatsConfigurationIntent: WidgetConfigurationIntent {
  public static var title: LocalizedStringResource = "Stats Configuration"

  @Parameter(title: "Type", default: .adsBlocked)
  public var statKind: StatKind

  public init() {}
}

public struct ShortcutsConfigurationIntent: WidgetConfigurationIntent {
  public static var title: LocalizedStringResource = "Shortcuts Configuration"

  @Parameter(title: "First shortcut", default: .newTab)
  public var slot1: WidgetShortcut
  @Parameter(title: "Second shortcut", default: .newPrivateTab)
  public var slot2: WidgetShortcut
  @Parameter(title: "Third shortcut", default: .bookmarks)
  public var slot3: WidgetShortcut
  @Parameter(title: "Fourth shortcut", default: .search)
  public var slot4: WidgetShortcut

  public init() {}
}

public struct LockScreenShortcutConfigurationIntent: WidgetConfigurationIntent {
  public static var title: LocalizedStringResource = "Lock Screen Shortcut"

  @Parameter(title: "Shortcut", default: .newTab)
  public var shortcut: WidgetShortcut

  public init() {}
}

public struct FavoriteEntry: AppEntity, Hashable, Sendable {
  public static var typeDisplayRepresentation: TypeDisplayRepresentation = "Favorite"
  public static var defaultQuery = FavoriteEntryQuery()

  public let id: String
  public let title: String
  public let url: URL

  public init(url: URL, title: String?) {
    self.id = url.absoluteString
    self.title = title ?? url.absoluteString
    self.url = url
  }

  public var displayRepresentation: DisplayRepresentation {
    DisplayRepresentation(
      title: LocalizedStringResource(stringLiteral: title),
      subtitle: LocalizedStringResource(stringLiteral: url.absoluteString)
    )
  }
}

public struct FavoriteEntryQuery: EntityQuery {
  public init() {}

  public func entities(for identifiers: [FavoriteEntry.ID]) async throws -> [FavoriteEntry] {
    let favorites = await FavoritesWidgetData.loadWidgetData() ?? []
    return favorites
      .filter { identifiers.contains($0.url.absoluteString) }
      .map { FavoriteEntry(url: $0.url, title: $0.title) }
  }

  public func suggestedEntities() async throws -> [FavoriteEntry] {
    let favorites = await FavoritesWidgetData.loadWidgetData() ?? []
    return favorites.map { FavoriteEntry(url: $0.url, title: $0.title) }
  }
}

public struct LockScreenFavoriteConfigurationIntent: WidgetConfigurationIntent {
  public static var title: LocalizedStringResource = "Lock Screen Favorite"

  @Parameter(title: "Favorite")
  public var favorite: FavoriteEntry?

  public init() {}
}
