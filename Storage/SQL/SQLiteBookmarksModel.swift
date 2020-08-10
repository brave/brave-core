/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared

private let log = Logger.syncLogger

extension Strings {
    public static let SQLLiteBookmarkDesktopBookmarksLabel = NSLocalizedString("SQLLiteBookmarkDesktopBookmarksLabel", tableName: "Storage", bundle: Bundle.storage, value: "Desktop Bookmarks", comment: "The folder name for the virtual folder that contains all desktop bookmarks.")
    public static let SQLLiteBookmarkDefaultFolderTitle = NSLocalizedString("SQLLiteBookmarkDefaultFolderTitle", tableName: "Storage", bundle: Bundle.storage, value: "Untitled", comment: "The default name for bookmark folders without titles.")
    public static let SQLLiteBookmarkDefaultItemTitle = NSLocalizedString("SQLLiteBookmarkDefaultItemTitle", tableName: "Storage", bundle: Bundle.storage, value: "Untitled", comment: "The default name for bookmark nodes without titles.")
}
