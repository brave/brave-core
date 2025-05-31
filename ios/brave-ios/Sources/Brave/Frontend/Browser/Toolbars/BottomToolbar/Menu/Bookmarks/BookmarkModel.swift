//
//  BookmarkModel.swift
//  Brave
//
//  Created by Brandon T on 2025-05-01.
//

import BraveCore
import Growth

enum BookmarkItemSelection {
  case openInNewTab
  case openInNewPrivateTab
  case copyLink
  case shareLink
}

class BookmarkModel: NSObject, ObservableObject {
  private let api: BraveBookmarksAPI?
  private weak var tabManager: TabManager?
  private weak var bookmarkManager: BookmarkManager?
  private weak var toolbarUrlActionsDelegate: ToolbarUrlActionsDelegate?
  private var dismiss: () -> Void

  @Published
  private(set) var isBookmarksServiceLoaded = false

  @Published
  private(set) var isPrivateBrowsing = false

  var lastVisitedFolder: Bookmarkv2? {
    bookmarkManager?.lastVisitedFolder()
  }

  var mobileBookmarksFolder: Bookmarkv2? {
    guard let api = api else {
      return nil
    }

    if let node = api.mobileNode {
      return Bookmarkv2(node)
    }
    return nil
  }

  init(
    api: BraveBookmarksAPI?,
    tabManager: TabManager?,
    bookmarksManager: BookmarkManager?,
    toolbarUrlActionsDelegate: ToolbarUrlActionsDelegate?,
    dismiss: @escaping () -> Void
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
  func bookmarks(for folder: Bookmarkv2? = nil, query: String?) async -> [Bookmarkv2] {
    if query == nil || query?.isEmpty == true {
      var items = [Bookmarkv2]()
      if let folder = folder {
        items.append(contentsOf: folder.children)
      } else if let api = api {
        if let node = api.mobileNode {
          items.append(Bookmarkv2(node))
        }

        if let node = api.desktopNode, node.childCount > 0 {
          items.append(Bookmarkv2(node))
        }

        if let node = api.otherNode, node.childCount > 0 {
          items.append(Bookmarkv2(node))
        }
      }
      return items
    }

    return await withCheckedContinuation { continuation in
      bookmarkManager?.byFrequency(query: query) {
        continuation.resume(returning: Array(($0 as! [Bookmarkv2]).uniqued()))
      }
    }
  }

  @MainActor
  func folders(excluding excludedFolder: Bookmarkv2? = nil) async -> [BraveBookmarkFolder] {
    guard let api = api else {
      return []
    }

    let nestedFolders = { (_ node: BookmarkNode, _ guid: String?) -> [BraveBookmarkFolder] in
      if let guid = guid {
        return node.nestedChildFolders.filter({ $0.bookmarkNode.guid != guid }).map({
          BraveBookmarkFolder($0)
        })
      }
      return node.nestedChildFolders.map({ BraveBookmarkFolder($0) })
    }

    var items = [BraveBookmarkFolder]()

    if let node = api.mobileNode {
      items.append(contentsOf: nestedFolders(node, excludedFolder?.bookmarkNode.guid))
    }

    if let node = api.desktopNode, node.childCount > 0 {
      items.append(contentsOf: nestedFolders(node, excludedFolder?.bookmarkNode.guid))
    }

    if let node = api.otherNode, node.childCount > 0 {
      items.append(contentsOf: nestedFolders(node, excludedFolder?.bookmarkNode.guid))
    }

    return items
  }

  func move(nodes: [Bookmarkv2], to index: Int) {
    nodes.enumerated().forEach({
      if let parent = $0.element.parent?.bookmarkNode ?? api?.mobileNode {
        $0.element.bookmarkNode.move(toParent: parent, index: UInt(index))
      }
    })
  }

  func delete(nodes: [Bookmarkv2]) {
    nodes.forEach({
      if $0.canBeDeleted {
        api?.removeBookmark($0.bookmarkNode)
      }
    })
  }

  func deleteAll() {
    api?.removeAll()
  }

  func handleBookmarkItemSelection(_ selection: BookmarkItemSelection, node: Bookmarkv2) {
    guard let urlString = node.url, let url = URL(string: urlString) else {
      return
    }

    switch selection {
    case .openInNewTab:
      dismiss()
      toolbarUrlActionsDelegate?.openInNewTab(url, isPrivate: false)
    case .openInNewPrivateTab:
      dismiss()
      toolbarUrlActionsDelegate?.openInNewTab(url, isPrivate: true)
    case .copyLink:
      toolbarUrlActionsDelegate?.copy(url)
    case .shareLink:
      toolbarUrlActionsDelegate?.share(url)
    }
  }

  @discardableResult
  public func addFolder(title: String, in parentFolder: Bookmarkv2? = nil) -> Bookmarkv2? {
    guard let api = api else {
      return nil
    }

    if let parentNode = parentFolder?.bookmarkNode {
      if let result = api.createFolder(withParent: parentNode, title: title) {
        return Bookmarkv2(result)
      }
      return nil
    }

    if let result = api.createFolder(withTitle: title) {
      return Bookmarkv2(result)
    }
    return nil
  }

  public func addBookmark(url: URL, title: String?, in parentFolder: Bookmarkv2? = nil) {
    guard let api = api else {
      return
    }

    if let parentNode = parentFolder?.bookmarkNode {
      api.createBookmark(withParent: parentNode, title: title ?? "", with: url)
    } else {
      api.createBookmark(withTitle: title ?? "", url: url)
    }

    AppReviewManager.shared.processSubCriteria(for: .numberOfBookmarks)
  }
}
