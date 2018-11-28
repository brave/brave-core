/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import Data
import Deferred
import BraveShared

// A base protocol for something that can be cleared.
protocol Clearable {
    func clear() -> Success
    var label: String { get }
}

class ClearableError: MaybeErrorType {
    fileprivate let msg: String
    init(msg: String) {
        self.msg = msg
    }
    
    var description: String { return msg }
}

struct ClearableErrorType: MaybeErrorType {
    let err: Error
    
    init(err: Error) {
        self.err = err
    }
    
    var description: String {
        return "Couldn't clear: \(err)."
    }
}

// Delete all the contents of a the folder, and verify using validateClearedWithNameContains that critical files are removed (any remaining file must not contain the specified substring(s))
// Alert the user if these files still exist after clearing.
// validateClearedWithNameContains can be nil, in which case the check is skipped or pass [] as a special case to verify that
// the directory is empty.
private func deleteLibraryFolderContents(_ folder: String, validateClearedExceptFor: [String]?) throws {
    let manager = FileManager.default
    let library = manager.urls(for: FileManager.SearchPathDirectory.libraryDirectory, in: .userDomainMask)[0]
    let dir = library.appendingPathComponent(folder)
    let contents = try manager.contentsOfDirectory(atPath: dir.path)
    for content in contents {
        do {
            try manager.removeItem(at: dir.appendingPathComponent(content))
        } catch where ((error as NSError).userInfo[NSUnderlyingErrorKey] as? NSError)?.code == Int(EPERM) {
            // "Not permitted". We ignore this.
            // Snapshots directory is an example of a Cache dir that is not permitted on device (but is permitted on simulator)
        }
    }
    
    #if DEBUG
    guard let allowedFileNames = validateClearedExceptFor else { return }
    contents = try manager.contentsOfDirectoryAtPath(dir.path, withFilenamePrefix: "")
    for content in contents {
        for name in allowedFileNames {
            if !content.contains(name) {
                let alert = UIAlertController(title: "Error clearing data", message: "Item not cleared: \(content)", preferredStyle: .alert)
                alert.addAction(UIAlertAction(title: "OK", style: .default, handler: nil))
                UIApplication.shared.keyWindow?.rootViewController?.present(alert, animated: true)
            }
        }
    }
    #endif
}

private func deleteLibraryFolder(_ folder: String) throws {
    let manager = FileManager.default
    let library = manager.urls(for: FileManager.SearchPathDirectory.libraryDirectory, in: .userDomainMask)[0]
    let dir = library.appendingPathComponent(folder)
    try manager.removeItem(at: dir)
}

// Remove all cookies stored by the site. This includes localStorage, sessionStorage, and WebSQL/IndexedDB.
class CookiesClearable: Clearable {
    
    var label: String {
        return Strings.Cookies
    }
    
    func clear() -> Success {
        UserDefaults.standard.synchronize()
        
        let result = Deferred<Maybe<()>>()
        // need event loop to run to autorelease UIWebViews fully
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
            // Now we wipe the system cookie store (for our app).
            let storage = HTTPCookieStorage.shared
            if let cookies = storage.cookies {
                for cookie in cookies {
                    storage.deleteCookie(cookie)
                }
            }
            UserDefaults.standard.synchronize()
            
            // And just to be safe, we also wipe the Cookies directory.
            do {
                try deleteLibraryFolderContents("Cookies", validateClearedExceptFor: [])
            } catch {
                return result.fill(Maybe<()>(failure: ClearableErrorType(err: error)))
            }
            return result.fill(Maybe<()>(success: ()))
        }
        return result
    }
}

// Clear the web cache. Note, this has to close all open tabs in order to ensure the data
// cached in them isn't flushed to disk.
class CacheClearable: Clearable {
    
    var label: String {
        return Strings.Cache
    }
    
    func clear() -> Success {
        let result = Deferred<Maybe<()>>()
        // need event loop to run to autorelease UIWebViews fully
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
            URLCache.shared.memoryCapacity = 0
            URLCache.shared.diskCapacity = 0
            // Remove the basic cache.
            URLCache.shared.removeAllCachedResponses()
            
            var err: Error?
            for item in ["Caches", "Cookies", "WebKit"] {
                do {
                    try deleteLibraryFolderContents(item, validateClearedExceptFor: ["Snapshots"])
                } catch {
                    err = error
                }
            }
            if let err = err {
                return result.fill(Maybe<()>(failure: ClearableErrorType(err: err)))
            }
            
            // Clear image cache
            ImageCache.shared.clear()
            
            // Leave the cache off in the error cases above
            URLCache.shared.setupBraveDefaults()
            result.fill(Maybe<()>(success: ()))
        }
        
        return result
    }
}

// Clears our browsing history, including favicons and thumbnails.
class HistoryClearable: Clearable {
    init() {
    }
    
    var label: String {
        return Strings.Browsing_History
    }
    
    func clear() -> Success {
        let result = Success()
        History.deleteAll {
            NotificationCenter.default.post(name: .PrivateDataClearedHistory, object: nil)
            result.fill(Maybe<()>(success: ()))
        }
        return result
    }
}

// Clear all stored passwords. This will clear SQLite storage and the system shared credential storage.
class PasswordsClearable: Clearable {
    let profile: Profile
    init(profile: Profile) {
        self.profile = profile
    }
    
    var label: String {
        return Strings.Saved_Logins
    }
    
    func clear() -> Success {
        // Clear our storage
        return profile.logins.removeAll() >>== { res in
            let storage = URLCredentialStorage.shared
            let credentials = storage.allCredentials
            for (space, credentials) in credentials {
                for (_, credential) in credentials {
                    storage.remove(credential, for: space)
                }
            }
            return succeed()
        }
    }
}
