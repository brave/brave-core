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
    case filterListURL(uuid: String)
    
    /// The order of this source relative to other sources.
    ///
    /// Used to compute an overall order of the resources
    fileprivate var relativeOrder: Int {
      switch self {
      case .adBlock: return 0
      case .cosmeticFilters: return 3
      case .filterList: return 100
      case .filterListURL: return 200
      }
    }
  }
  
  /// The type of resource so we know how to compile it and add it into the engine
  public enum ResourceType: CaseIterable {
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
  
  /// An object containing the resource and version.
  ///
  /// Stored in this way so we can replace resources with an older version
  public struct ResourceWithVersion: Hashable {
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
  
  public static var shared = AdBlockEngineManager()
  
  /// The stats to store the engines on
  private let stats: AdBlockStats
  /// The repeating build task
  private var endlessBuildTask: Task<(), Error>?
  /// The current set resources which will be compiled and loaded
  var enabledResources: Set<ResourceWithVersion>
  /// The compile results
  var compiledResources: Set<ResourceWithVersion>
  
  /// The amount of time to wait before checking if new entries came in
  private static let buildSleepTime: TimeInterval = {
    #if DEBUG
    return 10
    #else
    return 1.minutes
    #endif
  }()
  
  /// Tells us if all the enabled resources are synced
  /// (i.e. we didn't compile too little or too many resources and don't need to recompile them)
  private var isSynced: Bool {
    return enabledResources == compiledResources
  }
  
  init(stats: AdBlockStats = AdBlockStats.shared) {
    self.stats = stats
    self.enabledResources = []
    self.compiledResources = []
  }
  
  /// Tells this manager to add this resource next time it compiles this engine
  func add(resource: Resource, fileURL: URL, version: String?, relativeOrder: Int = 0) async {
    let resourceWithVersion = ResourceWithVersion(
      resource: resource,
      fileURL: fileURL,
      version: version,
      relativeOrder: relativeOrder
    )
    
    add(resource: resourceWithVersion)
  }
  
  /// Tells this manager to remove all resources for the given source next time it compiles this engine
  func removeResources(for source: Source, resourceTypes: Set<ResourceType> = Set(ResourceType.allCases)) async {
    self.enabledResources = self.enabledResources.filter { resourceWithVersion in
      let resource = resourceWithVersion.resource
      guard resource.source == source && resourceTypes.contains(resource.type) else { return true }
      return false
    }
  }
  
  /// Start a timer that will compile resources if they change
  public func startTimer() {
    guard endlessBuildTask == nil else { return }
    
    self.endlessBuildTask = Task.detached(priority: .background) {
      do {
        while true {
          try await Task.sleep(seconds: Self.buildSleepTime)
          guard await !self.isSynced else { continue }
          await self.compileResources()
        }
      } catch is CancellationError {
        await self.clearBuildTask()
      }
    }
  }
  
  /// Compile all resources
  public func compileResources() async {
    let resourcesWithVersion = self.enabledResources.sorted(by: {
      $0.order < $1.order
    })
    
    let results = await AdblockEngine.createEngines(from: resourcesWithVersion)
    self.compiledResources = Set(resourcesWithVersion)
    await stats.set(engines: results.engines)
    #if DEBUG
    debug(compiledResults: results.compileResults)
    #endif
  }
  
  private func clearBuildTask() {
    endlessBuildTask = nil
  }
  
  /// Tells this manager to add this resource next time it compiles this engine
  private func add(resource: ResourceWithVersion) {
    // Remove existing resources that have a new version order, file url, etc
    if let existingResoure = enabledResources.first(where: { $0.resource == resource.resource }), existingResoure != resource {
      enabledResources.remove(existingResoure)
      compiledResources.remove(existingResoure)
    }
    
    // Add the new entry back in
    enabledResources.insert(resource)
  }
}

#if DEBUG
private extension AdBlockEngineManager {
  /// A method that logs info on the given resources
  func debug(compiledResults: [ResourceWithVersion: Result<Void, Error>]) {
    log.debug("Loaded \(compiledResults.count) (total) engine resources:")
    
    compiledResults.sorted(by: { $0.key.order < $1.key.order })
      .forEach { (resourceWithVersion, compileResult) in
        let resultString: String
        
        switch compileResult {
        case .success:
          resultString = "✔︎"
        case .failure(let error):
          resultString = "\(error)"
        }
        
        let sourceDebugString = [
          "", resourceWithVersion.debugDescription,
          "\(resultString)",
        ].joined(separator: " ")
        
        log.debug("\(sourceDebugString)")
      }
  }
}

extension AdBlockEngineManager.ResourceWithVersion: CustomDebugStringConvertible {
  public var debugDescription: String {
    return [
      "#\(order)",
      "\(resource.source.debugDescription).\(resource.type.debugDescription)",
      "v\(version ?? "nil")",
    ].joined(separator: " ")
  }
}

extension AdBlockEngineManager.Source: CustomDebugStringConvertible {
  public var debugDescription: String {
    switch self {
    case .filterList(let uuid): return "filterList(\(uuid))"
    case .filterListURL(let uuid): return "filterListURL(\(uuid))"
    case .adBlock: return "adBlock"
    case .cosmeticFilters: return "cosmeticFilters"
    }
  }
}

extension AdBlockEngineManager.ResourceType: CustomDebugStringConvertible {
  public var debugDescription: String {
    switch self {
    case .dat: return "dat"
    case .jsonResources: return "jsonResources"
    case .ruleList: return "ruleList"
    }
  }
}
#endif
