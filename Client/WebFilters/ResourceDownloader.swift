// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveCore

/// A ganeric resource downloader class that is responsible for fetching resources
actor ResourceDownloader: Sendable {
  enum Resource: Hashable {
    /// Rules for debouncing links
    case debounceRules
    /// Generic filter rules for any locale
    case genericFilterRules
    /// Generic iOS only content blocking behaviours used for the iOS content blocker
    case genericContentBlockingBehaviors
    /// Cosmetic filter rules
    case generalCosmeticFilters
    /// Resources for cosmetic filters
    case generalScriptletResources
    /// Adblock rules for a filter list
    case filterListAdBlockRules(uuid: String, componentId: String)
    /// iOS only content blocking behaviours used for the iOS content blocker for a given filter list
    case filterListContentBlockingBehaviors(uuid: String, componentId: String)
    /// General external file
    case dataFile(URL, cacheFolderName: String, cacheFileName: String)
    
    /// The folder name under which this data should be saved under
    var cacheFolderName: String {
      switch self {
      case .debounceRules:
        return "debounce-data"
      case .filterListContentBlockingBehaviors(_, let componentId), .filterListAdBlockRules(_, let componentId):
        return ["filter-lists", componentId].joined(separator: "/")
      case .genericFilterRules, .genericContentBlockingBehaviors:
        return "abp-data"
      case .generalCosmeticFilters, .generalScriptletResources:
        return "cmf-data"
      case .dataFile(_, let cacheFolderName, _):
        return cacheFolderName
      }
    }
    
    /// The name of the etag save into the cache folder
    fileprivate var etagFileName: String {
      return [cacheFileName, "etag"].joined(separator: ".")
    }
    
    /// Get the file name that is stored on the device
    var cacheFileName: String {
      switch self {
      case .debounceRules:
        return "ios-debouce.json"
      case .filterListAdBlockRules(let uuid, _):
        return "\(uuid)-latest.txt"
      case .filterListContentBlockingBehaviors(let uuid, _):
        return "\(uuid)-latest.json"
      case .genericFilterRules:
        return "latest.txt"
      case .genericContentBlockingBehaviors:
        return "latest.json"
      case .generalCosmeticFilters:
        return "ios-cosmetic-filters.dat"
      case .generalScriptletResources:
        return "scriptlet-resources.json"
      case .dataFile(_, _, let cacheFileName):
        return cacheFileName
      }
    }
    
    /// The base s3 environment url that hosts the debouncing (and other) files.
    /// Cannot be used as-is and must be combined with a path
    private static var baseResourceURL: URL = {
      if AppConstants.buildChannel.isPublic {
        return URL(string: "https://adblock-data.s3.brave.com")!
      } else {
        return URL(string: "https://adblock-data-staging.s3.bravesoftware.com")!
      }
    }()
    
    /// Get the external path for the given filter list and this resource type
    var externalURL: URL {
      switch self {
      case .debounceRules:
        return Self.baseResourceURL.appendingPathComponent("/ios/debounce.json")
      case .filterListContentBlockingBehaviors(let uuid, _):
        return Self.baseResourceURL.appendingPathComponent("/ios/\(uuid)-latest.json")
      case .filterListAdBlockRules(let uuid, _):
        return Self.baseResourceURL.appendingPathComponent("/ios/\(uuid)-latest.txt")
      case .genericFilterRules:
        return Self.baseResourceURL.appendingPathComponent("/ios/latest.txt")
      case .genericContentBlockingBehaviors:
        return Self.baseResourceURL.appendingPathComponent("/ios/latest.json")
      case .generalCosmeticFilters:
        return Self.baseResourceURL.appendingPathComponent("/ios/ios-cosmetic-filters.dat")
      case .generalScriptletResources:
        return Self.baseResourceURL.appendingPathComponent("/ios/scriptlet-resources.json")
      case .dataFile(let url, _, _):
        return url
      }
    }
  }
  
  /// An object representing errors with the resource downloader
  enum ResourceDownloaderError: Error {
    case failedToCreateCacheFolder
  }
  
  /// An object representing errors during a resource download
  enum DownloadResultError: Error {
    case noData
  }
  
  /// An object represening the download result
  enum DownloadResult<Result> {
    case notModified(URL, Date)
    case downloaded(Result, Date)
  }
  
  /// The directory to which we should store all the dowloaded files into
  private static var cacheFolderDirectory: FileManager.SearchPathDirectory {
    return FileManager.SearchPathDirectory.applicationSupportDirectory
  }
  
  private static var defaultFetchInterval: TimeInterval {
    return AppConstants.buildChannel.isPublic ? 6.hours : 10.minutes
  }
  
  /// The name of the info plist key that contains the service key
  private static let servicesKeyName = "SERVICES_KEY"
  /// The name of the header value that contains the service key
  private static let servicesKeyHeaderValue = "BraveServiceKey"
  /// The netowrk manager performing the requests
  private let networkManager: NetworkManager
  
  /// Initialize this class with the given network manager
  init(networkManager: NetworkManager = NetworkManager()) {
    self.networkManager = networkManager
  }
  
  func downloadStream(for resource: Resource, every fetchInterval: TimeInterval = defaultFetchInterval) -> ResourceDownloaderStream {
    return ResourceDownloaderStream(resource: resource, resourceDownloader: self, fetchInterval: fetchInterval)
  }
  
  /// Download the give resource type for the filter list and store it into the cache folder url
  @discardableResult
  func download(resource: Resource) async throws -> DownloadResult<URL> {
    let result = try await downloadInternal(resource: resource)
    
    switch result {
    case .downloaded(let networkResource, let date):
      // Clear any old data
      try Self.removeFile(for: resource)
      // Make a cache folder if needed
      let cacheFolderURL = try Self.getOrCreateCacheFolder(for: resource)
      // Save the data to file
      let fileURL = cacheFolderURL.appendingPathComponent(resource.cacheFileName)
      try Self.writeDataToDisk(data: networkResource.data, toFileURL: fileURL)
      // Save the etag to file
      if let data = networkResource.etag?.data(using: .utf8) {
        try Self.writeDataToDisk(
          data: data,
          toFileURL: cacheFolderURL.appendingPathComponent(resource.etagFileName)
        )
      }
      // Return the file URL
      let creationDate = try? Self.creationDate(for: resource)
      return .downloaded(fileURL, creationDate ?? date)
    case .notModified(let url, let date):
      let creationDate = try? Self.creationDate(for: resource)
      return .notModified(url, creationDate ?? date)
    }
  }
  
  private func downloadInternal(resource: Resource) async throws -> DownloadResult<CachedNetworkResource> {
    var headers = [String: String]()
    
    if let servicesKeyValue = Bundle.main.getPlistString(for: Self.servicesKeyName) {
      headers[Self.servicesKeyHeaderValue] = servicesKeyValue
    }
    
    let etag = try? Self.etag(for: resource)
    
    do {
      let networkResource = try await self.networkManager.downloadResource(
        with: resource.externalURL,
        resourceType: .cached(etag: etag),
        checkLastServerSideModification: !AppConstants.buildChannel.isPublic,
        customHeaders: headers)
      
      guard !networkResource.data.isEmpty else {
        throw DownloadResultError.noData
      }
      
      return .downloaded(networkResource, Date())
    } catch let error as String {
      if error == "File not modified", let fileURL = Self.downloadedFileURL(for: resource) {
        return .notModified(fileURL, Date())
      } else {
        throw error
      }
    }
  }
  
  /// Get or create a cache folder for the given `Resource`
  ///
  /// - Note: This technically can't really return nil as the location and folder are hard coded
  private static func getOrCreateCacheFolder(for resource: Resource) throws -> URL {
    guard let folderURL = FileManager.default.getOrCreateFolder(
      name: resource.cacheFolderName,
      location: Self.cacheFolderDirectory
    ) else {
      throw ResourceDownloaderError.failedToCreateCacheFolder
    }
    
    return folderURL
  }
  
  /// Load the data for the given `Resource` if it exists.
  ///
  /// - Note: Return nil if the data does not exist
  static func data(for resource: Resource) throws -> Data? {
    guard let fileUrl = downloadedFileURL(for: resource) else { return nil }
    return FileManager.default.contents(atPath: fileUrl.path)
  }
  
  /// Load the string for the given `Resource` if it exists.
  ///
  /// - Note: Return nil if the data does not exist
  static func string(for resource: Resource) throws -> String? {
    guard let data = try self.data(for: resource) else { return nil }
    return String(data: data, encoding: .utf8)
  }
  
  /// Get the downloaded file URL for the filter list and resource type
  ///
  /// - Note: Returns nil if the file does not exist
  static func downloadedFileURL(for resource: Resource) -> URL? {
    guard let cacheFolderURL = createdCacheFolderURL(for: resource) else {
      return nil
    }
    
    let fileURL = cacheFolderURL.appendingPathComponent(resource.cacheFileName)
    
    if FileManager.default.fileExists(atPath: fileURL.path) {
      return fileURL
    } else {
      return nil
    }
  }
  
  /// Get the file url for the downloaded file's etag
  ///
  /// - Note: Returns nil if the etag does not exist
  static func etagURL(for resource: Resource) -> URL? {
    guard let cacheFolderURL = createdCacheFolderURL(for: resource) else { return nil }
    let fileURL = cacheFolderURL.appendingPathComponent(resource.etagFileName)
    
    if FileManager.default.fileExists(atPath: fileURL.path) {
      return fileURL
    } else {
      return nil
    }
  }
  
  /// Get an existing etag for the given `Resource`
  static func creationDate(for resource: Resource) throws -> Date? {
    guard let fileURL = downloadedFileURL(for: resource) else { return nil }
    let fileAttributes = try FileManager.default.attributesOfItem(atPath: fileURL.path)
    return fileAttributes[.creationDate] as? Date
  }
  
  /// Get an existing etag for the given `Resource`
  static func etag(for resource: Resource) throws -> String? {
    guard let fileURL = etagURL(for: resource) else { return nil }
    guard let data = FileManager.default.contents(atPath: fileURL.path) else { return nil }
    return String(data: data, encoding: .utf8)
  }
  
  /// Get the cache folder for the given `Resource`
  ///
  /// - Note: Returns nil if the cache folder does not exist
  static func createdCacheFolderURL(for resource: Resource) -> URL? {
    guard let folderURL = cacheFolderDirectory.url else { return nil }
    let cacheFolderURL = folderURL.appendingPathComponent(resource.cacheFolderName)
    
    if FileManager.default.fileExists(atPath: cacheFolderURL.path) {
      return cacheFolderURL
    } else {
      return nil
    }
  }
  
  /// Removes all the data for the given `Resource`
  static func removeFile(for resource: Resource) throws {
    guard
      let fileURL = self.downloadedFileURL(for: resource)
    else {
      return
    }
    
    try FileManager.default.removeItem(atPath: fileURL.path)
  }

  /// Write the given `Data` to disk into to the specified file `URL`
  /// into the `applicationSupportDirectory` `SearchPathDirectory`.
  ///
  /// - Note: `fileName` must contain the full file name including the extension.
  private static func writeDataToDisk(data: Data, toFileURL fileURL: URL) throws {
    try data.write(to: fileURL, options: [.atomic])
  }
  
  /// Removes all the data for the given `Resource`
  private static func removeCacheFolder(for resource: Resource) throws {
    guard
      let folderURL = self.createdCacheFolderURL(for: resource)
    else {
      return
    }
    
    try FileManager.default.removeItem(atPath: folderURL.path)
  }
  
  #if DEBUG
  /// Convenience method for tests
  public static func getMockResponse(
    for resource: ResourceDownloader.Resource,
    statusCode code: Int = 200,
    headerFields: [String: String]? = nil
  ) -> HTTPURLResponse {
    return HTTPURLResponse(
      url: resource.externalURL, statusCode: code,
      httpVersion: "HTTP/1.1", headerFields: headerFields)!
  }
  #endif
}
