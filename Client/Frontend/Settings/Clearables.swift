/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import Data
import BraveShared
import WebKit

private let log = Logger.browserLogger

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

// Remove all cookies and website data stored by the site.
// This includes localStorage, sessionStorage, and WebSQL/IndexedDB and web cache.
class CookiesAndCacheClearable: Clearable {
    
    var label: String {
        return Strings.cookies
    }
    
    func clear() -> Success {
        UserDefaults.standard.synchronize()
        let result = Deferred<Maybe<()>>()
        // need event loop to run to autorelease UIWebViews fully
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
            WKWebsiteDataStore.default().removeData(ofTypes: WKWebsiteDataStore.allWebsiteDataTypes(), modifiedSince: Date(timeIntervalSinceReferenceDate: 0)) {
                UserDefaults.standard.synchronize()
                BraveWebView.sharedNonPersistentStore().removeData(ofTypes: WKWebsiteDataStore.allWebsiteDataTypes(), modifiedSince: Date(timeIntervalSinceReferenceDate: 0)) {
                    UserDefaults.standard.synchronize()
                    result.fill(Maybe<()>(success: ()))
                }
            }
        }
        return result
    }
}

// Clear the web cache. Note, this has to close all open tabs in order to ensure the data
// cached in them isn't flushed to disk.
class CacheClearable: Clearable {
    
    var label: String {
        return Strings.cache
    }
    
    func clear() -> Success {
        let result = Deferred<Maybe<()>>()
        // need event loop to run to autorelease UIWebViews fully
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
            let localStorageClearables: Set<String> = [WKWebsiteDataTypeDiskCache,
                                                       WKWebsiteDataTypeServiceWorkerRegistrations,
                                                       WKWebsiteDataTypeOfflineWebApplicationCache,
                                                       WKWebsiteDataTypeMemoryCache,
                                                       WKWebsiteDataTypeFetchCache]
            WKWebsiteDataStore.default().removeData(ofTypes: localStorageClearables, modifiedSince: Date(timeIntervalSinceReferenceDate: 0)) {
                WebImageCacheManager.shared.clearDiskCache()
                WebImageCacheManager.shared.clearMemoryCache()
                WebImageCacheWithNoPrivacyProtectionManager.shared.clearDiskCache()
                WebImageCacheWithNoPrivacyProtectionManager.shared.clearMemoryCache()
                
                BraveWebView.sharedNonPersistentStore().removeData(ofTypes: localStorageClearables, modifiedSince: Date(timeIntervalSinceReferenceDate: 0)) {
                    result.fill(Maybe<()>(success: ()))
                }
            }
        }
        
        return result
    }
}

// Clears our browsing history, including favicons and thumbnails.
class HistoryClearable: Clearable {
    init() {
    }
    
    var label: String {
        return Strings.browsingHistory
    }
    
    func clear() -> Success {
        let result = Success()
        History.deleteAll {
            NotificationCenter.default.post(name: .privateDataClearedHistory, object: nil)
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
        return Strings.savedLogins
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

/// Clears all files in Downloads folder.
class DownloadsClearable: Clearable {
    var label: String {
        return Strings.downloadedFiles
    }
    
    func clear() -> Success {
        do {
            let fileManager = FileManager.default
            let downloadsLocation = try FileManager.default.downloadsPath()
            let filePaths = try fileManager.contentsOfDirectory(atPath: downloadsLocation.path)
            
            try filePaths.forEach {
                var fileUrl = downloadsLocation
                fileUrl.appendPathComponent($0)
                try fileManager.removeItem(atPath: fileUrl.path)
            }
        } catch {
            // Not logging the `error` because downloaded file names can be sensitive to some users.
            log.error("Could not remove downloaded file")
        }
        
        return succeed()
        
    }
}

class BraveTodayClearable: Clearable {
    
    let feedDataSource: FeedDataSource
    
    init(feedDataSource: FeedDataSource) {
        self.feedDataSource = feedDataSource
    }
    
    var label: String {
        return Strings.BraveToday.braveToday
    }
    
    func clear() -> Success {
        feedDataSource.clearCachedFiles()
        return succeed()
    }
}

class PlayListCacheClearable: Clearable {
        
    init() { }
    
    var label: String {
        return Strings.PlayList.playlistOfflineDataToggleOption
    }
    
    func clear() -> Success {
        PlaylistManager.shared.deleteAllItems(cacheOnly: true)
        return succeed()
    }
}

class PlayListDataClearable: Clearable {
    
    init() { }
    
    var label: String {
        return Strings.PlayList.playlistMediaAndOfflineDataToggleOption
    }
    
    func clear() -> Success {
        PlaylistManager.shared.deleteAllItems()
        return succeed()
    }
}
