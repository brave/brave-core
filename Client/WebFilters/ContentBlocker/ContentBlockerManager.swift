// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
@preconcurrency import WebKit.WKContentRuleListStore
import Data
import Shared
import BraveShared
import os.log

// TODO: Convert this class to `actor`(#6018)
/// A class responsible for compiling content blocker lists
final public class ContentBlockerManager: Sendable {
  // TODO: Use a proper logger system once implemented and adblock files are moved to their own module(#5928).
  /// Logger to use for debugging.
  static let log = Logger(subsystem: "com.brave.ios", category: "adblock")
  
  /// An object representing a rule type and a source type.
  public struct RuleTypeWithSourceType: Hashable {
    let ruleType: BlocklistRuleType
    let sourceType: BlocklistSourceType
  }
  
  /// An object representing the source of a block-list
  public enum BlocklistSourceType: Hashable, Sendable {
    /// A block list that is bundled with this application
    case bundled
    /// A block list that is downloaded from a server
    case downloaded(version: String?)
  }
  
  /// An object representing the type of block list
  public enum GeneralBlocklistTypes: String, CaseIterable {
    case blockAds = "block-ads"
    case blockCookies = "block-cookies"
    case upgradeHTTP = "upgrade-http"
    case blockTrackers = "block-trackers"
    
    var fileName: String {
      return rawValue
    }
    
    /// List of all bundled content blockers.
    /// Regional lists are downloaded on fly and not included here.
    static var validLists: Set<GeneralBlocklistTypes> {
      // TODO: Downgrade to 14.5 once api becomes available.
      if #available(iOS 15, *) {
        return Set(allCases).subtracting([.upgradeHTTP])
      } else {
        return Set(allCases)
      }
    }
  }
  
  /// Represents the rule type
  public enum BlocklistRuleType: Hashable {
    case general(GeneralBlocklistTypes)
    case filterList(uuid: String)
    
    var identifier: String {
      switch self {
      case .general(let storedType): return ["stored-type", storedType.fileName].joined(separator: "-")
      case .filterList(let uuid): return ["filter-list", uuid].joined(separator: "-")
      }
    }
  }
  
  enum CompileError: Error {
    case fileNotFound(identifier: String)
    case noRuleListReturned(identifier: String)
    case invalidResourceString(identifier: String)
  }
  
  /// Represents a resource that needs to be compiled
  struct Resource: Hashable {
    /// The local url of this resource
    let url: URL
    /// The source type of this resource
    let sourceType: BlocklistSourceType
  }
  
  private actor SyncData {
    /// The resources that need to be compiled
    private(set) var enabledResources: [String: Resource]
    private(set) var compiledResources: [String: Resource]
    
    var pendingResources: [String: Resource] {
      var results: [String: Resource] = [:]
      
      enabledResources.forEach { key, resource in
        guard compiledResources[key] == nil || compiledResources[key] != resource else { return }
        results[key] = resource
      }
      
      return results
    }
    
    init() {
      self.enabledResources = [:]
      self.compiledResources = [:]
    }
    
    var isSynced: Bool {
      return pendingResources.isEmpty
    }
    
    /// Set a resource for the given rule type
    func set(enabledResource: Resource, for ruleType: BlocklistRuleType) {
      enabledResources[ruleType.identifier] = enabledResource
    }
    
    /// Remove all resources for the given rule and source type
    func removeEnabledResource(for ruleType: BlocklistRuleType) {
      enabledResources.removeValue(forKey: ruleType.identifier)
    }
    
    /// Remove all resources for the given rule and source type
    func movePendingResource(forIdentifier identifier: String) {
      compiledResources[identifier] = enabledResources[identifier]
    }
  }
  
  public static var shared = ContentBlockerManager()
  /// The store in which these rule lists should be compiled
  let ruleStore: WKContentRuleListStore
  /// Compile results
  private var cachedCompileResults: [String: (sourceType: BlocklistSourceType, result: Result<WKContentRuleList, Error>)]
  /// The actor in which all of our sync data is stored on
  private var data: SyncData
  /// The repeating task that contnually compiles new blocklists as they come in
  private var endlessCompileTask: Task<(), Error>?
  /// The amount of time to wait before checking if new entries came in
  private static let compileSleepTime: TimeInterval = {
    #if DEBUG
    return 10
    #else
    return 1.minutes
    #endif
  }()
  
  init(ruleStore: WKContentRuleListStore = .default()) {
    self.ruleStore = ruleStore
    self.cachedCompileResults = [:]
    self.data = SyncData()
  }
  
  public func loadCachedCompileResults() async {
    guard let identifiers = await ruleStore.availableIdentifiers() else { return }
    let loadedCachedResults = await load(identifiers: identifiers)
    
    await MainActor.run {
      for cachedResults in loadedCachedResults {
        self.cachedCompileResults[cachedResults.0] = (.bundled, cachedResults.1)
      }
    }
  }
  
  private func cleanupRuleLists() async {
    guard let identifiers = await ruleStore.availableIdentifiers() else { return }
    let enabledResources = await data.enabledResources
    
    await remove(identifiers: identifiers.filter({ identifier in
      !enabledResources.contains(where: { $0.key == identifier })
    }))
  }
  
  private func remove(identifiers: [String]) async {
    return await withTaskGroup(of: Void.self) { group in
      for identifier in identifiers {
        group.addTask {
          do {
            try await self.ruleStore.removeContentRuleList(forIdentifier: identifier)
          } catch {
            Self.log.error("\(error.localizedDescription)")
          }
        }
      }
    }
  }
  
  private func load(identifiers: [String]) async -> [(String, Result<WKContentRuleList, Error>)] {
    return await withTaskGroup(of: (identifier: String, result: Result<WKContentRuleList, Error>?).self, returning: [(String, Result<WKContentRuleList, Error>)].self) { group in
      for identifier in identifiers {
        group.addTask {
          do {
            guard let ruleList = try await self.ruleStore.contentRuleList(forIdentifier: identifier) else {
              return (identifier, nil)
            }
            
            return (identifier, .success(ruleList))
          } catch {
            return (identifier, .failure(error))
          }
        }
      }
      
      return await group.reduce([(String, Result<WKContentRuleList, Error>)](), { partialResult, nextResult in
        guard let result = nextResult.result else { return partialResult }
        var partialResult = partialResult
        partialResult.append((nextResult.identifier, result))
        return partialResult
      })
    }
  }
  
  public func loadBundledResources() async {
    await withTaskGroup(of: Void.self) { group in
      for type in GeneralBlocklistTypes.validLists {
        group.addTask(operation: {
          guard let resource = await self.getBundledResource(for: type) else { return }
          await self.data.set(enabledResource: resource, for: .general(type))
        })
      }
    }
  }
  
  /// Start the timer that will continually compile new resources
  public func startTimer() {
    guard endlessCompileTask == nil else { return }
    
    self.endlessCompileTask = Task.detached {
      try await withTaskCancellationHandler(operation: {
        while true {
          try await Task.sleep(seconds: Self.compileSleepTime)
          guard await !self.data.isSynced else { return }
          await self.compilePendingResources()
          await self.cleanupRuleLists()
        }
      }, onCancel: {
        Task { @MainActor in
          self.endlessCompileTask = nil
        }
      })
    }
  }
  
  /// Set a resource for the given rule type
  func set(resource: Resource, for ruleType: BlocklistRuleType) async {
    await self.data.set(enabledResource: resource, for: ruleType)
  }
  
  /// Remove all resources for the given rule and source type
  func removeResource(for ruleType: BlocklistRuleType) async {
    await self.data.removeEnabledResource(for: ruleType)
    
    switch ruleType {
    case .general(let generalBlocklistTypes):
      /// Add back the bundled rule type if we need to. We always want at least the bundled resource type
      switch cachedCompileResults[ruleType.identifier]?.sourceType {
      case .downloaded, .none:
        guard let resource = await getBundledResource(for: generalBlocklistTypes) else { return }
        await self.data.set(enabledResource: resource, for: ruleType)
      case .bundled:
        // We're good
        break
      }
    case .filterList:
      break
    }
    
    // Cleanup some stored data
    await cleanupRuleLists()
  }
  
  /// Compile all the resources
  func compilePendingResources() async {
    let resources = await self.data.pendingResources
    
    await withTaskGroup(of: Void.self) { group in
      for (identifier, resource) in resources {
        group.addTask { @MainActor in
          do {
            let ruleList = try await self.compile(resource: resource, forIdentifier: identifier)
            self.cachedCompileResults[identifier] = (resource.sourceType, .success(ruleList))
          } catch {
            Self.log.error("\(error.localizedDescription)")
            self.cachedCompileResults[identifier] = (resource.sourceType, .failure(error))
          }
          await self.data.movePendingResource(forIdentifier: identifier)
        }
      }
    }
    
    #if DEBUG
    Self.log.debug("ContentBlockerManager")
    debug(resources: resources)
    #endif
  }
  
  /// Compile the given resource
  private func compile(resource: Resource, forIdentifier identifier: String) async throws -> WKContentRuleList {
    guard let jsonData = FileManager.default.contents(atPath: resource.url.path) else {
      throw CompileError.fileNotFound(identifier: identifier)
    }
    
    guard let json = String(data: jsonData, encoding: .utf8) else {
      throw CompileError.invalidResourceString(identifier: identifier)
    }
                
    let ruleList = try await ruleStore.compileContentRuleList(forIdentifier: identifier, encodedContentRuleList: json)
    
    guard let ruleList = ruleList else {
      throw CompileError.noRuleListReturned(identifier: identifier)
    }
    
    return ruleList
  }
  
  /// Return the enabled rule types for this domain and the enabled settings
  public func compiledRuleTypes(for domain: Domain) -> Set<RuleTypeWithSourceType> {
    let filterLists = FilterListResourceDownloader.shared.filterLists
    
    if domain.shield_allOff == 1 {
      return []
    }
    
    var results = Set<RuleTypeWithSourceType>()

    // Get domain specific rule types
    if domain.isShieldExpected(.AdblockAndTp, considerAllShieldsOption: true) {
      if let sourceType = self.sourceType(for: .general(.blockAds)) {
        results.insert(RuleTypeWithSourceType(ruleType: .general(.blockAds), sourceType: sourceType))
      }
      
      if let sourceType = self.sourceType(for: .general(.blockTrackers)) {
        results.insert(RuleTypeWithSourceType(ruleType: .general(.blockTrackers), sourceType: sourceType))
      }
    }
    
    // Get filter list specific rule types
    filterLists.forEach { filterList in
      guard filterList.isEnabled else { return }
      let ruleType = filterList.makeRuleType()
      
      guard let sourceType = self.sourceType(for: ruleType) else {
        return
      }
      
      guard cachedRuleList(for: ruleType) != nil else {
        return
      }
      
      results.insert(RuleTypeWithSourceType(ruleType: ruleType, sourceType: sourceType))
    }
    
    // Get global rule types
    if Preferences.Privacy.blockAllCookies.value {
      if let sourceType = sourceType(for: .general(.blockCookies)) {
        results.insert(RuleTypeWithSourceType(ruleType: .general(.blockCookies), sourceType: sourceType))
      }
    }
    
    return results
  }
  
  /// Return a source type for the given rule type
  public func sourceType(for ruleType: BlocklistRuleType) -> BlocklistSourceType? {
    guard let compileResult = cachedCompileResults[ruleType.identifier] else { return nil }
    
    switch compileResult.result {
    case .success:
      return compileResult.sourceType
    case .failure:
      return nil
    }
  }
  
  /// Get the cached rule list for this rule type. Returns nil if it's either not compiled or there were errors during compilation
  public func cachedRuleList(for ruleType: BlocklistRuleType) -> WKContentRuleList? {
    guard let compileResult = cachedCompileResults[ruleType.identifier] else { return nil }
    
    switch compileResult.result {
    case .success(let ruleList): return ruleList
    case .failure: return nil
    }
  }
  
  /// Load the rule lists for the given rule types
  public func loadRuleLists(for ruleTypes: Set<BlocklistRuleType>) async -> [WKContentRuleList] {
    return await withTaskGroup(of: WKContentRuleList?.self) { group in
      for ruleType in ruleTypes {
        group.addTask {
          do {
            return try await self.loadRuleList(for: ruleType)
          } catch {
            Self.log.error("\(error.localizedDescription)")
            return nil
          }
        }
      }
      
      var results: [WKContentRuleList] = []
      for await ruleList in group {
        guard let ruleList = ruleList else { continue }
        results.append(ruleList)
      }
      return results
    }
  }
  
  /// Load the rule list for the given rule type
  public func loadRuleList(for ruleType: BlocklistRuleType) async throws -> WKContentRuleList? {
    return try await ruleStore.contentRuleList(forIdentifier: ruleType.identifier)
  }

  /// Compile the data and set it for the given general type.
  private func getBundledResource(for type: GeneralBlocklistTypes) async -> Resource? {
    guard let url = await loadBundledURL(for: type) else { return nil }
    return Resource(url: url, sourceType: .bundled)
  }

  /// Get the bundled URL for the given type
  private func loadBundledURL(for type: GeneralBlocklistTypes) async -> URL? {
    return await withCheckedContinuation { continuation in
      guard let fileURL = Bundle.current.url(forResource: type.rawValue, withExtension: "json") else {
        continuation.resume(returning: nil)
        return
      }
      
      continuation.resume(returning: fileURL)
    }
  }
  
  #if DEBUG
  /// A method that logs info on the given resources
  private func debug(resources: [String: Resource]) {
    let resourcesString = resources
      .sorted(by: { lhs, rhs in
        lhs.value.url.absoluteString < rhs.value.url.absoluteString
      })
      .map { identifier, compiledResource -> String in
        let sourceString: String
        let versionString: String
        let resultString: String
        
        switch compiledResource.sourceType {
        case .bundled:
          sourceString = "bundled"
          versionString = "nil"
        case .downloaded(let version):
          sourceString = "downloaded"
          versionString = version ?? "nil"
        }
        
        switch self.cachedCompileResults[identifier]?.result {
        case .failure(let error):
          resultString = error.localizedDescription
        case .success:
          resultString = "success"
        case .none:
          resultString = "nil"
        }
        
        let resourcesDebugString =
        """
        {
        identifier: \(identifier)
        fileName: \(compiledResource.url.lastPathComponent)
        source: \(sourceString)
        version: \(versionString)
        result: \(resultString)
        }
        """
        
        return resourcesDebugString
      }

    Self.log.debug("Compiled \(resources.count, privacy: .public) additional block list resources: \n\(resourcesString, privacy: .public))")
  }
  #endif
}
