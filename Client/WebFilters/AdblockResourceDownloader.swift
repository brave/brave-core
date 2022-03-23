// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveShared

private let log = Logger.browserLogger

private struct AdBlockNetworkResource {
  let resource: CachedNetworkResource
  let fileType: FileType
  let type: AdblockerType
}

class AdblockResourceDownloader {
  static let shared = AdblockResourceDownloader()

  private let networkManager: NetworkManager
  private let locale: String

  static let folderName = "abp-data"
  private let servicesKeyName = "SERVICES_KEY"
  private let servicesKeyHeaderValue = "BraveServiceKey"

  static let endpoint = "https://adblock-data.s3.brave.com/ios"

  init(networkManager: NetworkManager = NetworkManager(), locale: String? = Locale.current.languageCode) {
    if locale == nil {
      log.warning("No locale provided, using default one(\"en\")")
    }
    self.locale = locale ?? "en"
    self.networkManager = networkManager

    Preferences.Shields.useRegionAdBlock.observe(from: self)
  }

  /// Initialized with year 1970 to force adblock fetch at first launch.
  private(set) var lastFetchDate = Date(timeIntervalSince1970: 0)

  func startLoading() {
    let now = Date()
    let fetchInterval = AppConstants.buildChannel.isPublic ? 6.hours : 10.minutes

    if now.timeIntervalSince(lastFetchDate) >= fetchInterval {
      lastFetchDate = now

      Task.detached(priority: .userInitiated) {
        do {
          try await withThrowingTaskGroup(of: Void.self) { [weak self] group in
            guard let self = self else { return }
            group.addTask { try await self.regionalAdblockResourcesSetup() }
            group.addTask { try await self.generalAdblockResourcesSetup() }
            try await group.waitForAll()
          }
        } catch {
          log.error("Failed to Download Adblock-Resources: \(error)")
        }
      }
    }
  }

  func regionalAdblockResourcesSetup() async throws {
    if !Preferences.Shields.useRegionAdBlock.value {
      log.debug("Regional adblocking disabled, aborting attempt to download regional resources")
      return
    }

    try await downloadResources(
      type: .regional(locale: locale),
      queueName: "Regional adblock setup")
    log.debug("Regional blocklists download and setup completed.")
    Preferences.Debug.lastRegionalAdblockUpdate.value = Date()
  }

  func generalAdblockResourcesSetup() async throws {
    try await downloadResources(
      type: .general,
      queueName: "General adblock setup")
    log.debug("General blocklists download and setup completed.")
    Preferences.Debug.lastGeneralAdblockUpdate.value = Date()
  }

  private func downloadResources(type: AdblockerType, queueName: String) async throws {
    let nm = networkManager
    let folderName = AdblockResourceDownloader.folderName

    // file name of which the file will be saved on disk
    let fileName = type.identifier

    async let completedDownloads = type.associatedFiles.asyncConcurrentCompactMap { [weak self] fileType -> AdBlockNetworkResource? in
      guard let self = self else { return nil }

      let fileExtension = fileType.rawValue
      let etagExtension = fileExtension + ".etag"

      guard let resourceName = type.resourceName(for: fileType),
        var url = URL(string: AdblockResourceDownloader.endpoint)
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

      return AdBlockNetworkResource(
        resource: resource,
        fileType: fileType,
        type: type)
    }

    // json to content rules compilation happens first, otherwise it makes no sense to proceed further
    // and overwrite old files that were working before.
    try await compileContentBlocker(resources: completedDownloads)
    if try await writeFilesToDisk(resources: completedDownloads, name: fileName) {
      try await setUpFiles(resources: completedDownloads, compileJsonRules: false)
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

  private func compileContentBlocker(resources: [AdBlockNetworkResource]) async {
    await resources.filter { $0.fileType == .json }
      .asyncConcurrentForEach { res in
        guard let blockList = res.type.blockListName else { return }
        await blockList.compile(data: res.resource.data)
      }
  }

  private func writeFilesToDisk(resources: [AdBlockNetworkResource], name: String) async -> Bool {
    var fileSaveCompletions = [Bool]()
    let fm = FileManager.default
    let folderName = AdblockResourceDownloader.folderName

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

  private func setUpFiles(resources: [AdBlockNetworkResource], compileJsonRules: Bool) async throws {
    try await resources.asyncConcurrentForEach {
      switch $0.fileType {
      case .dat:
        try await AdBlockStats.shared.setDataFile(
          data: $0.resource.data,
          id: $0.type.identifier)
      case .json:
        if compileJsonRules {
          await self.compileContentBlocker(resources: resources)
        }
      case .tgz:
        break  // TODO: Add downloadable httpse list
      }
    }
  }
}

extension AdblockResourceDownloader: PreferencesObserver {
  func preferencesDidChange(for key: String) {
    let regionalAdblockPref = Preferences.Shields.useRegionAdBlock
    if key == regionalAdblockPref.key {
      Task {
        do {
          try await regionalAdblockResourcesSetup()
        } catch {
          log.error(error)
        }
      }
    }
  }
}
