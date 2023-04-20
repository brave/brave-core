// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
@preconcurrency import WebKit
import Data
import Shared
import Preferences
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
    case noRuleListReturned(identifier: String)
  }
  
  public enum GenericBlocklistType: Hashable, CaseIterable {
    case blockAds
    case blockCookies
    case blockTrackers
    
    var bundledFileName: String {
      switch self {
      case .blockAds: return "block-ads"
      case .blockCookies: return "block-cookies"
      case .blockTrackers: return "block-trackers"
      }
    }
  }
  
  /// An object representing the type of block list
  public enum BlocklistType: Hashable {
    fileprivate static let genericPrifix = "stored-type"
    fileprivate static let filterListPrefix = "filter-list"
    fileprivate static let filterListURLPrefix = "filter-list-url"
    
    case generic(GenericBlocklistType)
    case filterList(uuid: String)
    case customFilterList(uuid: String)
    
    var identifier: String {
      switch self {
      case .generic(let type):
        return [Self.genericPrifix, type.bundledFileName].joined(separator: "-")
      case .filterList(let uuid):
        return [Self.filterListPrefix, uuid].joined(separator: "-")
      case .customFilterList(let uuid):
        return [Self.filterListURLPrefix, uuid].joined(separator: "-")
      }
    }
  }
  
  public static var shared = ContentBlockerManager()
  /// The store in which these rule lists should be compiled
  let ruleStore: WKContentRuleListStore
  /// We cached the rule lists so that we can return them quicker if we need to
  private var cachedRuleLists: [String: Result<WKContentRuleList, Error>]
  
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
      guard !validTypes.contains(where: { $0.identifier == identifier }) else { return }
      
      // Only allow certain prefixed identifiers to be removed so as not to remove something apple adds
      let prefixes = [BlocklistType.genericPrifix, BlocklistType.filterListPrefix, BlocklistType.filterListURLPrefix]
      guard prefixes.contains(where: { identifier.hasPrefix($0) }) else { return }
      
      do {
        try await self.removeRuleList(forIdentifier: identifier)
      } catch {
        assertionFailure()
      }
    }
  }
  
  /// Compile the given resource and store it in cache for the given blocklist type
  func compile(encodedContentRuleList: String, for type: BlocklistType, options: CompileOptions = []) async throws {
    do {
      let cleanedRuleList = try await performOperations(encodedContentRuleList: encodedContentRuleList, options: options)
      let ruleList = try await ruleStore.compileContentRuleList(forIdentifier: type.identifier, encodedContentRuleList: cleanedRuleList)
      
      guard let ruleList = ruleList else {
        throw CompileError.noRuleListReturned(identifier: type.identifier)
      }
      
      self.cachedRuleLists[type.identifier] = .success(ruleList)
    } catch {
      self.cachedRuleLists[type.identifier] = .failure(error)
      throw error
    }
    
    #if DEBUG
    ContentBlockerManager.log.debug("Compiled rule list `\(type.identifier)`")
    #endif
  }
  
  /// Check if a rule list is compiled for this type
  func hasRuleList(for type: BlocklistType) async -> Bool {
    do {
      return try await ruleList(for: type) != nil
    } catch {
      return false
    }
  }
  
  /// Remove the rule list for the blocklist type
  func removeRuleList(for type: BlocklistType) async throws {
    try await removeRuleList(forIdentifier: type.identifier)
  }
  
  /// Load a rule list from the rule store and return it. Will use cached results if they exist
  func ruleList(for type: BlocklistType) async throws -> WKContentRuleList? {
    if let result = cachedRuleLists[type.identifier] { return try result.get() }
    return try await loadRuleList(for: type)
  }
  
  /// Load a rule list from the rule store and return it. Will not use cached results
  private func loadRuleList(for type: BlocklistType) async throws -> WKContentRuleList? {
    do {
      guard let ruleList = try await ruleStore.contentRuleList(forIdentifier: type.identifier) else {
        return nil
      }
      
      self.cachedRuleLists[type.identifier] = .success(ruleList)
      return ruleList
    } catch {
      throw error
    }
  }
  
  /// Compiles the bundled file for the given generic type
  /// - Warning: This may replace any downloaded versions with the bundled ones in the rulestore
  /// for example, the `adBlock` rule type may replace the `genericContentBlockingBehaviors` downloaded version.
  func compileBundledRuleList(for genericType: GenericBlocklistType) async throws {
    guard let fileURL = Bundle.module.url(forResource: genericType.bundledFileName, withExtension: "json") else {
      assertionFailure("A bundled file shouldn't fail to load")
      return
    }
    
    let encodedContentRuleList = try String(contentsOf: fileURL)
    try await compile(encodedContentRuleList: encodedContentRuleList, for: .generic(genericType))
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
  @MainActor func validBlocklistTypes(for domain: Domain) -> Set<BlocklistType> {
    guard !domain.areAllShieldsOff else { return [] }
    
    // Get the generic types
    let genericTypes = validGenericTypes(for: domain)
    
    let genericRuleLists = genericTypes.map { genericType -> BlocklistType in
      return .generic(genericType)
    }
    
    // Get rule lists for filter lists
    let filterLists = FilterListStorage.shared.filterLists
    let additionalRuleLists = filterLists.compactMap { filterList -> BlocklistType? in
      guard filterList.isEnabled else { return nil }
      return .filterList(uuid: filterList.uuid)
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
    
    return await Set(validBlocklistTypes.asyncConcurrentCompactMap({ blocklistType -> WKContentRuleList? in
      do {
        return try await self.ruleList(for: blocklistType)
      } catch {
        return nil
      }
    }))
  }
  
  /// Remove the rule list for the given identifier. This will remove them from this local cache and from the rule store.
  private func removeRuleList(forIdentifier identifier: String) async throws {
    self.cachedRuleLists.removeValue(forKey: identifier)
    try await ruleStore.removeContentRuleList(forIdentifier: identifier)
    Self.log.debug("Removed rule list for `\(identifier)`")
  }
  
  /// Perform operations of the rule list given by the provided options
  private func performOperations(encodedContentRuleList: String, options: CompileOptions) async throws -> String {
    guard !options.isEmpty else { return encodedContentRuleList }
    
    guard let blocklistData = encodedContentRuleList.data(using: .utf8) else {
      assertionFailure()
      return encodedContentRuleList
    }
    
    guard var jsonArray = try JSONSerialization.jsonObject(with: blocklistData) as? [[String: Any]] else {
      return encodedContentRuleList
    }
    
    #if DEBUG
    let originalCount = jsonArray.count
    ContentBlockerManager.log.debug("Cleanining up \(originalCount) rules")
    #endif
    
    if options.contains(.stripContentBlockers) {
      jsonArray = await stripCosmeticFilters(jsonArray: jsonArray)
    }
    
    if options.contains(.punycodeDomains) {
      jsonArray = await punycodeDomains(jsonArray: jsonArray)
    }
    
    #if DEBUG
    let count = originalCount - jsonArray.count
    ContentBlockerManager.log.debug("Filtered out \(count) rules")
    #endif
    
    let modifiedData = try JSONSerialization.data(withJSONObject: jsonArray)
    return String(bytes: modifiedData, encoding: .utf8) ?? encodedContentRuleList
  }
  
  /// This will remove cosmetic filters from the provided encoded rule list. These are any rules that have a `selector` in the `action` field.
  /// We do this because our cosmetic filtering is handled via the `SelectorsPoller.js` file and these selectors come from the engine directly.
  private func stripCosmeticFilters(jsonArray: [[String: Any]]) async -> [[String: Any]] {
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
  private func punycodeDomains(jsonArray: [[String: Any]]) async -> [[String: Any]] {
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
