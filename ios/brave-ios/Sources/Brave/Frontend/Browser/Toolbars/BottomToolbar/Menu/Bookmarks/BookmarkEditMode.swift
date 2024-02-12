// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Data
import Shared
import BraveCore

/// Bookmark editing has four states.
/// each state has small differences in presentation as well as business logic.
enum BookmarkEditMode {
  case addBookmark(title: String, url: String)
  case addFolder(title: String)
  case addFolderUsingTabs(title: String, tabList: [Tab])
  case editBookmark(_ bookmark: Bookmarkv2)
  case editFolder(_ folder: Bookmarkv2)
  case editFavorite(_ favorite: Bookmarkv2)

  /// Returns a initial, default save location if none is provided
  var initialSaveLocation: BookmarkSaveLocation {
    switch self {
    case .addBookmark(_, _), .addFolder(_), .addFolderUsingTabs(_, _):
      return .rootLevel
    // Set current parent folder if possible, fallback to root folder
    case .editBookmark(let bookmark):
      return folderOrRoot(bookmarkOrFolder: bookmark)
    case .editFolder(let folder):
      return folderOrRoot(bookmarkOrFolder: folder)
    case .editFavorite(_):
      return .favorites
    }
  }

  /// Returns a folder which is edited, otherwise retuns nil.
  /// This is required to exclude the folder from showing in
  /// folder hierarchy.
  var folder: Bookmarkv2? {
    switch self {
    case .editFolder(let folder): return folder
    default: return nil
    }
  }

  /// Returns a title for view controller.
  var title: String {
    switch self {
    case .addBookmark(_, _): return Strings.newBookmarkTitle
    case .addFolder(_), .addFolderUsingTabs(_, _): return Strings.newFolderTitle
    case .editBookmark(_): return Strings.editBookmarkTitle
    case .editFolder(_): return Strings.editFolderTitle
    case .editFavorite(_): return Strings.editFavoriteTitle
    }
  }

  private func folderOrRoot(bookmarkOrFolder: Bookmarkv2) -> BookmarkSaveLocation {
    guard let parent = bookmarkOrFolder.parent else { return .rootLevel }
    return .folder(folder: parent)
  }

  var specialCells: [AddEditBookmarkTableViewController.SpecialCell] {
    // Order of cells matters.
    switch self {
    case .addFolder, .addFolderUsingTabs, .editFolder: return [.rootLevel]
    case .addBookmark, .editBookmark, .editFavorite: return [.addFolder, .favorites, .rootLevel]
    }
  }
}
