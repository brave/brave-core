// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Shared
import Strings
import SwiftUI

enum QuickSearchItemID: Hashable {
  case engine(String)
  case customEngine(String)
  case aiChat
  case braveVertical(String)

  private enum Prefix {
    static let engine = "engine:"
    static let customEngine = "custom:"
    static let braveVertical = "vertical:"
    static let aiChat = "ai-chat"
  }

  var rawValue: String {
    switch self {
    case .engine(let id): return Prefix.engine + id
    case .customEngine(let id): return Prefix.customEngine + id
    case .aiChat: return Prefix.aiChat
    case .braveVertical(let id): return Prefix.braveVertical + id
    }
  }

  init(engine: OpenSearchEngine) {
    if engine.isCustomEngine {
      self = .customEngine(engine.searchTemplate)
    } else if let id = engine.engineID, !id.isEmpty {
      self = .engine(id.lowercased())
    } else {
      assertionFailure("bundled engine without engineID")
      self = .customEngine(engine.searchTemplate)
    }
  }
}

extension QuickSearchItemID {
  /// The bundled Brave Search engine, which the Brave Search verticals are introduced behind.
  static let braveSearchEngine = QuickSearchItemID.engine(
    InitialSearchEngines.SearchEngineID.braveSearch.rawValue.lowercased()
  )

  var isBraveVertical: Bool {
    if case .braveVertical = self {
      return true
    }
    return false
  }

  init?(rawValue: String) {
    func value(after prefix: String) -> String? {
      guard rawValue.hasPrefix(prefix) else { return nil }
      let value = String(rawValue.dropFirst(prefix.count))
      return value.isEmpty ? nil : value
    }

    if rawValue == Prefix.aiChat {
      self = .aiChat
    } else if let id = value(after: Prefix.engine) {
      self = .engine(id.lowercased())
    } else if let id = value(after: Prefix.customEngine) {
      self = .customEngine(id)
    } else if let id = value(after: Prefix.braveVertical) {
      self = .braveVertical(id)
    } else {
      return nil
    }
  }
}

struct BraveSearchVertical: Identifiable {
  let id: String
  let displayName: String
  let iconName: String
  /// `%s` is the query insertion point, following the custom engine convention. `source=ios` is
  /// what lets widget search attribution rewrite the URL, same as `bravesearch.xml` declares.
  let searchTemplate: String

  var icon: Image {
    Image(braveSystemName: iconName)
  }

  func searchURL(for query: String, isWidgetSearchAttribution: Bool = false) -> URL? {
    guard
      let escapedQuery = query.addingPercentEncoding(withAllowedCharacters: .searchTermsAllowed),
      let url = URL(string: searchTemplate.replacingOccurrences(of: "%s", with: escapedQuery))
    else { return nil }

    return isWidgetSearchAttribution
      ? OpenSearchEngine.applyWidgetSearchAttribution(to: url) : url
  }

  enum Kind: CaseIterable {
    case images
    case videos
    case news
    case maps
    case goggles
  }

  init(kind: BraveSearchVertical.Kind) {
    switch kind {
    case .images:
      self.id = "brave-search-images"
      self.displayName = Strings.braveImagesSearchTitle
      self.iconName = "leo.image"
      self.searchTemplate = "https://search.brave.com/images?q=%s&source=ios"
    case .videos:
      self.id = "brave-search-video"
      self.displayName = Strings.braveVideosSearchTitle
      self.iconName = "leo.media.player"
      self.searchTemplate = "https://search.brave.com/videos?q=%s&source=ios"
    case .news:
      self.id = "brave-search-news"
      self.displayName = Strings.braveNewsSearchTitle
      self.iconName = "leo.product.brave-news"
      self.searchTemplate = "https://search.brave.com/news?q=%s&source=ios"
    case .maps:
      self.id = "brave-search-maps"
      self.displayName = Strings.braveMapsSearchTitle
      self.iconName = "leo.location.on"
      self.searchTemplate = "https://search.brave.com/maps?q=%s&source=ios"
    case .goggles:
      self.id = "brave-search-goggles"
      self.displayName = Strings.braveGogglesSearchTitle
      self.iconName = "leo.search.goggles"
      self.searchTemplate = "https://search.brave.com/goggles?q=%s&source=ios"
    }
  }
}

struct QuickSearchItem: Identifiable {

  enum Source {
    case engine(OpenSearchEngine)
    case aiChat
    case braveVertical(BraveSearchVertical)
  }

  let source: Source

  var id: QuickSearchItemID {
    switch source {
    case .engine(let openSearchEngine):
      return QuickSearchItemID(engine: openSearchEngine)
    case .aiChat:
      return .aiChat
    case .braveVertical(let vertical):
      return .braveVertical(vertical.id)
    }
  }

  var accessibilityTitle: String {
    switch source {
    case .engine(let openSearchEngine):
      return String(
        format: Strings.searchEngineFormatText,
        openSearchEngine.displayName
      )
    case .aiChat:
      return String(
        format: Strings.searchEngineFormatText,
        Strings.quickSearchAskLeoAccessibility
      )
    case .braveVertical(let vertical):
      return String(
        format: Strings.searchEngineFormatText,
        vertical.displayName
      )
    }
  }

  var icon: Image {
    switch source {
    case .engine(let openSearchEngine):
      return Image(uiImage: openSearchEngine.image)
    case .aiChat:
      return Image(braveSystemName: "leo.product.brave-leo")
    case .braveVertical(let vertical):
      return vertical.icon
    }
  }
}
