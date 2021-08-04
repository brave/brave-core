// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveRewards
import CoreData

protocol BookmarksV2FetchResultsDelegate: AnyObject {
    func controllerWillChangeContent(_ controller: BookmarksV2FetchResultsController)
    
    func controllerDidChangeContent(_ controller: BookmarksV2FetchResultsController)
    
    func controller(_ controller: BookmarksV2FetchResultsController, didChange anObject: Any, at indexPath: IndexPath?, for type: NSFetchedResultsChangeType, newIndexPath: IndexPath?)
    
    func controllerDidReloadContents(_ controller: BookmarksV2FetchResultsController)
}

protocol BookmarksV2FetchResultsController {
    /* weak */ var delegate: BookmarksV2FetchResultsDelegate? { get set }
    
    var fetchedObjects: [Bookmarkv2]? { get }
    var fetchedObjectsCount: Int { get }
    func performFetch() throws
    func object(at indexPath: IndexPath) -> Bookmarkv2?
}

class Bookmarkv2Fetcher: NSObject, BookmarksV2FetchResultsController {
    weak var delegate: BookmarksV2FetchResultsDelegate?
    private var bookmarkModelListener: BookmarkModelListener?
    private weak var bookmarksAPI: BraveBookmarksAPI?
    
    private let parentNode: BookmarkNode?
    private var children = [Bookmarkv2]()
    
    init(_ parentNode: BookmarkNode?, api: BraveBookmarksAPI) {
        self.parentNode = parentNode
        self.bookmarksAPI = api
        super.init()
        
        self.bookmarkModelListener = api.add(BookmarkModelStateObserver { [weak self] _ in
            guard let self = self else { return }
            DispatchQueue.main.async {
                self.delegate?.controllerDidReloadContents(self)
            }
        })
    }
    
    var fetchedObjects: [Bookmarkv2]? {
        return children
    }
    
    var fetchedObjectsCount: Int {
        return children.count
    }
    
    func performFetch() throws {
        children.removeAll()
        
        if let parentNode = self.parentNode {
            children.append(contentsOf: parentNode.children.map({ Bookmarkv2($0) }))
        } else {
            if let node = bookmarksAPI?.mobileNode {
                children.append(Bookmarkv2(node))
            }
            
            if let node = bookmarksAPI?.desktopNode, node.childCount > 0 {
                children.append(Bookmarkv2(node))
            }
            
            if let node = bookmarksAPI?.otherNode, node.childCount > 0 {
                children.append(Bookmarkv2(node))
            }
            
            if children.isEmpty {
                throw NSError(domain: "brave.core.migrator", code: -1, userInfo: [
                    NSLocalizedFailureReasonErrorKey: "Invalid Bookmark Nodes"
                ])
            }
        }
    }
    
    func object(at indexPath: IndexPath) -> Bookmarkv2? {
        return children[safe: indexPath.row]
    }
}

class Bookmarkv2ExclusiveFetcher: NSObject, BookmarksV2FetchResultsController {
    weak var delegate: BookmarksV2FetchResultsDelegate?
    private var bookmarkModelListener: BookmarkModelListener?
    
    private var excludedFolder: BookmarkNode?
    private var children = [Bookmarkv2]()
    private weak var bookmarksAPI: BraveBookmarksAPI?
    
    init(_ excludedFolder: BookmarkNode?, api: BraveBookmarksAPI) {
        self.excludedFolder = excludedFolder
        self.bookmarksAPI = api
        super.init()
        
        self.bookmarkModelListener = api.add(BookmarkModelStateObserver { [weak self] _ in
            guard let self = self else { return }
            DispatchQueue.main.async {
                self.delegate?.controllerDidReloadContents(self)
            }
        })
    }
    
    var fetchedObjects: [Bookmarkv2]? {
        return children
    }
    
    var fetchedObjectsCount: Int {
        return children.count
    }
    
    func performFetch() throws {
        children = []
        
        if let node = bookmarksAPI?.mobileNode {
            children.append(contentsOf: getNestedFolders(node, guid: excludedFolder?.guid))
        }
        
        if let node = bookmarksAPI?.desktopNode, node.childCount > 0 {
            children.append(contentsOf: getNestedFolders(node, guid: excludedFolder?.guid))
        }
        
        if let node = bookmarksAPI?.otherNode, node.childCount > 0 {
            children.append(contentsOf: getNestedFolders(node, guid: excludedFolder?.guid))
        }
        
        if children.isEmpty {
            throw NSError(domain: "brave.core.migrator", code: -1, userInfo: [
                NSLocalizedFailureReasonErrorKey: "Invalid Bookmark Nodes"
            ])
        }
    }
    
    func object(at indexPath: IndexPath) -> Bookmarkv2? {
        return children[safe: indexPath.row]
    }
    
    private func getNestedFolders(_ node: BookmarkNode, guid: String?) -> [Bookmarkv2] {
        if let guid = guid {
            return node.nestedChildFolders.filter({ $0.bookmarkNode.guid != guid }).map({ BraveBookmarkFolder($0) })
        }
        return node.nestedChildFolders.map({ BraveBookmarkFolder($0) })
    }
}
