// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared

private let log = Logger.browserLogger

extension FileManager {
    public enum Folder: String {
        case cookie = "/Cookies"
        case webSiteData = "/WebKit/WebsiteData"
    }
    public typealias FolderLockObj = (folder: Folder, lock: Bool)
    
    /// URL where files downloaded by user are stored.
    /// If the download folder doesn't exists it creates a new one
    func downloadsPath() throws -> URL {
        FileManager.default.getOrCreateFolder(name: "Downloads", excludeFromBackups: true, location: .documentDirectory)
        
        return try FileManager.default.url(for: .documentDirectory, in: .userDomainMask,
                                       appropriateFor: nil, create: false).appendingPathComponent("Downloads")
    }
    
    //Lock a folder using FolderLockObj provided.
    @discardableResult public func setFolderAccess(_ lockObjects: [FolderLockObj]) -> Bool {
        guard let baseDir = baseDirectory() else { return false }
        for lockObj in lockObjects {
            do {
                try self.setAttributes([.posixPermissions: (lockObj.lock ? 0 : 0o755)], ofItemAtPath: baseDir + lockObj.folder.rawValue)
            } catch {
                log.error("Failed to \(lockObj.lock ? "Lock" : "Unlock") item at path \(lockObj.folder.rawValue) with error: \n\(error)")
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
            log.error("Failed to check lock status on item at path \(folder.rawValue) with error: \n\(error)")
        }
        return false
    }
    
    func writeToDiskInFolder(_ data: Data, fileName: String, folderName: String,
                             location: SearchPathDirectory = .applicationSupportDirectory) -> Bool {
        
        guard let folderUrl = getOrCreateFolder(name: folderName, location: location) else { return false }
        
        do {
            let fileUrl = folderUrl.appendingPathComponent(fileName)
            try data.write(to: fileUrl, options: [.atomic])
        } catch {
            log.error("Failed to write data, error: \(error)")
            return false
        }

        return true
    }
    
    /// Creates a folder at given location and returns its URL.
    /// If folder already exists, returns its URL as well.
    @discardableResult
    func getOrCreateFolder(name: String, excludeFromBackups: Bool = true,
                           location: SearchPathDirectory = .applicationSupportDirectory) -> URL? {
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
            log.error("Failed to create folder, error: \(error)")
            return nil
        }
    }
    
    func removeFolder(withName name: String, location: SearchPathDirectory) {
        guard let locationUrl = location.url else { return }
        let fileUrl = locationUrl.appendingPathComponent(name)
        
        if !fileExists(atPath: fileUrl.path) {
            log.debug("File \(fileUrl) doesn't exist")
            return
        }
        
        do {
            try removeItem(at: fileUrl)
        } catch {
            log.error(error)
        }
    }
    
    func moveFile(sourceName: String, sourceLocation: SearchPathDirectory,
                  destinationName: String, destinationLocation: SearchPathDirectory) {
        guard let sourceLocation = sourceLocation.url,
            let destinationLocation = destinationLocation.url else {
            return
        }
        
        let sourceFileUrl = sourceLocation.appendingPathComponent(sourceName)
        let destinationFileUrl = destinationLocation.appendingPathComponent(destinationName)
        
        if !fileExists(atPath: sourceFileUrl.path) {
            log.debug("File \(sourceFileUrl) doesn't exist")
            return
        }
        
        do {
            try moveItem(at: sourceFileUrl, to: destinationFileUrl)
        } catch {
            log.error(error)
        }
    }
    
    private func baseDirectory() -> String? {
         return NSSearchPathForDirectoriesInDomains(.libraryDirectory, .userDomainMask, true).first
    }
}

extension FileManager.SearchPathDirectory {
    
    /// Returns first url in user domain mask of given search path directory
    var url: URL? {
        return FileManager.default.urls(for: self, in: .userDomainMask).first
    }
}
