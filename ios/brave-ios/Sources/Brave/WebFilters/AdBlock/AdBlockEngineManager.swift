// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Data
import Foundation
import Preferences
import os

/// A class for managing a single grouped engine
@MainActor class AdBlockEngineManager {
  enum CompileTarget: Hashable {
    case engine, contentBlockers
    var combinedFileName: String {
      switch self {
      case .engine: return "list.txt"
      case .contentBlockers: return "content-blocker-list.txt"
      }
    }
  }

  public struct FileInfo: Hashable, Equatable {
    let filterListInfo: GroupedAdBlockEngine.FilterListInfo
    let localFileURL: URL
  }

  /// The top level folder to create for caching engine data
  nonisolated private static let parentCacheFolderName = "engines"
  /// The subfolder that the cache data is stored.
  /// Since we can have multiple engines this folder will be unique per engine
  nonisolated private let cacheFolderName: String
  /// The type of engine this represents what kind of blocking we are doing. (aggressive or standard)
  /// The difference is in how we treat first party blocking. Aggressive also blocks first party content.
  nonisolated let engineType: GroupedAdBlockEngine.EngineType
  /// All the info that is currently available
  private var availableFiles: [FileInfo]
  /// The engine that is currently available for this manager
  private(set) var engine: GroupedAdBlockEngine?
  /// This is a task that we use to delay a requested compile.
  /// This allows us to wait for more sources and limit the number of times we compile
  private var delayTask: Task<Void, Error>?
  /// This is the current pending group we are compiling.
  /// This allows us to ensure we are not already compiling a newer version of the rules before setting the engine
  private var pendingGroup: GroupedAdBlockEngine.FilterListGroup?
  /// This is the current pending group we are compiling into content blockers
  /// This allows us to ensure we are not compiling the same rules more than once
  private var pendingContentBlockerGroup: GroupedAdBlockEngine.FilterListGroup?
  /// The blocklist type this manager represents if we are combining content blockers
  var blocklistType: ContentBlockerManager.BlocklistType {
    return .engineGroup(id: cacheFolderName, engineType: engineType)
  }

  /// This structure represents encodable info on what cached engine data contains
  private struct CachedEngineInfo: Codable {
    let infos: [GroupedAdBlockEngine.FilterListInfo]
    let fileType: GroupedAdBlockEngine.FileType
  }

  /// Get the already created cache folder
  ///
  /// - Note: Returns nil if the cache folder does not exist
  private var createdCacheFolderURL: URL? {
    guard
      let folderURL = try? AsyncFileManager.default.url(
        for: .cachesDirectory,
        in: .userDomainMask
      )
    else {
      return nil
    }
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
  func checkNeedsEngineCompile(for fileInfos: [AdBlockEngineManager.FileInfo]) -> Bool {
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
    guard let createdCacheFolderURL else { return false }
    do {
      guard let cachedGroupInfo = await loadCachedInfo(cacheFolderURL: createdCacheFolderURL) else {
        return false
      }
      let start = ContinuousClock().now
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

      self.set(engine: groupedEngine, start: start)
      return true
    } catch {
      ContentBlockerManager.log.error(
        "Failed to load engine from cache for `\(self.cacheFolderName)`: \(String(describing: error))"
      )

      return false
    }
  }

  // Delete the cache. Mostly used for testing
  func deleteCachedEngine() async throws {
    guard let cacheFolderURL = createdCacheFolderURL else { return }
    try await AsyncFileManager.default.removeItem(at: cacheFolderURL)
  }

  /// This will compile available data, but will wait a little bit in case something new gets downloaded.
  /// Especially needed during launch when we have a bunch of downloads coming at the same time.
  func compileDelayedIfNeeded(
    for sourceProvider: AdBlockGroupsManager.SourceProvider,
    resourcesInfo: GroupedAdBlockEngine.ResourcesInfo?,
    contentBlockerManager: ContentBlockerManager
  ) {
    // Cancel the previous task
    delayTask?.cancel()

    // Restart the task
    delayTask = Task {
      try await Task.sleep(seconds: 60)
      let enabledSources = sourceProvider.enabledSources(for: engineType)
      await compileAvailableEnginesIfNeeded(
        for: enabledSources,
        resourcesInfo: resourcesInfo
      )
      if engineType.combineContentBlockers {
        await ensureGroupedContentBlockers(
          for: enabledSources,
          contentBlockerManager: contentBlockerManager
        )
      }
    }
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

  /// This will compile available data right away if it is needed
  func compileAvailableEnginesIfNeeded(
    for enabledSources: [GroupedAdBlockEngine.Source],
    resourcesInfo: GroupedAdBlockEngine.ResourcesInfo?
  ) async {
    do {
      let compilableFiles = compilableFiles(for: enabledSources)

      guard self.checkNeedsEngineCompile(for: compilableFiles) else {
        return
      }

      try await compileAvailableEngines(
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
  private func compileAvailableEngines(
    for files: [AdBlockEngineManager.FileInfo],
    resourcesInfo: GroupedAdBlockEngine.ResourcesInfo?
  ) async throws {
    let start = ContinuousClock().now
    let engineType = self.engineType
    let group = try await combineRules(for: files, target: .engine)
    self.pendingGroup = group

    ContentBlockerManager.log.debug(
      """
      Compiling `\(self.cacheFolderName)` engine from \(group.infos.count) sources:
      \(group.makeDebugDescription(for: engineType))
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
    self.set(engine: groupedEngine, start: start)
    await cache(engine: groupedEngine)
    self.pendingGroup = nil
  }

  /// Compile content blockers for a group if the engine type supports it
  func ensureGroupedContentBlockers(
    for enabledSources: [GroupedAdBlockEngine.Source],
    contentBlockerManager: ContentBlockerManager
  ) async {
    // Only do this for content blockers that should be combined
    guard engineType.combineContentBlockers else { return }
    let compilableFiles = compilableFiles(for: enabledSources.map({ $0.contentBlockerSource }))
    guard !compilableFiles.isEmpty else { return }

    let version = compilableFiles.map({ $0.filterListInfo }).groupedVersion
    let modes = await contentBlockerManager.missingModes(
      for: blocklistType,
      version: version
    )

    guard !modes.isEmpty else {
      ContentBlockerManager.log.debug(
        "Rule lists already compiled for `\(self.engineType.debugDescription)` engine"
      )
      return
    }

    let clock = ContinuousClock()
    let start = clock.now
    do {
      let group = try await combineRules(for: compilableFiles, target: .contentBlockers)
      guard !group.infos.isEmpty else { return }

      // Ensure we're not already compiling the same group
      guard pendingContentBlockerGroup != group else { return }
      pendingContentBlockerGroup = group

      ContentBlockerManager.log.debug(
        """
        Compiling grouped Rule Lists for `\(self.engineType.debugDescription)` engine:
        \(group.makeDebugDescription(for: self.engineType))
        """
      )
      try await contentBlockerManager.compileRuleList(
        at: group.localFileURL,
        for: blocklistType,
        version: version,
        modes: modes
      )
      ContentBlockerManager.log.debug(
        """
        Compiled grouped Rule Lists for `\(self.engineType.debugDescription)` engine (\(clock.now.formatted(since: start))):
        \(group.makeDebugDescription(for: self.engineType))
        """
      )
    } catch {
      ContentBlockerManager.log.error(
        "Failed to compile grouped rule lists for `\(self.blocklistType.debugDescription)`"
      )
    }
    pendingContentBlockerGroup = nil
  }

  private func set(engine: GroupedAdBlockEngine, start: ContinuousClock.Instant) {
    let group = engine.group
    ContentBlockerManager.log.debug(
      """
      Set `\(self.cacheFolderName)` (\(group.fileType.debugDescription)) engine from \(group.infos.count) sources (\(ContinuousClock().now.formatted(since: start))):
      \(group.makeDebugDescription(for: self.engineType))
      """
    )
    self.engine = engine
  }

  /// Take all the filter lists and combine them into one then save them into a cache folder.
  private func combineRules(
    for compilableFiles: [AdBlockEngineManager.FileInfo],
    target: CompileTarget
  ) async throws -> GroupedAdBlockEngine.FilterListGroup {
    // 1. Create a temporary file in the caches directory
    let cachedFolder = try await getOrCreateCacheFolder()
    let temporaryFileURL = cachedFolder.appendingPathComponent(
      "combined-filter-list_\(UUID().uuidString).txt"
    )

    // Create an empty file so we have something to write to
    await AsyncFileManager.default.createFile(
      atPath: temporaryFileURL.path,
      contents: "".data(using: .utf8)
    )

    return try await Task.detached {
      let fileWriteHandle = try FileHandle(forWritingTo: temporaryFileURL)
      var compiledInfos: [GroupedAdBlockEngine.FilterListInfo] = []

      // 2. Write all the rules to a temporary file
      for fileInfo in compilableFiles {
        do {
          guard let data = try await fileInfo.getRulesData(engineType: self.engineType) else {
            continue
          }
          try fileWriteHandle.write(contentsOf: data)
          compiledInfos.append(fileInfo.filterListInfo)
        } catch {
          ContentBlockerManager.log.error(
            "Could not load rules for \(fileInfo.filterListInfo.debugDescription): \(error)"
          )
        }
      }

      // 3. Save the files into storage
      try fileWriteHandle.close()
      let fileURL = cachedFolder.appendingPathComponent(target.combinedFileName)
      if await AsyncFileManager.default.fileExists(atPath: fileURL.path) {
        try await AsyncFileManager.default.removeItem(at: fileURL)
      }
      try await AsyncFileManager.default.moveItem(at: temporaryFileURL, to: fileURL)

      // 4. Return a group containing info on this new file
      return GroupedAdBlockEngine.FilterListGroup(
        infos: compiledInfos,
        localFileURL: fileURL,
        fileType: .text
      )
    }.value
  }

  /// Get or create a cache folder for the given `Resource`
  ///
  /// - Note: This technically can't really return nil as the location and folder are hard coded
  private func getOrCreateCacheFolder() async throws -> URL {
    try await AsyncFileManager.default.url(
      for: .cachesDirectory,
      appending: [Self.parentCacheFolderName, cacheFolderName].joined(separator: "/"),
      create: true
    )
  }

  private func cache(engine: GroupedAdBlockEngine) async {
    let encoder = JSONEncoder()

    do {
      let folderURL = try await getOrCreateCacheFolder()

      // Write the serialized engine
      let serializedEngineData = try await engine.serialize()
      let serializedEngineURL = folderURL.appendingPathComponent("list.dat", conformingTo: .data)

      if await AsyncFileManager.default.fileExists(atPath: serializedEngineURL.path) {
        try await AsyncFileManager.default.removeItem(at: serializedEngineURL)
      }

      // Write the data to file
      await AsyncFileManager.default.createFile(
        atPath: serializedEngineURL.path,
        contents: serializedEngineData
      )

      // Write the info about the engine
      let info = CachedEngineInfo(infos: engine.group.infos, fileType: .data)
      let data = try encoder.encode(info)
      let infoFileURL = folderURL.appendingPathComponent("engine_info.json", conformingTo: .json)

      if await AsyncFileManager.default.fileExists(atPath: infoFileURL.path) {
        try await AsyncFileManager.default.removeItem(at: infoFileURL)
      }

      // Write the file info to file
      await AsyncFileManager.default.createFile(
        atPath: infoFileURL.path,
        contents: data
      )
    } catch {
      ContentBlockerManager.log.error(
        "Failed to save cache info for `\(self.cacheFolderName)`: \(String(describing: error))"
      )
    }
  }

  private func loadCachedInfo(
    cacheFolderURL: URL
  ) async -> GroupedAdBlockEngine.FilterListGroup? {
    let cachedEngineURL = cacheFolderURL.appendingPathComponent("list.dat", conformingTo: .data)
    guard await AsyncFileManager.default.fileExists(atPath: cachedEngineURL.path) else {
      return nil
    }
    let cachedInfoURL = cacheFolderURL.appendingPathComponent("engine_info", conformingTo: .json)
    guard await AsyncFileManager.default.fileExists(atPath: cachedInfoURL.path) else { return nil }
    let decoder = JSONDecoder()

    guard let data = await AsyncFileManager.default.contents(atPath: cachedInfoURL.path) else {
      return nil
    }

    do {
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
  var contentBlockerSource: GroupedAdBlockEngine.Source {
    switch self {
    case .filterList(let componentId):
      // We replace the default filter list with the slim list when we compile content blockers
      return AdblockFilterListCatalogEntry.defaultListComponentID == componentId ? .slimList : self
    case .filterListURL, .filterListText, .slimList:
      return self
    }
  }

  /// Return the blocklist type for this source if it is supported
  func blocklistType(
    engineType: GroupedAdBlockEngine.EngineType
  ) -> ContentBlockerManager.BlocklistType? {
    guard engineType == .aggressive else { return nil }
    return .engineSource(self, engineType: engineType)
  }
}

extension AdblockFilterListCatalogEntry {
  var engineSource: GroupedAdBlockEngine.Source {
    return .filterList(componentId: componentId)
  }
}

extension CustomFilterListSetting {
  @MainActor var engineSource: GroupedAdBlockEngine.Source {
    return .filterListURL(uuid: uuid)
  }
}

extension Array where Element == GroupedAdBlockEngine.FilterListInfo {
  var groupedVersion: String {
    return map { $0.version }.joined(separator: ",")
  }
}

extension AdBlockEngineManager.FileInfo {
  fileprivate func getRulesData(engineType: GroupedAdBlockEngine.EngineType) async throws -> Data? {
    guard let data = await AsyncFileManager.default.contents(atPath: localFileURL.path) else {
      return nil
    }
    if filterListInfo.source.onlyExceptions(for: engineType) {
      return String(data: data, encoding: .utf8)?
        .components(separatedBy: .newlines)
        .filter({ $0.contains("@@") || $0.contains("@#@") })
        .joined(separator: "\n")
        .data(using: .utf8)
    } else {
      return data
    }
  }
}
