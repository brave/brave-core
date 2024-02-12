// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Data
import Foundation
import Preferences
import os

/// This object holds on to our adblock engines and returns information needed for stats tracking as well as some conveniences
/// for injected scripts needed during web navigation and cosmetic filters models needed by the `SelectorsPollerScript.js` script.
@MainActor public class AdBlockGroupsManager {
  typealias CosmeticFilterModelTuple = (isAlwaysAggressive: Bool, model: CosmeticFilterModel)
  public static let shared = AdBlockGroupsManager(
    standardManager: GroupedAdBlockEngine.EngineType.standard.makeDefaultManager(),
    aggressiveManager: GroupedAdBlockEngine.EngineType.aggressive.makeDefaultManager()
  )

  private let standardManager: AdBlockEngineManager
  private let aggressiveManager: AdBlockEngineManager

  private var allManagers: [AdBlockEngineManager] {
    return [standardManager, aggressiveManager]
  }

  /// The info for the resource file. This is a shared file used by all filter lists that contain scriplets. This information is used for lazy loading.
  public var resourcesInfo: GroupedAdBlockEngine.ResourcesInfo?

  init(standardManager: AdBlockEngineManager, aggressiveManager: AdBlockEngineManager) {
    self.standardManager = standardManager
    self.aggressiveManager = aggressiveManager
    self.resourcesInfo = nil
  }

  /// Handle memory warnings by freeing up some memory
  func didReceiveMemoryWarning() async {
    standardManager.engine?.clearCaches()
    aggressiveManager.engine?.clearCaches()
  }

  /// Load any cache data so its ready right during launch
  func loadFromCache() async {
    if let resourcesFolderURL = FilterListSetting.makeFolderURL(
      forComponentFolderPath: Preferences.AppState.lastAdBlockResourcesFolderPath.value
    ), FileManager.default.fileExists(atPath: resourcesFolderURL.path),
      let resourcesInfo = getResourcesInfo(fromFolderURL: resourcesFolderURL)
    {
      // We need this for all filter lists so we can't compile anything until we download it
      self.resourcesInfo = resourcesInfo
    }

    await allManagers.asyncConcurrentForEach { manager in
      guard manager.engineType.loadFromCache else { return }

      do {
        try await manager.loadFromCache(resourcesInfo: self.resourcesInfo)
      } catch {
        ContentBlockerManager.log.error("Failed to load engine from cache: \(error)")
      }
    }
  }

  /// Inform this manager of updates to the resources so our engines can be updated
  func didUpdateResourcesComponent(folderURL: URL) async {
    await Task { @MainActor in
      let folderSubPath = FilterListSetting.extractFolderPath(fromComponentFolderURL: folderURL)
      Preferences.AppState.lastAdBlockResourcesFolderPath.value = folderSubPath
    }.value

    let version = folderURL.lastPathComponent
    let resourcesInfo = GroupedAdBlockEngine.ResourcesInfo(
      localFileURL: folderURL.appendingPathComponent("resources.json", conformingTo: .json),
      version: version
    )

    updateIfNeeded(resourcesInfo: resourcesInfo)
  }

  /// Handle updated filter list info
  func updated(
    filterListInfo: GroupedAdBlockEngine.FilterListInfo,
    engineType: GroupedAdBlockEngine.EngineType
  ) async {
    let manager = getManager(for: engineType)
    // Always update the info on the manager
    manager.add(info: filterListInfo)

    if manager.checkNeedsCompile() {
      manager.compileDelayed(resourcesInfo: resourcesInfo)
    }

    // Compile content blockers if this filter list is enabled
    if manager.isEnabled(source: filterListInfo.source) {
      await manager.ensureContentBlockers(for: filterListInfo)
    }
  }

  /// Ensure all engines and content blockers are compiled
  func ensureEnabledEngines() async {
    guard let resourcesInfo = self.resourcesInfo else { return }

    await allManagers.asyncConcurrentForEach { manager in
      // Compile engines
      do {
        if manager.checkNeedsCompile() {
          try await manager.compileAvailable(resourcesInfo: resourcesInfo)
        }
      } catch {
        // Ignore cancellation errors
      }

      // Compile all content blockers for the given manager
      await manager.compilableInfos.asyncForEach { filterListInfo in
        await manager.ensureContentBlockers(for: filterListInfo)
      }
    }
  }

  private func getResourcesInfo(fromFolderURL folderURL: URL) -> GroupedAdBlockEngine.ResourcesInfo?
  {
    let version = folderURL.lastPathComponent
    return GroupedAdBlockEngine.ResourcesInfo(
      localFileURL: folderURL.appendingPathComponent("resources.json", conformingTo: .json),
      version: version
    )
  }

  /// Add or update `resourcesInfo` if it is a newer version. This information is used for lazy loading.
  private func updateIfNeeded(resourcesInfo: GroupedAdBlockEngine.ResourcesInfo) {
    guard self.resourcesInfo == nil || resourcesInfo.version > self.resourcesInfo!.version else {
      return
    }
    self.resourcesInfo = resourcesInfo

    allManagers.forEach { manager in
      do {
        try manager.update(resourcesInfo: resourcesInfo)
      } catch {

      }
    }

    if #available(iOS 16.0, *) {
      ContentBlockerManager.log.debug(
        "Updated resources component: `\(resourcesInfo.localFileURL.path(percentEncoded: false))`"
      )
    }
  }

  /// Checks the general and regional engines to see if the request should be blocked
  func shouldBlock(
    requestURL: URL,
    sourceURL: URL,
    resourceType: AdblockEngine.ResourceType,
    domain: Domain
  ) async -> Bool {
    return await cachedEngines(for: domain).asyncConcurrentMap({ cachedEngine in
      return cachedEngine.shouldBlock(
        requestURL: requestURL,
        sourceURL: sourceURL,
        resourceType: resourceType,
        isAggressiveMode: domain.blockAdsAndTrackingLevel.isAggressive
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
        return try cachedEngine.makeEngineScriptTypes(
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
  @MainActor func cachedEngines(for domain: Domain) async -> [GroupedAdBlockEngine] {
    guard domain.isShieldExpected(.adblockAndTp, considerAllShieldsOption: true) else { return [] }
    return allManagers.compactMap({ $0.engine })
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
        return (cachedEngine.type.isAlwaysAggressive, model)
      } catch {
        assertionFailure()
        return nil
      }
    }
  }

  private func getManager(for engineType: GroupedAdBlockEngine.EngineType) -> AdBlockEngineManager {
    switch engineType {
    case .standard: return standardManager
    case .aggressive: return aggressiveManager
    }
  }
}

extension GroupedAdBlockEngine.EngineType {
  fileprivate var defaultCachedFolderName: String {
    switch self {
    case .standard: return "standard"
    case .aggressive: return "aggressive"
    }
  }

  fileprivate var loadFromCache: Bool {
    switch self {
    case .standard: return true
    case .aggressive: return false
    }
  }

  @MainActor fileprivate func makeDefaultManager() -> AdBlockEngineManager {
    return AdBlockEngineManager(engineType: self, cacheFolderName: defaultCachedFolderName)
  }
}
