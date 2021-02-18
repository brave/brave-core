// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveShared

public extension Preferences {
    
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
        
        public static let bookmark_v1_12_1RestorationCompleted
            = Option<Bool>(key: "database.1_12_1-bookmark-restoration-completed", default: false)
    }
}
