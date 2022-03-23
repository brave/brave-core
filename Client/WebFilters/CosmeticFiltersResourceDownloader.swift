// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveShared

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

class CosmeticFiltersResourceDownloader {
  private static let queue = DispatchQueue(label: "com.brave.cosmecitc-filters-dispatch-queue")
  static let shared = CosmeticFiltersResourceDownloader()

  private let networkManager: NetworkManager

  static let folderName = "cmf-data"
  private let servicesKeyName = "SERVICES_KEY"
  private let servicesKeyHeaderValue = "BraveServiceKey"
  private var engine = AdblockRustEngine()
  private var didInitialLoad = false

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

  func startLoading() {
    let now = Date()
    let fetchInterval = AppConstants.buildChannel.isPublic ? 6.hours : 10.minutes

    if now.timeIntervalSince(lastFetchDate) >= fetchInterval {
      lastFetchDate = now

      if !didInitialLoad {
        Task.detached(priority: .userInitiated) { [weak self] in
          guard let self = self else { return }

          // Load files from disk into the original engine engine
          do {
            try await self.loadDownloadedFiles(into: self.engine)
            self.didInitialLoad = true
          } catch {
            log.error("Error Loading Cosmetic-Filters: \(error)")
          }
        }
      }

      Task.detached(priority: .userInitiated) { [weak self] in
        guard let self = self else { return }

        do {
          // All operations must be done on a temp engine,
          // otherwise we get insane load times when calling:
          // `engine_add_resources` on an existing engine
          // This is because `engine_add_resources` will ADD resources, and not delete old ones
          // Thus we get a huge amount of memory usage and slow down.
          let tempEngine = AdblockRustEngine()

          try await withThrowingTaskGroup(of: Void.self) { [weak self] group in
            guard let self = self else { return }
            group.addTask { try await self.downloadCosmeticSamples(with: tempEngine) }
            group.addTask { try await self.downloadResourceSamples(with: tempEngine) }
            try await group.waitForAll()
          }

          // Load downloaded files into the new engine
          do {
            let newEngine = AdblockRustEngine()
            try await self.loadDownloadedFiles(into: newEngine)

            // Once the new engine is setup, we can replace the old engine with the new one.
            self.engine = newEngine
          } catch {
            log.error("Error Loading Cosmetic-Filters: \(error)")
          }
        } catch {
          log.error("Failed to Download Cosmetic-Filters: \(error)")
        }
      }
    }
  }

  func cssRules(for url: URL) -> String? {
    engine.cssRules(for: url)
  }

  private func loadDownloadedFiles(into engine: AdblockRustEngine) async throws {
    let fm = FileManager.default
    guard let folderUrl = fm.getOrCreateFolder(name: CosmeticFiltersResourceDownloader.folderName) else {
      log.error("Could not get directory with .dat and .json files")
      return
    }

    let enumerator = fm.enumerator(at: folderUrl, includingPropertiesForKeys: nil)
    let filePaths = enumerator?.allObjects as? [URL]
    let datFileUrls = filePaths?.filter { $0.pathExtension == "dat" }
    let jsonFileUrls = filePaths?.filter { $0.pathExtension == "json" }

    try await datFileUrls?.asyncForEach({
      let fileName = $0.deletingPathExtension().lastPathComponent
      if let data = fm.contents(atPath: $0.path) {
        try await self.setDataFile(into: engine, data: data, id: fileName)
      }
    })

    try await jsonFileUrls?.asyncForEach {
      let fileName = $0.deletingPathExtension().lastPathComponent
      if let data = fm.contents(atPath: $0.path) {
        try await self.setJSONFile(into: engine, data: data, id: fileName)
      }
    }
  }

  private func downloadCosmeticSamples(with engine: AdblockRustEngine) async throws {
    try await downloadResources(for: engine, type: .cosmeticSample)
    log.debug("Downloaded Cosmetic Filters CSS Samples")
    Preferences.Debug.lastCosmeticFiltersCSSUpdate.value = Date()
  }

  private func downloadResourceSamples(with engine: AdblockRustEngine) async throws {
    try await downloadResources(for: engine, type: .resourceSample)
    log.debug("Downloaded Cosmetic Filters Scriptlets Samples")
    Preferences.Debug.lastCosmeticFiltersScripletsUpdate.value = Date()
  }

  private func downloadResources(
    for engine: AdblockRustEngine,
    type: CosmeticFilterType
  ) async throws {
    let nm = networkManager
    let folderName = CosmeticFiltersResourceDownloader.folderName

    // file name of which the file will be saved on disk
    let fileName = type.identifier

    async let completedDownloads = type.associatedFiles.asyncConcurrentCompactMap { [weak self] fileType -> CosmeticFilterNetworkResource? in
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

      let resource = try await nm.downloadResource(
        with: url,
        resourceType: .cached(etag: etag),
        checkLastServerSideModification: !AppConstants.buildChannel.isPublic,
        customHeaders: headers)

      if resource.data.isEmpty {
        return nil
      }

      return CosmeticFilterNetworkResource(
        resource: resource,
        fileType: fileType,
        type: type)
    }

    if try await self.writeFilesToDisk(resources: completedDownloads, name: fileName) {
      try await self.setUpFiles(into: engine, resources: completedDownloads)
    }
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

  private func writeFilesToDisk(resources: [CosmeticFilterNetworkResource], name: String) async -> Bool {
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
    into engine: AdblockRustEngine,
    resources: [CosmeticFilterNetworkResource]
  ) async throws {
    try await resources.asyncConcurrentForEach {
      switch $0.fileType {
      case .dat:
        try await self.setDataFile(
          into: engine,
          data: $0.resource.data,
          id: $0.type.identifier)
      case .json:
        try await self.setJSONFile(
          into: engine,
          data: $0.resource.data,
          id: $0.type.identifier)
      case .tgz:
        break
      }
    }
  }

  private func setDataFile(into engine: AdblockRustEngine, data: Data, id: String) async throws {
    try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Void, Error>) in
      CosmeticFiltersResourceDownloader.queue.async {
        do {
          try Task.checkCancellation()
        } catch {
          continuation.resume(throwing: error)
          return
        }

        if engine.set(data: data) {
          continuation.resume()
        } else {
          continuation.resume(throwing: "Failed to deserialize adblock list with id: \(id)")
        }
      }
    }
  }

  private func setJSONFile(into engine: AdblockRustEngine, data: Data, id: String) async throws {
    try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Void, Error>) in
      CosmeticFiltersResourceDownloader.queue.async {
        do {
          try Task.checkCancellation()
        } catch {
          continuation.resume(throwing: error)
          return
        }

        if !CosmeticFiltersResourceDownloader.isValidJSONData(data) {
          continuation.resume(throwing: "Invalid JSON Data")
          return
        }
        
        if engine.set(json: data) {
          continuation.resume()
        } else {
          continuation.resume(throwing: "Invalid JSON String - Bad Encoding")
        }
      }
    }
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
