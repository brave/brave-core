// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShields
import Data
import Foundation
import Preferences
import Shared
import WebKit
import os.log

/// A class that aids in the managment of rule lists on the rule store.
@MainActor class ContentBlockerManager {
  /// Logger to use for debugging.
  nonisolated static let log = Logger(
    subsystem: Bundle.main.bundleIdentifier!,
    category: "adblock"
  )

  static let signpost = OSSignposter(logger: log)
  private static let maxContentBlockerSize = 150_000

  enum CompileError: Error {
    case noRuleListReturned
    case invalidJSONArray
  }

  /// These are the adblocking level that a particular BlocklistType can support
  enum BlockingMode: CaseIterable {
    /// This is a general version that is supported on both standard and aggressive mode
    case general
    /// This indicates a less aggressive (or general) blocking version of the content blocker.
    ///
    /// In this version we will not block 1st party ad content.
    /// We will apped a rule that specifies that 1st party content should be ignored.
    case standard
    /// This indicates a more aggressive blocking version of the content blocker.
    ///
    /// In this version we will block 1st party ad content.
    /// We will not append a rule that specifies that 1st party content should be ignored.
    case aggressive
  }

  public enum GenericBlocklistType: Hashable, CaseIterable {
    case blockAds
    case blockCookies
    case blockTrackers
    case upgradeMixedContent

    func mode(isAggressiveMode: Bool) -> BlockingMode {
      switch self {
      case .blockAds:
        if isAggressiveMode {
          return .aggressive
        } else {
          return .standard
        }
      case .blockCookies, .blockTrackers, .upgradeMixedContent:
        return .general
      }
    }

    var bundledFileName: String {
      switch self {
      case .blockAds: return "block-ads"
      case .blockCookies: return "block-cookies"
      case .blockTrackers: return "block-trackers"
      case .upgradeMixedContent: return "mixed-content-upgrade"
      }
    }
  }

  enum CompileResult {
    case empty
    case success(WKContentRuleList)
    case failure(Error)

    func get() throws -> WKContentRuleList? {
      switch self {
      case .empty: return nil
      case .success(let ruleList): return ruleList
      case .failure(let error): throw error
      }
    }
  }

  /// An object representing the type of block list
  public enum BlocklistType: Hashable, CustomDebugStringConvertible {
    fileprivate static let genericPrifix = "stored-type"
    fileprivate static let filterListPrefix = "filter-list"
    fileprivate static let filterListURLPrefix = "filter-list-url"

    /// These are all types that are non-configurable by the user
    /// and don't need additional stored or fetched catalogues to get a complete list.
    static var allStaticTypes: Set<BlocklistType> {
      return Set(ContentBlockerManager.GenericBlocklistType.allCases.map { .generic($0) })
    }

    case generic(GenericBlocklistType)
    case filterList(componentId: String, isAlwaysAggressive: Bool)
    case filterListURL(uuid: String)
    case filterListText

    private var identifier: String {
      switch self {
      case .generic(let type):
        return [Self.genericPrifix, type.bundledFileName].joined(separator: "-")
      case .filterList(let componentId, _):
        return [Self.filterListPrefix, componentId].joined(separator: "-")
      case .filterListURL(let uuid):
        return [Self.filterListURLPrefix, uuid].joined(separator: "-")
      case .filterListText:
        return "filter-list-text"
      }
    }

    func mode(isAggressiveMode: Bool) -> BlockingMode {
      switch self {
      case .filterListURL, .filterListText:
        return .general
      case .filterList(_, let isAlwaysAggressive):
        if isAlwaysAggressive || isAggressiveMode {
          return .aggressive
        } else {
          return .standard
        }
      case .generic(let genericType):
        return genericType.mode(isAggressiveMode: isAggressiveMode)
      }
    }

    var allowedModes: [BlockingMode] {
      var allowedModes: Set<BlockingMode> = []
      allowedModes.insert(mode(isAggressiveMode: true))
      allowedModes.insert(mode(isAggressiveMode: false))
      return BlockingMode.allCases.filter({ allowedModes.contains($0) })
    }

    func makeIdentifier(for mode: BlockingMode) -> String {
      switch mode {
      case .general:
        return identifier
      case .aggressive:
        return [self.identifier, "aggressive"].joined(separator: "-")
      case .standard:
        return [self.identifier, "standard"].joined(separator: "-")
      }
    }

    public var debugDescription: String {
      return identifier
    }
  }

  public static var shared = ContentBlockerManager()
  /// The store in which these rule lists should be compiled
  let ruleStore: WKContentRuleListStore
  /// We cached the rule lists so that we can return them quicker if we need to
  private var cachedRuleLists: [String: CompileResult]
  /// The available known versions of the compiled block lists
  private let versions: Preferences.Option<[String: String]>

  init(
    ruleStore: WKContentRuleListStore = .default(),
    container: UserDefaults = Preferences.defaultContainer
  ) {
    self.ruleStore = ruleStore
    self.cachedRuleLists = [:]
    self.versions = Preferences.Option(
      key: "content-blocker.versions",
      default: [:],
      container: container
    )
  }

  /// Remove all rule lists minus the given valid types.
  /// Should be used only as a cleanup once during launch to get rid of unecessary/old data.
  /// This is mostly for custom filter lists a user may have added.
  public func cleaupInvalidRuleLists(validTypes: Set<BlocklistType>) async {
    let signpostID = Self.signpost.makeSignpostID()
    let state = Self.signpost.beginInterval("cleaupInvalidRuleLists", id: signpostID)
    let availableIdentifiers = await ruleStore.availableIdentifiers() ?? []

    await availableIdentifiers.asyncForEach { identifier in
      guard
        !validTypes.contains(where: { type in
          type.allowedModes.contains(where: { type.makeIdentifier(for: $0) == identifier })
        })
      else { return }

      self.versions.value.removeValue(forKey: identifier)
      // Only allow certain prefixed identifiers to be removed so as not to remove something apple adds
      let prefixes = [
        BlocklistType.genericPrifix, BlocklistType.filterListPrefix,
        BlocklistType.filterListURLPrefix,
      ]
      guard prefixes.contains(where: { identifier.hasPrefix($0) }) else { return }

      do {
        try await self.removeRuleList(forIdentifier: identifier, force: true)
      } catch {
        assertionFailure()
      }
    }

    Self.signpost.endInterval("cleaupInvalidRuleLists", state)
  }

  /// Test the rules and find the broken rules, their line numbers and associated errors
  public func testRules(
    forFilterSet filterSet: String
  ) async -> (rule: String, line: Int, error: Error)? {
    let rules = filterSet.components(separatedBy: .newlines)
    return await testRulesBinarySearch(rules: rules, range: 0..<rules.count)
  }

  /// Test the rules within a range and find the broken rules, their line numbers and associated errors
  private func testRulesBinarySearch(
    rules: [String],
    range: Range<Int>
  ) async -> (rule: String, line: Int, error: Error)? {
    let rangedRules = rules[range]
    guard rangedRules.count > 0 else { return nil }

    do {
      // 1. Test engine (we don't care about the results)
      _ = try AdblockEngine(rules: rangedRules.joined(separator: "\n"))

      // 2. Test content blockers
      let results = try AdblockEngine.contentBlockerRules(
        fromFilterSet: rangedRules.joined(separator: "\n")
      )

      let decodedRuleList = try decode(encodedContentRuleList: results.rulesJSON)

      if decodedRuleList.count > 0 {
        try await ruleStore.compileContentRuleList(
          forIdentifier: "test-identifier",
          encodedContentRuleList: results.rulesJSON
        )

        try await ruleStore.removeContentRuleList(forIdentifier: "test-identifier")
      }

      return nil
    } catch {
      if rangedRules.count == 1 {
        // Found the culprit line
        return (rules[range.lowerBound], range.lowerBound, error)
      }

      let middle = range.count / 2 + range.lowerBound

      // Test left
      if middle > range.lowerBound,
        let failure = await testRulesBinarySearch(
          rules: rules,
          range: range.lowerBound..<middle
        )
      {
        return failure
      }

      // Test right
      if middle < range.upperBound,
        let failure = await testRulesBinarySearch(
          rules: rules,
          range: middle..<range.upperBound
        )
      {
        return failure
      }

      return nil
    }
  }

  /// Compile the rule list found in the given local URL using the specified modes
  func compileRuleList(
    at localFileURL: URL,
    for blocklistType: BlocklistType,
    version: String,
    modes: [BlockingMode]
  ) async {
    guard !modes.isEmpty else { return }

    let result: ContentBlockingRulesResult
    let signpostID = Self.signpost.makeSignpostID()
    let state = Self.signpost.beginInterval(
      "convertRules",
      id: signpostID,
      "\(blocklistType.debugDescription) v\(version)"
    )

    do {
      do {
        let filterSet = try String(contentsOf: localFileURL)
        result = try AdblockEngine.contentBlockerRules(fromFilterSet: filterSet)
        Self.signpost.endInterval("convertRules", state)
      } catch {
        Self.signpost.endInterval("convertRules", state, "\(error.localizedDescription)")
        throw error
      }

      try await compile(
        encodedContentRuleList: result.rulesJSON,
        for: blocklistType,
        version: version,
        modes: modes
      )
    } catch {
      ContentBlockerManager.log.error(
        "Failed to compile rule list for `\(blocklistType.debugDescription)`"
      )
    }
  }

  /// Compile the given resource and store it in cache for the given blocklist type and specified modes
  func compile(
    encodedContentRuleList: String,
    for type: BlocklistType,
    version: String,
    modes: [BlockingMode]
  ) async throws {
    guard !modes.isEmpty else { return }
    var foundError: Error?
    let ruleList = try decode(encodedContentRuleList: encodedContentRuleList)

    guard !ruleList.isEmpty else {
      for mode in modes {
        self.cachedRuleLists[type.makeIdentifier(for: mode)] = .empty
      }

      ContentBlockerManager.log.debug(
        "Empty filter set for `\(type.debugDescription)` v\(version)"
      )
      return
    }

    for mode in modes {
      let identifier = type.makeIdentifier(for: mode)

      do {
        let moddedRuleList = try await self.modify(
          ruleList: ruleList,
          for: mode
        )
        let ruleList = try await compile(
          encodedContentRuleList: moddedRuleList ?? encodedContentRuleList,
          for: type,
          version: version,
          mode: mode
        )

        self.cachedRuleLists[identifier] = .success(ruleList)
        Self.log.debug("Compiled rule list for `\(identifier)` v\(version)")
      } catch {
        self.cachedRuleLists[identifier] = .failure(error)
        Self.log.debug(
          "Failed to compile rule list for `\(identifier)` v\(version): \(String(describing: error))"
        )
        foundError = error
      }

      self.versions.value[identifier] = version
    }

    if let error = foundError {
      throw error
    }
  }

  private func modify(
    ruleList: [[String: Any?]],
    for mode: BlockingMode
  ) async throws -> String? {
    return try await Task.detached {
      switch mode {
      case .aggressive, .general:
        // Aggressive mode and general mode has no modification to the rules
        let modifiedData = try JSONSerialization.data(withJSONObject: ruleList)
        return String(bytes: modifiedData, encoding: .utf8)

      case .standard:
        var ruleList = ruleList
        let maxContentBlockerSize = await Self.maxContentBlockerSize
        // We need to make sure we are not going over the limit
        // So we make space for the added rule
        if ruleList.count >= (maxContentBlockerSize) {
          ruleList = Array(ruleList[..<(maxContentBlockerSize - 1)])
        }

        ruleList.append([
          "action": ["type": "ignore-previous-rules"],
          "trigger": [
            "url-filter": ".*",
            "load-type": ["first-party"],
          ] as [String: Any?],
        ])

        let modifiedData = try JSONSerialization.data(withJSONObject: ruleList)
        return String(bytes: modifiedData, encoding: .utf8)
      }
    }.value
  }

  /// Compile the given resource and store it in cache for the given blocklist type
  private func compile(
    encodedContentRuleList: String,
    for type: BlocklistType,
    version: String,
    mode: BlockingMode
  ) async throws -> WKContentRuleList {
    let identifier = type.makeIdentifier(for: mode)
    let signpostID = Self.signpost.makeSignpostID()
    let state = Self.signpost.beginInterval(
      "compileRuleList",
      id: signpostID,
      "`\(identifier)` v\(version)"
    )

    do {
      let ruleList = try await ruleStore.compileContentRuleList(
        forIdentifier: identifier,
        encodedContentRuleList: encodedContentRuleList
      )
      guard let ruleList = ruleList else { throw CompileError.noRuleListReturned }
      Self.signpost.endInterval("compileRuleList", state)
      return ruleList
    } catch {
      Self.signpost.endInterval("compileRuleList", state, "\(error.localizedDescription)")
      throw error
    }
  }

  /// Return all the modes that need to be compiled for the given type
  func missingModes(for type: BlocklistType, version: String) async -> [BlockingMode] {
    return await type.allowedModes.asyncFilter { mode in
      let identifier = type.makeIdentifier(for: mode)

      // If the file wasn't modified, make sure we have something compiled.
      // We should, but this can be false during upgrades if the identifier changed for some reason.
      if await hasRuleList(for: type, mode: mode) {
        if let existingVersion = versions.value[identifier], existingVersion < version {
          return true
        } else {
          ContentBlockerManager.log.debug(
            "Rule list already compiled for `\(type.makeIdentifier(for: mode))` v\(version)"
          )

          return false
        }
      } else {
        return true
      }
    }
  }

  /// Check if a rule list is compiled for this type
  func hasRuleList(for type: BlocklistType, mode: BlockingMode) async -> Bool {
    do {
      return try await ruleList(for: type, mode: mode) != nil
    } catch {
      return false
    }
  }

  /// Remove the rule list for the blocklist type
  public func removeRuleLists(for type: BlocklistType, force: Bool = false) async throws {
    for mode in type.allowedModes {
      try await removeRuleList(forIdentifier: type.makeIdentifier(for: mode), force: force)
    }
  }

  /// Load a rule list from the rule store and return it. Will use cached results if they exist
  func ruleList(for type: BlocklistType, mode: BlockingMode) async throws -> WKContentRuleList? {
    if let result = cachedRuleLists[type.makeIdentifier(for: mode)] {
      return try result.get()
    }

    return try await loadRuleList(for: type, mode: mode)
  }

  /// Load a rule list from the rule store and return it. Will not use cached results
  private func loadRuleList(
    for type: BlocklistType,
    mode: BlockingMode
  ) async throws -> WKContentRuleList? {
    let identifier = type.makeIdentifier(for: mode)

    do {
      guard let ruleList = try await ruleStore.contentRuleList(forIdentifier: identifier) else {
        return nil
      }

      self.cachedRuleLists[identifier] = .success(ruleList)
      return ruleList
    } catch {
      throw error
    }
  }

  /// Compiles the bundled file for the given generic type
  /// - Warning: This may replace any downloaded versions with the bundled ones in the rulestore
  /// for example, the `adBlock` rule type may replace the `adBlockRules` downloaded version.
  func compileBundledRuleList(for genericType: GenericBlocklistType) async throws {
    let type = BlocklistType.generic(genericType)
    try await compileBundledRuleList(for: genericType, modes: type.allowedModes)
  }

  /// Compiles the bundled file for the given generic type
  /// - Warning: This may replace any downloaded versions with the bundled ones in the rulestore
  /// for example, the `adBlock` rule type may replace the `genericContentBlockingBehaviors` downloaded version.
  func compileBundledRuleList(
    for genericType: GenericBlocklistType,
    modes: [BlockingMode]
  ) async throws {
    guard !modes.isEmpty else { return }
    guard let fileURL = genericType.fileURL else {
      assertionFailure("A bundled file shouldn't fail to load")
      return
    }

    let encodedContentRuleList = try String(contentsOf: fileURL)
    let type = BlocklistType.generic(genericType)
    try await compile(
      encodedContentRuleList: encodedContentRuleList,
      for: type,
      version: genericType.version,
      modes: modes
    )
  }

  /// Return the valid generic types for the given domain
  public func validGenericTypes(for domain: Domain) -> Set<GenericBlocklistType> {
    guard !domain.areAllShieldsOff else { return [] }
    var results = Set<GenericBlocklistType>()

    // Get domain specific rule types
    if domain.isShieldExpected(.adblockAndTp, considerAllShieldsOption: true) {
      results = results.union([.blockAds, .blockTrackers])
    }

    // Get global rule types
    if Preferences.Privacy.blockAllCookies.value {
      results.insert(.blockCookies)
    }

    // Always upgrade mixed content
    results.insert(.upgradeMixedContent)

    return results
  }

  /// Return the enabled blocklist types for the given domain
  private func validBlocklistTypes(for domain: Domain) -> Set<(BlocklistType)> {
    guard !domain.areAllShieldsOff else { return [] }

    // Get the generic types
    let genericTypes = validGenericTypes(for: domain)

    let genericRuleLists = genericTypes.map { genericType -> BlocklistType in
      return .generic(genericType)
    }

    guard domain.isShieldExpected(.adblockAndTp, considerAllShieldsOption: true) else {
      return Set(genericRuleLists)
    }

    // Get rule lists for filter lists
    let filterLists = FilterListStorage.shared.filterLists
    let additionalRuleLists = filterLists.compactMap { filterList -> BlocklistType? in
      guard filterList.isEnabled else { return nil }
      return .filterList(
        componentId: filterList.entry.componentId,
        isAlwaysAggressive: filterList.engineType.isAlwaysAggressive
      )
    }

    // Get rule lists for custom filter lists
    let customFilterLists = CustomFilterListStorage.shared.filterListsURLs
    var customRuleLists = customFilterLists.compactMap { customURL -> BlocklistType? in
      guard customURL.setting.isEnabled else { return nil }
      return .filterListURL(uuid: customURL.setting.uuid)
    }

    // Add any custom filter list text files
    if (try? CustomFilterListStorage.shared.savedCustomRulesFileURL()) != nil {
      customRuleLists.append(.filterListText)
    }

    return Set(genericRuleLists).union(additionalRuleLists)
      .union(customRuleLists)
  }

  /// Return the enabled rule types for this domain and the enabled settings.
  /// It will attempt to return cached results if they exist otherwise it will attempt to load results from the rule store
  public func ruleLists(for domain: Domain) async -> Set<WKContentRuleList> {
    let validBlocklistTypes = self.validBlocklistTypes(for: domain)
    let level = domain.blockAdsAndTrackingLevel

    return await Set(
      validBlocklistTypes.asyncConcurrentCompactMap({ blocklistType -> WKContentRuleList? in
        let mode = blocklistType.mode(isAggressiveMode: level.isAggressive)

        do {
          return try await self.ruleList(for: blocklistType, mode: mode)
        } catch {
          // We can't log the error because some rules have empty rules. This is normal
          // But on relaunches we try to reload the filter list and this will give us an error.
          // Need to find a more graceful way of handling this so error here can be logged properly
          return nil
        }
      })
    )
  }

  /// Remove the rule list for the given identifier. This will remove them from this local cache and from the rule store.
  private func removeRuleList(forIdentifier identifier: String, force: Bool) async throws {
    guard force || self.cachedRuleLists[identifier] != nil else { return }
    self.cachedRuleLists.removeValue(forKey: identifier)
    self.versions.value.removeValue(forKey: identifier)
    try await ruleStore.removeContentRuleList(forIdentifier: identifier)
    Self.log.debug("Removed rule list for `\(identifier)`")
  }

  private func decode(encodedContentRuleList: String) throws -> [[String: Any?]] {
    guard let blocklistData = encodedContentRuleList.data(using: .utf8) else {
      assertionFailure()
      throw CompileError.invalidJSONArray
    }

    guard let jsonArray = try JSONSerialization.jsonObject(with: blocklistData) as? [[String: Any]]
    else {
      throw CompileError.invalidJSONArray
    }

    return jsonArray
  }

  private func encode(ruleList: [[String: Any?]], isAggressive: Bool) throws -> String {
    let modifiedData = try JSONSerialization.data(withJSONObject: ruleList)

    guard let result = String(bytes: modifiedData, encoding: .utf8) else {
      throw CompileError.invalidJSONArray
    }

    return result
  }
}

extension ShieldLevel {
  var preferredBlockingModes: Set<ContentBlockerManager.BlockingMode> {
    if isAggressive {
      return [.aggressive, .general]
    } else {
      return [.standard, .general]
    }
  }
}

extension ContentBlockerManager {
  fileprivate static var versionDateFormatter: ISO8601DateFormatter {
    return ISO8601DateFormatter()
  }
}

extension ContentBlockerManager.GenericBlocklistType {
  var fileURL: URL? {
    return Bundle.module.url(
      forResource: bundledFileName,
      withExtension: "json"
    )
  }

  @MainActor private var fileDate: Date? {
    guard let fileURL = fileURL else { return nil }
    let attributes = try? FileManager.default.attributesOfItem(atPath: fileURL.path)
    return (attributes?[.modificationDate] as? Date) ?? (attributes?[.creationDate] as? Date)
  }

  /// Return a version string which is gathered from the date
  @MainActor var version: String {
    guard let fileDate = fileDate else { return "0" }
    return ContentBlockerManager.versionDateFormatter.string(from: fileDate)
  }
}

extension ResourceDownloader.DownloadResult {
  /// Return a version string which is gathered from the date
  @MainActor var version: String {
    return ContentBlockerManager.versionDateFormatter.string(from: date)
  }
}
