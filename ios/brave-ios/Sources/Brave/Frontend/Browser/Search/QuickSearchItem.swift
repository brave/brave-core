// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

enum QuickSearchItemID: Hashable {
  case engine(String)
  case customEngine(String)
  case aiChat
  case braveVertical(String)

  var rawValue: String {
    switch self {
    case .engine(let id): return "engine:\(id)"
    case .customEngine(let id): return "custom:\(id)"
    case .aiChat: return "ai-chat"
    case .braveVertical(let id): return "vertical:\(id)"
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
