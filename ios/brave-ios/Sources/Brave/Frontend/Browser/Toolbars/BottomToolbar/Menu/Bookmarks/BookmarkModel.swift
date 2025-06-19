// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Growth

enum BookmarkItemSelection {
  case openInNewTab
  case openInNewPrivateTab
  case copyLink
  case shareLink
}

@Observable
class BookmarkModel: NSObject {
  private let api: BraveBookmarksAPI?
  private weak var tabManager: TabManager?
  private weak var bookmarkManager: BookmarkManager?
  private weak var toolbarUrlActionsDelegate: ToolbarUrlActionsDelegate?
  private var dismiss: (() -> Void)?
  private(set) var isPrivateBrowsing = false

  var lastVisitedFolder: BookmarkNode? {
    bookmarkManager?.lastVisitedFolder()
  }

  var lastVisitedFolderPath: [BookmarkNode] {
    bookmarkManager?.lastVisitedFolderPath() ?? []
  }

  var mobileBookmarksFolder: BookmarkNode? {
    guard let api = api else {
      return nil
    }

    if let node = api.mobileNode {
      return node
    }
    return nil
  }

  init(
    api: BraveBookmarksAPI?,
    tabManager: TabManager? = nil,
    bookmarksManager: BookmarkManager? = nil,
    toolbarUrlActionsDelegate: ToolbarUrlActionsDelegate? = nil,
    dismiss: (() -> Void)? = nil
  ) {
    self.api = api
    self.tabManager = tabManager
    self.bookmarkManager = bookmarksManager
    self.toolbarUrlActionsDelegate = toolbarUrlActionsDelegate
    self.dismiss = dismiss
    self.isPrivateBrowsing = tabManager?.privateBrowsingManager.isPrivateBrowsing == true
    super.init()
  }

  func addListener(_ listener: BookmarkModelStateObserver) -> BookmarkModelListener? {
    return api?.add(listener)
  }

  @MainActor
  func bookmarks(for folder: BookmarkNode? = nil, query: String?) async -> [BookmarkNode] {
    if query == nil || query?.isEmpty == true {
      var items = [BookmarkNode]()
      if let folder = folder {
        items.append(contentsOf: folder.children)
      } else if let api = api {
        if let node = api.mobileNode {
          items.append(node)
        }

        if let node = api.desktopNode, node.childCount > 0 {
          items.append(node)
        }

        if let node = api.otherNode, node.childCount > 0 {
          items.append(node)
        }
      }
      return items
    }

    return await withCheckedContinuation { continuation in
      bookmarkManager?.byFrequency(query: query) {
        continuation.resume(returning: Array($0.uniqued()))
      }
    }
  }

  @MainActor
  func folders(excluding excludedFolder: BookmarkNode? = nil) async -> [BookmarkFolder] {
    guard let api = api else {
      return []
    }

    let nestedFolders = { (_ node: BookmarkNode, _ guid: String?) -> [BookmarkFolder] in
      if let guid = guid {
        return node.nestedChildFolders.filter({ $0.bookmarkNode.guid != guid })
      }
      return node.nestedChildFolders
    }

    var items = [BookmarkFolder]()

    if let node = api.mobileNode {
      items.append(contentsOf: nestedFolders(node, excludedFolder?.guid))
    }

    if let node = api.desktopNode, node.childCount > 0 {
      items.append(contentsOf: nestedFolders(node, excludedFolder?.guid))
    }

    if let node = api.otherNode, node.childCount > 0 {
      items.append(contentsOf: nestedFolders(node, excludedFolder?.guid))
    }

    return items
  }

  func bookmarks(in folder: BookmarkNode) -> [URL] {
    guard folder.isFolder else {
      return []
    }

    var urls: [URL] = []
    var children = folder.children

    while !children.isEmpty {
      let bookmarkItem = children.removeFirst()
      if bookmarkItem.isFolder {
        children.insert(contentsOf: bookmarkItem.children, at: 0)
      } else if let bookmarkURL = bookmarkItem.url {
        urls.append(bookmarkURL)
      }
    }
    return urls
  }

  func move(nodes: [BookmarkNode], to index: Int) {
    nodes.enumerated().forEach({
      if let parent = $0.element.parentNode ?? api?.mobileNode {
        $0.element.move(toParent: parent, index: UInt(index))
      }
    })
  }

  func delete(nodes: [BookmarkNode]) {
    nodes.forEach({
      if $0.canBeDeleted {
        api?.removeBookmark($0)
      }
    })
  }

  func deleteAll() {
    api?.removeAll()
  }

  func openAllBookmarks(node: BookmarkNode) {
    dismiss?()
    toolbarUrlActionsDelegate?.batchOpen(bookmarks(in: node))
  }

  func handleBookmarkItemSelection(_ selection: BookmarkItemSelection, node: BookmarkNode) {
    guard let url = node.url else {
      return
    }

    switch selection {
    case .openInNewTab:
      dismiss?()
      toolbarUrlActionsDelegate?.openInNewTab(url, isPrivate: false)
    case .openInNewPrivateTab:
      dismiss?()
      toolbarUrlActionsDelegate?.openInNewTab(url, isPrivate: true)
    case .copyLink:
      toolbarUrlActionsDelegate?.copy(url)
    case .shareLink:
      toolbarUrlActionsDelegate?.share(url)
    }
  }

  @discardableResult
  public func addFolder(title: String, in parent: BookmarkNode? = nil) -> BookmarkNode? {
    guard let api = api else {
      return nil
    }

    if let parent = parent {
      return api.createFolder(withParent: parent, title: title)
    }

    return api.createFolder(withTitle: title)
  }

  public func addBookmark(url: URL, title: String?, in parent: BookmarkNode? = nil) {
    guard let api = api else {
      return
    }

    if let parent = parent {
      api.createBookmark(withParent: parent, title: title ?? "", with: url)
    } else {
      api.createBookmark(withTitle: title ?? "", url: url)
    }

    AppReviewManager.shared.processSubCriteria(for: .numberOfBookmarks)
  }
}
