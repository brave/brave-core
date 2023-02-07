// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

extension AdblockEngine {
  public enum CompileError: Error {
    case invalidResourceJSON
    case fileNotFound
    case couldNotDeserializeDATFile
  }
  
  static func createEngines(
    from resources: [AdBlockEngineManager.ResourceWithVersion]
  ) async -> (engines: [CachedAdBlockEngine], compileResults: [AdBlockEngineManager.ResourceWithVersion: Result<Void, Error>]) {
    let groupedResources = Dictionary(grouping: resources, by: \.resource.source)
    
    let enginesWithCompileResults = await groupedResources.asyncConcurrentMap { source, resources -> (engine: CachedAdBlockEngine, compileResults: [AdBlockEngineManager.ResourceWithVersion: Result<Void, Error>]) in
      let results = await CachedAdBlockEngine.createEngine(from: resources, source: source)
      return (results.engine, results.compileResults)
    }
    
    var allCompileResults: [AdBlockEngineManager.ResourceWithVersion: Result<Void, Error>] = [:]
    let engines = enginesWithCompileResults.map({ $0.engine })
    
    for result in enginesWithCompileResults {
      for compileResult in result.1 {
        allCompileResults[compileResult.key] = compileResult.value
      }
    }
    
    return (engines, allCompileResults)
  }
  
  /// Create an engine from the given resources.
  ///
  /// - Warning: You should only have at max dat file in this list. Each dat file can only represent a single engine
  public static func createEngine(from resources: [AdBlockEngineManager.ResourceWithVersion]) -> (engine: AdblockEngine, compileResults: [AdBlockEngineManager.ResourceWithVersion: Result<Void, Error>]) {
    let combinedRuleLists = Self.combineAllRuleLists(from: resources)
    // Create an engine with the combined rule lists
    let engine = AdblockEngine(rules: combinedRuleLists)
    // Compile remaining resources
    let compileResults = engine.compile(resources: resources)
    // Return the compiled data
    return (engine, compileResults)
  }
  
  /// Combine all resources of type rule lists to one single string
  private static func combineAllRuleLists(from resourcesWithVersion: [AdBlockEngineManager.ResourceWithVersion]) -> String {
    // Combine all rule lists that need to be injected during initialization
    let allResults = resourcesWithVersion.compactMap { resourceWithVersion -> String? in
      switch resourceWithVersion.resource.type {
      case .ruleList:
        guard let data = FileManager.default.contents(atPath: resourceWithVersion.fileURL.path) else {
          return nil
        }
        
        return String(data: data, encoding: .utf8)
      case .dat, .jsonResources:
        return nil
      }
    }
    
    let combinedRules = allResults.joined(separator: "\n")
    return combinedRules
  }
  
  /// Compile all the resources on a detached task
  func compile(resources: [AdBlockEngineManager.ResourceWithVersion]) -> [AdBlockEngineManager.ResourceWithVersion: Result<Void, Error>] {
    var compileResults: [AdBlockEngineManager.ResourceWithVersion: Result<Void, Error>] = [:]
    
    for resourceWithVersion in resources {
      do {
        try self.compile(resource: resourceWithVersion)
        compileResults[resourceWithVersion] = .success(Void())
      } catch {
        compileResults[resourceWithVersion] = .failure(error)
      }
    }
    
    return compileResults
  }
  
  /// Compile the given resource into the given engine
  func compile(resource: AdBlockEngineManager.ResourceWithVersion) throws {
    switch resource.resource.type {
    case .dat:
      guard let data = FileManager.default.contents(atPath: resource.fileURL.path) else {
        throw CompileError.fileNotFound
      }
      
      if !deserialize(data: data) {
        throw CompileError.couldNotDeserializeDATFile
      }
    case .jsonResources:
      guard let data = FileManager.default.contents(atPath: resource.fileURL.path) else {
        throw CompileError.fileNotFound
      }
      
      guard let json = try self.validateJSON(data) else {
        return
      }
      
      useResources(json)
    case .ruleList:
      // This is added during engine initialization
      break
    }
  }
  
  /// Return a `JSON` string if this data is valid
  private func validateJSON(_ data: Data) throws -> String? {
    let value = try JSONSerialization.jsonObject(with: data, options: [])
    
    if let value = value as? NSArray {
      guard value.count > 0 else { return nil }
      return String(data: data, encoding: .utf8)
    }
    
    guard let value = value as? NSDictionary else {
      throw CompileError.invalidResourceJSON
    }
      
    guard value.count > 0 else { return nil }
    return String(data: data, encoding: .utf8)
  }
}

extension AdblockEngine {
  /// Parse a `CosmeticFilterModel` from the engine
  func cosmeticFilterModel(forFrameURL frameURL: URL) throws -> CosmeticFilterModel? {
    let rules = cosmeticResourcesForURL(frameURL.absoluteString)
    guard let data = rules.data(using: .utf8) else { return nil }
    return try JSONDecoder().decode(CosmeticFilterModel.self, from: data)
  }
}
