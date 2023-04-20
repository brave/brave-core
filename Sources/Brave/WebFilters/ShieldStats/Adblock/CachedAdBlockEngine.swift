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
  /// We cache the models so that they load faster when we need to poll information about the frame
  private var cachedCosmeticFilterModels = FifoDict<URL, CosmeticFilterModel?>()
  /// We cache the models so that they load faster when doing stats tracking or request blocking
  private var cachedShouldBlockResult = FifoDict<String, Bool>()
  /// We cache the user scripts so that they load faster on refreshes and back and forth
  private var cachedFrameScriptTypes = FifoDict<URL, Set<UserScriptType>>()
  
  private let engine: AdblockEngine
  private let source: AdBlockEngineManager.Source
  private let serialQueue: DispatchQueue
  
  init(engine: AdblockEngine, source: AdBlockEngineManager.Source, serialQueue: DispatchQueue) {
    self.engine = engine
    self.source = source
    self.serialQueue = serialQueue
  }
  
  /// Checks the general and regional engines to see if the request should be blocked.
  func shouldBlock(requestURL: URL, sourceURL: URL, resourceType: AdblockEngine.ResourceType) async -> Bool {
    return await withCheckedContinuation { continuation in
      serialQueue.async { [weak self] in
        let shouldBlock = self?.shouldBlock(requestURL: requestURL, sourceURL: sourceURL, resourceType: resourceType) == true
        continuation.resume(returning: shouldBlock)
      }
    }
  }
  
  /// Returns all the models for this frame URL
  /// The results are cached per url, so you may call this method as many times for the same url without any performance implications.
  func cosmeticFilterModel(forFrameURL frameURL: URL) async throws -> CosmeticFilterModel? {
    return try await withCheckedThrowingContinuation { continuation in
      serialQueue.async { [weak self] in
        if let model = self?.cachedCosmeticFilterModels.getElement(frameURL) {
          continuation.resume(returning: model)
          return
        }
        
        do {
          let model = try self?.engine.cosmeticFilterModel(forFrameURL: frameURL)
          self?.cachedCosmeticFilterModels.addElement(model, forKey: frameURL)
          continuation.resume(returning: model)
        } catch {
          continuation.resume(throwing: error)
        }
      }
    }
  }
  
  /// Return the selectors that need to be hidden given the frameURL, ids and classes
  func selectorsForCosmeticRules(frameURL: URL, ids: [String], classes: [String]) async throws -> [String] {
    let exceptions = try await cosmeticFilterModel(forFrameURL: frameURL)?.exceptions ?? []

    return try await withCheckedThrowingContinuation { continuation in
      serialQueue.async { [weak self] in
        let selectorsJSON = self?.engine.stylesheetForCosmeticRulesIncluding(classes: classes, ids: ids, exceptions: exceptions)

        guard let data = selectorsJSON?.data(using: .utf8) else {
          continuation.resume(returning: [])
          return
        }
        
        let decoder = JSONDecoder()
        
        do {
          let result = try decoder.decode([String].self, from: data)
          continuation.resume(returning: result)
        } catch {
          continuation.resume(throwing: error)
        }
      }
    }
  }
  
  /// Checks the general and regional engines to see if the request should be blocked
  private func shouldBlock(requestURL: URL, sourceURL: URL, resourceType: AdblockEngine.ResourceType) -> Bool {
    let key = [requestURL.absoluteString, sourceURL.absoluteString, resourceType.rawValue].joined(separator: "_")
    
    if let cachedResult = cachedShouldBlockResult.getElement(key) {
        return cachedResult
    }
    
    let shouldBlock = engine.shouldBlock(
      requestURL: requestURL,
      sourceURL: sourceURL,
      resourceType: resourceType
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
  
  /// Returns a boolean indicating if the engine is enabled for the given domain.
  ///
  /// This is determined by checking the source of the engine and checking the appropriate shields.
  @MainActor func isEnabled(for domain: Domain) -> Bool {
    switch source {
    case .adBlock, .filterList, .filterListURL:
      // This engine source type is enabled only if shields are enabled
      // for the given domain
      return domain.isShieldExpected(.AdblockAndTp, considerAllShieldsOption: true)
    case .cosmeticFilters:
      // The cosmetic filters are always applied
      return true
    }
  }
  
  /// Create an engine from the given resources
  static func createEngine(from resources: [AdBlockEngineManager.ResourceWithVersion], source: AdBlockEngineManager.Source) async -> (engine: CachedAdBlockEngine, compileResults: [AdBlockEngineManager.ResourceWithVersion: Result<Void, Error>]) {
    return await withCheckedContinuation { continuation in
      let serialQueue = DispatchQueue(label: "com.brave.WrappedAdBlockEngine.\(UUID().uuidString)")
      
      serialQueue.async {
        let results = AdblockEngine.createEngine(from: resources)
        let cachedEngine = CachedAdBlockEngine(engine: results.engine, source: source, serialQueue: serialQueue)
        continuation.resume(returning: (cachedEngine, results.compileResults))
      }
    }
  }
}
