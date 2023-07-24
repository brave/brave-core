/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import BraveShared
import Combine
import BraveCore
import Data
import os.log

/// This object holds on to our adblock engines and returns information needed for stats tracking as well as some conveniences
/// for injected scripts needed during web navigation and cosmetic filters models needed by the `SelectorsPollerScript.js` script.
public class AdBlockStats {
  public static let shared = AdBlockStats()

  // Adblock engine for general adblock lists.
  private var cachedEngines: [CachedAdBlockEngine]

  init() {
    cachedEngines = []
  }
  
  /// Clear the caches on all of the engines
  func clearCaches() {
    cachedEngines.forEach({ $0.clearCaches() })
  }
  
  /// Checks the general and regional engines to see if the request should be blocked
  ///
  /// - Warning: This method needs to be synced on `AdBlockStatus.adblockSerialQueue`
  func shouldBlock(requestURL: URL, sourceURL: URL, resourceType: AdblockEngine.ResourceType, isAggressiveMode: Bool) async -> Bool {
    return await cachedEngines.asyncConcurrentMap({ cachedEngine in
      return await cachedEngine.shouldBlock(
        requestURL: requestURL,
        sourceURL: sourceURL,
        resourceType: resourceType,
        isAggressiveMode: isAggressiveMode
      )
    }).contains(where: { $0 })
  }
  
  @MainActor func set(engines: [CachedAdBlockEngine]) {
    self.cachedEngines = engines
  }
  
  /// This returns all the user script types for the given frame
  func makeEngineScriptTypes(frameURL: URL, isMainFrame: Bool, domain: Domain) async -> Set<UserScriptType> {
    // Add any engine scripts for this frame
    return await cachedEngines.enumerated().asyncMap({ (index, cachedEngine) -> Set<UserScriptType> in
      guard await cachedEngine.isEnabled(for: domain) else { return [] }
      
      do {
        return try await cachedEngine.makeEngineScriptTypes(
          frameURL: frameURL, isMainFrame: isMainFrame, domain: domain, index: index
        )
      } catch {
        assertionFailure()
        return []
      }
    }).reduce(Set<UserScriptType>(), { partialResult, scriptTypes in
      return partialResult.union(scriptTypes)
    })
  }
  
  /// Returns all the models for this frame URL
  @MainActor func cachedEngines(for domain: Domain) -> [CachedAdBlockEngine] {
    return cachedEngines.filter({ cachedEngine in
      return cachedEngine.isEnabled(for: domain)
    })
  }
  
  /// Returns all the models for this frame URL
  func cosmeticFilterModels(forFrameURL frameURL: URL, domain: Domain) async -> [CachedAdBlockEngine.CosmeticFilterModelTuple] {
    return await cachedEngines(for: domain).asyncConcurrentCompactMap { cachedEngine in
      do {
        return try await cachedEngine.cosmeticFilterModel(forFrameURL: frameURL)
      } catch {
        assertionFailure()
        return nil
      }
    }
  }
}
