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

extension BraveBookmarksAPI {

    // MARK: Internal
    
    // Returns the last visited folder
    // If no folder was visited, returns the mobile bookmarks folder
    // If the root folder was visited, returns nil
    func lastVisitedFolder() -> BookmarkNode? {
        guard Preferences.General.showLastVisitedBookmarksFolder.value,
              let nodeId = Preferences.Chromium.lastBookmarksFolderNodeId.value else {
            // Default folder is the mobile node..
            return mobileNode
        }
        
        // Display root folder instead of mobile node..
        if nodeId == -1 {
            return nil
        }
        
        // Display last visited folder..
        if let folderNode = getNodeById(nodeId),
           folderNode.isVisible {
            return folderNode
        }
        
        // Default folder is the mobile node..
        return mobileNode
    }
    
    func lastFolderPath() -> [BookmarkNode] {
        if Preferences.General.showLastVisitedBookmarksFolder.value,
           let nodeId = Preferences.Chromium.lastBookmarksFolderNodeId.value,
           var folderNode = getNodeById(nodeId),
           folderNode.isVisible {
            
            // We don't ever display the root node
            // It is the mother of all nodes
            let rootNodeGuid = rootNode?.guid
            
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
            return nodes.map({ $0 }).reversed()
        }
        
        // Default folder is the mobile node..
        if let mobileNode = mobileNode {
            return [mobileNode]
        }
        
        return []
    }
    
    @discardableResult
    func addFolder(title: String, parentFolder: BookmarkNode? = nil) -> BookmarkNode? {
        if let parentFolder = parentFolder {
            return createFolder(withParent: parentFolder, title: title)
        } else {
            return createFolder(withTitle: title)
        }
    }
    
    func add(url: URL, title: String?, parentFolder: BookmarkNode? = nil) {
        if let parentFolder = parentFolder {
            createBookmark(withParent: parentFolder, title: title ?? "", with: url)
        } else {
            createBookmark(withTitle: title ?? "", url: url)
        }
    }
    
    func frc(parent: BookmarkNode?) -> BookmarksV2FetchResultsController? {
        return Bookmarkv2Fetcher(parent, api: self)
    }
    
    func foldersFrc(excludedFolder: BookmarkNode? = nil) -> BookmarksV2FetchResultsController? {
        return Bookmarkv2ExclusiveFetcher(excludedFolder, api: self)
    }
    
    func getChildren(forFolder folder: BookmarkNode, includeFolders: Bool) -> [BookmarkNode]? {
        return includeFolders ? folder.children : folder.children.filter({ $0.isFolder == false })
    }
    
    func byFrequency(query: String, completion: @escaping ([BookmarkNode]) -> Void) {
        // Invalid query.. BraveCore doesn't store bookmarks based on last visited.
        // Any last visited bookmarks would show up in `History` anyway.
        // BraveCore automatically sorts them by date as well.
        guard !query.isEmpty else {
            completion([])
            return
        }
        
        return search(withQuery: query, maxCount: 200, completion: { nodes in
            completion(nodes.compactMap({ return !$0.isFolder ? $0 : nil }))
        })
    }
    
    func reorderBookmarks(frc: BookmarksV2FetchResultsController, sourceIndexPath: IndexPath, destinationIndexPath: IndexPath) {
        if let node = frc.object(at: sourceIndexPath),
           let parent = node.parent ?? mobileNode {
            
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
    
    func updateWithNewLocation(_ bookmarkItem: BookmarkNode, customTitle: String?, url: URL?, location: BookmarkNode?) {
        guard let location = location ?? mobileNode else {
            log.error("Error: Moving bookmark - Cannot move a bookmark to Root.")
            
            return
        }
        
        if location.guid != bookmarkItem.parent?.guid {
            bookmarkItem.move(toParent: location)
        }
        
        if let customTitle = customTitle {
            bookmarkItem.setTitle(customTitle)
        }
        
        if let url = url, !bookmarkItem.isFolder {
            bookmarkItem.url = url
        } else if url != nil {
            log.error("Error: Moving bookmark - Cannot convert a folder into a bookmark with url.")
        }
    }
    
    func addFavIconObserver(_ bookmarkItem: BookmarkNode, observer: @escaping () -> Void) {
        let observer = BookmarkModelStateObserver { [weak self] state in
            if case .favIconChanged(let node) = state {
                if node.isValid && bookmarkItem.isValid
                    && node.guid == bookmarkItem.guid {
                    
                    if bookmarkItem.isFavIconLoaded {
                        self?.removeFavIconObserver(bookmarkItem)
                    }
                    
                    observer()
                }
            }
        }
        
        bookmarkItem.bookmarkFavIconObserver = add(observer)
    }
    
    // MARK: Private
        
    private struct AssociatedObjectKeys {
        static var modelStateListener: Int = 0
    }
    
    private var observer: BookmarkModelListener? {
        get { objc_getAssociatedObject(self, &AssociatedObjectKeys.modelStateListener) as? BookmarkModelListener }
        set { objc_setAssociatedObject(self, &AssociatedObjectKeys.modelStateListener, newValue, .OBJC_ASSOCIATION_RETAIN_NONATOMIC) }
    }
    
    private func removeFavIconObserver(_ bookmarkItem: BookmarkNode) {
        bookmarkItem.bookmarkFavIconObserver = nil
    }
}

// MARK: Brave-Core Only

extension BraveBookmarksAPI {
  
    func waitForBookmarkModelLoaded(_ completion: @escaping () -> Void) {
        if isLoaded {
            DispatchQueue.main.async {
                completion()
            }
        } else {
            observer = add(BookmarkModelStateObserver({ [weak self] in
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
