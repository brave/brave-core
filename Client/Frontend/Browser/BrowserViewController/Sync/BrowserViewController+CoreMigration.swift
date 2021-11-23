// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Shared
import BraveShared
import Storage
import UIKit
import WebKit
import XCGLogger

private let log = Logger.browserLogger

// MARK: - Core Migration Browser Extension

extension BrowserViewController {
    
    func doSyncMigration() {
        // We stop ever attempting migration after 3 times.
        if Preferences.Chromium.syncV2ObjectMigrationCount.value < 3 {
            self.migrateToSyncObjects { error in
                if let error = error, error == .failedBookmarksMigration {
                    DispatchQueue.main.async {
                        let alert = UIAlertController(title: error.failureReason,
                                                      message: error.localizedDescription,
                                                      preferredStyle: .alert)
                        alert.addAction(UIAlertAction(title: Strings.OKString, style: .default, handler: nil))
                        self.present(alert, animated: true)
                    }
                }
            }
        } else {
            // After 3 tries, we mark Migration as successful.
            // There is nothing more we can do for the user other than to let them export/import bookmarks.
            Preferences.Chromium.syncV2BookmarksMigrationCompleted.value = true
            // Also marking the history migration completed after 3 tries
            Preferences.Chromium.syncV2HistoryMigrationCompleted.value = true
        }
    }
    
    private func migrateToSyncObjects(_ completion: @escaping ((BraveCoreMigrator.MigrationError?) -> Void)) {
        let showInterstitialPage = { (url: URL?) -> Bool in
            guard let url = url else {
                log.error("Cannot open bookmarks page in new tab")
                return false
            }
            
            return BookmarksInterstitialPageHandler.showBookmarksPage(tabManager: self.tabManager, url: url)
        }
        
        guard let migrator = migration?.braveCoreSyncObjectsMigrator else { return }
        
        migrator.migrate { error in
            Preferences.Chromium.syncV2ObjectMigrationCount.value += 1
            
            guard let error = error else {
                completion(nil)
                return
            }
            
            switch error {
                case .failedBookmarksMigration:
                    guard let url = migrator.datedBookmarksURL else {
                        completion(showInterstitialPage(migrator.bookmarksURL) ? nil : error)
                        return
                    }

                    migrator.exportBookmarks(to: url) { success in
                        if success {
                            completion(showInterstitialPage(url) ? nil : error)
                        } else {
                            completion(showInterstitialPage(migrator.bookmarksURL) ? nil : error)
                        }
                    }
                default:
                   completion(error)
            }
        }
    }
}
