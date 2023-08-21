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
  
  /// Create an engine from the given resources.
  ///
  /// - Warning: You should only have at max dat file in this list. Each dat file can only represent a single engine
  public static func createEngine(
    from resources: [AdBlockEngineManager.ResourceWithVersion],
    scripletResourcesURL: URL?
  ) throws -> AdblockEngine {
    let combinedRuleLists = Self.combineAllRuleLists(from: resources)
    // Create an engine with the combined rule lists
    let engine = try AdblockEngine(rules: combinedRuleLists)
    // Compile remaining resources
    try engine.compile(resources: resources)

    // Add scriplets if available
    if let scripletResourcesURL = scripletResourcesURL,
       let data = FileManager.default.contents(atPath: scripletResourcesURL.path),
       let json = try validateJSON(data) {
      engine.useResources(json)
    }

    // Return the compiled data
    return engine
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
      case .dat:
        return nil
      }
    }
    
    let combinedRules = allResults.joined(separator: "\n")
    return combinedRules
  }
  
  /// Compile all the resources on a detached task
  private func compile(resources: [AdBlockEngineManager.ResourceWithVersion]) throws {
    for resourceWithVersion in resources {
      try self.compile(resource: resourceWithVersion)
    }
  }
  
  /// Compile the given resource into the given engine
  private func compile(resource: AdBlockEngineManager.ResourceWithVersion) throws {
    switch resource.resource.type {
    case .dat:
      guard let data = FileManager.default.contents(atPath: resource.fileURL.path) else {
        throw CompileError.fileNotFound
      }
      
      if !deserialize(data: data) {
        throw CompileError.couldNotDeserializeDATFile
      }
      
    case .ruleList:
      // This is added during engine initialization
      break
    }
  }
  
  /// Return a `JSON` string if this data is valid
  private static func validateJSON(_ data: Data) throws -> String? {
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
