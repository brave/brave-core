/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared

public func titleForSpecialGUID(_ guid: GUID) -> String? {
    switch guid {
    case BookmarkRoots.RootGUID:
        return "<Root>"
    case BookmarkRoots.MobileFolderGUID:
        return Strings.BookmarksFolderTitleMobile
    case BookmarkRoots.ToolbarFolderGUID:
        return Strings.BookmarksFolderTitleToolbar
    case BookmarkRoots.MenuFolderGUID:
        return Strings.BookmarksFolderTitleMenu
    case BookmarkRoots.UnfiledFolderGUID:
        return Strings.BookmarksFolderTitleUnsorted
    default:
        return nil
    }
}

extension Strings {
    public static let BookmarksFolderTitleMobile = NSLocalizedString("BookmarksFolderTitleMobile", tableName: "Storage", bundle: Bundle.storage, value: "Mobile Bookmarks", comment: "The title of the folder that contains mobile bookmarks. This should match bookmarks.folder.mobile.label on Android.")
    public static let BookmarksFolderTitleMenu = NSLocalizedString("BookmarksFolderTitleMenu", tableName: "Storage", bundle: Bundle.storage, value: "Bookmarks Menu", comment: "The name of the folder that contains desktop bookmarks in the menu. This should match bookmarks.folder.menu.label on Android.")
    public static let BookmarksFolderTitleToolbar = NSLocalizedString("BookmarksFolderTitleToolbar", tableName: "Storage", bundle: Bundle.storage, value: "Bookmarks Toolbar", comment: "The name of the folder that contains desktop bookmarks in the toolbar. This should match bookmarks.folder.toolbar.label on Android.")
    public static let BookmarksFolderTitleUnsorted = NSLocalizedString("BookmarksFolderTitleUnsorted", tableName: "Storage", bundle: Bundle.storage, value: "Unsorted Bookmarks", comment: "The name of the folder that contains unsorted desktop bookmarks. This should match bookmarks.folder.unfiled.label on Android.")
}
