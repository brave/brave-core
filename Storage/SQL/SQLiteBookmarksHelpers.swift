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

class BookmarkURLTooLargeError: MaybeErrorType {
    init() {
    }
    var description: String {
        return "URL too long to bookmark."
    }
}

extension String {
    public func truncateToUTF8ByteCount(_ keep: Int) -> String {
        let byteCount = self.lengthOfBytes(using: .utf8)
        if byteCount <= keep {
            return self
        }
        let toDrop = keep - byteCount

        // If we drop this many characters from the string, we will drop at least this many bytes.
        // That's aggressive, but that's OK for our purposes.
        guard let endpoint = self.index(self.endIndex, offsetBy: toDrop, limitedBy: self.startIndex) else {
            return ""
        }
        return String(self[..<endpoint])
    }
}

extension SQLiteBookmarks: ShareToDestination {
    public func addToMobileBookmarks(_ url: URL, title: String, favicon: Favicon?) -> Success {
        if url.absoluteString.lengthOfBytes(using: .utf8) > AppConstants.DB_URL_LENGTH_MAX {
            return deferMaybe(BookmarkURLTooLargeError())
        }

        let title = title.truncateToUTF8ByteCount(AppConstants.DB_TITLE_LENGTH_MAX)

        return isBookmarked(String(describing: url), direction: Direction.local)
            >>== { yes in
                guard !yes else { return succeed() }
                return self.insertBookmark(url, title: title, favicon: favicon,
                                           intoFolder: BookmarkRoots.MobileFolderGUID,
                                           withTitle: Strings.BookmarksFolderTitleMobile)
        }
    }

    public func shareItem(_ item: ShareItem) -> Success {
        // We parse here in anticipation of getting real URLs at some point.
        if let url = item.url.asURL {
            let title = item.title ?? url.absoluteString
            return self.addToMobileBookmarks(url, title: title, favicon: item.favicon)
        }
        return succeed()
    }
}

extension Strings {
    public static let BookmarksFolderTitleMobile = NSLocalizedString("BookmarksFolderTitleMobile", tableName: "Storage", bundle: Bundle.storage, value: "Mobile Bookmarks", comment: "The title of the folder that contains mobile bookmarks. This should match bookmarks.folder.mobile.label on Android.")
    public static let BookmarksFolderTitleMenu = NSLocalizedString("BookmarksFolderTitleMenu", tableName: "Storage", bundle: Bundle.storage, value: "Bookmarks Menu", comment: "The name of the folder that contains desktop bookmarks in the menu. This should match bookmarks.folder.menu.label on Android.")
    public static let BookmarksFolderTitleToolbar = NSLocalizedString("BookmarksFolderTitleToolbar", tableName: "Storage", bundle: Bundle.storage, value: "Bookmarks Toolbar", comment: "The name of the folder that contains desktop bookmarks in the toolbar. This should match bookmarks.folder.toolbar.label on Android.")
    public static let BookmarksFolderTitleUnsorted = NSLocalizedString("BookmarksFolderTitleUnsorted", tableName: "Storage", bundle: Bundle.storage, value: "Unsorted Bookmarks", comment: "The name of the folder that contains unsorted desktop bookmarks. This should match bookmarks.folder.unfiled.label on Android.")
}
