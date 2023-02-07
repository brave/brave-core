// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

class BookmarkModelStateObserver: BraveServiceStateObserver, BookmarkModelObserver {
  private let listener: (StateChange) -> Void

  enum StateChange {
    case modelLoaded
    case nodeChanged(BookmarkNode)
    case favIconChanged(BookmarkNode)
    case childrenChanged(BookmarkNode)
    case nodeMoved(_ node: BookmarkNode, _ from: BookmarkNode, _ to: BookmarkNode)
    case nodeDeleted(_ node: BookmarkNode, _ from: BookmarkNode)
    case allRemoved
  }

  init(_ listener: @escaping (StateChange) -> Void) {
    self.listener = listener
  }

  func bookmarkModelLoaded() {
    self.listener(.modelLoaded)

    postServiceLoadedNotification()
  }

  func bookmarkNodeChanged(_ bookmarkNode: BookmarkNode) {
    self.listener(.nodeChanged(bookmarkNode))
  }

  func bookmarkNodeFaviconChanged(_ bookmarkNode: BookmarkNode) {
    self.listener(.favIconChanged(bookmarkNode))
  }

  func bookmarkNodeChildrenChanged(_ bookmarkNode: BookmarkNode) {
    self.listener(.childrenChanged(bookmarkNode))
  }

  func bookmarkNode(_ bookmarkNode: BookmarkNode, movedFromParent oldParent: BookmarkNode, toParent newParent: BookmarkNode) {
    self.listener(.nodeMoved(bookmarkNode, oldParent, newParent))
  }

  func bookmarkNodeDeleted(_ node: BookmarkNode, fromFolder folder: BookmarkNode) {
    self.listener(.nodeDeleted(node, folder))
  }

  func bookmarkModelRemovedAllNodes() {
    self.listener(.allRemoved)
  }
}
