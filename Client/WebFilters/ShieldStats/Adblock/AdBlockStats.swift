/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import BraveShared
import Combine
import BraveCore
import os.log

struct CosmeticFilterModel: Codable {
  let hideSelectors: [String]
  let styleSelectors: [String: [String]]
  let exceptions: [String]
  let injectedScript: String
  let genericHide: Bool
  
  enum CodingKeys: String, CodingKey {
    case hideSelectors = "hide_selectors"
    case styleSelectors = "style_selectors"
    case exceptions = "exceptions"
    case injectedScript = "injected_script"
    case genericHide = "generichide"
  }
  
  func makeCSSRules() -> String {
    let hideRules = hideSelectors.reduce("") { partialResult, rule in
      return [partialResult, rule, "{display: none !important}\n"].joined()
    }
    
    let styleRules = styleSelectors.reduce("") { partialResult, entry in
      let subRules = entry.value.reduce("") { partialResult, subRule in
        return [partialResult, subRule, ";"].joined()
      }
      
      return [partialResult, entry.key, "{", subRules, " !important}\n"].joined()
    }
    
    return [hideRules, styleRules].joined()
  }
}

/// An object that wraps around an `AdblockEngine` and caches some results
public class CachedAdBlockEngine {
  /// We cache the models so that they load faster when we need to poll information about the frame
  private var cachedCosmeticFilterModels = FifoDict<URL, CosmeticFilterModel?>()
  private var cachedShouldBlockResult = FifoDict<String, Bool>()
  let engine: AdblockEngine
  
  /// Returns all the models for this frame URL
  /// The results are cached per url, so you may call this method as many times for the same url without any performance implications.
  func cosmeticFilterModel(forFrameURL frameURL: URL) throws -> CosmeticFilterModel? {
    if let model = cachedCosmeticFilterModels.getElement(frameURL) {
      return model
    }
    
    let model = try engine.cosmeticFilterModel(forFrameURL: frameURL)
    cachedCosmeticFilterModels.addElement(model, forKey: frameURL)
    return model
  }
  
  /// Return the selectors that need to be hidden given the frameURL, ids and classes
  func selectorsForCosmeticRules(frameURL: URL, ids: [String], classes: [String]) throws -> [String] {
    let model = try cosmeticFilterModel(forFrameURL: frameURL)
    
    let selectorsJSON = engine.stylesheetForCosmeticRulesIncluding(
      classes: classes,
      ids: ids,
      exceptions: model?.exceptions ?? []
    )
    
    guard let data = selectorsJSON.data(using: .utf8) else { return [] }
    let decoder = JSONDecoder()
    return try decoder.decode([String].self, from: data)
  }
  
  /// Checks the general and regional engines to see if the request should be blocked
  func shouldBlock(requestURL: URL, sourceURL: URL, resourceType: AdblockEngine.ResourceType) -> Bool {
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
  
  /// Clear the caches. Useful 
  func clearCaches() {
    cachedCosmeticFilterModels = FifoDict()
    cachedShouldBlockResult = FifoDict()
  }
  
  init(engine: AdblockEngine) {
    self.engine = engine
  }
}

public class AdBlockStats {
  public static let shared = AdBlockStats()
  
  /// We cache the user scripts so that they load faster on refreshes and back and forth
  private var cachedFrameScriptTypes = FifoDict<URL, Set<UserScriptType>>()

  // Adblock engine for general adblock lists.
  private(set) var cachedEngines: [CachedAdBlockEngine]

  /// The task that downloads all the files. Can be cancelled
  private var downloadTask: AnyCancellable?

  init() {
    cachedEngines = []
  }

  static let adblockSerialQueue = DispatchQueue(label: "com.brave.adblock-dispatch-queue")
  
  func clearCaches(clearEngineCaches: Bool = true) {
    cachedFrameScriptTypes = FifoDict()
    guard clearEngineCaches else { return }
    cachedEngines.forEach({ $0.clearCaches() })
  }
  
  /// Checks the general and regional engines to see if the request should be blocked.
  ///
  /// - Note: This method is should not be synced on `AdBlockStatus.adblockSerialQueue` and the result is synced on the main thread.
  func shouldBlock(requestURL: URL, sourceURL: URL, resourceType: AdblockEngine.ResourceType, callback: @escaping (Bool) -> Void) {
    Self.adblockSerialQueue.async { [weak self] in
      let shouldBlock = self?.shouldBlock(requestURL: requestURL, sourceURL: sourceURL, resourceType: resourceType) == true
      
      DispatchQueue.main.async {
        callback(shouldBlock)
      }
    }
  }
  
  /// Checks the general and regional engines to see if the request should be blocked
  ///
  /// - Warning: This method needs to be synced on `AdBlockStatus.adblockSerialQueue`
  func shouldBlock(requestURL: URL, sourceURL: URL, resourceType: AdblockEngine.ResourceType) -> Bool {
    return cachedEngines.contains(where: { cachedEngine in
      return cachedEngine.shouldBlock(
        requestURL: requestURL,
        sourceURL: sourceURL,
        resourceType: resourceType
      )
    })
  }
  
  func set(engines: [AdblockEngine]) {
    self.cachedEngines = engines.map({ CachedAdBlockEngine(engine: $0) })
    self.clearCaches(clearEngineCaches: false)
  }
  
  /// This returns all the user script types for the given frame
  func makeEngineScriptTypes(frameURL: URL, isMainFrame: Bool) -> Set<UserScriptType> {
    if let userScriptTypes = cachedFrameScriptTypes.getElement(frameURL) {
      return userScriptTypes
    }
    
    // Grab the relevant models for this frame
    let models = cosmeticFilterModels(forFrameURL: frameURL)
    
    // Add the selectors poller scripts for this frame
    var userScriptTypes: Set<UserScriptType> = []
    
    // Add any engine scripts for this frame
    for (index, model) in models.enumerated() {
      let source = model.injectedScript
      guard !source.isEmpty else { continue }
      
      let configuration = UserScriptType.EngineScriptConfiguration(
        frameURL: frameURL, isMainFrame: isMainFrame, source: source, order: index,
        isDeAMPEnabled: Preferences.Shields.autoRedirectAMPPages.value
      )
      
      userScriptTypes.insert(.engineScript(configuration))
    }
    
    cachedFrameScriptTypes.addElement(userScriptTypes, forKey: frameURL)
    return userScriptTypes
  }
  
  /// Returns all the models for this frame URL
  func cosmeticFilterModels(forFrameURL frameURL: URL) -> [CosmeticFilterModel] {
    return cachedEngines.compactMap({ cachedEngine -> CosmeticFilterModel? in
      do {
        return try cachedEngine.cosmeticFilterModel(forFrameURL: frameURL)
      } catch {
        assertionFailure(error.localizedDescription)
        return nil
      }
    })
  }
}

private extension AdblockEngine {
  func cosmeticFilterModel(forFrameURL frameURL: URL) throws -> CosmeticFilterModel? {
    let rules = cosmeticResourcesForURL(frameURL.absoluteString)
    guard let data = rules.data(using: .utf8) else { return nil }
    return try JSONDecoder().decode(CosmeticFilterModel.self, from: data)
  }
}
