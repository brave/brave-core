// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Data

/// A location where a Bookmark will be saved to.
enum BookmarkSaveLocation {
  /// A thumbnail on home screen
  case favorites
  /// Root level in bookmarks screen
  case rootLevel
  /// Custom folder in bookmarks screen
  case folder(folder: Bookmarkv2)

  /// Returns a folder where a Bookmark will be saved to.
  /// This only applies to custom folders, root level and favorites
  /// are not folder per se, and nil is returned in this case.
  var getFolder: Bookmarkv2? {
    switch self {
    case .folder(let folder): return folder
    default: return nil
    }
  }

  /// Making sure the folder we are saving to still exists in database.
  /// This could happen if the folder got deleted by another device in sync chain.
  /// Returns nil if location other than `folder` is selected
  var folderExists: Bool? {
    // Root level and favorites locations are permanent, only custom folder needs to be checked.
    switch self {
    case .folder(let folder):
      return folder.existsInPersistentStore()
    default:
      // Non-folder locations return nil
      return nil
    }
  }
}
