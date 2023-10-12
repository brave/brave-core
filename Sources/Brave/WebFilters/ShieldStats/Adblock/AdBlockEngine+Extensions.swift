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
  
  convenience init(textFileURL fileURL: URL, resourcesFileURL: URL) throws {
    try self.init(rules: String(contentsOf: fileURL))
    try useResources(fromFileURL: resourcesFileURL)
  }
  
  convenience init(datFileURL fileURL: URL, resourcesFileURL: URL) throws {
    self.init()
    
    if try !deserialize(data: Data(contentsOf: fileURL)) {
      throw CompileError.couldNotDeserializeDATFile
    }
    
    try useResources(fromFileURL: resourcesFileURL)
  }
  
  /// Combine all resources of type rule lists to one single string
  private static func combineAllRuleLists(from infos: [CachedAdBlockEngine.FilterListInfo]) -> String {
    // Combine all rule lists that need to be injected during initialization
    let allResults = infos.compactMap { info -> String? in
      switch info.fileType {
      case .text:
        guard let data = FileManager.default.contents(atPath: info.localFileURL.path) else {
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
  
  private func useResources(fromFileURL fileURL: URL) throws {
    // Add scriplets if available
    if let json = try Self.validateJSON(Data(contentsOf: fileURL)) {
      useResources(json)
    }
  }
  
  /// Return a `JSON` string if this data is valid
  static func validateJSON(_ data: Data) throws -> String? {
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
