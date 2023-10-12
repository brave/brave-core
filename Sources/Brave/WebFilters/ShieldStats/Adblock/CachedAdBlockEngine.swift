// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import Data
import Preferences

/// An object that wraps around an `AdblockEngine` and caches some results
/// and ensures information is always returned on the correct thread on the engine.
public class CachedAdBlockEngine {
  public enum Source: Hashable, CustomDebugStringConvertible {
    case adBlock
    case filterList(componentId: String)
    case filterListURL(uuid: String)
    
    public var debugDescription: String {
      switch self {
      case .adBlock: return "adBlock"
      case .filterList(let componentId): return "filterList(\(componentId))"
      case .filterListURL(let uuid): return "filterListURL(\(uuid))"
      }
    }
  }
  
  public enum FileType: Hashable, CustomDebugStringConvertible {
    case dat, text
    
    public var debugDescription: String {
      switch self {
      case .dat: return "dat"
      case .text: return "txt"
      }
    }
  }
  
  public struct FilterListInfo: Hashable, Equatable, CustomDebugStringConvertible {
    let source: Source
    let localFileURL: URL
    let version: String
    let fileType: FileType
    
    public var debugDescription: String {
      return "`\(source.debugDescription)` v\(version) (\(fileType.debugDescription))"
    }
  }
  
  public struct ResourcesInfo: Hashable, Equatable {
    let localFileURL: URL
    let version: String
  }
  
  /// We cache the models so that they load faster when we need to poll information about the frame
  private var cachedCosmeticFilterModels = FifoDict<URL, CosmeticFilterModel?>()
  /// We cache the models so that they load faster when doing stats tracking or request blocking
  private var cachedShouldBlockResult = FifoDict<String, Bool>()
  /// We cache the user scripts so that they load faster on refreshes and back and forth
  private var cachedFrameScriptTypes = FifoDict<URL, Set<UserScriptType>>()
  
  private let engine: AdblockEngine
  private let serialQueue: DispatchQueue
  
  let isAlwaysAggressive: Bool
  let filterListInfo: FilterListInfo
  let resourcesInfo: ResourcesInfo
  
  init(engine: AdblockEngine, filterListInfo: FilterListInfo, resourcesInfo: ResourcesInfo, serialQueue: DispatchQueue, isAlwaysAggressive: Bool) {
    self.engine = engine
    self.filterListInfo = filterListInfo
    self.resourcesInfo = resourcesInfo
    self.serialQueue = serialQueue
    self.isAlwaysAggressive = isAlwaysAggressive
  }
  
  /// Checks the general and regional engines to see if the request should be blocked.
  func shouldBlock(requestURL: URL, sourceURL: URL, resourceType: AdblockEngine.ResourceType, isAggressiveMode: Bool) async -> Bool {
    return await withCheckedContinuation { continuation in
      serialQueue.async { [weak self] in
        let shouldBlock = self?.shouldBlock(
          requestURL: requestURL, sourceURL: sourceURL, resourceType: resourceType,
          isAggressiveMode: isAggressiveMode
        ) == true
        
        continuation.resume(returning: shouldBlock)
      }
    }
  }
  
  /// Returns all the models for this frame URL
  /// The results are cached per url, so you may call this method as many times for the same url without any performance implications.
  func cosmeticFilterModel(forFrameURL frameURL: URL) async throws -> CosmeticFilterModel? {
    return try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<CosmeticFilterModel?, Error>) in
      serialQueue.async { [weak self] in
        guard let self = self else {
          continuation.resume(returning: nil)
          return
        }
        
        do {
          if let model = try self.cachedCosmeticFilterModel(forFrameURL: frameURL) {
            continuation.resume(returning: model)
          } else {
            continuation.resume(returning: nil)
          }
        } catch {
          continuation.resume(throwing: error)
        }
      }
    }
  }
  
  /// Return the selectors that need to be hidden given the frameURL, ids and classes
  func selectorsForCosmeticRules(frameURL: URL, ids: [String], classes: [String]) async throws -> Set<String>? {
    return try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Set<String>?, Error>) in
      serialQueue.async { [weak self] in
        guard let self = self else {
          continuation.resume(returning: nil)
          return
        }
        
        do {
          let model = try self.cachedCosmeticFilterModel(forFrameURL: frameURL)
          
          let selectors = try self.engine.stylesheetForCosmeticRulesIncluding(
            classes: classes, ids: ids, exceptions: model?.exceptions ?? []
          )

          continuation.resume(returning: Set(selectors))
        } catch {
          continuation.resume(throwing: error)
        }
      }
    }
  }
  
  /// Return a cosmetic filter modelf or the given frameURL
  ///
  /// - Warning: The caller is responsible for syncing on the `serialQueue`
  private func cachedCosmeticFilterModel(forFrameURL frameURL: URL) throws -> CosmeticFilterModel? {
    if let result = self.cachedCosmeticFilterModels.getElement(frameURL) {
      return result
    }
    
    let model = try self.engine.cosmeticFilterModel(forFrameURL: frameURL)
    self.cachedCosmeticFilterModels.addElement(model, forKey: frameURL)
    return model
  }
  
  /// Checks the general and regional engines to see if the request should be blocked
  private func shouldBlock(requestURL: URL, sourceURL: URL, resourceType: AdblockEngine.ResourceType, isAggressiveMode: Bool) -> Bool {
    let key = [requestURL.absoluteString, sourceURL.absoluteString, resourceType.rawValue].joined(separator: "_")
    
    if let cachedResult = cachedShouldBlockResult.getElement(key) {
        return cachedResult
    }
    
    let shouldBlock = engine.shouldBlock(
      requestURL: requestURL,
      sourceURL: sourceURL,
      resourceType: resourceType,
      isAggressive: isAggressiveMode || self.isAlwaysAggressive
    )
    
    cachedShouldBlockResult.addElement(shouldBlock, forKey: key)
    return shouldBlock
  }
  
  /// This returns all the user script types for the given frame
  @MainActor func makeEngineScriptTypes(frameURL: URL, isMainFrame: Bool, domain: Domain, index: Int) async throws -> Set<UserScriptType> {
    if let userScriptTypes = cachedFrameScriptTypes.getElement(frameURL) {
      return userScriptTypes
    }
    
    // Add the selectors poller scripts for this frame
    var userScriptTypes: Set<UserScriptType> = []
    
    if let source = try await cosmeticFilterModel(forFrameURL: frameURL)?.injectedScript, !source.isEmpty {
      let configuration = UserScriptType.EngineScriptConfiguration(
        frameURL: frameURL, isMainFrame: isMainFrame, source: source, order: index,
        isDeAMPEnabled: Preferences.Shields.autoRedirectAMPPages.value
      )
      
      userScriptTypes.insert(.engineScript(configuration))
    }
      
    cachedFrameScriptTypes.addElement(userScriptTypes, forKey: frameURL)
    return userScriptTypes
  }
  
  /// Clear the caches.
  func clearCaches() {
    cachedCosmeticFilterModels = FifoDict()
    cachedShouldBlockResult = FifoDict()
    cachedFrameScriptTypes = FifoDict()
  }
  
  /// Create an engine from the given resources
  public static func compile(
    filterListInfo: FilterListInfo, resourcesInfo: ResourcesInfo, isAlwaysAggressive: Bool
  ) throws -> CachedAdBlockEngine {
    switch filterListInfo.fileType {
    case .dat:
      let engine = try AdblockEngine(datFileURL: filterListInfo.localFileURL, resourcesFileURL: resourcesInfo.localFileURL)
      let serialQueue = DispatchQueue(label: "com.brave.WrappedAdBlockEngine.\(UUID().uuidString)")
      return CachedAdBlockEngine(
        engine: engine, filterListInfo: filterListInfo, resourcesInfo: resourcesInfo,
        serialQueue: serialQueue, isAlwaysAggressive: isAlwaysAggressive
      )
    case .text:
      let engine = try AdblockEngine(textFileURL: filterListInfo.localFileURL, resourcesFileURL: resourcesInfo.localFileURL)
      let serialQueue = DispatchQueue(label: "com.brave.WrappedAdBlockEngine.\(UUID().uuidString)")
      return CachedAdBlockEngine(
        engine: engine, filterListInfo: filterListInfo, resourcesInfo: resourcesInfo,
        serialQueue: serialQueue, isAlwaysAggressive: isAlwaysAggressive
      )
    }
  }
}
