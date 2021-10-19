// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Data
import BraveCore
import BraveShared
import CoreData
import Shared

private let log = Logger.browserLogger

class BookmarkManager {

    // MARK: Lifecycle
    
    init(bookmarksAPI: BraveBookmarksAPI?) {
        self.bookmarksAPI = bookmarksAPI        
        BookmarkManager.rootNodeId = bookmarksAPI?.rootNode?.guid
    }
    
    // MARK: Internal
    
    public static var rootNodeId: String?

    // Returns the last visited folder
    // If no folder was visited, returns the mobile bookmarks folder
    // If the root folder was visited, returns nil
    public func lastVisitedFolder() -> Bookmarkv2? {
        guard let bookmarksAPI = bookmarksAPI else {
            return nil
        }
        
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
        if let folderNode = bookmarksAPI.getNodeById(nodeId),
           folderNode.isVisible {
            return Bookmarkv2(folderNode)
        }
        
        // Default folder is the mobile node..
        if let mobileNode = bookmarksAPI.mobileNode {
            return Bookmarkv2(mobileNode)
        }
        return nil
    }
    
    public func lastFolderPath() -> [Bookmarkv2] {
        guard let bookmarksAPI = bookmarksAPI else {
            return []
        }
        
        if Preferences.General.showLastVisitedBookmarksFolder.value,
           let nodeId = Preferences.Chromium.lastBookmarksFolderNodeId.value,
           var folderNode = bookmarksAPI.getNodeById(nodeId),
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
    
    public func mobileNode() -> Bookmarkv2? {
        guard let bookmarksAPI = bookmarksAPI else {
            return nil
        }
        
        if let node = bookmarksAPI.mobileNode {
            return Bookmarkv2(node)
        }
        return nil
    }
    
    public func fetchParent(_ bookmarkItem: Bookmarkv2?) -> Bookmarkv2? {
        guard let bookmarkItem = bookmarkItem, let bookmarksAPI = bookmarksAPI else {
            return nil
        }
        
        if let parent = bookmarkItem.bookmarkNode.parent {
            // Return nil if the parent is the ROOT node
            // because AddEditBookmarkTableViewController.sortFolders
            // sorts root folders by having a nil parent.
            // If that code changes, we should change here to match.
            if bookmarkItem.bookmarkNode.parent?.guid != bookmarksAPI.rootNode?.guid {
                return Bookmarkv2(parent)
            }
        }
        return nil
    }
    
    @discardableResult
    public func addFolder(title: String, parentFolder: Bookmarkv2? = nil) -> BookmarkNode? {
        guard let bookmarksAPI = bookmarksAPI else {
            return nil
        }
        
        if let parentFolder = parentFolder?.bookmarkNode {
            return bookmarksAPI.createFolder(withParent: parentFolder, title: title)
        } else {
            return bookmarksAPI.createFolder(withTitle: title)
        }
    }
    
    public func add(url: URL, title: String?, parentFolder: Bookmarkv2? = nil) {
        guard let bookmarksAPI = bookmarksAPI else {
            return
        }
        
        if let parentFolder = parentFolder?.bookmarkNode {
            bookmarksAPI.createBookmark(withParent: parentFolder, title: title ?? "", with: url)
        } else {
            bookmarksAPI.createBookmark(withTitle: title ?? "", url: url)
        }
    }
    
    public func frc(parent: Bookmarkv2?) -> BookmarksV2FetchResultsController? {
        guard let bookmarksAPI = bookmarksAPI else {
            return nil
        }
        
        return Bookmarkv2Fetcher(parent?.bookmarkNode, api: bookmarksAPI)
    }
    
    public func foldersFrc(excludedFolder: Bookmarkv2? = nil) -> BookmarksV2FetchResultsController? {
        guard let bookmarksAPI = bookmarksAPI else {
            return nil
        }
        
        return Bookmarkv2ExclusiveFetcher(excludedFolder?.bookmarkNode, api: bookmarksAPI)
    }
    
    public func getChildren(forFolder folder: Bookmarkv2, includeFolders: Bool) -> [Bookmarkv2]? {
        let result = folder.bookmarkNode.children.map({ Bookmarkv2($0) })
        return includeFolders ? result : result.filter({ $0.isFolder == false })
    }
    
    public func byFrequency(query: String? = nil, completion: @escaping ([WebsitePresentable]) -> Void) {
        // Invalid query.. BraveCore doesn't store bookmarks based on last visited.
        // Any last visited bookmarks would show up in `History` anyway.
        // BraveCore automatically sorts them by date as well.
        guard let query = query, !query.isEmpty, let bookmarksAPI = bookmarksAPI else {
            completion([])
            return
        }
        
        return bookmarksAPI.search(withQuery: query, maxCount: 200, completion: { nodes in
            completion(nodes.compactMap({ return !$0.isFolder ? Bookmarkv2($0) : nil }))
        })
    }
    
    public func reorderBookmarks(frc: BookmarksV2FetchResultsController?, sourceIndexPath: IndexPath, destinationIndexPath: IndexPath) {
        guard let frc = frc, let bookmarksAPI = bookmarksAPI else {
            return
        }
        
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
    
    public func delete(_ bookmarkItem: Bookmarkv2) {
        guard let bookmarksAPI = bookmarksAPI else {
            return
        }
        
        if bookmarkItem.canBeDeleted {
            bookmarksAPI.removeBookmark(bookmarkItem.bookmarkNode)
        }
    }
    
    public func updateWithNewLocation(_ bookmarkItem: Bookmarkv2, customTitle: String?, url: URL?, location: Bookmarkv2?) {
        guard let bookmarksAPI = bookmarksAPI else {
            return
        }
        
        if let location = location?.bookmarkNode ?? bookmarksAPI.mobileNode {
            if location.guid != bookmarkItem.bookmarkNode.parent?.guid {
                bookmarkItem.bookmarkNode.move(toParent: location)
            }
            
            if let customTitle = customTitle {
                bookmarkItem.bookmarkNode.setTitle(customTitle)
            }
            
            if let url = url, !bookmarkItem.bookmarkNode.isFolder {
                bookmarkItem.bookmarkNode.url = url
            } else if url != nil {
                log.error("Error: Moving bookmark - Cannot convert a folder into a bookmark with url.")
            }
        } else {
            log.error("Error: Moving bookmark - Cannot move a bookmark to Root.")
        }
    }
    
    public func addFavIconObserver(_ bookmarkItem: Bookmarkv2, observer: @escaping () -> Void) {
        guard let bookmarksAPI = bookmarksAPI else {
            return
        }
        
        let observer = BookmarkModelStateObserver { [weak self] state in
            if case .favIconChanged(let node) = state {
                if node.isValid && bookmarkItem.bookmarkNode.isValid
                    && node.guid == bookmarkItem.bookmarkNode.guid {
                    
                    if bookmarkItem.bookmarkNode.isFavIconLoaded {
                        self?.removeFavIconObserver(bookmarkItem)
                    }
                    
                    observer()
                }
            }
        }
        
        bookmarkItem.bookmarkFavIconObserver = bookmarksAPI.add(observer)
    }
    
    // MARK: Private
    
    private var observer: BookmarkModelListener?
    private let bookmarksAPI: BraveBookmarksAPI?
    
    private func removeFavIconObserver(_ bookmarkItem: Bookmarkv2) {
        bookmarkItem.bookmarkFavIconObserver = nil
    }
}

// MARK: Brave-Core Only

extension BookmarkManager {
  
    public func waitForBookmarkModelLoaded(_ completion: @escaping () -> Void) {
        guard let bookmarksAPI = bookmarksAPI else {
            return
        }
        
        if bookmarksAPI.isLoaded {
            DispatchQueue.main.async {
                completion()
            }
        } else {
            observer = bookmarksAPI.add(BookmarkModelStateObserver({ [weak self] in
                if case .modelLoaded = $0 {
                    self?.observer?.destroy()
                    self?.observer = nil
                    
                    DispatchQueue.main.async {
                        completion()
                    }
                }
            }))
        }
    }
}
