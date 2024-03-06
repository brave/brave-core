// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Data
import Foundation
import Preferences
import os

/// A class for managing a grouped engine
@MainActor class AdBlockEngineManager {
  /// The directory to which we should store the unified list into
  private static var cacheFolderDirectory: FileManager.SearchPathDirectory {
    return FileManager.SearchPathDirectory.applicationSupportDirectory
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
  private var cacheFolderName: String
  /// The type of engine this represents what kind of blocking we are doing. (aggressive or standard)
  /// The difference is in how we treat first party blocking. Aggressive also blocks first party content.
  let engineType: GroupedAdBlockEngine.EngineType
  private(set) var engine: GroupedAdBlockEngine?

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

  /// Return an array of all sources that are enabled according to user's settings
  /// - Note: This does not take into account the domain or global adblock toggle
  private var enabledSources: [GroupedAdBlockEngine.Source] {
    var enabledSources = FilterListStorage.shared.enabledSources(engineType: engineType)
    if engineType.isAlwaysAggressive {
      enabledSources.append(contentsOf: CustomFilterListStorage.shared.enabledSources)
    }
    return enabledSources
  }

  /// All the infos that are compilable based on the enabled sources and available infos
  var compilableFiles: [FileInfo] {
    return enabledSources.compactMap { source in
      return availableFiles.first(where: { $0.filterListInfo.source == source })
    }
  }

  init(engineType: GroupedAdBlockEngine.EngineType, cacheFolderName: String) {
    self.availableFiles = []
    self.engine = nil
    self.engineType = engineType
    self.cacheFolderName = cacheFolderName
  }

  /// Tells us if this source should be loaded.
  func isEnabled(source: GroupedAdBlockEngine.Source) -> Bool {
    return enabledSources.contains(source)
  }

  /// Add the info to the available list
  func add(fileInfo: FileInfo) {
    availableFiles.removeAll { existingFileInfo in
      return existingFileInfo.filterListInfo == fileInfo.filterListInfo
    }

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
  func checkNeedsCompile() -> Bool {
    guard let engine = engine else { return true }
    let compilableInfos = compilableFiles.map({ $0.filterListInfo })
    guard !compilableInfos.isEmpty else { return false }
    return compilableInfos != engine.group.infos
  }

  /// Checks to see if we need to compile or recompile the engine based on the available info
  func checkHasAllInfo() -> Bool {
    return compilableFiles == availableFiles
  }

  /// Load the engine from cache so it can be ready during launch
  func loadFromCache(resourcesInfo: GroupedAdBlockEngine.ResourcesInfo?) async {
    do {
      guard let cachedGroupInfo = loadCachedInfo() else { return }
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
    } catch {
      ContentBlockerManager.log.error(
        "Failed to load engine from cache for `\(self.cacheFolderName)`: \(String(describing: error))"
      )
    }
  }

  /// This is a task that we use to delay a requested compile.
  /// This allows us to wait for more sources and limit the number of times we compile
  private var delayTask: Task<Void, Error>?
  /// This is the current pending group we are compiling.
  /// This allows us to ensure we are not compiling a newer version of the rules
  private var pendingGroup: GroupedAdBlockEngine.FilterListGroup?

  /// This will compile available data, but will wait a little bit in case something new gets downloaded.
  /// Especially needed during launch when we have a bunch of downloads coming at the same time.
  func compileDelayedIfNeeded(resourcesInfo: GroupedAdBlockEngine.ResourcesInfo?) {
    // Cancel the previous task
    delayTask?.cancel()

    // Restart the task
    delayTask = Task {
      let hasAllInfo = checkHasAllInfo()
      try await Task.sleep(seconds: hasAllInfo ? 10 : 60)
      guard checkNeedsCompile() else { return }
      try await compileAvailable(resourcesInfo: resourcesInfo)
    }
  }

  /// Compile an engine from all available data
  func compileAvailable(resourcesInfo: GroupedAdBlockEngine.ResourcesInfo?) async throws {
    // 1. Create the group
    let group = try combineRules()
    let engineType = self.engineType
    self.pendingGroup = group

    // 2. Compile the engine
    let groupedEngine = try await Task.detached(priority: .background) {
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
    let infosString = engine.group.infos.map({ " \($0.debugDescription)" }).joined(separator: "\n")
    ContentBlockerManager.log.debug(
      "Set `\(self.cacheFolderName)` (\(engine.group.fileType.debugDescription)) engine from \(engine.group.infos.count) sources:\n\(infosString)"
    )
    self.engine = engine
  }

  /// Add or update `resourcesInfo` if it is a newer version. This information is used for lazy loading.
  func update(resourcesInfo: GroupedAdBlockEngine.ResourcesInfo) async {
    do {
      try await engine?.useResources(from: resourcesInfo)
    } catch {
      ContentBlockerManager.log.error(
        "Failed to update `\(self.cacheFolderName)` engne with resources"
      )
    }
  }

  func ensureContentBlockers(for fileInfo: FileInfo) async {
    guard
      let blocklistType = fileInfo.filterListInfo.source.blocklistType(
        isAlwaysAggressive: engineType.isAlwaysAggressive
      )
    else {
      return
    }

    let modes = await ContentBlockerManager.shared.missingModes(for: blocklistType)
    guard !modes.isEmpty else { return }

    do {
      try await ContentBlockerManager.shared.compileRuleList(
        at: fileInfo.localFileURL,
        for: blocklistType,
        modes: modes
      )
    } catch {
      ContentBlockerManager.log.error(
        "Failed to compile rule list for \(fileInfo.filterListInfo.debugDescription)"
      )
    }
  }

  /// Take all the filter lists and combine them into one then save them into a cache folder.
  private func combineRules() throws -> GroupedAdBlockEngine.FilterListGroup {
    // 1. Grab all the needed infos
    let compilableFiles = compilableFiles

    // 2. Create a file url
    let cachedFolder = try getOrCreateCacheFolder()
    let fileURL = cachedFolder.appendingPathComponent("list.txt", conformingTo: .text)
    var compiledInfos: [GroupedAdBlockEngine.FilterListInfo] = []
    var unifiedRules = ""
    // 3. Join all the rules together
    compilableFiles.forEach { fileInfo in
      do {
        let fileContents = try String(contentsOf: fileInfo.localFileURL)
        compiledInfos.append(fileInfo.filterListInfo)
        unifiedRules = [unifiedRules, fileContents].joined(separator: "\n")
      } catch {
        ContentBlockerManager.log.error(
          "Could not load rules for `\(fileInfo.filterListInfo.debugDescription)`: \(error)"
        )
      }
    }

    // 4. Save the files into storage
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

extension CustomFilterListSetting {
  @MainActor var engineSource: GroupedAdBlockEngine.Source {
    return .filterListURL(uuid: uuid)
  }
}
