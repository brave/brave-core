// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Data
import BraveRewards
import CoreData
import Shared

private let log = Logger.browserLogger

// A Lightweight wrapper around BraveCore bookmarks
// with the same layout/interface as `Bookmark (from CoreData)`
class Bookmarkv2 {
    private let bookmarkNode: BookmarkNode
    private var observer: BookmarkModelListener?
    private static let bookmarksAPI = BraveBookmarksAPI()
    
    init(_ bookmarkNode: BookmarkNode) {
        self.bookmarkNode = bookmarkNode
    }
    
    public var isFolder: Bool {
        return bookmarkNode.isFolder == true
    }
    
    public var title: String? {
        return bookmarkNode.titleUrlNodeTitle
    }
    
    public var customTitle: String? {
        return self.title
    }
    
    public var displayTitle: String? {
        return self.customTitle
    }
    
    public var url: String? {
        bookmarkNode.titleUrlNodeUrl?.absoluteString
    }
    
    public var domain: Domain? {
        if let url = bookmarkNode.titleUrlNodeUrl {
            return Domain.getOrCreate(forUrl: url, persistent: true)
        }
        return nil
    }
    
    public var created: Date? {
        get {
            return bookmarkNode.dateAdded
        }
        
        set {
            bookmarkNode.dateAdded = newValue ?? Date()
        }
    }
    
    public var parent: Bookmarkv2? {
        if let parent = bookmarkNode.parent {
            // Return nil if the parent is the ROOT node
            // because AddEditBookmarkTableViewController.sortFolders
            // sorts root folders by having a nil parent.
            // If that code changes, we should change here to match.
            if bookmarkNode.parent?.guid != Bookmarkv2.bookmarksAPI.rootNode?.guid {
                return Bookmarkv2(parent)
            }
        }
        return nil
    }
    
    public var children: [Bookmarkv2]? {
        return bookmarkNode.children.map({ Bookmarkv2($0) })
    }
    
    public var canBeDeleted: Bool {
        return bookmarkNode.isPermanentNode == false
    }
    
    public var objectID: Int {
        return Int(bookmarkNode.nodeId)
    }
    
    public var order: Int16 {
        let defaultOrder = 0 //taken from CoreData
        
        guard let children = bookmarkNode.parent?.children else {
            return Int16(defaultOrder)
        }
        
        return Int16(children.firstIndex(where: { $0.guid == self.bookmarkNode.guid }) ?? defaultOrder)
    }
    
    public func delete() {
        if self.canBeDeleted {
            Bookmarkv2.bookmarksAPI.removeBookmark(bookmarkNode)
        }
    }
}

// Bookmarks Fetching
extension Bookmarkv2 {
    
    public class func mobileNode() -> Bookmarkv2? {
        if let node = Bookmarkv2.bookmarksAPI.mobileNode {
            return Bookmarkv2(node)
        }
        return nil
    }
    
    public class func addFolder(title: String, parentFolder: Bookmarkv2? = nil) {
        if let parentFolder = parentFolder?.bookmarkNode {
            Bookmarkv2.bookmarksAPI.createFolder(withParent: parentFolder, title: title)
        } else {
            Bookmarkv2.bookmarksAPI.createFolder(withTitle: title)
        }
    }
    
    public class func add(url: URL, title: String?, parentFolder: Bookmarkv2? = nil) {
        if let parentFolder = parentFolder?.bookmarkNode {
            Bookmarkv2.bookmarksAPI.createBookmark(withParent: parentFolder, title: title ?? "", with: url)
        } else {
            Bookmarkv2.bookmarksAPI.createBookmark(withTitle: title ?? "", url: url)
        }
    }
    
    public func existsInPersistentStore() -> Bool {
        return bookmarkNode.isValid && bookmarkNode.parent != nil
    }
    
    public static func frc(parent: Bookmarkv2?) -> BookmarksV2FetchResultsController? {
        return Bookmarkv2Fetcher(parent?.bookmarkNode, api: Bookmarkv2.bookmarksAPI)
    }
    
    public static func foldersFrc(excludedFolder: Bookmarkv2? = nil) -> BookmarksV2FetchResultsController {
        return Bookmarkv2ExclusiveFetcher(excludedFolder?.bookmarkNode, api: Bookmarkv2.bookmarksAPI)
    }
    
    public static func getChildren(forFolder folder: Bookmarkv2, includeFolders: Bool) -> [Bookmarkv2]? {
        let result = folder.bookmarkNode.children.map({ Bookmarkv2($0) })
        return includeFolders ? result : result.filter({ $0.isFolder == false })
    }
    
    public func update(customTitle: String?, url: String?) {
        bookmarkNode.setTitle(customTitle ?? "")
        bookmarkNode.url = URL(string: url ?? "")
    }
    
    public func updateWithNewLocation(customTitle: String?, url: String?, location: Bookmarkv2?) {
        if let location = location?.bookmarkNode ?? Bookmarkv2.bookmarksAPI.mobileNode {
            bookmarkNode.move(toParent: location)
            
            if let customTitle = customTitle {
                bookmarkNode.setTitle(customTitle)
            }
            
            if let url = url, !bookmarkNode.isFolder {
                bookmarkNode.url = URL(string: url)
            } else if url != nil {
                log.error("Error: Moving bookmark - Cannot convert a folder into a bookmark with url.")
            }
        } else {
            log.error("Error: Moving bookmark - Cannot move a bookmark to Root.")
        }
    }
    
    public class func reorderBookmarks(frc: BookmarksV2FetchResultsController?, sourceIndexPath: IndexPath,
                                       destinationIndexPath: IndexPath) {
        if let node = frc?.object(at: sourceIndexPath)?.bookmarkNode,
           let parent = node.parent ?? bookmarksAPI.mobileNode {
            
            //Moving to the very last index.. same as appending..
            if destinationIndexPath.row == parent.children.count - 1 {
                node.move(toParent: parent)
            } else {
                node.move(toParent: parent, index: UInt(destinationIndexPath.row))
            }
        }
    }
}

// Brave-Core only
extension Bookmarkv2 {
    public var icon: UIImage? {
        return bookmarkNode.icon
    }
    
    public var isFavIconLoading: Bool {
        return bookmarkNode.isFavIconLoading
    }
    
    public var isFavIconLoaded: Bool {
        return bookmarkNode.isFavIconLoaded
    }
    
    public func addFavIconObserver(_ observer: @escaping () -> Void) {
        let observer = BookmarkModelStateObserver { [weak self] state in
            guard let self = self else { return }
            
            if case .favIconChanged(let node) = state {
                if node.guid == self.bookmarkNode.guid {
                    observer()
                }
            }
        }
        
        self.observer = Bookmarkv2.bookmarksAPI.add(observer)
    }
    
    public func removeFavIconObserver() {
        observer = nil
    }
}
