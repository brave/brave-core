// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared

private let log = Logger.browserLogger

public extension FileManager {
    public enum Folder: String {
        case cookie = "/Cookies"
        case webSiteData = "/WebKit/WebsiteData"
    }
    typealias FolderLockObj = (folder: Folder, lock: Bool)
    
    //Lock a folder using FolderLockObj provided.
    @discardableResult public func setFolderAccess(_ lockObjects: [FolderLockObj]) -> Bool {
        let baseDir = NSSearchPathForDirectoriesInDomains(.libraryDirectory, .userDomainMask, true)[0]
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
        let baseDir = NSSearchPathForDirectoriesInDomains(.libraryDirectory, .userDomainMask, true)[0]
        do {
            if let lockValue = try self.attributesOfItem(atPath: baseDir + folder.rawValue)[.posixPermissions] as? NSNumber {
                return lockValue == 0o755
            }
        } catch {
            log.error("Failed to check lock status on item at path \(folder.rawValue) with error: \n\(error)")
        }
        return false
    }
}
