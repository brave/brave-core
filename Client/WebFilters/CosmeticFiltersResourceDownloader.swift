// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveShared
import Combine
import BraveCore

private let log = Logger.browserLogger

struct CosmeticFilterModel: Codable {
  let hideSelectors: [String]
  let styleSelectors: [String: [String]]
  let exceptions: [String]
  let injectedScript: String
  let genericHide: Bool
  
  enum CodingKeys: String, CodingKey {
    case hideSelectors = "hide_selectors"
    case styleSelectors = "style_selectors"
    case exceptions = "exceptions"
    case injectedScript = "injected_script"
    case genericHide = "generichide"
  }
}

private struct CosmeticFilterNetworkResource {
  let resource: CachedNetworkResource
  let fileType: FileType
  let type: CosmeticFiltersResourceDownloader.CosmeticFilterType
}

public class CosmeticFiltersResourceDownloader {
  private static let queue = DispatchQueue(label: "com.brave.cosmecitc-filters-dispatch-queue")
  public static let shared = CosmeticFiltersResourceDownloader()

  private let networkManager: NetworkManager

  static let folderName = "cmf-data"
  private let servicesKeyName = "SERVICES_KEY"
  private let servicesKeyHeaderValue = "BraveServiceKey"
  private var engine = AdblockEngine()
  private var initialLoad: AnyCancellable?
  private var downloadTask: AnyCancellable?

  static let endpoint = { () -> String in
    if !AppConstants.buildChannel.isPublic {
      return "https://adblock-data-staging.s3.bravesoftware.com/ios"
    }
    return "https://adblock-data.s3.brave.com/ios"
  }()

  private init(networkManager: NetworkManager = NetworkManager()) {
    self.networkManager = networkManager
  }

  /// Initialized with year 1970 to force adblock fetch at first launch.
  private(set) var lastFetchDate = Date(timeIntervalSince1970: 0)

  public func startLoading() {
    let now = Date()
    let fetchInterval = AppConstants.buildChannel.isPublic ? 6.hours : 10.minutes

    if now.timeIntervalSince(lastFetchDate) >= fetchInterval {
      lastFetchDate = now

      if initialLoad == nil {
        // Load files from disk into the original engine engine
        initialLoad = loadDownloadedFiles(into: engine)
          .receive(on: DispatchQueue.main)
          .sink { res in
            if case .failure(let error) = res {
              log.error("Error Loading Cosmetic-Filters: \(error)")
            }
          } receiveValue: { _ in
            log.debug("Successfully Loaded Cosmetic-Filters")
          }
      }
      
      // All operations must be done on a temp engine,
      // otherwise we get insane load times when calling:
      // `engine_add_resources` on an existing engine
      // This is because `engine_add_resources` will ADD resources, and not delete old ones
      // Thus we get a huge amount of memory usage and slow down.
      let tempEngine = AdblockEngine()
      
      let cosmeticSamplesTask = downloadCosmeticSamples(with: tempEngine).catch { error -> AnyPublisher<Void, Never> in
        log.error("Failed to Download Cosmetic-Filters (CosmeticSamples): \(error)")
        return Just(()).eraseToAnyPublisher()
      }
      
      let resourceSamplesTask = downloadResourceSamples(with: tempEngine).catch { error -> AnyPublisher<Void, Never> in
        log.error("Failed to Download Cosmetic-Filters (ResourceSamples): \(error)")
        return Just(()).eraseToAnyPublisher()
      }
      
      downloadTask = Publishers.Merge(cosmeticSamplesTask, resourceSamplesTask)
        .collect()
        .subscribe(on: DispatchQueue.global(qos: .userInitiated))
        .receive(on: DispatchQueue.main)
        .map { [weak self] _ -> AnyPublisher<AdblockEngine, Error> in
          guard let self = self else {
            return Fail(error: "Error: Cosmetic-Filters Downloader Deallocated").eraseToAnyPublisher()
          }
          
          // Load downloaded files into the new engine
          // This is because the Rust Engine will LEAK memory
          // if any functions are called more than once
          // We cannot `reset` the Rust Engine
          // So instead, we discard it and replace it with a new one
          let newEngine = AdblockEngine()
          return self.loadDownloadedFiles(into: newEngine).map({ newEngine }).eraseToAnyPublisher()
        }
        .flatMap { $0 }
        .sink { res in
          if case .failure(let error) = res {
            log.error("Failed to Setup Cosmetic-Filters: \(error)")
          }
        } receiveValue: { [weak self] engine in
          self?.engine = engine
          log.debug("Successfully Setup Cosmetic-Filters")
        }
    }
  }

  func cssRules(for url: URL) -> String {
    engine.cosmeticResourcesForURL(url.absoluteString)
  }

  private func loadDownloadedFiles(into engine: AdblockEngine) -> AnyPublisher<Void, Error> {
    let fm = FileManager.default
    guard let folderUrl = fm.getOrCreateFolder(name: CosmeticFiltersResourceDownloader.folderName) else {
      return Fail(error: "Could not get directory with .dat and .json files").eraseToAnyPublisher()
    }

    let enumerator = fm.enumerator(at: folderUrl, includingPropertiesForKeys: nil)
    let filePaths = enumerator?.allObjects as? [URL]
    let datFileUrls = filePaths?.filter { $0.pathExtension == "dat" }
    let jsonFileUrls = filePaths?.filter { $0.pathExtension == "json" }

    let dataFilesSetup: [AnyPublisher<Void, Error>] = datFileUrls?.compactMap({
      let fileName = $0.deletingPathExtension().lastPathComponent
      guard let data = fm.contents(atPath: $0.path) else { return nil }
      return self.setDataFile(into: engine, data: data, id: fileName)
    }) ?? []

    let jsonFilesSetup: [AnyPublisher<Void, Error>] = jsonFileUrls?.compactMap({
      let fileName = $0.deletingPathExtension().lastPathComponent
      guard let data = fm.contents(atPath: $0.path) else { return nil }
      return self.setJSONFile(into: engine, data: data, id: fileName)
    }) ?? []
    
    return Publishers.MergeMany(dataFilesSetup + jsonFilesSetup)
      .collect()
      .subscribe(on: DispatchQueue.global(qos: .userInitiated))
      .map({ _ in () })
      .eraseToAnyPublisher()
  }

  private func downloadCosmeticSamples(with engine: AdblockEngine) -> AnyPublisher<Void, Error> {
    downloadResources(for: engine, type: .cosmeticSample)
      .receive(on: DispatchQueue.main)
      .map {
        log.debug("Downloaded Cosmetic Filters CSS Samples")
        Preferences.Debug.lastCosmeticFiltersCSSUpdate.value = Date()
      }
      .eraseToAnyPublisher()
  }

  private func downloadResourceSamples(with engine: AdblockEngine) -> AnyPublisher<Void, Error> {
    return downloadResources(for: engine, type: .resourceSample)
      .receive(on: DispatchQueue.main)
      .map {
        log.debug("Downloaded Cosmetic Filters Scriptlets Samples")
        Preferences.Debug.lastCosmeticFiltersScripletsUpdate.value = Date()
      }
      .eraseToAnyPublisher()
  }

  private func downloadResources(
    for engine: AdblockEngine,
    type: CosmeticFilterType
  ) -> AnyPublisher<Void, Error> {
    let nm = networkManager
    let folderName = CosmeticFiltersResourceDownloader.folderName

    // file name of which the file will be saved on disk
    let fileName = type.identifier

    let completedDownloads = type.associatedFiles.compactMap({ [weak self] fileType -> AnyPublisher<CosmeticFilterNetworkResource, Error>? in
      guard let self = self else { return nil }

      let fileExtension = fileType.rawValue
      let etagExtension = fileExtension + ".etag"

      guard let resourceName = type.resourceName(for: fileType),
        var url = URL(string: CosmeticFiltersResourceDownloader.endpoint)
      else {
        return nil
      }

      url.appendPathComponent(resourceName)
      url.appendPathExtension(fileExtension)

      var headers = [String: String]()
      if let servicesKeyValue = Bundle.main.getPlistString(for: self.servicesKeyName) {
        headers[self.servicesKeyHeaderValue] = servicesKeyValue
      }

      let etag = self.fileFromDocumentsAsString("\(fileName).\(etagExtension)", inFolder: folderName)

      return nm.downloadResource(
        with: url,
        resourceType: .cached(etag: etag),
        checkLastServerSideModification: !AppConstants.buildChannel.isPublic,
        customHeaders: headers)
        .compactMap({ resource in
          if resource.data.isEmpty {
            return nil
          }

          return CosmeticFilterNetworkResource(
            resource: resource,
            fileType: fileType,
            type: type)
        }).eraseToAnyPublisher()
    })
    
    return Publishers.MergeMany(completedDownloads)
      .collect()
      .subscribe(on: DispatchQueue.global(qos: .userInitiated))
      .compactMap { resources in
        return self.writeFilesToDisk(resources: resources, name: fileName) ? resources : nil
      }
      .flatMap { resources in
        self.setUpFiles(into: engine, resources: resources)
      }
      .map({ _ in () })
      .eraseToAnyPublisher()
  }

  private func fileFromDocumentsAsString(_ name: String, inFolder folder: String) -> String? {
    guard let folderUrl = FileManager.default.getOrCreateFolder(name: folder) else {
      log.error("Failed to get folder: \(folder)")
      return nil
    }

    let fileUrl = folderUrl.appendingPathComponent(name)
    guard let data = FileManager.default.contents(atPath: fileUrl.path) else { return nil }
    return String(data: data, encoding: .utf8)
  }

  private func writeFilesToDisk(resources: [CosmeticFilterNetworkResource], name: String) -> Bool {
    var fileSaveCompletions = [Bool]()
    let fm = FileManager.default
    let folderName = CosmeticFiltersResourceDownloader.folderName

    resources.forEach {
      let fileName = name + ".\($0.fileType.rawValue)"
      fileSaveCompletions.append(
        fm.writeToDiskInFolder(
          $0.resource.data, fileName: fileName,
          folderName: folderName))

      if let etag = $0.resource.etag, let data = etag.data(using: .utf8) {
        let etagFileName = fileName + ".etag"
        fileSaveCompletions.append(
          fm.writeToDiskInFolder(
            data, fileName: etagFileName,
            folderName: folderName))
      }

      if let lastModified = $0.resource.lastModifiedTimestamp,
        let data = String(lastModified).data(using: .utf8) {
        let lastModifiedFileName = fileName + ".lastmodified"
        fileSaveCompletions.append(
          fm.writeToDiskInFolder(
            data, fileName: lastModifiedFileName,
            folderName: folderName))
      }

    }

    // Returning true if all file saves completed succesfully
    return !fileSaveCompletions.contains(false)
  }

  private func setUpFiles(
    into engine: AdblockEngine,
    resources: [CosmeticFilterNetworkResource]
  ) -> AnyPublisher<Void, Error> {
    if resources.isEmpty {
      return Fail(error: "No Cosmetic Filters Resource to Setup").eraseToAnyPublisher()
    }
    
    let resources: [AnyPublisher<Void, Error>] = resources.compactMap({
      switch $0.fileType {
      case .dat:
        return self.setDataFile(
          into: engine,
          data: $0.resource.data,
          id: $0.type.identifier)
      case .json:
        return self.setJSONFile(
          into: engine,
          data: $0.resource.data,
          id: $0.type.identifier)
      }
    })
    
    return Publishers.MergeMany(resources)
      .collect()
      .subscribe(on: DispatchQueue.global(qos: .userInitiated))
      .map({ _ in () })
      .eraseToAnyPublisher()
  }

  private func setDataFile(into engine: AdblockEngine, data: Data, id: String) -> AnyPublisher<Void, Error> {
    Combine.Deferred {
      Future { completion in
        CosmeticFiltersResourceDownloader.queue.async {
          if engine.deserialize(data: data) {
            completion(.success(()))
          } else {
            completion(.failure("Failed to deserialize adblock list with id: \(id)"))
          }
        }
      }
    }.eraseToAnyPublisher()
  }

  private func setJSONFile(into engine: AdblockEngine, data: Data, id: String) -> AnyPublisher<Void, Error> {
    Combine.Deferred {
      Future { completion in
        CosmeticFiltersResourceDownloader.queue.async {
          if !CosmeticFiltersResourceDownloader.isValidJSONData(data) {
            completion(.failure("Invalid JSON Data"))
            return
          }
          
          if let json = String(data: data, encoding: .utf8) {
            engine.addResources(json)
            completion(.success(()))
          } else {
            completion(.failure("Invalid JSON String - Bad Encoding"))
          }
        }
      }
    }.eraseToAnyPublisher()
  }
  
  private static func isValidJSONData(_ data: Data) -> Bool {
    do {
      let value = try JSONSerialization.jsonObject(with: data, options: [])
      if let value = value as? NSArray {
        return value.count > 0
      }
      
      if let value = value as? NSDictionary {
        return value.count > 0
      }
      
      log.error("JSON Must have a top-level type of Array of Dictionary.")
      return false
    } catch {
      log.error("JSON Deserialization Failed: \(error)")
      return false
    }
  }
}

extension CosmeticFiltersResourceDownloader {
  enum CosmeticFilterType {
    case cosmeticSample
    case resourceSample

    var identifier: String {
      switch self {
      case .cosmeticSample: return "ios-cosmetic-filters"
      case .resourceSample: return "scriptlet-resources"
      }
    }

    var associatedFiles: [FileType] {
      switch self {
      case .cosmeticSample: return [.dat]
      case .resourceSample: return [.json]
      }
    }

    func resourceName(for fileType: FileType) -> String? {
      identifier
    }
  }
}
