// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
@preconcurrency import WebKit
import Data
import Shared
import Preferences
import BraveShields
import BraveCore
import os.log

/// A class that aids in the managment of rule lists on the rule store.
actor ContentBlockerManager {
  // TODO: Use a proper logger system once implemented and adblock files are moved to their own module(#5928).
  /// Logger to use for debugging.
  static let log = Logger(subsystem: Bundle.main.bundleIdentifier!, category: "adblock")
  
  struct CompileOptions: OptionSet {
    let rawValue: Int
    
    static let stripContentBlockers = CompileOptions(rawValue: 1 << 0)
    static let punycodeDomains = CompileOptions(rawValue: 1 << 1)
    static let all: CompileOptions = [.stripContentBlockers, .punycodeDomains]
  }
  
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
    
    func mode(isAggressiveMode: Bool) -> BlockingMode {
      switch self {
      case .blockAds:
        if isAggressiveMode {
          return .aggressive
        } else {
          return .standard
        }
      case .blockCookies, .blockTrackers:
        return .general
      }
    }
    
    var bundledFileName: String {
      switch self {
      case .blockAds: return "block-ads"
      case .blockCookies: return "block-cookies"
      case .blockTrackers: return "block-trackers"
      }
    }
  }
  
  /// An object representing the type of block list
  public enum BlocklistType: Hashable, CustomDebugStringConvertible {
    fileprivate static let genericPrifix = "stored-type"
    fileprivate static let filterListPrefix = "filter-list"
    fileprivate static let filterListURLPrefix = "filter-list-url"
    
    case generic(GenericBlocklistType)
    case filterList(componentId: String, isAlwaysAggressive: Bool)
    case customFilterList(uuid: String)
    
    private var identifier: String {
      switch self {
      case .generic(let type):
        return [Self.genericPrifix, type.bundledFileName].joined(separator: "-")
      case .filterList(let componentId, _):
        return [Self.filterListPrefix, componentId].joined(separator: "-")
      case .customFilterList(let uuid):
        return [Self.filterListURLPrefix, uuid].joined(separator: "-")
      }
    }
    
    func mode(isAggressiveMode: Bool) -> BlockingMode {
      switch self {
      case .customFilterList:
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
  private var cachedRuleLists: [String: Result<WKContentRuleList, Error>]
  /// A list of etld+1s that are always aggressive
  /// TODO: @JS Replace this with the 1st party ad-block list
  let alwaysAggressiveETLDs: Set<String> = ["youtube.com"]
  
  init(ruleStore: WKContentRuleListStore = .default()) {
    self.ruleStore = ruleStore
    self.cachedRuleLists = [:]
  }
  
  /// Remove all rule lists minus the given valid types.
  /// Should be used only as a cleanup once during launch to get rid of unecessary/old data.
  /// This is mostly for custom filter lists a user may have added.
  public func cleaupInvalidRuleLists(validTypes: Set<BlocklistType>) async {
    let availableIdentifiers = await ruleStore.availableIdentifiers() ?? []
    
    await availableIdentifiers.asyncConcurrentForEach { identifier in
      guard !validTypes.contains(where: { type in
        type.allowedModes.contains(where: { type.makeIdentifier(for: $0) == identifier })
      }) else { return }
      
      // Only allow certain prefixed identifiers to be removed so as not to remove something apple adds
      let prefixes = [BlocklistType.genericPrifix, BlocklistType.filterListPrefix, BlocklistType.filterListURLPrefix]
      guard prefixes.contains(where: { identifier.hasPrefix($0) }) else { return }
      
      do {
        try await self.removeRuleList(forIdentifier: identifier, force: true)
      } catch {
        assertionFailure()
      }
    }
  }
  
  /// Compile the rule list found in the given local URL using the specified modes
  func compileRuleList(at localFileURL: URL, for type: BlocklistType, options: CompileOptions = [], modes: [BlockingMode]) async throws {
    let filterSet = try String(contentsOf: localFileURL)
    let result = try AdblockEngine.contentBlockerRules(fromFilterSet: filterSet)
    try await compile(encodedContentRuleList: result.rulesJSON, for: type, options: options, modes: modes)
  }
  
  /// Compile the given resource and store it in cache for the given blocklist type and specified modes
  func compile(encodedContentRuleList: String, for type: BlocklistType, options: CompileOptions = [], modes: [BlockingMode]) async throws {
    guard !modes.isEmpty else { return }
    let cleanedRuleList: [[String: Any?]]
    
    do {
      cleanedRuleList = try await process(encodedContentRuleList: encodedContentRuleList, for: type, with: options)
    } catch {
      for mode in modes {
        self.cachedRuleLists[type.makeIdentifier(for: mode)] = .failure(error)
      }
      throw error
    }
    
    var foundError: Error?
    
    for mode in modes {
      let moddedRuleList = self.set(mode: mode, forRuleList: cleanedRuleList)
      let identifier = type.makeIdentifier(for: mode)
      
      do {
        let ruleList = try await compile(ruleList: moddedRuleList, for: type, mode: mode)
        self.cachedRuleLists[identifier] = .success(ruleList)
        Self.log.debug("Compiled rule list for `\(identifier)`")
      } catch {
        self.cachedRuleLists[identifier] = .failure(error)
        Self.log.debug("Failed to compile rule list for `\(identifier)`: \(String(describing: error))")
        foundError = error
      }
    }
    
    if let error = foundError {
      throw error
    }
  }
  
  private func set(mode: BlockingMode, forRuleList ruleList: [[String: Any?]]) -> [[String: Any?]] {
    guard let lastRule = ruleList.last else { return ruleList }
    
    switch mode {
    case .aggressive:
      guard isFirstPartyException(jsonObject: lastRule) else { return ruleList }
      
      // Remove this rule to make it aggressive
      var ruleList = ruleList
      ruleList.removeLast()
      return ruleList
      
    case .standard:
      guard !isFirstPartyException(jsonObject: lastRule) else { return ruleList }
      
      // Add the ignore first party rule to make it standard
      var ruleList = ruleList
      ruleList.append([
        "action": ["type": "ignore-previous-rules"],
        "trigger": [
          "url-filter": ".*",
          "load-type": ["first-party"]
        ] as [String: Any?]
      ])
      return ruleList
      
    case .general:
      // Nothing needs to be done
      return ruleList
    }
  }
  
  /// Compile the given resource and store it in cache for the given blocklist type
  private func compile(ruleList: [[String: Any?]], for type: BlocklistType, mode: BlockingMode) async throws -> WKContentRuleList {
    let identifier = type.makeIdentifier(for: mode)
    let modifiedData = try JSONSerialization.data(withJSONObject: ruleList)
    let cleanedRuleList = String(bytes: modifiedData, encoding: .utf8)
    let ruleList = try await ruleStore.compileContentRuleList(
      forIdentifier: identifier, encodedContentRuleList: cleanedRuleList)
    
    guard let ruleList = ruleList else {
      throw CompileError.noRuleListReturned
    }
    
    return ruleList
  }
  
  /// Return all the modes that need to be compiled for the given type
  func missingModes(for type: BlocklistType) async -> [BlockingMode] {
    return await type.allowedModes.asyncFilter { mode in
      // If the file wasn't modified, make sure we have something compiled.
      // We should, but this can be false during upgrades if the identifier changed for some reason.
      if await hasRuleList(for: type, mode: mode) {
        ContentBlockerManager.log.debug("Rule list already compiled for `\(type.makeIdentifier(for: mode))`")
        return false
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
  private func loadRuleList(for type: BlocklistType, mode: BlockingMode) async throws -> WKContentRuleList? {
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
  func compileBundledRuleList(for genericType: GenericBlocklistType, modes: [BlockingMode]) async throws {
    guard !modes.isEmpty else { return }
    guard let fileURL = Bundle.module.url(forResource: genericType.bundledFileName, withExtension: "json") else {
      assertionFailure("A bundled file shouldn't fail to load")
      return
    }
    
    let encodedContentRuleList = try String(contentsOf: fileURL)
    let type = BlocklistType.generic(genericType)
    try await compile(
      encodedContentRuleList: encodedContentRuleList,
      for: type, modes: modes
    )
  }
  
  /// Return the valid generic types for the given domain
  @MainActor public func validGenericTypes(for domain: Domain) -> Set<GenericBlocklistType> {
    guard !domain.areAllShieldsOff else { return [] }
    var results = Set<GenericBlocklistType>()

    // Get domain specific rule types
    if domain.isShieldExpected(.AdblockAndTp, considerAllShieldsOption: true) {
      results = results.union([.blockAds, .blockTrackers])
    }
    
    // Get global rule types
    if Preferences.Privacy.blockAllCookies.value {
      results.insert(.blockCookies)
    }
    
    return results
  }
  
  /// Return the enabled blocklist types for the given domain
  @MainActor private func validBlocklistTypes(for domain: Domain) -> Set<(BlocklistType)> {
    guard !domain.areAllShieldsOff else { return [] }
    
    // Get the generic types
    let genericTypes = validGenericTypes(for: domain)
    
    let genericRuleLists = genericTypes.map { genericType -> BlocklistType in
      return .generic(genericType)
    }
    
    guard domain.isShieldExpected(.AdblockAndTp, considerAllShieldsOption: true) else {
      return Set(genericRuleLists)
    }
    
    // Get rule lists for filter lists
    let filterLists = FilterListStorage.shared.filterLists
    let additionalRuleLists = filterLists.compactMap { filterList -> BlocklistType? in
      guard filterList.isEnabled else { return nil }
      return .filterList(componentId: filterList.entry.componentId, isAlwaysAggressive: filterList.isAlwaysAggressive)
    }
    
    // Get rule lists for custom filter lists
    let customFilterLists = CustomFilterListStorage.shared.filterListsURLs
    let customRuleLists = customFilterLists.compactMap { customURL -> BlocklistType? in
      guard customURL.setting.isEnabled else { return nil }
      return .customFilterList(uuid: customURL.setting.uuid)
    }
    
    return Set(genericRuleLists).union(additionalRuleLists).union(customRuleLists)
  }
  
  /// Return the enabled rule types for this domain and the enabled settings.
  /// It will attempt to return cached results if they exist otherwise it will attempt to load results from the rule store
  public func ruleLists(for domain: Domain) async -> Set<WKContentRuleList> {
    let validBlocklistTypes = await self.validBlocklistTypes(for: domain)
    let level = await domain.blockAdsAndTrackingLevel
    
    return await Set(validBlocklistTypes.asyncConcurrentCompactMap({ blocklistType -> WKContentRuleList? in
      let mode = blocklistType.mode(isAggressiveMode: level.isAggressive)
      
      do {
        return try await self.ruleList(for: blocklistType, mode: mode)
      } catch {
        // We can't log the error because some rules have empty rules. This is normal
        // But on relaunches we try to reload the filter list and this will give us an error.
        // Need to find a more graceful way of handling this so error here can be logged properly
        return nil
      }
    }))
  }
  
  /// Remove the rule list for the given identifier. This will remove them from this local cache and from the rule store.
  private func removeRuleList(forIdentifier identifier: String, force: Bool) async throws {
    guard force || self.cachedRuleLists[identifier] != nil else { return }
    self.cachedRuleLists.removeValue(forKey: identifier)
    try await ruleStore.removeContentRuleList(forIdentifier: identifier)
    Self.log.debug("Removed rule list for `\(identifier)`")
  }
  
  private func decode(encodedContentRuleList: String) throws -> [[String: Any?]] {
    guard let blocklistData = encodedContentRuleList.data(using: .utf8) else {
      assertionFailure()
      throw CompileError.invalidJSONArray
    }
    
    guard let jsonArray = try JSONSerialization.jsonObject(with: blocklistData) as? [[String: Any]] else {
      throw CompileError.invalidJSONArray
    }
    
    return jsonArray
  }
  
  /// Perform operations of the rule list given by the provided options
  func process(encodedContentRuleList: String, for type: BlocklistType, with options: CompileOptions) async throws -> [[String: Any?]] {
    var ruleList = try decode(encodedContentRuleList: encodedContentRuleList)
    if options.isEmpty { return ruleList }
    
    #if DEBUG
    let originalCount = ruleList.count
    #endif
    
    if options.contains(.stripContentBlockers) {
      ruleList = await stripCosmeticFilters(jsonArray: ruleList)
    }
    
    if options.contains(.punycodeDomains) {
      ruleList = await punycodeDomains(jsonArray: ruleList)
    }
    
    #if DEBUG
    let count = originalCount - ruleList.count
    if count > 0 {
      Self.log.debug("Filtered out \(count) rules for `\(type.debugDescription)`")
    }
    #endif
    
    return ruleList
  }
  
  private func encode(ruleList: [[String: Any?]], isAggressive: Bool) throws -> String {
    let modifiedData = try JSONSerialization.data(withJSONObject: ruleList)
    
    guard let result = String(bytes: modifiedData, encoding: .utf8) else {
      throw CompileError.invalidJSONArray
    }
    
    return result
  }
  
  private func isFirstPartyException(jsonObject: [String: Any?]) -> Bool {
    guard
      let actionDictionary = jsonObject["action"] as? [String: Any],
      let actionType = actionDictionary["type"] as? String, actionType == "ignore-previous-rules",
      let triggerDictionary = jsonObject["trigger"] as? [String: Any],
      let urlFilter = triggerDictionary["url-filter"] as? String, urlFilter == ".*",
      let loadType = triggerDictionary["load-type"] as? [String], loadType == ["first-party"],
      triggerDictionary["resource-type"] == nil
    else {
      return false
    }
    
    return true
  }
  
  /// This will remove cosmetic filters from the provided encoded rule list. These are any rules that have a `selector` in the `action` field.
  /// We do this because our cosmetic filtering is handled via the `SelectorsPoller.js` file and these selectors come from the engine directly.
  private func stripCosmeticFilters(jsonArray: [[String: Any?]]) async -> [[String: Any?]] {
    let updatedArray = await jsonArray.asyncConcurrentCompactMap { dictionary in
      guard let actionDictionary = dictionary["action"] as? [String: Any] else {
        return dictionary
      }
      
      // Filter out with any dictionaries with `selector` actions
      if actionDictionary["selector"] != nil {
        return nil
      } else {
        return dictionary
      }
    }
    
    return updatedArray
  }
  
  /// Convert all domain in the `if-domain` and `unless-domain` fields.
  ///
  /// Sometimes we get non-punycoded domans in our JSON and apple does not allow non-punycoded domains to be passed to the rule store.
  private func punycodeDomains(jsonArray: [[String: Any?]]) async -> [[String: Any?]] {
    var jsonArray = jsonArray
    
    await jsonArray.enumerated().asyncConcurrentForEach({ index, dictionary in
      guard var triggerObject = dictionary["trigger"] as? [String: Any] else {
        return
      }
      
      if let domainArray = triggerObject["if-domain"] as? [String] {
        triggerObject["if-domain"] = self.punycodeConversion(domains: domainArray)
      }
      
      if let domainArray = triggerObject["unless-domain"] as? [String] {
        triggerObject["unless-domain"] = self.punycodeConversion(domains: domainArray)
      }
      
      jsonArray[index]["trigger"] = triggerObject
    })
    
    return jsonArray
  }
  
  /// Punycode an array of `domains` and return the punycoded results.
  /// The array size shoud be unchanged but this is not guarantted.
  private func punycodeConversion(domains: [String]) -> [String] {
    return domains.compactMap { domain -> String? in
      guard domain.allSatisfy({ $0.isASCII }) else {
        if let result = NSURL(idnString: domain)?.absoluteString {
          #if DEBUG
          Self.log.debug("Punycoded domain: \(domain) -> \(result)")
          #endif
          return result
        } else {
          #if DEBUG
          Self.log.debug("Could not punycode domain: \(domain)")
          #endif
          
          return nil
        }
      }
      
      return domain
    }
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
