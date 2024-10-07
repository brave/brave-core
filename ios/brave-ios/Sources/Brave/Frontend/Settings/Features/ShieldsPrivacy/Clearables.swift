// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveNews
import BraveShared
import Data
import Favicon
import Foundation
import Playlist
import Shared
import WebKit
import os.log

// A base protocol for something that can be cleared.
protocol Clearable {
  @MainActor func clear() async throws
  var label: String { get }
}

class ClearableError: Error {
  fileprivate let msg: String
  init(msg: String) {
    self.msg = msg
  }

  var description: String { return msg }
}

struct ClearableErrorType: Error {
  let err: Error

  init(err: Error) {
    self.err = err
  }

  var description: String {
    return "Couldn't clear: \(err)."
  }
}

// Remove all cookies and website data stored by the site.
// This includes localStorage, sessionStorage, WebSQL/IndexedDB, web cache and wallet eth permissions.
class CookiesAndCacheClearable: Clearable {

  var label: String {
    return Strings.cookies
  }

  func clear() async throws {
    UserDefaults.standard.synchronize()
    // need event loop to run to autorelease UIWebViews fully
    try await Task.sleep(nanoseconds: NSEC_PER_MSEC * 100)
    await WKWebsiteDataStore.default().removeData(
      ofTypes: WKWebsiteDataStore.allWebsiteDataTypes(),
      modifiedSince: Date(timeIntervalSinceReferenceDate: 0)
    )
    UserDefaults.standard.synchronize()
    await BraveWebView.sharedNonPersistentStore().removeData(
      ofTypes: WKWebsiteDataStore.allWebsiteDataTypes(),
      modifiedSince: Date(timeIntervalSinceReferenceDate: 0)
    )
    UserDefaults.standard.synchronize()
    for coin in [BraveWallet.CoinType.eth, BraveWallet.CoinType.sol] {
      await Domain.clearAllWalletPermissions(for: coin)
    }
  }
}

// Clear the web cache. Note, this has to close all open tabs in order to ensure the data
// cached in them isn't flushed to disk.
class CacheClearable: Clearable {

  var label: String {
    return Strings.cache
  }

  func clear() async throws {
    // need event loop to run to autorelease UIWebViews fully
    try await Task.sleep(nanoseconds: NSEC_PER_MSEC * 100)
    let localStorageClearables: Set<String> = [
      WKWebsiteDataTypeDiskCache,
      WKWebsiteDataTypeServiceWorkerRegistrations,
      WKWebsiteDataTypeOfflineWebApplicationCache,
      WKWebsiteDataTypeMemoryCache,
      WKWebsiteDataTypeFetchCache,
    ]
    await WKWebsiteDataStore.default().removeData(
      ofTypes: localStorageClearables,
      modifiedSince: Date(timeIntervalSinceReferenceDate: 0)
    )
    WebImageCacheManager.shared.clearDiskCache()
    WebImageCacheManager.shared.clearMemoryCache()
    WebImageCacheWithNoPrivacyProtectionManager.shared.clearDiskCache()
    WebImageCacheWithNoPrivacyProtectionManager.shared.clearMemoryCache()
    await FaviconFetcher.clearCache()

    await BraveWebView.sharedNonPersistentStore().removeData(
      ofTypes: localStorageClearables,
      modifiedSince: Date(timeIntervalSinceReferenceDate: 0)
    )
  }
}

// Clears our browsing history, including favicons and thumbnails.
class HistoryClearable: Clearable {
  let historyAPI: BraveHistoryAPI
  let httpsUpgradeService: HttpsUpgradeService?

  init(historyAPI: BraveHistoryAPI, httpsUpgradeService: HttpsUpgradeService?) {
    self.historyAPI = historyAPI
    self.httpsUpgradeService = httpsUpgradeService
  }

  var label: String {
    return Strings.browsingHistory
  }

  func clear() async throws {
    return await withCheckedContinuation { continuation in
      self.historyAPI.deleteAll {
        self.httpsUpgradeService?.clearAllowlist(fromStart: .distantPast, end: .distantFuture)
        NotificationCenter.default.post(name: .privateDataClearedHistory, object: nil)
        continuation.resume()
      }
    }
  }
}

// Clear all stored passwords. This will clear the system shared credential storage.
class PasswordsClearable: Clearable {
  let profile: Profile
  init(profile: Profile) {
    self.profile = profile
  }

  var label: String {
    return Strings.savedLogins
  }

  func clear() async throws {
    // Clear our storage
    let storage = URLCredentialStorage.shared
    let credentials = storage.allCredentials
    for (space, credentials) in credentials {
      for (_, credential) in credentials {
        storage.remove(credential, for: space)
      }
    }
  }
}

/// Clears all files in Downloads folder.
class DownloadsClearable: Clearable {
  var label: String {
    return Strings.downloadedFiles
  }

  func clear() async throws {
    do {
      let fileManager = AsyncFileManager.default
      let downloadsLocation = try await fileManager.downloadsPath()
      let filePaths = try await fileManager.contentsOfDirectory(atPath: downloadsLocation.path)

      for filePath in filePaths {
        let fileUrl = downloadsLocation.appending(path: filePath)
        try await fileManager.removeItem(atPath: fileUrl.path)
      }
    } catch {
      // Not logging the `error` because downloaded file names can be sensitive to some users.
      Logger.module.error("Could not remove downloaded file")
    }
  }
}

class BraveNewsClearable: Clearable {

  let feedDataSource: FeedDataSource

  init(feedDataSource: FeedDataSource) {
    self.feedDataSource = feedDataSource
  }

  var label: String {
    return Strings.BraveNews.braveNews
  }

  func clear() async throws {
    await feedDataSource.clearCachedFiles()
  }
}

class PlayListCacheClearable: Clearable {

  init() {}

  var label: String {
    return Strings.PlayList.playlistOfflineDataToggleOption
  }

  func clear() async throws {
    PlaylistCoordinator.shared.destroyPiP()
    await PlaylistManager.shared.deleteAllItems(cacheOnly: true)

    // Backup in case there is folder corruption, so we delete the cache anyway
    if let playlistDirectory = await PlaylistDownloadManager.playlistDirectory {
      do {
        try await AsyncFileManager.default.removeItem(at: playlistDirectory)
      } catch {
        Logger.module.error("Error Deleting Playlist directory: \(error.localizedDescription)")
      }
    }
  }
}

class PlayListDataClearable: Clearable {

  init() {}

  var label: String {
    return Strings.PlayList.playlistMediaAndOfflineDataToggleOption
  }

  func clear() async throws {
    PlaylistCoordinator.shared.destroyPiP()
    await PlaylistManager.shared.deleteAllItems(cacheOnly: false)

    // Backup in case there is folder corruption, so we delete the cache anyway
    if let playlistDirectory = await PlaylistDownloadManager.playlistDirectory {
      do {
        try await AsyncFileManager.default.removeItem(at: playlistDirectory)
      } catch {
        Logger.module.error("Error Deleting Playlist directory: \(error.localizedDescription)")
      }
    }
  }
}

class RecentSearchClearable: Clearable {

  init() {}

  var label: String {
    return Strings.recentSearchClearDataToggleOption
  }

  func clear() async throws {
    RecentSearch.removeAll()
  }
}

class BraveAdsDataClearable: Clearable {

  private let rewards: BraveRewards?

  init(rewards: BraveRewards?) {
    self.rewards = rewards
  }

  var label: String {
    return Strings.Ads.braveAdsDataToggleOption
  }

  func clear() async throws {
    await rewards?.clearAdsData()
  }
}
