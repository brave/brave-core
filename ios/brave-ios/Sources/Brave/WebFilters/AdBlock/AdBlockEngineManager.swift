// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Data
import Foundation
import Preferences
import os

/// A class for managing a single grouped engine
@MainActor class AdBlockEngineManager {
  /// The directory to which we should store the unified list into
  private static var cacheFolderDirectory: FileManager.SearchPathDirectory {
    return FileManager.SearchPathDirectory.cachesDirectory
  }

  public struct FileInfo: Hashable, Equatable {
    let filterListInfo: GroupedAdBlockEngine.FilterListInfo
    let localFileURL: URL
  }

  /// The top level folder to create for caching engine data
  private static let parentCacheFolderName = "engines"
  /// All the info that is currently available
  private var availableFiles: [FileInfo]
  /// The subfolder that the cache data is stored.
  /// Since we can have multiple engines this folder will be unique per engine
  private let cacheFolderName: String
  /// The type of engine this represents what kind of blocking we are doing. (aggressive or standard)
  /// The difference is in how we treat first party blocking. Aggressive also blocks first party content.
  let engineType: GroupedAdBlockEngine.EngineType
  /// The engine that is currently available for this manager
  private(set) var engine: GroupedAdBlockEngine?
  /// This is a task that we use to delay a requested compile.
  /// This allows us to wait for more sources and limit the number of times we compile
  private var delayTask: Task<Void, Error>?
  /// This is the current pending group we are compiling.
  /// This allows us to ensure we are not already compiling a newer version of the rules before setting the engine
  private var pendingGroup: GroupedAdBlockEngine.FilterListGroup?

  /// This structure represents encodable info on what cached engine data contains
  private struct CachedEngineInfo: Codable {
    let infos: [GroupedAdBlockEngine.FilterListInfo]
    let fileType: GroupedAdBlockEngine.FileType
  }

  /// Get the already created cache folder
  ///
  /// - Note: Returns nil if the cache folder does not exist
  private var createdCacheFolderURL: URL? {
    guard let folderURL = Self.cacheFolderDirectory.url else { return nil }
    let cacheFolderURL = folderURL.appendingPathComponent(
      Self.parentCacheFolderName,
      conformingTo: .folder
    )
    .appendingPathComponent(cacheFolderName, conformingTo: .folder)

    if FileManager.default.fileExists(atPath: cacheFolderURL.path) {
      return cacheFolderURL
    } else {
      return nil
    }
  }

  init(engineType: GroupedAdBlockEngine.EngineType, cacheFolderName: String) {
    self.availableFiles = []
    self.engine = nil
    self.engineType = engineType
    self.cacheFolderName = cacheFolderName
  }

  /// All the infos that are compilable based on the enabled sources and available infos
  func compilableFiles(
    for enabledSources: [GroupedAdBlockEngine.Source]
  ) -> [FileInfo] {
    return enabledSources.compactMap { source in
      return availableFiles.first(where: {
        FileManager.default.fileExists(atPath: $0.localFileURL.path)
          && $0.filterListInfo.source == source
      })
    }
  }

  /// Add the info to the available list
  func add(fileInfo: FileInfo) {
    removeInfo(for: fileInfo.filterListInfo.source)
    availableFiles.append(fileInfo)
  }

  /// Remove any info from the available list given by the source
  /// Mostly used for custom filter lists
  func removeInfo(for source: GroupedAdBlockEngine.Source) {
    availableFiles.removeAll { fileInfo in
      return fileInfo.filterListInfo.source == source
    }
  }

  /// Checks to see if we need to compile or recompile the engine based on the available info
  func checkNeedsCompile(for fileInfos: [AdBlockEngineManager.FileInfo]) -> Bool {
    if let pendingGroup = pendingGroup {
      return pendingGroup.infos != fileInfos.map({ $0.filterListInfo })
    }

    let compilableInfos = fileInfos.map({ $0.filterListInfo })
    guard !compilableInfos.isEmpty else { return engine?.group.infos.isEmpty == false }
    guard let engine = engine else { return true }
    return compilableInfos != engine.group.infos
  }

  /// Load the engine from cache so it can be ready during launch
  func loadFromCache(resourcesInfo: GroupedAdBlockEngine.ResourcesInfo?) async -> Bool {
    do {
      guard let cachedGroupInfo = loadCachedInfo() else { return false }

      let engineType = self.engineType
      let groupedEngine = try await Task.detached(priority: .high) {
        let engine = try GroupedAdBlockEngine.compile(
          group: cachedGroupInfo,
          type: engineType
        )

        if let resourcesInfo = resourcesInfo {
          try await engine.useResources(from: resourcesInfo)
        }

        return engine
      }.value

      self.set(engine: groupedEngine)
      return true
    } catch {
      ContentBlockerManager.log.error(
        "Failed to load engine from cache for `\(self.cacheFolderName)`: \(String(describing: error))"
      )

      return false
    }
  }

  // Delete the cache. Mostly used for testing
  func deleteCachedEngine() throws {
    guard let cacheFolderURL = createdCacheFolderURL else { return }
    try FileManager.default.removeItem(at: cacheFolderURL)
  }

  /// This will compile available data, but will wait a little bit in case something new gets downloaded.
  /// Especially needed during launch when we have a bunch of downloads coming at the same time.
  func compileDelayedIfNeeded(
    for enabledSources: [GroupedAdBlockEngine.Source],
    resourcesInfo: GroupedAdBlockEngine.ResourcesInfo?
  ) {
    // Cancel the previous task
    delayTask?.cancel()

    // Restart the task
    delayTask = Task {
      try await Task.sleep(seconds: 60)
      await compileAvailableIfNeeded(
        for: enabledSources,
        resourcesInfo: resourcesInfo
      )
    }
  }

  /// This will compile available data right away if it is needed and cancel any delayedTasks
  func compileImmediatelyIfNeeded(
    for enabledSources: [GroupedAdBlockEngine.Source],
    resourcesInfo: GroupedAdBlockEngine.ResourcesInfo?
  ) async {
    delayTask?.cancel()

    await self.compileAvailableIfNeeded(
      for: enabledSources,
      resourcesInfo: resourcesInfo
    )
  }

  /// Add or update `resourcesInfo` if it is a newer version. This information is used for lazy loading.
  func update(resourcesInfo: GroupedAdBlockEngine.ResourcesInfo) async {
    do {
      try await engine?.useResources(from: resourcesInfo)
    } catch {
      ContentBlockerManager.log.error(
        "Failed to update engine resources for `\(self.cacheFolderName)`: \(String(describing: error))"
      )
    }
  }

  func checkHasAllInfo(for sources: [GroupedAdBlockEngine.Source]) -> Bool {
    let availableSources = compilableFiles(for: sources).map({ $0.filterListInfo.source })
    return sources.allSatisfy({ availableSources.contains($0) })
  }

  /// This will compile available data right away if it is needed
  private func compileAvailableIfNeeded(
    for enabledSources: [GroupedAdBlockEngine.Source],
    resourcesInfo: GroupedAdBlockEngine.ResourcesInfo?
  ) async {
    do {
      let compilableFiles = compilableFiles(for: enabledSources)
      guard self.checkNeedsCompile(for: compilableFiles) else { return }
      try await compileAvailable(
        for: compilableFiles,
        resourcesInfo: resourcesInfo
      )
    } catch {
      ContentBlockerManager.log.error(
        "Failed to compile engine for `\(self.cacheFolderName)`: \(String(describing: error))"
      )
    }
  }

  /// Compile an engine from all available data
  private func compileAvailable(
    for files: [AdBlockEngineManager.FileInfo],
    resourcesInfo: GroupedAdBlockEngine.ResourcesInfo?
  ) async throws {
    let engineType = self.engineType
    let group = try combineRules(for: files)
    self.pendingGroup = group

    ContentBlockerManager.log.debug(
      """
      Compiling `\(self.cacheFolderName)` engine from \(group.infos.count) sources:
      \(group.debugDescription)"
      """
    )

    // 2. Compile the engine
    let groupedEngine = try await Task.detached(priority: .high) {
      let engine = try GroupedAdBlockEngine.compile(group: group, type: engineType)

      if let resourcesInfo {
        try await engine.useResources(from: resourcesInfo)
      }

      return engine
    }.value

    // 3. Ensure our file is still up to date before setting it
    // (avoid race conditiions)
    guard pendingGroup == group else { return }
    self.set(engine: groupedEngine)
    await cache(engine: groupedEngine)
    self.pendingGroup = nil
  }

  private func set(engine: GroupedAdBlockEngine) {
    let group = engine.group
    ContentBlockerManager.log.debug(
      """
      Set `\(self.cacheFolderName)` (\(group.fileType.debugDescription)) engine from \(group.infos.count) sources:
      \(group.debugDescription)
      """
    )
    self.engine = engine
  }

  /// Take all the filter lists and combine them into one then save them into a cache folder.
  private func combineRules(
    for compilableFiles: [AdBlockEngineManager.FileInfo]
  ) throws -> GroupedAdBlockEngine.FilterListGroup {
    // 1. Create a file url
    let cachedFolder = try getOrCreateCacheFolder()
    let fileURL = cachedFolder.appendingPathComponent("list.txt", conformingTo: .text)
    var compiledInfos: [GroupedAdBlockEngine.FilterListInfo] = []
    var unifiedRules = ""
    // 2. Join all the rules together
    compilableFiles.forEach { fileInfo in
      do {
        let fileContents = try String(contentsOf: fileInfo.localFileURL)
        compiledInfos.append(fileInfo.filterListInfo)
        unifiedRules = [unifiedRules, fileContents].joined(separator: "\n")
      } catch {
        ContentBlockerManager.log.error(
          "Could not load rules for \(fileInfo.filterListInfo.debugDescription): \(error)"
        )
      }
    }

    // 3. Save the files into storage
    if FileManager.default.fileExists(atPath: fileURL.path) {
      try FileManager.default.removeItem(at: fileURL)
    }
    try unifiedRules.write(to: fileURL, atomically: true, encoding: .utf8)

    // 4. Return a group containing info on this new file
    return GroupedAdBlockEngine.FilterListGroup(
      infos: compiledInfos,
      localFileURL: fileURL,
      fileType: .text
    )
  }

  /// Get or create a cache folder for the given `Resource`
  ///
  /// - Note: This technically can't really return nil as the location and folder are hard coded
  private func getOrCreateCacheFolder() throws -> URL {
    guard
      let folderURL = FileManager.default.getOrCreateFolder(
        name: [Self.parentCacheFolderName, cacheFolderName].joined(separator: "/"),
        location: Self.cacheFolderDirectory
      )
    else {
      throw ResourceFileError.failedToCreateCacheFolder
    }

    return folderURL
  }

  private func cache(engine: GroupedAdBlockEngine) async {
    let encoder = JSONEncoder()

    do {
      let folderURL = try getOrCreateCacheFolder()

      // Write the serialized engine
      let serializedEngine = try await engine.serialize()
      let serializedEngineURL = folderURL.appendingPathComponent("list.dat", conformingTo: .data)

      if FileManager.default.fileExists(atPath: serializedEngineURL.path) {
        try FileManager.default.removeItem(at: serializedEngineURL)
      }

      try serializedEngine.write(to: serializedEngineURL)

      // Write the info about the engine
      let info = CachedEngineInfo(infos: engine.group.infos, fileType: .data)
      let data = try encoder.encode(info)
      let infoFileURL = folderURL.appendingPathComponent("engine_info.json", conformingTo: .json)

      if FileManager.default.fileExists(atPath: infoFileURL.path) {
        try FileManager.default.removeItem(at: infoFileURL)
      }

      try data.write(to: infoFileURL)
    } catch {
      ContentBlockerManager.log.error(
        "Failed to save cache info for `\(self.cacheFolderName)`: \(String(describing: error))"
      )
    }
  }

  private func loadCachedInfo() -> GroupedAdBlockEngine.FilterListGroup? {
    guard let cacheFolderURL = createdCacheFolderURL else { return nil }
    let cachedEngineURL = cacheFolderURL.appendingPathComponent("list.dat", conformingTo: .data)
    guard FileManager.default.fileExists(atPath: cachedEngineURL.path) else { return nil }
    let cachedInfoURL = cacheFolderURL.appendingPathComponent("engine_info", conformingTo: .json)
    guard FileManager.default.fileExists(atPath: cachedInfoURL.path) else { return nil }
    let decoder = JSONDecoder()

    do {
      let data = try Data(contentsOf: cachedInfoURL)
      let cachedInfo = try decoder.decode(CachedEngineInfo.self, from: data)

      return GroupedAdBlockEngine.FilterListGroup(
        infos: cachedInfo.infos,
        localFileURL: cachedEngineURL,
        fileType: cachedInfo.fileType
      )
    } catch {
      ContentBlockerManager.log.error(
        "Failed to load cache info for `\(self.cacheFolderName)`: \(String(describing: error))"
      )
      return nil
    }
  }
}

extension GroupedAdBlockEngine.Source {
  func blocklistType(isAlwaysAggressive: Bool) -> ContentBlockerManager.BlocklistType? {
    switch self {
    case .filterList(let componentId, let uuid):
      guard uuid != AdblockFilterListCatalogEntry.defaultFilterListComponentUUID else {
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

extension AdblockFilterListCatalogEntry {
  var engineSource: GroupedAdBlockEngine.Source {
    return .filterList(componentId: componentId, uuid: uuid)
  }
}

extension CustomFilterListSetting {
  @MainActor var engineSource: GroupedAdBlockEngine.Source {
    return .filterListURL(uuid: uuid)
  }
}
