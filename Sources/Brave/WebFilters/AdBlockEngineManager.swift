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
    case filterList(componentId: String, isAlwaysAggressive: Bool)
    case filterListURL(uuid: String)
    
    /// The order of this source relative to other sources.
    ///
    /// Used to compute an overall order of the resources
    fileprivate var relativeOrder: Int {
      switch self {
      case .adBlock: return 0
      case .filterList: return 100
      case .filterListURL: return 200
      }
    }
    
    /// Standard selectors allows us to unhide 1p content if standard mode is on.
    /// Agressive selectors never unhide even on standard mode
    ///
    /// The only allowed standard mode filter lists (i.e. ones that allow 1p unhiding) are the following:
    /// 1. Default filter lists (i.e. `adBlock`)
    /// 2. Regional filter lists (i.e. filter lists that have a language associated with them)
    ///
    /// All other filter lists are agressive (i.e. are always hidden regardless of 1p status)
    var isAlwaysAggressive: Bool {
      switch self {
      case .adBlock:
        // Our default filter lists are not agressive
        return false
      case .filterListURL:
        // Custom filter lists are always agressive
        return true
      case .filterList(_, let isAlwaysAggressive):
        return isAlwaysAggressive
      }
    }
  }
  
  /// The type of resource so we know how to compile it and add it into the engine
  public enum ResourceType: CaseIterable {
    case dat
    case ruleList
    
    /// The order of this resource type relative to other resource types.
    ///
    /// Used to compute an overall order of the resources
    var relativeOrder: Int {
      switch self {
      case .ruleList: return 0
      case .dat: return 1
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
  /// The path to the ad-block scriplet resources to be added to all engines
  private var scripletResourcesURL: URL?
  
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
  
  func set(scripletResourcesURL: URL) {
    guard scripletResourcesURL != self.scripletResourcesURL else { return }
    
    #if DEBUG
    ContentBlockerManager.log.debug("Set scriplet resources: \(scripletResourcesURL.lastPathComponent)")
    #endif
    self.scripletResourcesURL = scripletResourcesURL
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
    
    let results = await CachedAdBlockEngine.createEngines(
      from: resourcesWithVersion, scripletResourcesURL: scripletResourcesURL
    )

    let validEngines = results.compactMap { result in
      do {
        return try result.get()
      } catch {
        ContentBlockerManager.log.error("Failed to compile engine: \(String(describing: error))")
        return nil
      }
    }
    
    self.compiledResources = Set(resourcesWithVersion)
    await stats.set(engines: validEngines)
    
    #if DEBUG
    ContentBlockerManager.log.error("Recompiled engines")
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
