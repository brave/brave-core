// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import CoreData
import Data
import Foundation
import Shared

extension BookmarkNode: Identifiable {
  // MARK: Operators

  public var id: Int {
    Int(nodeId)
  }

  public var domain: Domain? {
    if let url = url {
      return Domain.getOrCreate(forUrl: url, persistent: true)
    }
    return nil
  }

  public var parentNode: BookmarkNode? {
    if let parent = self.parent {
      // Return nil if the parent is the ROOT node
      // because AddEditBookmarkTableViewController.sortFolders
      // sorts root folders by having a nil parent.
      // If that code changes, we should change here to match.
      if parent.guid != BookmarkManager.rootNodeId {
        return parent
      }
    }
    return nil
  }

  public func existsInPersistentStore() -> Bool {
    return isValid && parent != nil
  }

  public var canBeDeleted: Bool {
    return isPermanentNode == false
  }
}

// A Lightweight wrapper around BraveCore bookmarks
// with the WebsitePresentable implementation
class BookmarkRepresentable: WebsitePresentable {
  var title: String?
  var url: String?

  init(_ bookmarkNode: BookmarkNode) {
    self.title = bookmarkNode.title
    self.url = bookmarkNode.url?.absoluteString
  }
}
