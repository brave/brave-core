// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Foundation

/// Matches web search URLs produced by the search engines built into the OS (Safari, Spotlight,
/// Siri, etc.) across all regions, and extracts the search query from them.
struct SystemSearchURLMatcher {
  enum Engine: CaseIterable {
    case google
    case yahoo
    case yahooJP
    case bing
    case duckDuckGo
    case ecosia
    case baidu
    case sogou
    case so360
    case yandex

    /// Regular expression the URL host must fully match.
    fileprivate var hostPattern: String {
      switch self {
      case .google:
        return #"^(www\.)?google\.([a-z]{2,3}|co\.[a-z]{2}|com\.[a-z]{2})$"#
      case .yahoo:
        return #"^([a-z]{2}\.)?search\.yahoo\.com$"#
      case .yahooJP:
        return #"^search\.yahoo\.co\.jp$"#
      case .bing:
        return #"^(www\.|cn\.)?bing\.com$"#
      case .duckDuckGo:
        return #"^(www\.)?duckduckgo\.com$"#
      case .ecosia:
        return #"^(www\.)?ecosia\.org$"#
      case .baidu:
        return #"^(m\.|www\.)?baidu\.com$"#
      case .sogou:
        return #"^(m\.|www\.)?sogou\.com$"#
      case .so360:
        return #"^(m\.|www\.)?so\.com$"#
      case .yandex:
        return #"^(www\.)?yandex\.(ru|com|com\.tr|kz|by|ua)$"#
      }
    }

    /// Paths (without a trailing slash) that represent a search results page.
    fileprivate var paths: Set<String> {
      switch self {
      case .google, .yahoo, .yahooJP, .ecosia:
        return ["/search"]
      case .bing:
        return ["/search", "/images/search", "/videos/search", "/news/search"]
      case .duckDuckGo:
        return ["/"]
      case .baidu, .so360:
        return ["/s"]
      case .sogou:
        return ["/web/sl", "/web"]
      case .yandex:
        return ["/search", "/search/touch", "/search/pad"]
      }
    }

    /// Query parameter names that hold the search terms, in order of preference.
    fileprivate var queryParameterNames: [String] {
      switch self {
      case .google, .bing, .duckDuckGo, .ecosia, .so360:
        return ["q"]
      case .yahoo, .yahooJP:
        return ["p"]
      case .baidu:
        return ["word", "wd"]
      case .sogou:
        return ["keyword", "query"]
      case .yandex:
        return ["text"]
      }
    }

    /// Whether or not the given host belongs to this engine.
    func matchesHost(_ host: String) -> Bool {
      host.lowercased().range(of: hostPattern, options: .regularExpression) != nil
    }
  }

  struct Match: Equatable {
    let engine: Engine
    let query: String
  }

  /// Returns the engine whose search host matches the given URL, if any.
  static func engine(for url: URL) -> Engine? {
    guard let host = url.host, !host.isEmpty else { return nil }
    return Engine.allCases.first(where: { $0.matchesHost(host) })
  }

  /// Returns the engine and search query if the URL is a search results page of a known
  /// OS search engine.
  static func match(_ url: URL) -> Match? {
    guard let scheme = url.scheme?.lowercased(), scheme == "http" || scheme == "https",
      let engine = engine(for: url),
      let components = URLComponents(url: url, resolvingAgainstBaseURL: false)
    else {
      return nil
    }

    var path = components.percentEncodedPath
    while path.count > 1, path.hasSuffix("/") {
      path.removeLast()
    }
    if path.isEmpty {
      path = "/"
    }
    guard engine.paths.contains(path) else { return nil }

    let queryItems = components.percentEncodedQueryItems ?? []
    for name in engine.queryParameterNames {
      guard
        let value = queryItems.first(where: { $0.name == name })?.value,
        let query = value.replacingOccurrences(of: "+", with: " ").removingPercentEncoding,
        !query.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty
      else {
        continue
      }
      return Match(engine: engine, query: query)
    }
    return nil
  }
}

extension OpenSearchEngine {
  /// Whether or not this engine is the same search engine as the OS search engine that produced
  /// the given search URL.
  func isSameEngine(as systemEngine: SystemSearchURLMatcher.Engine, url: URL) -> Bool {
    guard let engineSearchURL = searchURLForQuery("test") else { return false }
    if SystemSearchURLMatcher.engine(for: engineSearchURL) == systemEngine {
      return true
    }
    if isCustomEngine, let engineDomain = engineSearchURL.baseDomain,
      let urlDomain = url.baseDomain
    {
      return engineDomain.caseInsensitiveCompare(urlDomain) == .orderedSame
    }
    return false
  }
}
