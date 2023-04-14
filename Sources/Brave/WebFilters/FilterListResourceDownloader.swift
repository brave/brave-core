// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Combine
import Data
import BraveCore
import Shared
import BraveShared
import os.log

/// An object responsible for fetching filer lists resources from multiple sources
public class FilterListResourceDownloader: ObservableObject {
  private class FilterListSettingsManager {
    /// Wether or not these settings are stored in memory or persisted
    private let inMemory: Bool
    
    /// A list of defaults that should be set once we load the filter lists.
    /// This is here in case the filter lists are not loaded but the user is already changing settings
    @MainActor var pendingDefaults: [String: Bool] = [:]
    
    /// This is a list of all available settings.
    ///
    /// - Warning: Do not call this before we load core data
    @MainActor public lazy var allFilterListSettings: [FilterListSetting] = {
      return FilterListSetting.loadAllSettings(fromMemory: inMemory)
    }()
    
    init(inMemory: Bool) {
      self.inMemory = inMemory
    }
    
    /// - Warning: Do not call this before we load core data
    @MainActor public func isEnabled(for componentID: String) -> Bool {
      guard let setting = allFilterListSettings.first(where: { $0.componentId == componentID }) else {
        return pendingDefaults[componentID] ?? FilterList.defaultOnComponentIds.contains(componentID)
      }
      
      return setting.isEnabled
    }
    
    /// Set the enabled status and componentId of a filter list setting if the setting exists.
    /// Otherwise it will create a new setting with the specified properties
    ///
    /// - Warning: Do not call this before we load core data
    @MainActor public func upsertSetting(uuid: String, componentId: String?, isEnabled: Bool, order: Int, allowCreation: Bool) {
      if allFilterListSettings.contains(where: { $0.uuid == uuid }) {
        updateSetting(
          uuid: uuid,
          componentId: componentId,
          isEnabled: isEnabled,
          order: order
        )
      } else if allowCreation {
        create(
          uuid: uuid,
          componentId: componentId,
          isEnabled: isEnabled,
          order: order
        )
      }
    }
    
    /// Set the enabled status of a filter list setting
    ///
    /// - Warning: Do not call this before we load core data
    @MainActor public func set(folderURL: URL, forUUID uuid: String) {
      guard let index = allFilterListSettings.firstIndex(where: { $0.uuid == uuid }) else {
        return
      }
      
      guard allFilterListSettings[index].folderURL != folderURL else { return }
      allFilterListSettings[index].folderURL = folderURL
      FilterListSetting.save(inMemory: inMemory)
    }
    
    @MainActor private func updateSetting(uuid: String, componentId: String?, isEnabled: Bool, order: Int) {
      guard let index = allFilterListSettings.firstIndex(where: { $0.uuid == uuid }) else {
        return
      }
      
      guard allFilterListSettings[index].isEnabled != isEnabled || allFilterListSettings[index].componentId != componentId || allFilterListSettings[index].order?.intValue != order else {
        // Ensure we stop if this is already in sync in order to avoid an event loop
        // And things hanging for too long.
        // This happens because we care about UI changes but not when our downloads finish
        return
      }
        
      allFilterListSettings[index].isEnabled = isEnabled
      allFilterListSettings[index].componentId = componentId
      allFilterListSettings[index].order = NSNumber(value: order)
      FilterListSetting.save(inMemory: inMemory)
    }
    
    /// Create a filter list setting for the given UUID and enabled status
    @MainActor private func create(uuid: String, componentId: String?, isEnabled: Bool, order: Int) {
      let setting = FilterListSetting.create(uuid: uuid, componentId: componentId, isEnabled: isEnabled, order: order, inMemory: inMemory)
      allFilterListSettings.append(setting)
    }
  }
  
  /// A shared instance of this class
  ///
  /// - Warning: You need to wait for `DataController.shared.initializeOnce()` to be called before using this instance
  public static let shared = FilterListResourceDownloader()
  
  /// Object responsible for getting component updates
  private var adBlockService: AdblockService?
  /// Manager that handles updates to filter list settings in core data
  private let settingsManager: FilterListSettingsManager
  /// The resource downloader that downloads our resources
  private let resourceDownloader: ResourceDownloader<BraveS3Resource>
  /// The filter list subscription
  private var filterListSubscription: AnyCancellable?
  /// Fetch content blocking tasks per filter list
  private var fetchTasks: [BraveS3Resource: Task<Void, Error>]
  /// Ad block service tasks per filter list UUID
  private var adBlockServiceTasks: [String: Task<Void, Error>]
  /// A marker that says if fetching has started
  private var startedFetching = false
  /// The filter lists wrapped up so we can contain
  @Published var filterLists: [FilterList]
  
  /// A formatter that is used to format a version number
  private lazy var fileVersionDateFormatter: DateFormatter = {
    let dateFormatter = DateFormatter()
    dateFormatter.locale = Locale(identifier: "en_US_POSIX")
    dateFormatter.dateFormat = "yyyy.MM.dd.HH.mm.ss"
    dateFormatter.timeZone = TimeZone(secondsFromGMT: 0)
    return dateFormatter
  }()
  
  init(networkManager: NetworkManager = NetworkManager(), persistChanges: Bool = true) {
    self.resourceDownloader = ResourceDownloader(networkManager: networkManager)
    self.settingsManager = FilterListSettingsManager(inMemory: !persistChanges)
    self.filterLists = []
    self.fetchTasks = [:]
    self.adBlockServiceTasks = [:]
    self.adBlockService = nil
    self.recordP3ACookieListEnabled()
  }
  
  public func loadCachedData() async {
    async let cachedFilterLists: Void = self.addEngineResourcesFromCachedFilterLists()
    async let cachedDefaultFilterList: Void = self.loadCachedDefaultFilterList()
    _ = await (cachedFilterLists, cachedDefaultFilterList)
  }
  
  /// This function adds engine resources to `AdBlockManager` for the cached filter lists
  @MainActor private func addEngineResourcesFromCachedFilterLists() async {
    let filterListSettings = settingsManager.allFilterListSettings
      
    await filterListSettings.asyncConcurrentForEach { setting in
      guard setting.isEnabled == true else { return }
      
      // Try to load the filter list folder. We always have to compile this at start
      if let folderURL = setting.folderURL, FileManager.default.fileExists(atPath: folderURL.path) {
        await self.addEngineResources(
          forFilterListUUID: setting.uuid, downloadedFolderURL: folderURL,
          relativeOrder: setting.order?.intValue ?? 0
        )
      }
    }
  }
  
  private func loadCachedDefaultFilterList() async {
    guard let folderURL = FilterListSetting.makeFolderURL(
      forFilterListFolderPath: Preferences.AppState.lastDefaultFilterListFolderPath.value
    ), FileManager.default.fileExists(atPath: folderURL.path) else {
      return
    }
    
    await loadShields(fromDefaultFilterListFolderURL: folderURL)
  }
  
  /// Start the resource subscriber.
  ///
  /// - Warning: You need to wait for `DataController.shared.initializeOnce()` to be called before invoking this method
  @MainActor public func start(with adBlockService: AdblockService) {
    self.adBlockService = adBlockService
    
    if let folderPath = adBlockService.shieldsInstallPath {
      didUpdateShieldComponent(folderPath: folderPath, adBlockFilterLists: adBlockService.regionalFilterLists ?? [])
    }
    
    adBlockService.shieldsComponentReady = { folderPath in
      guard let folderPath = folderPath else { return }
      
      Task { @MainActor in
        self.didUpdateShieldComponent(folderPath: folderPath, adBlockFilterLists: adBlockService.regionalFilterLists ?? [])
      }
    }
  }
  
  /// Enables a filter list for the given component ID. Returns true if the filter list exists or not.
  @MainActor public func enableFilterList(for componentID: String, isEnabled: Bool) {
    // Enable the setting
    defer { self.recordP3ACookieListEnabled() }
    
    if let index = filterLists.firstIndex(where: { $0.entry.componentId == componentID }) {
      // Only update the value if it has changed
      guard filterLists[index].isEnabled != isEnabled else { return }
      filterLists[index].isEnabled = isEnabled
    } else if let uuid = FilterList.componentToUUID[componentID] {
      let defaultToggle = FilterList.defaultOnComponentIds.contains(componentID)
      
      settingsManager.upsertSetting(
        uuid: uuid, componentId: componentID, isEnabled: isEnabled,
        order: 0,
        allowCreation: defaultToggle != isEnabled
      )
    } else {
      assertionFailure(
        "How can this be changed if we don't have a filter list or special shields toggle for this?"
      )
      
      // We haven't loaded the filter lists yet. Add it to the pending list.
      settingsManager.pendingDefaults[componentID] = isEnabled
    }
  }
  
  /// Tells us if the filter list is enabled for the given `componentID`
  @MainActor public func isEnabled(for componentID: String) -> Bool {
    return settingsManager.isEnabled(for: componentID)
  }
  
  /// Invoked when shield components are loaded
  ///
  /// This function will start fetching data and subscribe publishers once if it hasn't already done so.
  @MainActor private func didUpdateShieldComponent(folderPath: String, adBlockFilterLists: [AdblockFilterListCatalogEntry]) {
    if !startedFetching && !adBlockFilterLists.isEmpty {
      startedFetching = true
      let filterLists = loadFilterLists(from: adBlockFilterLists, filterListSettings: settingsManager.allFilterListSettings)
      self.filterLists = filterLists
      self.subscribeToFilterListChanges()
      self.registerAllEnabledFilterLists()
    }
    
    // Store the folder path so we can load it from cache next time we launch quicker
    // than waiting for the component updater to respond, which may take a few seconds
    let folderURL = URL(fileURLWithPath: folderPath)
    let folderSubPath = FilterListSetting.extractFolderPath(fromFilterListFolderURL: folderURL)
    Preferences.AppState.lastDefaultFilterListFolderPath.value = folderSubPath
    
    Task {
      await self.loadShields(fromDefaultFilterListFolderURL: folderURL)
    }
  }
  
  /// Load shields with the given `AdblockService` folder `URL`
  private func loadShields(fromDefaultFilterListFolderURL folderURL: URL) async {
    let version = folderURL.lastPathComponent
    
    // Lets add these new resources
    await AdBlockEngineManager.shared.add(
      resource: AdBlockEngineManager.Resource(type: .dat, source: .adBlock),
      fileURL: folderURL.appendingPathComponent("rs-ABPFilterParserData.dat"),
      version: version
    )
    
    await AdBlockEngineManager.shared.add(
      resource: AdBlockEngineManager.Resource(type: .jsonResources, source: .adBlock),
      fileURL: folderURL.appendingPathComponent("resources.json"),
      version: version
    )
  }
  
  /// Load filter lists from the ad block service
  @MainActor private func loadFilterLists(from regionalFilterLists: [AdblockFilterListCatalogEntry], filterListSettings: [FilterListSetting]) -> [FilterList] {
    return regionalFilterLists.map { adBlockFilterList in
      let setting = filterListSettings.first(where: { $0.uuid == adBlockFilterList.uuid })
      return FilterList(
        from: adBlockFilterList,
        isEnabled: setting?.isEnabled ?? adBlockFilterList.defaultToggle
      )
    }
  }
  
  /// Subscribe to the UI changes on the `filterLists` so that we can save settings and register or unregister the filter lists
  private func subscribeToFilterListChanges() {
    // Subscribe to changes on the filter list states
    filterListSubscription = $filterLists
      .sink { filterLists in
        DispatchQueue.main.async { [weak self] in
          for filterList in filterLists {
            self?.handleUpdate(to: filterList)
          }
        }
      }
  }
  
  /// Ensures settings are saved for the given filter list and that our publisher is aware of the changes
  @MainActor private func handleUpdate(to filterList: FilterList) {
    // Upsert (update or insert) the setting.
    //
    // However we create only when:
    // a) The filter list is enabled
    //    (this is because loading caches are based on created settings)
    // b) The filter list is different than the default
    //    (in order to respect the users preference if the default were to change in the future)
    settingsManager.upsertSetting(
      uuid: filterList.uuid,
      componentId: filterList.entry.componentId,
      isEnabled: filterList.isEnabled,
      order: filterLists.firstIndex(where: { $0.id == filterList.id }) ?? 0,
      allowCreation: filterList.entry.defaultToggle != filterList.isEnabled || filterList.isEnabled
    )
    
    // Register or unregister the filter list depending on its toggle state
    if filterList.isEnabled {
      register(filterList: filterList)
    } else {
      unregister(filterList: filterList)
    }
  }
  
  /// Register all enabled filter lists
  @MainActor private func registerAllEnabledFilterLists() {
    for filterList in filterLists {
      guard filterList.isEnabled else { continue }
      register(filterList: filterList)
    }
  }
  
  /// Register this filter list and start all additional resource downloads
  @MainActor private func register(filterList: FilterList) {
    guard adBlockServiceTasks[filterList.uuid] == nil else { return }
    guard let adBlockService = adBlockService else { return }
    guard let index = filterLists.firstIndex(where: { $0.id == filterList.id }) else { return }
    startFetchingGenericContentBlockingBehaviors(for: filterList)

    adBlockServiceTasks[filterList.uuid] = Task { @MainActor in
      for await folderURL in await adBlockService.register(filterList: filterList) {
        guard let folderURL = folderURL else { continue }
        guard self.isEnabled(for: filterList.entry.componentId) else { return }
        await self.addEngineResources(
          forFilterListUUID: filterList.uuid, downloadedFolderURL: folderURL, relativeOrder: index
        )
        
        // Save the downloaded folder for later (caching) purposes
        self.settingsManager.set(folderURL: folderURL, forUUID: filterList.uuid)
      }
    }
  }
  
  /// Unregister, cancel all of its downloads and remove any `ContentBlockerManager` and `AdBlockEngineManager` resources for this filter list
  @MainActor private func unregister(filterList: FilterList) {
    adBlockServiceTasks[filterList.uuid]?.cancel()
    adBlockServiceTasks.removeValue(forKey: filterList.uuid)
    stopFetching(resource: .filterListContentBlockingBehaviors(uuid: filterList.uuid, componentId: filterList.entry.componentId))
    
    Task {
      async let removeContentBlockerResource: Void = ContentBlockerManager.shared.removeRuleList(
        for: .filterList(uuid: filterList.uuid)
      )
      async let removeAdBlockEngineResource: Void = AdBlockEngineManager.shared.removeResources(
        for: .filterList(uuid: filterList.uuid)
      )
      _ = try await (removeContentBlockerResource, removeAdBlockEngineResource)
    }
  }
  
  /// Start fetching the resource for the given filter list
  private func startFetchingGenericContentBlockingBehaviors(for filterList: FilterList) {
    let resource = BraveS3Resource.filterListContentBlockingBehaviors(
      uuid: filterList.entry.uuid,
      componentId: filterList.entry.componentId
    )
    
    guard fetchTasks[resource] == nil else {
      // We're already fetching for this filter list
      return
    }
    
    fetchTasks[resource] = Task { @MainActor in
      try await withTaskCancellationHandler(operation: {
        for try await result in await self.resourceDownloader.downloadStream(for: resource) {
          switch result {
          case .success(let downloadResult):
            await handle(
              downloadResult: downloadResult, for: filterList
            )
          case .failure(let error):
            ContentBlockerManager.log.error("Failed to download resource \(resource.cacheFolderName): \(error)")
          }
        }
      }, onCancel: {
        self.fetchTasks.removeValue(forKey: resource)
      })
    }
  }
  
  /// Cancel all fetching tasks for the given resource
  private func stopFetching(resource: BraveS3Resource) {
    fetchTasks[resource]?.cancel()
    fetchTasks.removeValue(forKey: resource)
  }
  
  private func handle(downloadResult: ResourceDownloader<BraveS3Resource>.DownloadResult, for filterList: FilterListInterface) async {
    if !downloadResult.isModified {
      // if the file is not modified first we need to see if we already have a cached value loaded
      guard await !ContentBlockerManager.shared.hasRuleList(for: .filterList(uuid: filterList.uuid)) else {
        // We don't want to recompile something that we alrady have loaded
        return
      }
    }
    
    do {
      let encodedContentRuleList = try String(contentsOf: downloadResult.fileURL, encoding: .utf8)
      
      // We only want to compile cached values if they are not already loaded
      try await ContentBlockerManager.shared.compile(
        encodedContentRuleList: encodedContentRuleList,
        for: .filterList(uuid: filterList.uuid),
        options: .all
      )
    } catch {
      let debugTitle = await filterList.debugTitle
      ContentBlockerManager.log.error(
        "Failed to compile rule list for \(debugTitle) (`\(downloadResult.fileURL.absoluteString)`): \(error)"
      )
    }
  }
  
  /// Handle the downloaded folder url for the given filter list. The folder URL should point to a `AdblockFilterList` resource
  /// This will also start fetching any additional resources for the given filter list given it is still enabled.
  private func addEngineResources(forFilterListUUID uuid: String, downloadedFolderURL: URL, relativeOrder: Int) async {
    // Let's add the new ones in
    await AdBlockEngineManager.shared.add(
      resource: AdBlockEngineManager.Resource(type: .dat, source: .filterList(uuid: uuid)),
      fileURL: downloadedFolderURL.appendingPathComponent("rs-\(uuid).dat"),
      version: downloadedFolderURL.lastPathComponent, relativeOrder: relativeOrder
    )
    await AdBlockEngineManager.shared.add(
      resource: AdBlockEngineManager.Resource(type: .jsonResources, source: .filterList(uuid: uuid)),
      fileURL: downloadedFolderURL.appendingPathComponent("resources.json"),
      version: downloadedFolderURL.lastPathComponent,
      relativeOrder: relativeOrder
    )
  }
  
  // MARK: - P3A
  
  private func recordP3ACookieListEnabled() {
    // Q69 Do you have cookie consent notice blocking enabled?
    Task { @MainActor in
      UmaHistogramBoolean(
        "Brave.Shields.CookieListEnabled",
        isEnabled(for: FilterList.cookieConsentNoticesComponentID)
      )
    }
  }
}

/// Helpful extension to the AdblockService
private extension AdblockService {
  /// Register the filter list given by the uuid and streams its updates
  ///
  /// - Note: Cancelling this task will unregister this filter list from recieving any further updates
  @MainActor func register(filterList: FilterList) async -> AsyncStream<URL?> {
    return AsyncStream { continuation in
      registerFilterListComponent(filterList.entry, useLegacyComponent: true) { folderPath in
        guard let folderPath = folderPath else {
          continuation.yield(nil)
          return
        }
        
        let folderURL = URL(fileURLWithPath: folderPath)
        continuation.yield(folderURL)
      }
      
      continuation.onTermination = { @Sendable _ in
        self.unregisterFilterListComponent(filterList.entry, useLegacyComponent: true)
      }
    }
  }
}

// MARK: - FilterListLanguageProvider - A way to share `defaultToggle` logic between multiple structs/classes

private extension AdblockFilterListCatalogEntry {
  @available(iOS 16, *)
  /// A list of regions that this filter list focuses on.
  /// An empty set means this filter list doesn't focus on any specific region.
  var supportedLanguageCodes: Set<Locale.LanguageCode> {
    return Set(languages.map({ Locale.LanguageCode($0) }))
  }
  
  /// This method returns the default value for this filter list if the user does not manually toggle it.
  /// - Warning: Make sure you use `componentID` to identify the filter list, as `uuid` will be deprecated in the future.
  var defaultToggle: Bool {
    // First check if this is a statically set component
    if FilterList.defaultOnComponentIds.contains(componentId) {
      return true
    }
    
    // For compatibility reasons, we only enable certian regional filter lists
    // These are the ones that are known to be well maintained.
    guard FilterList.maintainedRegionalComponentIDs.contains(componentId) else {
      return false
    }
    
    if #available(iOS 16, *), let languageCode = Locale.current.language.languageCode {
      return supportedLanguageCodes.contains(languageCode)
    } else if let languageCode = Locale.current.languageCode {
      return languages.contains(languageCode)
    } else {
      return false
    }
  }
}
