// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import BraveShared
import Shared

private let log = ContentBlockerManager.log

/// This class is responsible for loading the engine when new resources arrive.
/// It ensures that we always have a fresh new engine as new data is downloaded
public actor AdBlockEngineManager: Sendable {
  /// The source of a resource. In some cases we need to remove all resources for a given source.
  enum Source: Hashable {
    case adBlock
    case cosmeticFilters
    case filterList(uuid: String)
    
    /// The order of this source relative to other sources.
    ///
    /// Used to compute an overall order of the resources
    fileprivate var relativeOrder: Int {
      switch self {
      case .adBlock: return 0
      case .cosmeticFilters: return 3
      case .filterList: return 7
      }
    }
  }
  
  /// The type of resource so we know how to compile it and add it into the engine
  enum ResourceType: CaseIterable {
    case dat
    case jsonResources
    case ruleList
    
    /// The order of this resource type relative to other resource types.
    ///
    /// Used to compute an overall order of the resources
    var relativeOrder: Int {
      switch self {
      case .ruleList: return 0
      case .dat: return 1
      case .jsonResources: return 2
      }
    }
  }
  
  /// An object representing a resource that can be compiled into our engine.
  struct Resource: Hashable {
    /// The type of resource so we know how to compile it and add it into the engine
    let type: ResourceType
    /// The source of this resource so we can remove all resources for this source at a later time
    let source: Source
    
    fileprivate var relativeOrder: Int {
      return type.relativeOrder + source.relativeOrder
    }
  }
  
  /// An aboject containing the resource and version.
  ///
  /// Stored in this way so we can replace resources with an older version
  private struct ResourceWithVersion: Hashable {
    /// The resource that we need to compile
    let resource: Resource
    /// The local file url for the given resource
    let fileURL: URL
    /// The version of this resource
    let version: String?
    /// The order of this resource relative to other resources.
    ///
    /// Used to compute an overall order of the resources
    let relativeOrder: Int
    
    /// The overal order of this resource taking account the order of the resource type, resource source and relative order
    fileprivate var order: Int {
      return resource.relativeOrder + relativeOrder
    }
  }
  
  enum CompileError: Error {
    case invalidResourceJSON
    case fileNotFound
    case couldNotDeserializeDATFile
  }
  
  private actor SyncData {
    /// The current set resources which will be compiled and loaded
    var enabledResources: Set<ResourceWithVersion>
    /// The compile results
    var compileResults: [ResourceWithVersion: Result<Void, Error>]
    /// The current compile task. Ensures we don't try to compile while we're already compiling
    var compileTask: Task<Void, Error>?
    /// Cached engines
    var cachedEngines: [Source: AdblockEngine]
    
    /// Tells us if all the enabled resources are synced
    /// (i.e. we didn't compile too little or too many resources and don't need to recompile them)
    var isSynced: Bool {
      return enabledResources.allSatisfy({ compileResults[$0] != nil }) && compileResults.allSatisfy({ key, _ in
        enabledResources.contains(key)
      })
    }
    
    init() {
      self.enabledResources = []
      self.compileResults = [:]
      self.cachedEngines = [:]
    }
    
    /// Tells this manager to add this resource next time it compiles this engine
    func add(resource: ResourceWithVersion) {
      self.enabledResources = enabledResources.filter({ resourceWithVersion in
        guard resourceWithVersion.resource == resource.resource else { return true }
        // Remove these compile results so we have to compile again
        compileResults.removeValue(forKey: resourceWithVersion)
        return false
      })
      
      enabledResources.insert(resource)
    }
    
    /// Tells this manager to remove all resources for the given source next time it compiles this engine
    func removeResources(for source: Source, resourceTypes: Set<ResourceType> = Set(ResourceType.allCases)) {
      self.enabledResources = self.enabledResources.filter { resourceWithVersion in
        let resource = resourceWithVersion.resource
        guard resource.source == source && resourceTypes.contains(resource.type) else { return true }
        return false
      }
    }
    
    /// Set the compile results so this manager can compute if its in sync or not
    func set(compileResults: [ResourceWithVersion: Result<Void, Error>]) {
      self.compileResults = compileResults
    }
    
    /// Set the compile results so this manager can compute if its in sync or not
    func add(engine: AdblockEngine, for source: Source) {
      self.cachedEngines[source] = engine
    }
    
    /// Set the current compile task to avoid overlaping compilations
    func set(compileTask: Task<Void, Error>?) {
      self.compileTask = compileTask
    }
    
    #if DEBUG
    /// A method that logs info on the given resources
    func debug(resources: [ResourceWithVersion]) {
      let resourcesString = resources
        .map { resourceWithVersion -> String in
          let resource = resourceWithVersion.resource
          let type: String
          let sourceString: String
          let resultString: String
          
          switch resource.type {
          case .dat:
            type = "dat"
          case .jsonResources:
            type = "jsonResources"
          case .ruleList:
            type = "ruleList"
          }
          
          switch resource.source {
          case .filterList(let uuid):
            sourceString = "filterList(\(uuid))"
          case .adBlock:
            sourceString = "adBlock"
          case .cosmeticFilters:
            sourceString = "cosmeticFilters"
          }
          
          switch compileResults[resourceWithVersion] {
          case .failure(let error):
            resultString = error.localizedDescription
          case .success:
            resultString = "success"
          case .none:
            resultString = "not compiled"
          }
          
          let sourceDebugString =
          """
          {
          order: \(resourceWithVersion.order)
          fileName: \(resourceWithVersion.fileURL.lastPathComponent)
          source: \(sourceString)
          version: \(resourceWithVersion.version ?? "nil")
          type: \(type)
          result: \(resultString)
          }
          """
          
          return sourceDebugString
        }

      log.debug("Loaded \(self.enabledResources.count, privacy: .public) (total) engine resources:\n\(resourcesString, privacy: .public)")
    }
    #endif
  }
  
  public static var shared = AdBlockEngineManager()
  
  /// The stats to store the engines on
  private let stats: AdBlockStats
  /// The actor in which all of our sync data is stored on
  private var data: SyncData
  /// The repeating build task
  private var endlessBuildTask: Task<(), Error>?
  /// The amount of time to wait before checking if new entries came in
  private static let buildSleepTime: TimeInterval = {
    #if DEBUG
    return 10
    #else
    return 1.minutes
    #endif
  }()
  
  init(stats: AdBlockStats = AdBlockStats.shared) {
    self.stats = stats
    self.data = SyncData()
  }
  
  /// Tells this manager to add this resource next time it compiles this engine
  func add(resource: Resource, fileURL: URL, version: String?, relativeOrder: Int = 0) async {
    let resourceWithVersion = ResourceWithVersion(
      resource: resource,
      fileURL: fileURL,
      version: version,
      relativeOrder: relativeOrder
    )
    await data.add(resource: resourceWithVersion)
  }
  
  /// Tells this manager to remove all resources for the given source next time it compiles this engine
  func removeResources(for source: Source, resourceTypes: Set<ResourceType> = Set(ResourceType.allCases)) async {
      await data.removeResources(for: source, resourceTypes: resourceTypes)
  }
  
  /// Start a timer that will compile resources if they change
  public func startTimer() {
    guard endlessBuildTask == nil else { return }
    
    self.endlessBuildTask = Task.detached {
      try await withTaskCancellationHandler(operation: {
        while true {
          try await Task.sleep(seconds: Self.buildSleepTime)
          guard await self.data.compileTask == nil else { continue }
          guard await !self.data.isSynced else { continue }
          await self.compileResources()
        }
      }, onCancel: {
        Task { @MainActor in
          await self.removeBuildTask()
        }
      })
    }
  }
  
  private func removeBuildTask() async {
    endlessBuildTask = nil
  }
  
  /// Compile all resources
  func compileResources() async {
    await data.compileTask?.cancel()
    await data.set(compileTask: nil)
    
    let task = Task.detached {
      let resourcesWithVersion = await self.data.enabledResources.sorted(by: {
        $0.order < $1.order
      })
      
      var allCompileResults: [ResourceWithVersion: Result<Void, Error>] = [:]
      var allEngines: [AdblockEngine] = []
      
      for (_, group) in await self.group(resources: resourcesWithVersion) {
        try Task.checkCancellation()
        
        // Combine all rule lists that need to be injected during initialization
        let combinedRuleLists = await self.combineAllRuleLists(from: group)
        // Create an engine with the combined rule lists
        let engine = AdblockEngine(rules: combinedRuleLists)
        // Compile remaining resources
        let compileResults = await self.compile(resources: group, into: engine)
        
        for (resourceWithVersion, compileResult) in compileResults {
          allCompileResults[resourceWithVersion] = compileResult
        }
        
        allEngines.append(engine)
      }
      
      // Set the results and clear some things
      try Task.checkCancellation()
      await self.data.set(compileResults: allCompileResults)
      let engines = allEngines
      
      try await MainActor.run {
        try Task.checkCancellation()
        self.stats.set(engines: engines)
      }
      
      #if DEBUG
      Task {
        log.debug("AdblockEngineManager")
        await self.data.debug(resources: resourcesWithVersion)
      }
      #endif
    }
    
    await data.set(compileTask: task)
    
    do {
      try await task.value
    } catch {
      log.error("\(error.localizedDescription)")
    }
    
    await data.set(compileTask: nil)
  }
  
  private func group(resources: [ResourceWithVersion]) -> [Source: [ResourceWithVersion]] {
    var groups: [Source: [ResourceWithVersion]] = [:]
    
    for resourceWithVersion in resources {
      var group = groups[resourceWithVersion.resource.source] ?? []
      group.append(resourceWithVersion)
      groups[resourceWithVersion.resource.source] = group
    }
    
    return groups
  }
  
  /// Compile all the resources on a detached task
  private func compile(resources: [ResourceWithVersion], into engine: AdblockEngine) async -> [ResourceWithVersion: Result<Void, Error>] {
    var compileResults: [ResourceWithVersion: Result<Void, Error>] = [:]
    
    for resourceWithVersion in resources {
      do {
        try await self.compile(resource: resourceWithVersion, into: engine)
        compileResults[resourceWithVersion] = .success(Void())
      } catch {
        compileResults[resourceWithVersion] = .failure(error)
        log.error("\(error.localizedDescription)")
      }
    }
    
    return compileResults
  }
  
  /// Combine all resources of type rule lists to one single string
  private func combineAllRuleLists(from resourcesWithVersion: [ResourceWithVersion]) async -> String {
    return await withTaskGroup(of: String?.self, returning: String.self) { group in
      // Combine all rule lists that need to be injected during initialization
      for resourceWithVersion in resourcesWithVersion {
        switch resourceWithVersion.resource.type {
        case .ruleList:
          group.addTask {
            return await Task.detached {
              guard let data = FileManager.default.contents(atPath: resourceWithVersion.fileURL.path) else {
                return nil
              }
              
              return String(data: data, encoding: .utf8)
            }.value
          }
        case .dat, .jsonResources:
          break
        }
      }
      
      let allResults = await group.reduce([String](), { partialResult, string in
        guard let string = string else { return partialResult }
        var fullResult = partialResult
        fullResult.append(string)
        return fullResult
      })
      
      let combinedRules = allResults.joined(separator: "\n")
      return combinedRules
    }
  }
  
  /// Compile the given resource into the given engine
  private func compile(resource: ResourceWithVersion, into engine: AdblockEngine) async throws {
    return try await withCheckedThrowingContinuation { continuation in
      switch resource.resource.type {
      case .dat:
        guard let data = FileManager.default.contents(atPath: resource.fileURL.path) else {
          continuation.resume(throwing: CompileError.fileNotFound)
          return
        }
        
        if engine.deserialize(data: data) {
          continuation.resume()
        } else {
          continuation.resume(throwing: CompileError.couldNotDeserializeDATFile)
        }
      case .jsonResources:
        guard let data = FileManager.default.contents(atPath: resource.fileURL.path) else {
          continuation.resume(throwing: CompileError.fileNotFound)
          return
        }
        
        do {
          guard let json = try self.validateJSON(data) else {
            continuation.resume()
            return
          }
          
          engine.addResources(json)
          continuation.resume()
        } catch {
          continuation.resume(throwing: error)
        }
      case .ruleList:
        // This is added during engine initualization
        continuation.resume()
      }
    }
  }
  
  /// Return a `JSON` string if this data is valid
  private func validateJSON(_ data: Data) throws -> String? {
    let value = try JSONSerialization.jsonObject(with: data, options: [])
    
    if let value = value as? NSArray {
      guard value.count > 0 else { return nil }
      return String(data: data, encoding: .utf8)
    }
    
    if let value = value as? NSDictionary {
      guard value.count > 0 else { return nil }
      return String(data: data, encoding: .utf8)
    }
    
    throw CompileError.invalidResourceJSON
  }
}
