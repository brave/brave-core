// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

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
  let accessibilityTitle: String
  let icon: Image
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
        vertical.accessibilityTitle
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
