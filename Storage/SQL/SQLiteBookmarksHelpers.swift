/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared

public func titleForSpecialGUID(_ guid: GUID) -> String? {
    switch guid {
    case BookmarkRoots.rootGUID:
        return "<Root>"
    case BookmarkRoots.mobileFolderGUID:
        return Strings.bookmarksFolderTitleMobile
    case BookmarkRoots.toolbarFolderGUID:
        return Strings.bookmarksFolderTitleToolbar
    case BookmarkRoots.menuFolderGUID:
        return Strings.bookmarksFolderTitleMenu
    case BookmarkRoots.unfiledFolderGUID:
        return Strings.bookmarksFolderTitleUnsorted
    default:
        return nil
    }
}

extension Strings {
    public static let bookmarksFolderTitleMobile = NSLocalizedString("BookmarksFolderTitleMobile", tableName: "Storage", bundle: Bundle.storage, value: "Mobile Bookmarks", comment: "The title of the folder that contains mobile bookmarks. This should match bookmarks.folder.mobile.label on Android.")
    public static let bookmarksFolderTitleMenu = NSLocalizedString("BookmarksFolderTitleMenu", tableName: "Storage", bundle: Bundle.storage, value: "Bookmarks Menu", comment: "The name of the folder that contains desktop bookmarks in the menu. This should match bookmarks.folder.menu.label on Android.")
    public static let bookmarksFolderTitleToolbar = NSLocalizedString("BookmarksFolderTitleToolbar", tableName: "Storage", bundle: Bundle.storage, value: "Bookmarks Toolbar", comment: "The name of the folder that contains desktop bookmarks in the toolbar. This should match bookmarks.folder.toolbar.label on Android.")
    public static let bookmarksFolderTitleUnsorted = NSLocalizedString("BookmarksFolderTitleUnsorted", tableName: "Storage", bundle: Bundle.storage, value: "Unsorted Bookmarks", comment: "The name of the folder that contains unsorted desktop bookmarks. This should match bookmarks.folder.unfiled.label on Android.")
}
