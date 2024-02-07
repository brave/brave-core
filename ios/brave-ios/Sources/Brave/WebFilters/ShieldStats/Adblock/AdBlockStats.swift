// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Combine
import Data
import Foundation
import Shared
import os.log

/// This object holds on to our adblock engines and returns information needed for stats tracking as well as some conveniences
/// for injected scripts needed during web navigation and cosmetic filters models needed by the `SelectorsPollerScript.js` script.
public actor AdBlockStats {
  typealias CosmeticFilterModelTuple = (isAlwaysAggressive: Bool, model: CosmeticFilterModel)
  public static let shared = AdBlockStats()

  /// An object containing the basic information to allow us to compile an engine
  public struct LazyFilterListInfo {
    let filterListInfo: CachedAdBlockEngine.FilterListInfo
    let isAlwaysAggressive: Bool
  }

  /// A list of filter list info that are available for compilation. This information is used for lazy loading.
  private(set) var availableFilterLists: [CachedAdBlockEngine.Source: LazyFilterListInfo]
  /// The info for the resource file. This is a shared file used by all filter lists that contain scriplets. This information is used for lazy loading.
  public private(set) var resourcesInfo: CachedAdBlockEngine.ResourcesInfo?
  /// Adblock engine for general adblock lists.
  private(set) var cachedEngines: [CachedAdBlockEngine.Source: CachedAdBlockEngine]
  /// The current task that is compiling.
  private var currentCompileTask: Task<(), Never>?

  /// Return an array of all sources that are enabled according to user's settings
  /// - Note: This does not take into account the domain or global adblock toggle
  @MainActor var enabledSources: [CachedAdBlockEngine.Source] {
    var enabledSources = FilterListStorage.shared.enabledSources
    enabledSources.append(contentsOf: CustomFilterListStorage.shared.enabledSources)
    return enabledSources
  }

  init() {
    cachedEngines = [:]
    availableFilterLists = [:]
  }

  /// Handle memory warnings by freeing up some memory
  func didReceiveMemoryWarning() async {
    cachedEngines.values.forEach({ $0.clearCaches() })
    await removeDisabledEngines()
  }

  /// Create and add an engine from the given resources.
  /// If an engine already exists for the given source, it will be replaced.
  public func compile(
    lazyInfo: LazyFilterListInfo,
    resourcesInfo: CachedAdBlockEngine.ResourcesInfo,
    compileContentBlockers: Bool
  ) async {
    await currentCompileTask?.value

    currentCompileTask = Task.detached {
      // Compile engine
      if await self.needsCompilation(for: lazyInfo.filterListInfo, resourcesInfo: resourcesInfo) {
        do {
          let engine = try CachedAdBlockEngine.compile(
            filterListInfo: lazyInfo.filterListInfo,
            resourcesInfo: resourcesInfo,
            isAlwaysAggressive: lazyInfo.isAlwaysAggressive
          )

          await self.add(engine: engine)
        } catch {
          ContentBlockerManager.log.error(
            "Failed to compile engine for \(lazyInfo.filterListInfo.source.debugDescription)"
          )
        }
      }

      // Compile content blockers
      if compileContentBlockers, let blocklistType = lazyInfo.blocklistType {
        let modes = await ContentBlockerManager.shared.missingModes(for: blocklistType)
        guard !modes.isEmpty else { return }

        do {
          try await ContentBlockerManager.shared.compileRuleList(
            at: lazyInfo.filterListInfo.localFileURL,
            for: blocklistType,
            modes: modes
          )
        } catch {
          ContentBlockerManager.log.error(
            "Failed to compile rule list for \(lazyInfo.filterListInfo.source.debugDescription)"
          )
        }
      }
    }

    await currentCompileTask?.value
  }

  /// Add a new engine to the list.
  /// If an engine already exists for the same source, it will be replaced instead.
  private func add(engine: CachedAdBlockEngine) {
    cachedEngines[engine.filterListInfo.source] = engine
    updateIfNeeded(resourcesInfo: engine.resourcesInfo)
    updateIfNeeded(
      filterListInfo: engine.filterListInfo,
      isAlwaysAggressive: engine.isAlwaysAggressive
    )
    ContentBlockerManager.log.debug("Added engine for \(engine.filterListInfo.debugDescription)")
  }

  /// Add or update `filterListInfo` if it is a newer version. This information is used for lazy loading.
  func updateIfNeeded(filterListInfo: CachedAdBlockEngine.FilterListInfo, isAlwaysAggressive: Bool)
  {
    if let existingLazyInfo = availableFilterLists[filterListInfo.source] {
      guard filterListInfo.version > existingLazyInfo.filterListInfo.version else { return }
    }

    availableFilterLists[filterListInfo.source] = LazyFilterListInfo(
      filterListInfo: filterListInfo,
      isAlwaysAggressive: isAlwaysAggressive
    )
  }

  /// Add or update `resourcesInfo` if it is a newer version. This information is used for lazy loading.
  func updateIfNeeded(resourcesInfo: CachedAdBlockEngine.ResourcesInfo) {
    guard self.resourcesInfo == nil || resourcesInfo.version > self.resourcesInfo!.version else {
      return
    }
    self.resourcesInfo = resourcesInfo

    if #available(iOS 16.0, *) {
      ContentBlockerManager.log.debug(
        "Updated resources component: `\(resourcesInfo.localFileURL.path(percentEncoded: false))`"
      )
    }
  }

  /// Remove all the engines
  func removeAllEngines() {
    cachedEngines.removeAll()
  }

  /// Remove all engines that have disabled sources
  func removeDisabledEngines() async {
    let sources = await Set(enabledSources)

    for source in cachedEngines.keys {
      guard !sources.contains(source) else { continue }
      // Remove the engine
      if let filterListInfo = cachedEngines[source]?.filterListInfo {
        cachedEngines.removeValue(forKey: source)
        ContentBlockerManager.log.debug("Removed engine for \(filterListInfo.debugDescription)")
      }

      // Delete the Content blockers
      if let lazyInfo = availableFilterLists[source], let blocklistType = lazyInfo.blocklistType {
        do {
          try await ContentBlockerManager.shared.removeRuleLists(for: blocklistType)
        } catch {
          ContentBlockerManager.log.error(
            "Failed to remove rule lists for \(lazyInfo.filterListInfo.debugDescription)"
          )
        }
      }
    }
  }

  /// Remove all engines that have disabled sources
  func ensureEnabledEngines() async {
    do {
      for source in await enabledSources {
        guard cachedEngines[source] == nil else { continue }
        guard let availableFilterList = availableFilterLists[source] else { continue }
        guard let resourcesInfo = self.resourcesInfo else { continue }

        await compile(
          lazyInfo: availableFilterList,
          resourcesInfo: resourcesInfo,
          compileContentBlockers: true
        )

        // Sleep for 1ms. This drastically reduces memory usage without much impact to usability
        try await Task.sleep(nanoseconds: 1_000_000)
      }
    } catch {
      // Ignore cancellation errors
    }
  }

  /// Tells us if this source should be loaded.
  @MainActor func isEnabled(source: CachedAdBlockEngine.Source) -> Bool {
    return enabledSources.contains(source)
  }

  /// Tells us if an engine needs compilation if it's missing or if its resources are outdated
  func needsCompilation(
    for filterListInfo: CachedAdBlockEngine.FilterListInfo,
    resourcesInfo: CachedAdBlockEngine.ResourcesInfo
  ) -> Bool {
    if let cachedEngine = cachedEngines[filterListInfo.source] {
      return cachedEngine.filterListInfo.version < filterListInfo.version
        && cachedEngine.resourcesInfo.version < resourcesInfo.version
    } else {
      return true
    }
  }

  /// Checks the general and regional engines to see if the request should be blocked
  func shouldBlock(
    requestURL: URL,
    sourceURL: URL,
    resourceType: AdblockEngine.ResourceType,
    isAggressiveMode: Bool
  ) async -> Bool {
    let sources = await self.enabledSources
    return await cachedEngines(for: sources).asyncConcurrentMap({ cachedEngine in
      return cachedEngine.shouldBlock(
        requestURL: requestURL,
        sourceURL: sourceURL,
        resourceType: resourceType,
        isAggressiveMode: isAggressiveMode
      )
    }).contains(where: { $0 })
  }

  /// This returns all the user script types for the given frame
  func makeEngineScriptTypes(
    frameURL: URL,
    isMainFrame: Bool,
    domain: Domain
  ) async -> Set<UserScriptType> {
    // Add any engine scripts for this frame
    return await cachedEngines(for: domain).enumerated().asyncMap({
      index,
      cachedEngine -> Set<UserScriptType> in
      do {
        return try await cachedEngine.makeEngineScriptTypes(
          frameURL: frameURL,
          isMainFrame: isMainFrame,
          domain: domain,
          index: index
        )
      } catch {
        assertionFailure()
        return []
      }
    }).reduce(
      Set<UserScriptType>(),
      { partialResult, scriptTypes in
        return partialResult.union(scriptTypes)
      }
    )
  }

  /// Returns all appropriate engines for the given domain
  @MainActor func cachedEngines(for domain: Domain) async -> [CachedAdBlockEngine] {
    let sources = enabledSources(for: domain)
    return await cachedEngines(for: sources)
  }

  /// Return all the cached engines for the given sources. If any filter list is not yet loaded, it will be lazily loaded
  private func cachedEngines(for sources: [CachedAdBlockEngine.Source]) -> [CachedAdBlockEngine] {
    return sources.compactMap { source -> CachedAdBlockEngine? in
      return cachedEngines[source]
    }
  }

  /// Returns all the models for this frame URL
  func cosmeticFilterModels(
    forFrameURL frameURL: URL,
    domain: Domain
  ) async -> [CosmeticFilterModelTuple] {
    return await cachedEngines(for: domain).asyncConcurrentCompactMap {
      cachedEngine -> CosmeticFilterModelTuple? in
      do {
        guard let model = try cachedEngine.cosmeticFilterModel(forFrameURL: frameURL) else {
          return nil
        }
        return (cachedEngine.isAlwaysAggressive, model)
      } catch {
        assertionFailure()
        return nil
      }
    }
  }

  /// Give us all the enabled sources for the given domain
  @MainActor private func enabledSources(for domain: Domain) -> [CachedAdBlockEngine.Source] {
    let enabledSources = self.enabledSources
    return enabledSources.filter({ $0.isEnabled(for: domain) })
  }
}

extension FilterListSetting {
  @MainActor var engineSource: CachedAdBlockEngine.Source? {
    guard let componentId = componentId else { return nil }
    return .filterList(componentId: componentId, uuid: uuid)
  }
}

extension FilterList {
  @MainActor var engineSource: CachedAdBlockEngine.Source {
    return .filterList(componentId: entry.componentId, uuid: self.entry.uuid)
  }
}

extension CustomFilterListSetting {
  @MainActor var engineSource: CachedAdBlockEngine.Source {
    return .filterListURL(uuid: uuid)
  }
}

extension CachedAdBlockEngine.Source {
  /// Returns a boolean indicating if the engine is enabled for the given domain.
  ///
  /// This is determined by checking the source of the engine and checking the appropriate shields.
  @MainActor fileprivate func isEnabled(for domain: Domain) -> Bool {
    switch self {
    case .filterList, .filterListURL:
      // This engine source type is enabled only if shields are enabled
      // for the given domain
      return domain.isShieldExpected(.adblockAndTp, considerAllShieldsOption: true)
    }
  }
}

extension AdBlockStats.LazyFilterListInfo {
  var blocklistType: ContentBlockerManager.BlocklistType? {
    switch filterListInfo.source {
    case .filterList(let componentId, let uuid):
      guard uuid != FilterList.defaultComponentUUID else {
        // For now we don't compile this into content blockers because we use the one coming from slim list
        // We might change this in the future as it ends up with 95k items whereas the limit is 150k.
        // So there is really no reason to use slim list except perhaps for performance which we need to test out.
        return nil
      }

      return .filterList(componentId: componentId, isAlwaysAggressive: isAlwaysAggressive)
    case .filterListURL(let uuid):
      return .customFilterList(uuid: uuid)
    }
  }
}
