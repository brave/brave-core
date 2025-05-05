//
//  BookmarkModel.swift
//  Brave
//
//  Created by Brandon T on 2025-05-01.
//

import BraveCore

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

  func delete(nodes: [Bookmarkv2]) {
    nodes.forEach({
      api?.removeBookmark($0.bookmarkNode)
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

  func model(for folder: Bookmarkv2) -> BookmarkModel {
    return BookmarkModel(
      api: api,
      tabManager: tabManager,
      bookmarksManager: bookmarkManager,
      toolbarUrlActionsDelegate: toolbarUrlActionsDelegate,
      dismiss: dismiss
    )
  }
}
