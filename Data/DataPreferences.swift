// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveShared

public extension Preferences {
    final class Sync {
        /// First two digits of special order for Sync, unified across all platforms.
        /// The first two digits are a platform number and device id.
        /// Default value is 0.0., `syncOrder` is also used in the local ordering algorithm, before joining a Sync group.
        static let baseSyncOrder = Option<String>(key: "sync.base-sync-order", default: "0.0.")
        
        /// Timestamp of last successful sync on this device.
        /// To count as a successful, the entire process must succeed: fetching, resolving, insertion/update, and save
        static let lastFetchTimestamp = Option<Int>(key: "sync.last-fetch-timestamp", default: 0)
        
        static let lastPreferenceFetchTimestamp = Option<Int>(key: "sync.last-fetch-timestamp-device", default: 0)
        
        /// We store this value to signify that a group has been joined
        /// This is _only_ used on a re-installation to know that the app was deleted and re-installed
        /// Real Sync seed is stored in a keychain, although preference name of this option is used for both.
        /// See Sync.syncSeed for more details.
        static let seedName = Option<Bool>(key: "sync.is-sync-seed-set", default: false)
    }
    
    final class Database {
        
        public final class DocumentToSupportDirectoryMigration {
            /// This indicates whether the associated Document -> Support directory migration was / is considered
            ///     successful or not. Once it is `true`, it should never be set to `false` again.
            public static let completed
                = Option<Bool>(key: "database.document-to-support-directory-migration.completed", default: false)
            
            /// Since the migration may need to be re-attempted on each Brave version update this is used to store
            ///   the past version attempt, so it can be determined if another migration attempt is due.
            public static let previousAttemptedVersion
                = Option<String?>(key: "database.document-to-support-directory-migration.previous-attempted-version", default: nil)
        }
    }
}
