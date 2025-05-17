import BraveCore
import BraveShared
import CoreData
import Data
// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
import Foundation
import Shared

// A Lightweight wrapper around BraveCore bookmarks
// with the same layout/interface as `Bookmark (from CoreData)`
class Bookmarkv2: WebsitePresentable, Identifiable, Equatable, Hashable {

  // MARK: Operators

  var id: Int {
    Int(bookmarkNode.nodeId)
  }

  static func == (lhs: Bookmarkv2, rhs: Bookmarkv2) -> Bool {
    return lhs.id == rhs.id
  }

  func hash(into hasher: inout Hasher) {
    hasher.combine(id)
  }

  // MARK: Lifecycle

  init(_ bookmarkNode: BookmarkNode) {
    self.bookmarkNode = bookmarkNode
  }

  // MARK: Internal

  public let bookmarkNode: BookmarkNode

  public var bookmarkFavIconObserver: BookmarkModelListener?

  public var isFolder: Bool {
    return bookmarkNode.isFolder == true
  }

  public var title: String? {
    return bookmarkNode.titleUrlNodeTitle
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

  public var parent: Bookmarkv2? {
    if let parent = bookmarkNode.parent {
      // Return nil if the parent is the ROOT node
      // because AddEditBookmarkTableViewController.sortFolders
      // sorts root folders by having a nil parent.
      // If that code changes, we should change here to match.
      if bookmarkNode.parent?.guid != BookmarkManager.rootNodeId {
        return Bookmarkv2(parent)
      }
    }
    return nil
  }

  public var children: [Bookmarkv2] {
    return bookmarkNode.children.map({ Bookmarkv2($0) })
  }

  public var canBeDeleted: Bool {
    return bookmarkNode.isPermanentNode == false
  }

  public func update(customTitle: String?, url: URL?) {
    bookmarkNode.setTitle(customTitle ?? "")
    bookmarkNode.url = url
  }

  public func existsInPersistentStore() -> Bool {
    return bookmarkNode.isValid && bookmarkNode.parent != nil
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
