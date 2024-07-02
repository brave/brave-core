// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

/// This is a list of search engines available to the user at first launch.
/// For user controlled search engines class look for `SearchEngines.swift`
class InitialSearchEngines {
  /// Type of search engine available to the user.
  enum SearchEngineID: String, CaseIterable {
    case google, braveSearch, bing, duckduckgo, yandex, qwant, startpage, ecosia, naver, daum

    /// Open Search Reference  for default search Engines
    var openSearchReference: String {
      switch self {
      case .google: return "google.com"
      case .braveSearch: return "search.brave"
      case .bing: return "bing.com"
      case .duckduckgo: return "duckduckgo.com/opensearch"
      case .yandex: return "yandex.com/search"
      case .qwant: return "qwant.com/opensearch"
      case .startpage: return "startpage.com/en/opensearch"
      case .ecosia: return "ecosia.org/opensearch"
      case .naver: return "naver.com"
      case .daum: return "search.daum.net/OpenSearch"
      }
    }

    /// Due to legacy reasons, the search engine name may need to use an old value.
    /// This is caused because we use 'display name' as a preference key.
    var legacyName: String? {
      switch self {
      case .braveSearch: return "Brave Search beta"
      default: return nil
      }
    }

    func excludedFromOnboarding(for locale: Locale) -> Bool {
      switch self {
      case .braveSearch: return true
      // In general we want all engines to be pickable,
      // hence using default clause here instead of picking engines one by one
      default: return false
      }
    }
  }

  struct SearchEngine: Equatable, CustomStringConvertible {
    /// ID of the engine, this is also used to look up the xml file of given search engine if `customId` is not provided.
    let id: SearchEngineID
    /// Some search engines have regional variations which correspond to different xml files in `SearchPlugins` folder.
    /// If you provide this custom id, it will be used instead of `regular` id when accessing the open search xml file.
    var customId: String?
    /// This is used to determine If a seach engine is added already
    /// Added to espeically to prevent Default Search engines using Open Search Auto-Add
    var reference: String? {
      return id.openSearchReference
    }

    // Only `id` mattera when comparing search engines.
    // This is to prevent adding more than 2 engines of the same type.
    static func == (lhs: Self, rhs: Self) -> Bool {
      lhs.id == rhs.id
    }

    var description: String {
      var desc = id.rawValue
      if let customId = customId {
        desc += " with '\(customId)' custom id"
      }
      return desc
    }
  }

  private let locale: Locale
  /// List of available engines for given locale. This list is sorted by with priority and default engines at the top.
  var engines: [SearchEngine]

  /// Lists of engines available during onboarding.
  var onboardingEngines: [SearchEngine] {
    engines.filter { !$0.id.excludedFromOnboarding(for: locale) }
  }

  let braveSearchDefaultRegions = [
    "US", "CA", "GB", "FR", "DE", "AD", "AT", "ES", "MX", "BR", "AR", "IN", "IT",
  ]
  let yandexDefaultRegions = ["AM", "AZ", "BY", "KG", "KZ", "MD", "RU", "TJ", "TM", "TZ"]
  let ecosiaEnabledRegions = [
    "AT", "AU", "BE", "CA", "DK", "ES", "FI", "GR", "HU", "IT",
    "LU", "NO", "PT", "US", "GB", "FR", "DE", "NL", "CH", "SE", "IE",
  ]
  let naverDefaultRegions = ["KR"]
  let daumEnabledRegions = ["KR"]

  /// Sets what should be the default search engine for given locale.
  /// If the engine does not exist in `engines` list, it is added to it.
  private(set) var defaultSearchEngine: SearchEngineID {
    didSet {
      if !engines.contains(.init(id: defaultSearchEngine)) {
        // As a fallback we add the missing engine
        engines.append(.init(id: defaultSearchEngine))
      }
    }
  }

  /// Sets what should be the default priority engine for given locale.
  /// Priority engines show at the top of search engine onboarding as well as in search engines setting unless user changes search engines order.
  /// If the engine does not exist in `engines` list, it is added to it.
  private(set) var priorityEngine: SearchEngineID? {
    didSet {
      guard let engine = priorityEngine else { return }
      if !engines.contains(.init(id: engine)) {
        // As a fallback we add the missing engine
        engines.append(.init(id: engine))
      }
    }
  }

  public var isBraveSearchDefaultRegion: Bool {
    guard let regionID = locale.region?.identifier ?? Locale.current.region?.identifier else {
      return false
    }

    return braveSearchDefaultRegions.contains(regionID)
  }

  init(locale: Locale = .current) {
    self.locale = locale

    // Default order and available search engines, applies to all locales
    engines = [
      .init(id: .braveSearch),
      .init(id: .google),
      .init(id: .bing),
      .init(id: .duckduckgo),
      .init(id: .qwant),
      .init(id: .startpage),
    ]
    defaultSearchEngine = .google

    // Locale and region specific overrides can be modified here.
    // For conflicting rules priorities are as follows:
    // 1. Priority rules, put whatever rules you want there.
    // 2. Region specific rules, this should apply to all rules within a given region
    regionOverrides()
    priorityOverrides()

    // Initial engines should always be sorted so priority and default search engines are at the top,
    // remaining search engines are left in order they were added.
    sortEngines()
  }

  // MARK: - Locale overrides

  private func regionOverrides() {
    guard let region = locale.region?.identifier else { return }

    if yandexDefaultRegions.contains(region) {
      defaultSearchEngine = .yandex
    }

    if ecosiaEnabledRegions.contains(region) {
      replaceOrInsert(engineId: .ecosia, customId: nil)
    }

    if braveSearchDefaultRegions.contains(region) {
      defaultSearchEngine = .braveSearch
    }

    if naverDefaultRegions.contains(region) {
      defaultSearchEngine = .naver
    }

    if daumEnabledRegions.contains(region) {
      replaceOrInsert(engineId: .daum, customId: nil)
    }
  }

  private func priorityOverrides() {
    // No priority engines are live at the moment.
  }

  // MARK: - Helpers

  private func sortEngines() {
    engines =
      engines
      .sorted { e, _ in e.id == defaultSearchEngine }
      .sorted { e, _ in e.id == priorityEngine }
  }

  private func replaceOrInsert(engineId: SearchEngineID, customId: String?) {
    guard let engineIndex = engines.firstIndex(where: { $0.id == engineId }) else {
      engines.append(.init(id: engineId, customId: customId))
      return
    }

    engines[engineIndex] = .init(id: engineId, customId: customId)
  }
}
