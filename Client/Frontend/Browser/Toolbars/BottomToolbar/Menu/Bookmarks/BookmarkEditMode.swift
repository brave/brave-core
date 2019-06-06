// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Data
import Shared

/// Bookmark editing has four states.
/// each state has small differences in presentation as well as business logic.
enum BookmarkEditMode {
    case addBookmark(title: String, url: String)
    case addFolder(title: String)
    case editBookmark(_ bookmark: Bookmark)
    case editFolder(_ folder: Bookmark)
    
    /// Returns a initial, default save location if none is provided
    var initialSaveLocation: BookmarkSaveLocation {
        switch self {
        case .addBookmark(_, _), .addFolder(_):
            return .rootLevel
        // Set current parent folder if possible, fallback to root folder
        case .editBookmark(let bookmark):
            return folderOrRoot(bookmarkOrFolder: bookmark)
        case .editFolder(let folder):
            return folderOrRoot(bookmarkOrFolder: folder)
        }
    }
    
    /// Returns a folder which is edited, otherwise retuns nil.
    /// This is required to exclude the folder from showing in
    /// folder hierarchy.
    var folder: Bookmark? {
        switch self {
        case .editFolder(let folder): return folder
        default: return nil
        }
    }
    
    /// Returns a title for view controller.
    var title: String {
        switch self {
        case .addBookmark(_, _): return Strings.NewBookmarkTitle
        case .addFolder(_): return  Strings.NewFolderTitle
        case .editBookmark(_): return  Strings.EditBookmarkTitle
        case .editFolder(_): return  Strings.EditFolderTitle
        }
    }
    
    private func folderOrRoot(bookmarkOrFolder: Bookmark) -> BookmarkSaveLocation {
        guard let parent = bookmarkOrFolder.parentFolder else { return .rootLevel }
        return .folder(folder: parent)
    }
    
    var specialCells: [AddEditBookmarkTableViewController.SpecialCell] {
        // Order of cells matters.
        switch self {
        case .addFolder(_), .editFolder(_): return [.rootLevel]
        case .addBookmark(_), .editBookmark(_): return [.addFolder, .favorites, .rootLevel]
        }
    }
}
