// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveRewards
import CoreData

protocol BookmarksV2FetchResultsDelegate: class {
    func controllerWillChangeContent(_ controller: BookmarksV2FetchResultsController)
    
    func controllerDidChangeContent(_ controller: BookmarksV2FetchResultsController)
    
    func controller(_ controller: BookmarksV2FetchResultsController, didChange anObject: Any, at indexPath: IndexPath?, for type: NSFetchedResultsChangeType, newIndexPath: IndexPath?)
    
    func controllerDidReloadContents(_ controller: BookmarksV2FetchResultsController)
}

protocol BookmarksV2FetchResultsController {
    /* weak */ var delegate: BookmarksV2FetchResultsDelegate? { get set }
    
    var fetchedObjects: [Bookmarkv2]? { get }
    func performFetch() throws
    func object(at indexPath: IndexPath) -> Bookmarkv2?
}

class Bookmarkv2Fetcher: NSObject, BookmarksV2FetchResultsController {
    weak var delegate: BookmarksV2FetchResultsDelegate?
    private var bookmarkModelListener: BookmarkModelListener?
    private weak var bookmarksAPI: BraveBookmarksAPI?
    
    private let parentNode: BookmarkNode?
    private var children = [BookmarkNode]()
    
    init(_ parentNode: BookmarkNode?, api: BraveBookmarksAPI) {
        self.parentNode = parentNode
        self.bookmarksAPI = api
        super.init()
        
        self.bookmarkModelListener = api.add(BookmarkModelStateObserver { [weak self] _ in
            guard let self = self else { return }
            self.delegate?.controllerDidReloadContents(self)
        })
    }
    
    var fetchedObjects: [Bookmarkv2]? {
        return children.map({ Bookmarkv2($0) })
    }
    
    func performFetch() throws {
        children.removeAll()
        
        if let parentNode = self.parentNode {
            children.append(contentsOf: parentNode.children)
        } else {
            if let node = bookmarksAPI?.mobileNode {
                children.append(node)
            }
            
            if let node = bookmarksAPI?.desktopNode, !node.children.isEmpty {
                children.append(node)
            }
            
            if let node = bookmarksAPI?.otherNode, !node.children.isEmpty {
                children.append(node)
            }
            
            if children.isEmpty {
                throw NSError(domain: "brave.core.migrator", code: -1, userInfo: [
                    NSLocalizedFailureReasonErrorKey: "Invalid Bookmark Nodes"
                ])
            }
        }
    }
    
    func object(at indexPath: IndexPath) -> Bookmarkv2? {
        guard let node = children[safe: indexPath.row] else { return nil }
        return Bookmarkv2(node)
    }
}

class Bookmarkv2ExclusiveFetcher: NSObject, BookmarksV2FetchResultsController {
    weak var delegate: BookmarksV2FetchResultsDelegate?
    private var bookmarkModelListener: BookmarkModelListener?
    
    private var excludedFolder: BookmarkNode?
    private var children = [BookmarkNode]()
    private weak var bookmarksAPI: BraveBookmarksAPI?
    
    init(_ excludedFolder: BookmarkNode?, api: BraveBookmarksAPI) {
        self.excludedFolder = excludedFolder
        self.bookmarksAPI = api
        super.init()
        
        self.bookmarkModelListener = api.add(BookmarkModelStateObserver { [weak self] _ in
            guard let self = self else { return }
            self.delegate?.controllerDidReloadContents(self)
        })
    }
    
    var fetchedObjects: [Bookmarkv2]? {
        return children.map({ Bookmarkv2($0) })
    }
    
    func performFetch() throws {
        children = []
        
        if let excludedFolder = self.excludedFolder {
            if let node = bookmarksAPI?.mobileNode {
                children.append(node)
                children.append(contentsOf: recurseNode(node).filter({ $0.isFolder && $0.guid != excludedFolder.guid }))
            }
            
            if let node = bookmarksAPI?.desktopNode, !node.children.isEmpty {
                children.append(node)
                children.append(contentsOf: recurseNode(node).filter({ $0.isFolder && $0.guid != excludedFolder.guid }))
            }
            
            if let node = bookmarksAPI?.otherNode, !node.children.isEmpty {
                children.append(node)
                children.append(contentsOf: recurseNode(node).filter({ $0.isFolder && $0.guid != excludedFolder.guid }))
            }
        } else {
            if let node = bookmarksAPI?.mobileNode {
                children.append(node)
                children.append(contentsOf: recurseNode(node).filter({ $0.isFolder }))
            }
            
            if let node = bookmarksAPI?.desktopNode, !node.children.isEmpty {
                children.append(node)
                children.append(contentsOf: recurseNode(node).filter({ $0.isFolder }))
            }
            
            if let node = bookmarksAPI?.otherNode, !node.children.isEmpty {
                children.append(node)
                children.append(contentsOf: recurseNode(node).filter({ $0.isFolder }))
            }
        }
        
        if children.isEmpty {
            throw NSError(domain: "brave.core.migrator", code: -1, userInfo: [
                NSLocalizedFailureReasonErrorKey: "Invalid Bookmark Nodes"
            ])
        }
    }
    
    func object(at indexPath: IndexPath) -> Bookmarkv2? {
        guard let node = children[safe: indexPath.row] else { return nil }
        return Bookmarkv2(node)
    }
    
    private func recurseNode(_ node: BookmarkNode) -> [BookmarkNode] {
        var result = [BookmarkNode]()
        
        for child in node.children {
            result += recurseNode(child)
            result.append(child)
        }
        return result
    }
}
