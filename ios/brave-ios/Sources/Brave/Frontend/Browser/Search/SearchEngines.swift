// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Foundation
import Growth
import Preferences
import Shared
import Storage
import UIKit
import os.log

private let customSearchEnginesFileName = "customEngines.plist"

// MARK: - SearchEngineError

enum SearchEngineError: Error {
  case duplicate
  case failedToSave
  case invalidQuery
  case missingInformation
  case invalidURL
}

// BRAVE TODO: Move to newer Preferences class(#259)
enum DefaultEngineType: String {
  case standard = "search.default.name"
  case privateMode = "search.defaultprivate.name"

  var option: Preferences.Option<String?> {
    switch self {
    case .standard: return Preferences.Search.defaultEngineName
    case .privateMode: return Preferences.Search.defaultPrivateEngineName
    }
  }
}

/// Manage a set of Open Search engines.
///
/// The search engines are ordered.  Individual search engines can be enabled and disabled.  The
/// first search engine is distinguished and labeled the "default" search engine; it can never be
/// disabled.  Search suggestions should always be sourced from the default search engine.
///
/// Two additional bits of information are maintained: whether the user should be shown "opt-in to
/// search suggestions" UI, and whether search suggestions are enabled.
///
/// Users can set standard tab default search engine and private tab search engine.
///
/// Consumers will almost always use `defaultEngine` if they want a single search engine, and
/// `quickSearchEngines()` if they want a list of enabled quick search engines (possibly empty,
/// since the default engine is never included in the list of enabled quick search engines, and
/// it is possible to disable every non-default quick search engine).
///
/// The search engines are backed by a write-through cache into a ProfilePrefs instance.  This class
/// is not thread-safe -- you should only access it on a single thread (usually, the main thread)!
public class SearchEngines {
  private let initialSearchEngines: InitialSearchEngines
  private let locale: Locale

  public init(locale: Locale = .current) {
    initialSearchEngines = InitialSearchEngines(locale: locale)
    self.locale = locale
    self.disabledEngineNames = getDisabledEngineNames()
  }

  public func searchEngineSetup() {
    let engine = initialSearchEngines.defaultSearchEngine
    setInitialDefaultEngine(engine.legacyName ?? engine.rawValue)
  }

  public func loadSearchEngines() async {
    await loadCustomEngines()
    orderedEngines = await getOrderedEngines()

    recordSearchEngineP3A()
    if let defaultEngine = defaultEngine(forType: .standard) {
      recordSearchEngineChangedP3A(from: defaultEngine)
    }
  }

  /// If no engine type is specified this method returns search engine for regular browsing.
  func defaultEngine(forType engineType: DefaultEngineType) -> OpenSearchEngine? {
    if let name = engineType.option.value,
      let defaultEngine = orderedEngines.first(where: {
        $0.engineID == name || $0.shortName == name
      })
    {
      return defaultEngine
    }

    let defaultEngineName = initialSearchEngines.defaultSearchEngine.rawValue

    let defaultEngine = orderedEngines.first(where: { $0.engineID == defaultEngineName })
    return defaultEngine ?? orderedEngines[0]
  }

  /// Initialize default engine and set order of remaining search engines.
  /// Call this method only at initialization(app launch or onboarding).
  /// For updating search engines use `updateDefaultEngine()` method.
  func setInitialDefaultEngine(_ engine: String) {
    // update engine
    DefaultEngineType.standard.option.value = engine
    DefaultEngineType.privateMode.option.value = engine

    let priorityEngine = initialSearchEngines.priorityEngine?.rawValue
    let defEngine = defaultEngine(forType: .standard)

    // Sort engines, priority engine at first place
    var newlyOrderedEngines =
      orderedEngines
      .filter { engine in engine.shortName != defEngine?.shortName }
      .sorted { e1, e2 in e1.shortName < e2.shortName }
      .sorted { e, _ in e.engineID == priorityEngine }

    if let defEngine {
      newlyOrderedEngines.insert(defEngine, at: 0)
    }
    orderedEngines = newlyOrderedEngines
  }

  /// Updates selected default engine, order of remaining search engines remains intact.
  func updateDefaultEngine(_ engine: String, forType type: DefaultEngineType) {
    let originalEngine = defaultEngine(forType: type)
    type.option.value = engine

    // The default engine is always enabled.
    guard let newDefaultEngine = defaultEngine(forType: type) else {
      return
    }

    enableEngine(newDefaultEngine)

    // When re-sorting engines only look at default search for standard browsing.
    if type == .standard {
      // Make sure we don't alter the private mode's default since it relies on order when its not set
      if Preferences.Search.defaultPrivateEngineName.value == nil,
        let firstEngine = orderedEngines.first
      {
        // So set the default engine for private mode to whatever the default was before we changed the standard
        updateDefaultEngine(firstEngine.shortName, forType: .privateMode)
      }
      // The default engine is always first in the list.
      var newlyOrderedEngines =
        orderedEngines.filter { engine in engine.shortName != newDefaultEngine.shortName
        }
      newlyOrderedEngines.insert(newDefaultEngine, at: 0)
      orderedEngines = newlyOrderedEngines
    }

    if type == .standard {
      recordSearchEngineP3A()
      if let originalEngine {
        recordSearchEngineChangedP3A(from: originalEngine)
      }
    }
  }

  func isEngineDefault(_ engine: OpenSearchEngine, type: DefaultEngineType) -> Bool {
    return defaultEngine(forType: type)?.shortName == engine.shortName
  }

  // The keys of this dictionary are used as a set.
  fileprivate var disabledEngineNames: [String: Bool]! {
    didSet {
      Preferences.Search.disabledEngines.value = Array(self.disabledEngineNames.keys)
    }
  }

  var orderedEngines: [OpenSearchEngine] = [] {
    didSet {
      Preferences.Search.orderedEngines.value = self.orderedEngines.map { $0.shortName }
    }
  }

  var quickSearchEngines: [OpenSearchEngine]! {
    return self.orderedEngines.filter({ (engine) in self.isEngineEnabled(engine) })
  }

  var shouldShowSearchSuggestionsOptIn: Bool {
    get { return Preferences.Search.shouldShowSuggestionsOptIn.value }
    set { Preferences.Search.shouldShowSuggestionsOptIn.value = newValue }
  }

  var shouldShowSearchSuggestions: Bool {
    get { return Preferences.Search.showSuggestions.value }
    set { Preferences.Search.showSuggestions.value = newValue }
  }

  var shouldShowRecentSearchesOptIn: Bool {
    get { return Preferences.Search.shouldShowRecentSearchesOptIn.value }
    set { Preferences.Search.shouldShowRecentSearchesOptIn.value = newValue }
  }

  var shouldShowRecentSearches: Bool {
    get { return Preferences.Search.shouldShowRecentSearches.value }
    set { Preferences.Search.shouldShowRecentSearches.value = newValue }
  }

  var shouldShowBrowserSuggestions: Bool {
    get { return Preferences.Search.showBrowserSuggestions.value }
    set { Preferences.Search.showBrowserSuggestions.value = newValue }
  }

  func isEngineEnabled(_ engine: OpenSearchEngine) -> Bool {
    return disabledEngineNames.index(forKey: engine.shortName) == nil
  }

  func enableEngine(_ engine: OpenSearchEngine) {
    disabledEngineNames.removeValue(forKey: engine.shortName)
  }

  func disableEngine(_ engine: OpenSearchEngine, type: DefaultEngineType) {
    if isEngineDefault(engine, type: type) {
      // Can't disable default engine.
      return
    }
    disabledEngineNames[engine.shortName] = true
  }

  func deleteCustomEngine(_ engine: OpenSearchEngine) async throws {
    // We can't delete a preinstalled engine
    if !engine.isCustomEngine {
      return
    }

    customEngines.remove(at: customEngines.firstIndex(of: engine)!)
    do {
      try await saveCustomEngines()
    } catch {
      throw SearchEngineError.failedToSave
    }

    orderedEngines = await getOrderedEngines()
  }

  /// Adds an engine to the front of the search engines list.
  func addSearchEngine(_ engine: OpenSearchEngine) async throws {
    guard orderedEngines.contains(where: { $0.searchTemplate != engine.searchTemplate }) else {
      throw SearchEngineError.duplicate
    }

    customEngines.append(engine)
    orderedEngines.insert(engine, at: 1)

    do {
      try await saveCustomEngines()
    } catch {
      throw SearchEngineError.failedToSave
    }
  }

  /// Edits an engine which was already in the list
  func editSearchEngine(_ engine: OpenSearchEngine) async throws {
    //    guard orderedEngines.contains(where: { $0.searchTemplate != engine.searchTemplate }) else {
    //      throw SearchEngineError.duplicate
    //    }
    //
    //    customEngines.append(engine)
    //    orderedEngines.insert(engine, at: 1)

    do {
      try await saveCustomEngines()
    } catch {
      throw SearchEngineError.failedToSave
    }
  }

  func queryForSearchURL(_ url: URL?, forType engineType: DefaultEngineType) -> String? {
    return defaultEngine(forType: engineType)?.queryForSearchURL(url)
  }

  fileprivate func getDisabledEngineNames() -> [String: Bool] {
    if let disabledEngineNames = Preferences.Search.disabledEngines.value {
      var disabledEngineDict = [String: Bool]()
      for engineName in disabledEngineNames {
        disabledEngineDict[engineName] = true
      }
      return disabledEngineDict
    } else {
      return [String: Bool]()
    }
  }

  fileprivate func customEngineFilePath() -> String {
    guard
      let profilePath = FileManager.default.containerURL(
        forSecurityApplicationGroupIdentifier: AppInfo.sharedContainerIdentifier
      )?.appendingPathComponent("profile.profile").path
    else {
      let documentsPath = NSSearchPathForDirectoriesInDomains(
        .documentDirectory,
        .userDomainMask,
        true
      )[0]
      return URL(fileURLWithPath: documentsPath).appendingPathComponent(customSearchEnginesFileName)
        .path
    }
    return URL(fileURLWithPath: profilePath).appendingPathComponent(customSearchEnginesFileName)
      .path
  }

  fileprivate var customEngines: [OpenSearchEngine] = []

  private func loadCustomEngines() async {
    do {
      let data = try Data(contentsOf: URL(fileURLWithPath: customEngineFilePath()))
      let unarchiver = try NSKeyedUnarchiver(forReadingFrom: data)
      unarchiver.requiresSecureCoding = true
      customEngines =
        unarchiver.decodeArrayOfObjects(
          ofClass: OpenSearchEngine.self,
          forKey: NSKeyedArchiveRootObjectKey
        ) ?? []
    } catch {
      Logger.module.error(
        "Failed to load custom search engines: \(error.localizedDescription, privacy: .public)"
      )
    }
  }

  fileprivate func saveCustomEngines() async throws {
    do {
      let data = try NSKeyedArchiver.archivedData(
        withRootObject: customEngines,
        requiringSecureCoding: true
      )
      let filePath = URL(fileURLWithPath: customEngineFilePath())
      try await AsyncFileManager.default.createDirectory(
        at: filePath.deletingLastPathComponent(),
        withIntermediateDirectories: true
      )
      try data.write(to: filePath)
    } catch {
      Logger.module.error(
        "Failed to save custom engines: \(error.localizedDescription, privacy: .public)"
      )
    }
  }

  /// Return all possible language identifiers in the order of most specific to least specific.
  /// For example, zh-Hans-CN will return [zh-Hans-CN, zh-CN, zh].
  class func possibilitiesForLanguageIdentifier(_ languageIdentifier: String) -> [String] {
    var possibilities: [String] = []
    let components = languageIdentifier.components(separatedBy: "-")
    possibilities.append(languageIdentifier)

    if components.count == 3, let first = components.first, let last = components.last {
      possibilities.append("\(first)-\(last)")
    }
    if components.count >= 2, let first = components.first {
      possibilities.append("\(first)")
    }
    return possibilities
  }

  /// Get all bundled (not custom) search engines, with the default search engine first,
  /// but the others in no particular order.
  class func getUnorderedBundledEngines(
    for selectedEngines: [String] = [],
    isOnboarding: Bool,
    locale: Locale
  ) async -> [OpenSearchEngine] {
    let parser = OpenSearchParser(pluginMode: true)

    guard let pluginDirectory = Bundle.module.resourceURL?.appendingPathComponent("SearchPlugins")
    else {
      assertionFailure("Search plugins not found. Check bundle")
      return []
    }

    let se = InitialSearchEngines(locale: locale)
    let engines = isOnboarding ? se.onboardingEngines : se.engines
    let engineIdentifiers: [(id: String, reference: String?)] = engines.map {
      (id: ($0.customId ?? $0.id.rawValue).lowercased(), reference: $0.reference)
    }
    assert(!engineIdentifiers.isEmpty, "No search engines")

    return await engineIdentifiers.map({
      (
        name: $0.id, path: pluginDirectory.appendingPathComponent("\($0.id).xml").path,
        reference: $0.reference
      )
    })
    .asyncFilter({ await AsyncFileManager.default.fileExists(atPath: $0.path) })
    .asyncCompactMap({ await parser.parse($0.path, engineID: $0.name, referenceURL: $0.reference) })
  }

  /// Get all known search engines, possibly as ordered by the user.
  fileprivate func getOrderedEngines() async -> [OpenSearchEngine] {
    let selectedSearchEngines = [
      Preferences.Search.defaultEngineName, Preferences.Search.defaultPrivateEngineName,
    ].compactMap { $0.value }
    let unorderedEngines =
      await SearchEngines.getUnorderedBundledEngines(
        for: selectedSearchEngines,
        isOnboarding: false,
        locale: locale
      ) + customEngines

    // might not work to change the default.
    guard let orderedEngineNames = Preferences.Search.orderedEngines.value else {
      // We haven't persisted the engine order, so return whatever order we got from disk.
      return unorderedEngines
    }

    // We have a persisted order of engines, so try to use that order.
    // We may have found engines that weren't persisted in the ordered list
    // (if the user changed locales or added a new engine); these engines
    // will be appended to the end of the list.
    return unorderedEngines.sorted { engine1, engine2 in
      let index1 = orderedEngineNames.firstIndex(of: engine1.shortName)
      let index2 = orderedEngineNames.firstIndex(of: engine2.shortName)

      if index1 == nil && index2 == nil {
        return engine1.shortName < engine2.shortName
      }

      // nil < N for all non-nil values of N.
      if index1 == nil || index2 == nil {
        return index1 ?? -1 > index2 ?? -1
      }

      return index1! < index2!
    }
  }

  // MARK: - P3A

  private enum P3ASearchEngineID: Int, CaseIterable, Codable {
    case other = 0
    case google = 1
    case duckduckgo = 2
    case startpage = 3
    case bing = 4
    case qwant = 5
    case yandex = 6
    case ecosia = 7
    case braveSearch = 8
    case naver = 9
    case daum = 10

    init(engine: OpenSearchEngine) {
      guard
        let defaultEngineID = InitialSearchEngines.SearchEngineID.allCases.first(where: {
          $0.openSearchReference == engine.referenceURL
        })
      else {
        self = .other
        return
      }
      switch defaultEngineID {
      case .google: self = .google
      case .duckduckgo: self = .duckduckgo
      case .startpage: self = .startpage
      case .bing: self = .bing
      case .qwant: self = .qwant
      case .yandex: self = .yandex
      case .ecosia: self = .ecosia
      case .braveSearch: self = .braveSearch
      case .naver: self = .naver
      case .daum: self = .daum
      }
    }
  }

  private func recordSearchEngineP3A() {
    guard let engine = defaultEngine(forType: .standard) else {
      return
    }
    let answer = P3ASearchEngineID(engine: engine)
    // Q20 Which is your currently selected search engine
    UmaHistogramEnumeration("Brave.Search.DefaultEngine.4", sample: answer)
  }

  private func recordSearchEngineChangedP3A(from previousEngine: OpenSearchEngine) {
    guard let engine = defaultEngine(forType: .standard) else {
      return
    }
    enum Answer: Int, CaseIterable {
      case noChange = 0
      case braveToGoogle = 1
      case braveToDuckDuckGo = 2
      case braveToOther = 3
      case googleToBrave = 4
      case duckDuckGoToBrave = 5
      case otherToBrave = 6
      case otherToOther = 7
    }
    struct Change: Codable {
      var from: P3ASearchEngineID
      var to: P3ASearchEngineID
    }
    let from: P3ASearchEngineID = .init(engine: previousEngine)
    let to: P3ASearchEngineID = .init(engine: engine)

    var storage = P3ATimedStorage<Change>(name: "search-engine-change", lifetimeInDays: 7)
    if from != to {
      storage.append(value: .init(from: from, to: to))
    }

    let answer: Answer = {
      guard let firstEngineSwitch = storage.records.first?.value.from, firstEngineSwitch != to
      else {
        return .noChange
      }
      switch (firstEngineSwitch, to) {
      case (.braveSearch, .google): return .braveToGoogle
      case (.braveSearch, .duckduckgo): return .braveToDuckDuckGo
      case (.braveSearch, _): return .braveToOther
      case (.google, .braveSearch): return .googleToBrave
      case (.duckduckgo, .braveSearch): return .duckDuckGoToBrave
      case (_, .braveSearch): return .otherToBrave
      default: return .otherToOther
      }
    }()
    UmaHistogramEnumeration("Brave.Search.SwitchEngine", sample: answer)
  }
}
