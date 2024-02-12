// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import UIKit
import os.log

extension FileManager {
  public enum Folder: String {
    case cookie
    case webSiteData
    
    public var rawValue: String {
      switch self {
      case .cookie: return "/Cookies"
      case .webSiteData:
        #if targetEnvironment(simulator)
        if let bundleId = Bundle.main.bundleIdentifier {
          return "/WebKit/\(bundleId)/WebsiteData"
        }
        #endif
        return "/WebKit/WebsiteData"
      }
    }
  }
  public typealias FolderLockObj = (folder: Folder, lock: Bool)

  /// URL where files downloaded by user are stored.
  /// If the download folder doesn't exists it creates a new one
  public func downloadsPath() throws -> URL {
    FileManager.default.getOrCreateFolder(name: "Downloads", excludeFromBackups: true, location: .documentDirectory)

    return try FileManager.default.url(
      for: .documentDirectory, in: .userDomainMask,
      appropriateFor: nil, create: false
    ).appendingPathComponent("Downloads")
  }

  // Lock a folder using FolderLockObj provided.
  @discardableResult public func setFolderAccess(_ lockObjects: [FolderLockObj]) -> Bool {
    guard let baseDir = baseDirectory() else { return false }
    for lockObj in lockObjects {
      do {
        try self.setAttributes([.posixPermissions: (lockObj.lock ? 0 : 0o755)], ofItemAtPath: baseDir + lockObj.folder.rawValue)
      } catch {
        Logger.module.error("Failed to \(lockObj.lock ? "Lock" : "Unlock") item at path \(lockObj.folder.rawValue) with error: \n\(error.localizedDescription)")
        return false
      }
    }
    return true
  }

  // Check the locked status of a folder. Returns true for locked.
  public func checkLockedStatus(folder: Folder) -> Bool {
    guard let baseDir = baseDirectory() else { return false }
    do {
      if let lockValue = try self.attributesOfItem(atPath: baseDir + folder.rawValue)[.posixPermissions] as? NSNumber {
        return lockValue == 0o755
      }
    } catch {
      Logger.module.error("Failed to check lock status on item at path \(folder.rawValue) with error: \n\(error.localizedDescription)")
    }
    return false
  }

  public func writeToDiskInFolder(
    _ data: Data, fileName: String, folderName: String,
    location: SearchPathDirectory = .applicationSupportDirectory
  ) -> Bool {

    guard let folderUrl = getOrCreateFolder(name: folderName, location: location) else { return false }

    do {
      let fileUrl = folderUrl.appendingPathComponent(fileName)
      try data.write(to: fileUrl, options: [.atomic])
    } catch {
      Logger.module.error("Failed to write data, error: \(error.localizedDescription)")
      return false
    }

    return true
  }

  /// Creates a folder at given location and returns its URL.
  /// If folder already exists, returns its URL as well.
  @discardableResult
  public func getOrCreateFolder(
    name: String, excludeFromBackups: Bool = true,
    location: SearchPathDirectory = .applicationSupportDirectory
  ) -> URL? {
    guard let documentsDir = location.url else { return nil }

    var folderDir = documentsDir.appendingPathComponent(name)

    if fileExists(atPath: folderDir.path) { return folderDir }

    do {
      try createDirectory(at: folderDir, withIntermediateDirectories: true, attributes: nil)

      if excludeFromBackups {
        var resourceValues = URLResourceValues()
        resourceValues.isExcludedFromBackup = true
        try folderDir.setResourceValues(resourceValues)
      }

      return folderDir
    } catch {
      Logger.module.error("Failed to create folder, error: \(error.localizedDescription)")
      return nil
    }
  }

  public func removeFolder(withName name: String, location: SearchPathDirectory) {
    guard let locationUrl = location.url else { return }
    let fileUrl = locationUrl.appendingPathComponent(name)

    if !fileExists(atPath: fileUrl.path) {
      Logger.module.debug("File \(fileUrl) doesn't exist")
      return
    }

    do {
      try removeItem(at: fileUrl)
    } catch {
      Logger.module.error("\(error.localizedDescription)")
    }
  }

  public func moveFile(
    sourceName: String, sourceLocation: SearchPathDirectory,
    destinationName: String, destinationLocation: SearchPathDirectory
  ) {
    guard let sourceLocation = sourceLocation.url,
      let destinationLocation = destinationLocation.url
    else {
      return
    }

    let sourceFileUrl = sourceLocation.appendingPathComponent(sourceName)
    let destinationFileUrl = destinationLocation.appendingPathComponent(destinationName)

    if !fileExists(atPath: sourceFileUrl.path) {
      Logger.module.debug("File \(sourceFileUrl) doesn't exist")
      return
    }

    do {
      try moveItem(at: sourceFileUrl, to: destinationFileUrl)
    } catch {
      Logger.module.error("\(error.localizedDescription)")
    }
  }

  private func baseDirectory() -> String? {
    return NSSearchPathForDirectoriesInDomains(.libraryDirectory, .userDomainMask, true).first
  }
  
  /// Returns size of contents of the directory in bytes.
  /// Returns nil if any error happened during the size calculation.
  public func directorySize(at directoryURL: URL) throws -> UInt64? {
    let allocatedSizeResourceKeys: Set<URLResourceKey> = [
        .isRegularFileKey,
        .fileAllocatedSizeKey,
        .totalFileAllocatedSizeKey,
    ]
    
    func fileSize(_ file: URL) throws -> UInt64 {
        let resourceValues = try file.resourceValues(forKeys: allocatedSizeResourceKeys)

        // We only look at regular files.
        guard resourceValues.isRegularFile ?? false else {
            return 0
        }

        // To get the file's size we first try the most comprehensive value in terms of what
        // the file may use on disk. This includes metadata, compression (on file system
        // level) and block size.
        // In case totalFileAllocatedSize is unavailable we use the fallback value (excluding
        // meta data and compression) This value should always be available.
        return UInt64(resourceValues.totalFileAllocatedSize ?? resourceValues.fileAllocatedSize ?? 0)
    }
    
    // We have to enumerate all directory contents, including subdirectories.
    guard let enumerator = self.enumerator(at: directoryURL,
                                     includingPropertiesForKeys: Array(allocatedSizeResourceKeys)) else { return nil }
    
    var totalSize: UInt64 = 0
    
    // Perform the traversal.
    for item in enumerator {
      // Add up individual file sizes.
      guard let contentItemURL = item as? URL else { continue }
      totalSize += try fileSize(contentItemURL)
    }
    
    return totalSize
  }
}

extension FileManager.SearchPathDirectory {

  /// Returns first url in user domain mask of given search path directory
  public var url: URL? {
    return FileManager.default.urls(for: self, in: .userDomainMask).first
  }
}

extension FileManager {

  /// Navigates to download Folder inside the application's folder
  public func openBraveDownloadsFolder(_ completion: @escaping (Bool) -> Void) {
    do {
      guard
        var downloadsPathComponents = URLComponents(
          url: try FileManager.default.downloadsPath(),
          resolvingAgainstBaseURL: false)
      else {
        completion(false)
        return
      }

      downloadsPathComponents.scheme = "shareddocuments"

      guard let braveFolderURL = downloadsPathComponents.url else {
        completion(false)
        return
      }

      UIApplication.shared.open(braveFolderURL) { success in
        completion(success)
      }
    } catch {
      completion(false)
      Logger.module.error("Unable to get downloads path: \(error.localizedDescription)")
    }
  }
}
