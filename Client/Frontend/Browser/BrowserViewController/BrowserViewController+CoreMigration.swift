// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Shared
import BraveShared
import Storage
import UIKit
import WebKit
import os.log

// MARK: - Core Migration Browser Extension

extension BrowserViewController {

  func doSyncMigration() {
    // We stop ever attempting migration after 3 times.
    if Preferences.Chromium.syncV2ObjectMigrationCount.value < 3 {
      self.migrateToSyncObjects { error in
        if let error = error, error == .failedBookmarksMigration {
          DispatchQueue.main.async {
            let alert = UIAlertController(
              title: error.failureReason,
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
      // Marking the history migration completed after 3 tries
      Preferences.Chromium.syncV2HistoryMigrationCompleted.value = true
      // Marking the password migration completed after 3 tries
      Preferences.Chromium.syncV2HistoryMigrationCompleted.value = true
    }
  }

  private func migrateToSyncObjects(_ completion: @escaping ((BraveCoreMigrator.MigrationError?) -> Void)) {
    guard let migrator = migration?.braveCoreSyncObjectsMigrator else { return }

    migrator.migrate { error in
      Preferences.Chromium.syncV2ObjectMigrationCount.value += 1
      completion(error)
    }
  }
}
