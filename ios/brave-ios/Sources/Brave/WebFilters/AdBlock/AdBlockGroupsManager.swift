// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Data
import Foundation
import Preferences
import os

/// A class that helps manage file sources and enabled filter lists so that 2 engines (standard and aggressive) are continually updated.
@MainActor public class AdBlockGroupsManager {
  typealias CosmeticFilterModelTuple = (isAlwaysAggressive: Bool, model: CosmeticFilterModel)
  public static let shared = AdBlockGroupsManager(
    standardManager: GroupedAdBlockEngine.EngineType.standard.makeDefaultManager(),
    aggressiveManager: GroupedAdBlockEngine.EngineType.aggressive.makeDefaultManager()
  )

  private let standardManager: AdBlockEngineManager
  private let aggressiveManager: AdBlockEngineManager

  /// The info for the resource file. This is a shared file used by all filter lists that contain scriplets. This information is used for lazy loading.
  public var resourcesInfo: GroupedAdBlockEngine.ResourcesInfo?

  /// Return an array of all sources that are enabled according to user's settings
  /// - Note: This does not take into account the domain or global adblock toggle
  private var enabledSources: [GroupedAdBlockEngine.Source] {
    var enabledSources = FilterListStorage.shared.enabledSources
    enabledSources.append(contentsOf: CustomFilterListStorage.shared.enabledSources)
    return enabledSources
  }

  init(standardManager: AdBlockEngineManager, aggressiveManager: AdBlockEngineManager) {
    self.standardManager = standardManager
    self.aggressiveManager = aggressiveManager
    self.resourcesInfo = nil
  }

  /// Handle memory warnings by freeing up some memory
  func didReceiveMemoryWarning() async {
    await standardManager.engine?.clearCaches()
    await aggressiveManager.engine?.clearCaches()
  }

  /// Load any cache data so its ready right during launch
  func loadResourcesFromCache() async {
    if let resourcesFolderURL = FilterListSetting.makeFolderURL(
      forComponentFolderPath: Preferences.AppState.lastAdBlockResourcesFolderPath.value
    ), FileManager.default.fileExists(atPath: resourcesFolderURL.path) {
      // We need this for all filter lists so we can't compile anything until we download it
      let resourcesInfo = getResourcesInfo(fromFolderURL: resourcesFolderURL)
      self.resourcesInfo = resourcesInfo

      if #available(iOS 16.0, *) {
        ContentBlockerManager.log.debug(
          "Loaded resources component from cache: `\(resourcesInfo.localFileURL.path(percentEncoded: false))`"
        )
      } else {
        ContentBlockerManager.log.debug(
          "Loaded resources component from cache: `\(resourcesInfo.localFileURL.path)`"
        )
      }
    }
  }

  func loadEnginesFromCache() async {
    await GroupedAdBlockEngine.EngineType.allCases.asyncConcurrentForEach { engineType in
      await self.loadEngineFromCache(for: engineType)
    }
  }

  private func loadEngineFromCache(for engineType: GroupedAdBlockEngine.EngineType) async {
    let manager = getManager(for: engineType)

    if await !manager.loadFromCache(resourcesInfo: self.resourcesInfo) {
      // If we didn't load the main engine from cache we need to load using the old cache mechanism
      // This is only temporary so we're not left with no ad-block during the upgrade.
      // We can drop all of this in future upgrades as by then we will have files cached in the new format
      for setting in FilterListStorage.shared.allFilterListSettings
        .filter({ $0.isAlwaysAggressive == engineType.isAlwaysAggressive })
        .sorted(by: { $0.order?.intValue ?? 0 <= $1.order?.intValue ?? 0 })
      {
        guard let folderURL = setting.folderURL else { continue }
        guard let source = setting.engineSource else { continue }
        guard let fileInfo = Self.fileInfo(for: source, folderURL: folderURL) else { continue }
        manager.add(fileInfo: fileInfo)
      }

      manager.compileImmediatelyIfNeeded(
        for: enabledSources,
        resourcesInfo: self.resourcesInfo,
        priority: .high
      )
    }
  }

  static func fileInfo(
    for source: GroupedAdBlockEngine.Source,
    folderURL: URL
  ) -> AdBlockEngineManager.FileInfo? {
    let version = folderURL.lastPathComponent
    let localFileURL = folderURL.appendingPathComponent("list.txt")

    guard FileManager.default.fileExists(atPath: localFileURL.relativePath) else {
      // We are loading the old component from cache. We don't want this file to be loaded.
      // When we download the new component shortly we will update our cache.
      // This should only trigger after an app update and eventually this check can be removed.
      return nil
    }

    let filterListInfo = GroupedAdBlockEngine.FilterListInfo(
      source: source,
      version: version
    )

    return AdBlockEngineManager.FileInfo(filterListInfo: filterListInfo, localFileURL: localFileURL)
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
    fileInfo: AdBlockEngineManager.FileInfo,
    engineType: GroupedAdBlockEngine.EngineType
  ) async {
    let manager = getManager(for: engineType)
    let enabledSources = enabledSources

    // Compile content blockers if this filter list is enabled
    if enabledSources.contains(fileInfo.filterListInfo.source) {
      await manager.ensureContentBlockers(for: fileInfo)
    }

    // Always update the info on the manager
    manager.add(fileInfo: fileInfo)
    manager.compileDelayedIfNeeded(
      for: enabledSources,
      resourcesInfo: resourcesInfo,
      priority: .background
    )

  }

  /// Ensure all engines and content blockers are compiled
  func compileEnginesIfNeeded() {
    let enabledSources = enabledSources
    GroupedAdBlockEngine.EngineType.allCases.forEach { engineType in
      let manager = self.getManager(for: engineType)
      manager.compileImmediatelyIfNeeded(
        for: enabledSources,
        resourcesInfo: resourcesInfo,
        priority: .high
      )
      manager.ensureContentBlockers(for: enabledSources)
    }
  }

  private func getResourcesInfo(fromFolderURL folderURL: URL) -> GroupedAdBlockEngine.ResourcesInfo
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

    GroupedAdBlockEngine.EngineType.allCases.forEach { engineType in
      let manager = self.getManager(for: engineType)

      Task {
        await manager.update(resourcesInfo: resourcesInfo)
      }
    }

    if #available(iOS 16.0, *) {
      ContentBlockerManager.log.debug(
        "Updated resources component: `\(resourcesInfo.localFileURL.path(percentEncoded: false))`"
      )
    } else {
      ContentBlockerManager.log.debug(
        "Updated resources component: `\(resourcesInfo.localFileURL.path)`"
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
      return await cachedEngine.shouldBlock(
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
    isDeAmpEnabled: Bool,
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
          isDeAmpEnabled: isDeAmpEnabled,
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
  func cachedEngines(for domain: Domain) -> [GroupedAdBlockEngine] {
    guard domain.isShieldExpected(.adblockAndTp, considerAllShieldsOption: true) else { return [] }
    return GroupedAdBlockEngine.EngineType.allCases.compactMap({ getManager(for: $0).engine })
  }

  /// Returns all the models for this frame URL
  func cosmeticFilterModels(
    forFrameURL frameURL: URL,
    domain: Domain
  ) async -> [CosmeticFilterModelTuple] {
    return await cachedEngines(for: domain).asyncConcurrentCompactMap {
      cachedEngine -> CosmeticFilterModelTuple? in
      do {
        guard let model = try await cachedEngine.cosmeticFilterModel(forFrameURL: frameURL) else {
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

  @MainActor fileprivate func makeDefaultManager() -> AdBlockEngineManager {
    return AdBlockEngineManager(engineType: self, cacheFolderName: defaultCachedFolderName)
  }
}

extension FilterListSetting {
  @MainActor var engineSource: GroupedAdBlockEngine.Source? {
    guard let componentId = componentId else { return nil }
    return .filterList(componentId: componentId, uuid: uuid)
  }
}

extension FilterList {
  @MainActor var engineSource: GroupedAdBlockEngine.Source {
    return .filterList(componentId: entry.componentId, uuid: self.entry.uuid)
  }
}
