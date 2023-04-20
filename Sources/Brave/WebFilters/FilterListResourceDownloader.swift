// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Combine
import Data
import BraveCore
import Shared
import Preferences
import os.log

/// An object responsible for fetching filer lists resources from multiple sources
public class FilterListResourceDownloader {
  /// A shared instance of this class
  ///
  /// - Warning: You need to wait for `DataController.shared.initializeOnce()` to be called before using this instance
  public static let shared = FilterListResourceDownloader()
  
  /// Object responsible for getting component updates
  private var adBlockService: AdblockService?
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
    self.fetchTasks = [:]
    self.adBlockServiceTasks = [:]
    self.adBlockService = nil
  }
  
  /// This loads the filter list settings from core data.
  /// It uses these settings and other stored properties to load the enabled general shields and filter lists.
  ///
  /// - Warning: This method loads filter list settings.
  /// You need to wait for `DataController.shared.initializeOnce()` to be called first before invoking this method
  public func loadFilterListSettingsAndCachedData() async {
    async let cachedFilterLists: Void = self.addEngineResourcesFromCachedFilterLists()
    async let cachedGeneralFilterList: Void = self.addEngineResourcesFromCachedGeneralFilterList()
    _ = await (cachedFilterLists, cachedGeneralFilterList)
  }
  
  /// This function adds engine resources to `AdBlockManager` from cached data representing the enabled filter lists.
  ///
  /// The filter lists are additional blocking content that can be added to the`AdBlockEngine` and as iOS content blockers.
  /// It represents the items found in the "Filter lists" section of the "Shields" menu.
  ///
  /// - Note: The content blockers for these filter lists are not loaded at this point. They are cached by iOS and there is no need to reload them.
  ///
  /// - Warning: This method loads filter list settings.
  /// You need to wait for `DataController.shared.initializeOnce()` to be called first before invoking this method
  private func addEngineResourcesFromCachedFilterLists() async {
    await FilterListStorage.shared.loadFilterListSettings()
    let filterListSettings = await FilterListStorage.shared.allFilterListSettings
      
    await filterListSettings.asyncConcurrentForEach { setting in
      guard await setting.isEnabled == true else { return }
      
      // Try to load the filter list folder. We always have to compile this at start
      if let folderURL = await setting.folderURL, FileManager.default.fileExists(atPath: folderURL.path) {
        await self.addEngineResources(
          forFilterListUUID: setting.uuid, downloadedFolderURL: folderURL,
          relativeOrder: setting.order?.intValue ?? 0
        )
      }
    }
  }
  
  /// This function adds engine resources to `AdBlockManager` from the "general" filter list.
  ///
  /// The "general" filter list is a built in filter list that is added to the`AdBlockEngine` and as content blockers.
  /// It represents the "Block cross site trackers" toggle in the "Shields" menu.
  ///
  /// - Note: The content blockers for this "general" filter list are handled using the `AdBlockResourceDownloader` and are not loaded here at this point.
  private func addEngineResourcesFromCachedGeneralFilterList() async {
    guard let folderURL = await FilterListSetting.makeFolderURL(
      forFilterListFolderPath: Preferences.AppState.lastDefaultFilterListFolderPath.value
    ), FileManager.default.fileExists(atPath: folderURL.path) else {
      return
    }
    
    await addEngineResources(fromGeneralFilterListFolderURL: folderURL)
  }
  
  /// Start the adblock service to get updates to the `shieldsInstallPath`
  @MainActor public func start(with adBlockService: AdblockService) {
    self.adBlockService = adBlockService
    
    // Start listening to changes to the install url
    Task {
      for await folderURL in adBlockService.shieldsInstallURL {
        self.didUpdateShieldComponent(
          folderURL: folderURL,
          adBlockFilterLists: adBlockService.regionalFilterLists ?? []
        )
      }
    }
  }
  
  /// Invoked when shield components are loaded
  ///
  /// This function will start fetching data and subscribe publishers once if it hasn't already done so.
  @MainActor private func didUpdateShieldComponent(folderURL: URL, adBlockFilterLists: [AdblockFilterListCatalogEntry]) {
    if !startedFetching && !adBlockFilterLists.isEmpty {
      // This is the first time we load ad-block filters.
      // We need to perform some initial setup (but only do this once)
      startedFetching = true
      FilterListStorage.shared.loadFilterLists(from: adBlockFilterLists)
      
      self.subscribeToFilterListChanges()
      self.registerAllEnabledFilterLists()
    }
    
    // Store the folder path so we can load it from cache next time we launch quicker
    // than waiting for the component updater to respond, which may take a few seconds
    let folderSubPath = FilterListSetting.extractFolderPath(fromFilterListFolderURL: folderURL)
    Preferences.AppState.lastDefaultFilterListFolderPath.value = folderSubPath
    
    Task {
      await self.addEngineResources(fromGeneralFilterListFolderURL: folderURL)
    }
  }
  
  /// Load shields with the given `AdblockService` folder `URL`
  private func addEngineResources(fromGeneralFilterListFolderURL folderURL: URL) async {
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
  
  /// Subscribe to the UI changes on the `filterLists` so that we can save settings and register or unregister the filter lists
  @MainActor private func subscribeToFilterListChanges() {
    // Subscribe to changes on the filter list states
    filterListSubscription = FilterListStorage.shared.$filterLists
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
    FilterListStorage.shared.handleUpdate(to: filterList)
    
    // Register or unregister the filter list depending on its toggle state
    if filterList.isEnabled {
      register(filterList: filterList)
    } else {
      unregister(filterList: filterList)
    }
  }
  
  /// Register all enabled filter lists
  @MainActor private func registerAllEnabledFilterLists() {
    for filterList in FilterListStorage.shared.filterLists {
      guard filterList.isEnabled else { continue }
      register(filterList: filterList)
    }
  }
  
  /// Register this filter list and start all additional resource downloads
  @MainActor private func register(filterList: FilterList) {
    guard adBlockServiceTasks[filterList.uuid] == nil else { return }
    guard let adBlockService = adBlockService else { return }
    startFetchingGenericContentBlockingBehaviors(for: filterList)

    adBlockServiceTasks[filterList.uuid] = Task { @MainActor in
      for await folderURL in adBlockService.register(filterList: filterList) {
        guard let folderURL = folderURL else { continue }
        guard FilterListStorage.shared.isEnabled(for: filterList.entry.componentId) else { return }
        
        await self.addEngineResources(
          forFilterListUUID: filterList.uuid, downloadedFolderURL: folderURL, relativeOrder: filterList.order
        )
        
        // Save the downloaded folder for later (caching) purposes
        FilterListStorage.shared.set(folderURL: folderURL, forUUID: filterList.uuid)
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
}

/// Helpful extension to the AdblockService
private extension AdblockService {
  /// Stream the URL updates to the `shieldsInstallPath`
  ///
  /// - Warning: You should never do this more than once. Only one callback can be registered to the `shieldsComponentReady` callback.
  @MainActor var shieldsInstallURL: AsyncStream<URL> {
    return AsyncStream { continuation in
      if let folderPath = shieldsInstallPath {
        let folderURL = URL(fileURLWithPath: folderPath)
        continuation.yield(folderURL)
      }
      
      guard shieldsComponentReady == nil else {
        assertionFailure("You have already set the `shieldsComponentReady` callback. Setting this more than once replaces the previous callback.")
        return
      }
      
      shieldsComponentReady = { folderPath in
        guard let folderPath = folderPath else {
          return
        }
        
        let folderURL = URL(fileURLWithPath: folderPath)
        continuation.yield(folderURL)
      }
      
      continuation.onTermination = { @Sendable _ in
        self.shieldsComponentReady = nil
      }
    }
  }
  
  /// Register the filter list given by the uuid and streams its updates
  ///
  /// - Note: Cancelling this task will unregister this filter list from recieving any further updates
  @MainActor func register(filterList: FilterList) -> AsyncStream<URL?> {
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
