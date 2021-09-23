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

extension BookmarkNode {
        
    // MARK: Internal
    
    public var domain: Domain? {
        if let url = titleUrlNodeUrl {
            return Domain.getOrCreate(forUrl: url, persistent: true)
        }
        return nil
    }
    
    public var parentNode: BookmarkNode? {
        // Return nil if the parent is the ROOT node
        // because AddEditBookmarkTableViewController.sortFolders
        // sorts root folders by having a nil parent.
        // If that code changes, we should change here to match.
        guard parent?.guid != BookmarkNode.rootNodeGuid else {
            return nil
        }
        
        return parent
    }
    
    public var objectID: Int {
        return Int(nodeId)
    }
    
    public func update(customTitle: String?, url: URL?) {
        setTitle(customTitle ?? "")
        self.url = url
    }
    
    public var existsInPersistentStore: Bool {
        isValid && parent != nil
    }
    
    private struct AssociatedObjectKeys {
        static var faviconObserver: Int = 0
    }
    
    public var bookmarkFavIconObserver: BookmarkModelListener? {
        get { objc_getAssociatedObject(self, &AssociatedObjectKeys.faviconObserver) as? BookmarkModelListener }
        set { objc_setAssociatedObject(self, &AssociatedObjectKeys.faviconObserver, newValue, .OBJC_ASSOCIATION_RETAIN_NONATOMIC) }
    }
}

class BraveBookmarkFolder: BookmarkNode {
    public let indentationLevel: Int
    
    private init(_ bookmarkNode: BookmarkNode) {
        self.indentationLevel = 0
        super.init()
    }
    
    public init(_ bookmarkFolder: BookmarkFolder) {
        self.indentationLevel = bookmarkFolder.indentationLevel
        super.init()
    }
}
