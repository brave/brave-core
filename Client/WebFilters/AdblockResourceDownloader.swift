// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveShared
import Combine

private let log = Logger.browserLogger

private struct AdBlockNetworkResource {
  let resource: CachedNetworkResource
  let fileType: FileType
  let type: AdblockerType
}

public class AdblockResourceDownloader {
  public static let shared = AdblockResourceDownloader()

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
  private var adblockResourceDownloader: AnyCancellable?

  public func startLoading() {
    let now = Date()
    let fetchInterval = AppConstants.buildChannel.isPublic ? 6.hours : 10.minutes

    if now.timeIntervalSince(lastFetchDate) >= fetchInterval {
      lastFetchDate = now
      
      let regionalSetup = regionalAdblockResourcesSetup().catch { error -> AnyPublisher<Void, Never> in
        log.error("Failed to Download Regional Adblock-Resources: \(error)")
        return Just(()).eraseToAnyPublisher()
      }
      
      let generalSetup = generalAdblockResourcesSetup().catch { error -> AnyPublisher<Void, Never> in
        log.error("Failed to Download General Adblock-Resources: \(error)")
        return Just(()).eraseToAnyPublisher()
      }
      
      adblockResourceDownloader = Publishers.Merge(regionalSetup, generalSetup)
        .collect()
        .receive(on: DispatchQueue.main)
        .sink { _ in
          log.debug("Finished Setting Up Adblock-Resources")
        }
    }
  }

  func regionalAdblockResourcesSetup() -> AnyPublisher<Void, Error> {
    if !Preferences.Shields.useRegionAdBlock.value {
      log.debug("Regional adblocking disabled, aborting attempt to download regional resources")
      return Empty(outputType: Void.self, failureType: Error.self).eraseToAnyPublisher()
    }

    return downloadResources(type: .regional(locale: locale))
      .receive(on: DispatchQueue.main)
      .map {
        log.debug("Regional blocklists download and setup completed.")
        Preferences.Debug.lastRegionalAdblockUpdate.value = Date()
      }.eraseToAnyPublisher()
  }

  func generalAdblockResourcesSetup() -> AnyPublisher<Void, Error> {
    return downloadResources(type: .general)
      .receive(on: DispatchQueue.main)
      .map {
        log.debug("General blocklists download and setup completed.")
        Preferences.Debug.lastGeneralAdblockUpdate.value = Date()
      }.eraseToAnyPublisher()
  }

  private func downloadResources(type: AdblockerType) -> AnyPublisher<Void, Error> {
    let nm = networkManager
    let folderName = AdblockResourceDownloader.folderName

    // file name of which the file will be saved on disk
    let fileName = type.identifier
    
    let completedDownloads = type.associatedFiles.compactMap({ [weak self] fileType -> AnyPublisher<AdBlockNetworkResource, Error>? in
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
      return nm.downloadResource(
        with: url,
        resourceType: .cached(etag: etag),
        checkLastServerSideModification: !AppConstants.buildChannel.isPublic,
        customHeaders: headers)
        .compactMap({ resource in
          if resource.data.isEmpty {
            return nil
          }

          return AdBlockNetworkResource(
            resource: resource,
            fileType: fileType,
            type: type)
        }).eraseToAnyPublisher()
    })
    
    return Publishers.MergeMany(completedDownloads)
      .collect()
      .subscribe(on: DispatchQueue.global(qos: .userInitiated))
      .flatMap { resources in
        // json to content rules compilation happens first, otherwise it makes no sense to proceed further
        // and overwrite old files that were working before.
        self.compileContentBlocker(resources: resources).compactMap({
          self.writeFilesToDisk(resources: resources, name: fileName) ? resources : nil
        })
      }
      .flatMap {
        return self.setUpFiles(resources: $0, compileJsonRules: false)
      }
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

  private func compileContentBlocker(resources: [AdBlockNetworkResource]) -> AnyPublisher<Void, Error> {
    if resources.isEmpty {
      return Fail(error: "No Adblock Resource to Compile").eraseToAnyPublisher()
    }
    
    let lists = resources.filter({ $0.fileType == .json })
      .compactMap({ res in
        return res.type.blockListName?.compile(data: res.resource.data)
      })
    
    return Publishers.MergeMany(lists)
      .collect()
      .map { _ in () }
      .eraseToAnyPublisher()
  }

  private func writeFilesToDisk(resources: [AdBlockNetworkResource], name: String) -> Bool {
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

  private func setUpFiles(resources: [AdBlockNetworkResource], compileJsonRules: Bool) -> AnyPublisher<Void, Error> {
    if resources.isEmpty {
      return Fail(error: "No Adblock Resource to Setup").eraseToAnyPublisher()
    }
    
    let resources: [AnyPublisher<Void, Error>] = resources.compactMap {
      switch $0.fileType {
      case .dat:
        return AdBlockStats.shared.setDataFile(
          data: $0.resource.data,
          id: $0.type.identifier)
      case .json:
        if compileJsonRules {
          return self.compileContentBlocker(resources: resources)
        }
        return nil
      case .tgz:
        return nil  // TODO: Add downloadable httpse list
      }
    }
    
    return Publishers.MergeMany(resources)
      .collect()
      .map { _ in () }
      .eraseToAnyPublisher()
  }
}

extension AdblockResourceDownloader: PreferencesObserver {
  public func preferencesDidChange(for key: String) {
    let regionalAdblockPref = Preferences.Shields.useRegionAdBlock
    if key == regionalAdblockPref.key {
      var cancellable: AnyCancellable?
      cancellable = regionalAdblockResourcesSetup()
        .subscribe(on: DispatchQueue.global(qos: .userInitiated))
        .receive(on: DispatchQueue.main)
        .sink { res in
          if case .failure(let error) = res {
            log.error(error)
          }
          cancellable = nil
        } receiveValue: { _ in
          log.debug("Successfully Setup Adblock Regional Preferences")
        }
    }
  }
}
