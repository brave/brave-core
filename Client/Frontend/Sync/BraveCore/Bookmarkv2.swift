// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Data
import BraveRewards
import BraveShared
import CoreData
import Shared

private let log = Logger.browserLogger

// A Lightweight wrapper around BraveCore bookmarks
// with the same layout/interface as `Bookmark (from CoreData)`
class Bookmarkv2: WebsitePresentable {
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
        let defaultOrder = 0 // taken from CoreData

        // MUST Use childCount instead of children.count! for performance
        guard let childCount = bookmarkNode.parent?.childCount, childCount > 0 else {
            return Int16(defaultOrder)
        }

        // Do NOT change this to self.parent.children.indexOf(where: { self.id == $0.id })
        // Swift's performance on `Array` is abominable!
        // Therefore we call a native function `index(ofChild:)` to return the index.
        return Int16(self.parent?.bookmarkNode.index(ofChild: self.bookmarkNode) ?? defaultOrder)
    }
    
    public func delete() {
        if self.canBeDeleted {
            Bookmarkv2.bookmarksAPI.removeBookmark(bookmarkNode)
        }
    }
    
    // Returns the last visited folder
    // If no folder was visited, returns the mobile bookmarks folder
    // If the root folder was visited, returns nil
    public static func lastVisitedFolder() -> Bookmarkv2? {
        guard Preferences.General.showLastVisitedBookmarksFolder.value,
              let nodeId = Preferences.Chromium.lastBookmarksFolderNodeId.value else {
            // Default folder is the mobile node..
            if let mobileNode = bookmarksAPI.mobileNode {
                return Bookmarkv2(mobileNode)
            }
            return nil
        }
        
        // Display root folder instead of mobile node..
        if nodeId == -1 {
            return nil
        }
        
        // Display last visited folder..
        if let folderNode = Bookmarkv2.bookmarksAPI.getNodeById(nodeId),
           folderNode.isVisible {
            return Bookmarkv2(folderNode)
        }
        
        // Default folder is the mobile node..
        if let mobileNode = bookmarksAPI.mobileNode {
            return Bookmarkv2(mobileNode)
        }
        return nil
    }
    
    public static func lastFolderPath() -> [Bookmarkv2] {
        if Preferences.General.showLastVisitedBookmarksFolder.value,
           let nodeId = Preferences.Chromium.lastBookmarksFolderNodeId.value,
           var folderNode = Bookmarkv2.bookmarksAPI.getNodeById(nodeId),
           folderNode.isVisible {
            
            // We don't ever display the root node
            // It is the mother of all nodes
            let rootNodeGuid = bookmarksAPI.rootNode?.guid
            
            var nodes = [BookmarkNode]()
            nodes.append(folderNode)
            
            while true {
                if let parent = folderNode.parent, parent.isVisible, parent.guid != rootNodeGuid {
                    nodes.append(parent)
                    folderNode = parent
                    continue
                }
                break
            }
            return nodes.map({ Bookmarkv2($0) }).reversed()
        }
        
        // Default folder is the mobile node..
        if let mobileNode = bookmarksAPI.mobileNode {
            return [Bookmarkv2(mobileNode)]
        }
        
        return []
    }
}

class BraveBookmarkFolder: Bookmarkv2 {
    public let indentationLevel: Int
    
    private override init(_ bookmarkNode: BookmarkNode) {
        self.indentationLevel = 0
        super.init(bookmarkNode)
    }
    
    public init(_ bookmarkFolder: BookmarkFolder) {
        self.indentationLevel = bookmarkFolder.indentationLevel
        super.init(bookmarkFolder.bookmarkNode)
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
    
    public static func byFrequency(query: String? = nil) -> [WebsitePresentable] {
        // Invalid query.. BraveCore doesn't store bookmarks based on last visited.
        // Any last visited bookmarks would show up in `History` anyway.
        // BraveCore automatically sorts them by date as well.
        guard let query = query, !query.isEmpty else { return [] }
        return Bookmarkv2.bookmarksAPI.search(withQuery: query, maxCount: 200)
            .compactMap({ return !$0.isFolder ? Bookmarkv2($0) : nil })
    }
    
    public func update(customTitle: String?, url: URL?) {
        bookmarkNode.setTitle(customTitle ?? "")
        bookmarkNode.url = url
    }
    
    public func updateWithNewLocation(customTitle: String?, url: URL?, location: Bookmarkv2?) {
        if let location = location?.bookmarkNode ?? Bookmarkv2.bookmarksAPI.mobileNode {
            if location.guid != bookmarkNode.parent?.guid {
                bookmarkNode.move(toParent: location)
            }
            
            if let customTitle = customTitle {
                bookmarkNode.setTitle(customTitle)
            }
            
            if let url = url, !bookmarkNode.isFolder {
                bookmarkNode.url = url
            } else if url != nil {
                log.error("Error: Moving bookmark - Cannot convert a folder into a bookmark with url.")
            }
        } else {
            log.error("Error: Moving bookmark - Cannot move a bookmark to Root.")
        }
    }
    
    public class func reorderBookmarks(frc: BookmarksV2FetchResultsController?, sourceIndexPath: IndexPath,
                                       destinationIndexPath: IndexPath) {
        guard let frc = frc else { return }
        
        if let node = frc.object(at: sourceIndexPath)?.bookmarkNode,
           let parent = node.parent ?? bookmarksAPI.mobileNode {
            
            // Moving to the very last index.. same as appending..
            if destinationIndexPath.row == parent.children.count - 1 {
                node.move(toParent: parent)
            } else {
                node.move(toParent: parent, index: UInt(destinationIndexPath.row))
            }
            
            // Notify the delegate that items did move..
            // This is already done automatically in `Bookmarkv2Fetcher` listener.
            // However, the Brave-Core delegate is being called before the move is actually complete OR too quickly
            // So to fix it, we reload here AFTER the move is done so the UI can update accordingly.
            frc.delegate?.controllerDidReloadContents(frc)
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
                if node.isValid && self.bookmarkNode.isValid
                    && node.guid == self.bookmarkNode.guid {
                    observer()
                }
            }
        }
        
        self.observer = Bookmarkv2.bookmarksAPI.add(observer)
    }
    
    public func removeFavIconObserver() {
        observer = nil
    }
    
    public static func waitForBookmarkModelLoaded(_ completion: @escaping () -> Void) {
        if bookmarksAPI.isLoaded {
            DispatchQueue.main.async {
                completion()
            }
        } else {
            var observer: BookmarkModelListener?
            observer = Bookmarkv2.bookmarksAPI.add(BookmarkModelStateObserver({
                if case .modelLoaded = $0 {
                    observer?.destroy()
                    observer = nil
                    
                    DispatchQueue.main.async {
                        completion()
                    }
                }
            }))
        }
    }
}
