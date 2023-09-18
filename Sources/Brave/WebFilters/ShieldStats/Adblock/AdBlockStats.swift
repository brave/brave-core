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
public actor AdBlockStats {
  typealias CosmeticFilterModelTuple = (isAlwaysAggressive: Bool, model: CosmeticFilterModel)
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
  
  func add(engine: CachedAdBlockEngine) {
    if let index = cachedEngines.firstIndex(where: { $0.filterListInfo.source == engine.filterListInfo.source }) {
      cachedEngines[index] = engine
    } else {
      cachedEngines.append(engine)
    }
  }
  
  func removeEngine(for source: CachedAdBlockEngine.Source) {
    cachedEngines.removeAll { cachedEngine in
      cachedEngine.filterListInfo.source == source
    }
  }
  
  func needsCompilation(for filterListInfo: CachedAdBlockEngine.FilterListInfo, resourcesInfo: CachedAdBlockEngine.ResourcesInfo) -> Bool {
    return !cachedEngines.contains { cachedEngine in
      if cachedEngine.filterListInfo.source == filterListInfo.source {
        return cachedEngine.filterListInfo.version < filterListInfo.version
          || cachedEngine.resourcesInfo.version < resourcesInfo.version
      } else {
        return true
      }
    }
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
  
  /// This returns all the user script types for the given frame
  func makeEngineScriptTypes(frameURL: URL, isMainFrame: Bool, domain: Domain) async -> Set<UserScriptType> {
    // Add any engine scripts for this frame
    return await cachedEngines(for: domain).enumerated().asyncMap({ index, cachedEngine -> Set<UserScriptType> in
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
  func cachedEngines(for domain: Domain) async -> [CachedAdBlockEngine] {
    let enabledSources = await enabledSources(for: domain)
    
    let engines = await cachedEngines.asyncFilter({ cachedEngine in
      return enabledSources.contains(cachedEngine.filterListInfo.source)
    })
    
    return engines
  }
  
  /// Returns all the models for this frame URL
  func cosmeticFilterModels(forFrameURL frameURL: URL, domain: Domain) async -> [CosmeticFilterModelTuple] {
    return await cachedEngines(for: domain).asyncConcurrentCompactMap { cachedEngine -> CosmeticFilterModelTuple? in
      do {
        guard let model = try await cachedEngine.cosmeticFilterModel(forFrameURL: frameURL) else {
          return nil
        }
        return (cachedEngine.isAlwaysAggressive, model)
      } catch {
        assertionFailure()
        return nil
      }
    }
  }
  
  @MainActor private func enabledSources(for domain: Domain) -> Set<CachedAdBlockEngine.Source> {
    var enabledSources = FilterListStorage.shared.enabledSources.union(
      CustomFilterListStorage.shared.enabledSources
    )
    enabledSources.insert(.adBlock)
    return enabledSources.filter({ $0.isEnabled(for: domain )})
  }
}

private extension FilterListStorage {
  @MainActor var enabledSources: Set<CachedAdBlockEngine.Source> {
    let sources = allFilterListSettings.compactMap { setting -> CachedAdBlockEngine.Source? in
      guard setting.isEnabled else { return nil }
      
      if let componentId = setting.componentId {
        return .filterList(componentId: componentId)
      } else if let filterList = filterLists.first(where: { $0.uuid == setting.uuid }) {
        return .filterList(componentId: filterList.entry.componentId)
      } else {
        return nil
      }
    }
    
    return Set(sources)
  }
}

private extension CustomFilterListStorage {
  @MainActor var enabledSources: Set<CachedAdBlockEngine.Source> {
    let sources = filterListsURLs.compactMap { filterList -> CachedAdBlockEngine.Source? in
      guard filterList.setting.isEnabled else { return nil }
      return .filterListURL(uuid: filterList.setting.uuid)
    }
    
    return Set(sources)
  }
}

private extension CachedAdBlockEngine.Source {
  /// Returns a boolean indicating if the engine is enabled for the given domain.
  ///
  /// This is determined by checking the source of the engine and checking the appropriate shields.
  @MainActor func isEnabled(for domain: Domain) -> Bool {
    switch self {
    case .adBlock, .filterList, .filterListURL:
      // This engine source type is enabled only if shields are enabled
      // for the given domain
      return domain.isShieldExpected(.AdblockAndTp, considerAllShieldsOption: true)
    }
  }
}
