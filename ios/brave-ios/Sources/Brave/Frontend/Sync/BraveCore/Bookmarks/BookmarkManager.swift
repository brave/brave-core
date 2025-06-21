// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import CoreData
import Data
import Foundation
import Growth
import Preferences
import Shared
import os.log

class BookmarkManager {

  // MARK: Lifecycle

  init(bookmarksAPI: BraveBookmarksAPI?) {
    self.bookmarksAPI = bookmarksAPI
    BookmarkManager.rootNodeId = bookmarksAPI?.rootNode?.guid
    waitForBookmarkModelLoaded {
      self.recordTotalBookmarkCountP3A()
    }
  }

  // MARK: Internal

  public static var rootNodeId: String?

  // Returns the last visited folder
  // If no folder was visited, returns the mobile bookmarks folder
  // If the root folder was visited, returns nil
  public func lastVisitedFolder() -> BookmarkNode? {
    guard let bookmarksAPI = bookmarksAPI else {
      return nil
    }

    guard Preferences.General.showLastVisitedBookmarksFolder.value,
      let nodeId = Preferences.Chromium.lastBookmarksFolderNodeId.value
    else {
      // Default folder is the mobile node..
      if let mobileNode = bookmarksAPI.mobileNode {
        return mobileNode
      }
      return nil
    }

    // Display root folder instead of mobile node..
    if nodeId == -1 {
      return nil
    }

    // Display last visited folder..
    if let folderNode = bookmarksAPI.getNodeById(nodeId),
      folderNode.isVisible
    {
      return folderNode
    }

    // Default folder is the mobile node..
    if let mobileNode = bookmarksAPI.mobileNode {
      return mobileNode
    }
    return nil
  }

  public func lastVisitedFolderPath() -> [BookmarkNode] {
    guard let bookmarksAPI = bookmarksAPI else {
      return []
    }

    if Preferences.General.showLastVisitedBookmarksFolder.value,
      let nodeId = Preferences.Chromium.lastBookmarksFolderNodeId.value,
      var folderNode = bookmarksAPI.getNodeById(nodeId),
      folderNode.isVisible
    {

      // We don't ever display the root node
      // It is the mother of all nodes
      let rootNodeGuid = bookmarksAPI.rootNode?.guid

      var nodes = [BookmarkNode]()
      nodes.append(folderNode)

      while true {
        if let parent = folderNode.parentNode, parent.isVisible, parent.guid != rootNodeGuid {
          nodes.append(parent)
          folderNode = parent
          continue
        }
        break
      }
      return nodes.reversed()
    }

    // Default folder is the mobile node..
    if let mobileNode = bookmarksAPI.mobileNode {
      return [mobileNode]
    }

    return []
  }

  public func byFrequency(
    query: String? = nil,
    completion: @escaping ([BookmarkNode]) -> Void
  ) {
    // Invalid query.. BraveCore doesn't store bookmarks based on last visited.
    // Any last visited bookmarks would show up in `History` anyway.
    // BraveCore automatically sorts them by date as well.
    guard let query = query, !query.isEmpty, let bookmarksAPI = bookmarksAPI else {
      completion([])
      return
    }

    return bookmarksAPI.search(
      withQuery: query,
      maxCount: 200,
      completion: { nodes in
        completion(nodes.compactMap({ return !$0.isFolder ? $0 : nil }))
      }
    )
  }

  // MARK: Private

  private var observer: BookmarkModelListener?
  private let bookmarksAPI: BraveBookmarksAPI?

  // MARK: - P3A

  private func recordTotalBookmarkCountP3A() {
    // Q5 How many bookmarks do you have?
    guard let folders = bookmarksAPI?.mobileNode?.nestedChildFolders else { return }
    let count = folders.reduce(0, { $0 + $1.bookmarkNode.children.filter({ !$0.isFolder }).count })
    UmaHistogramRecordValueToBucket(
      "Brave.Core.BookmarksCountOnProfileLoad.2",
      buckets: [
        .r(0...5), .r(6...20), .r(21...100), .r(101...500),
        .r(501...1000), .r(1001...5000), .r(5001...10000), .r(10001...),
      ],
      value: Int(count)
    )
  }
}

// MARK: Brave-Core Only

extension BookmarkManager {

  public func waitForBookmarkModelLoaded(_ completion: @escaping () -> Void) {
    guard let bookmarksAPI = bookmarksAPI else {
      return
    }

    if bookmarksAPI.isLoaded {
      DispatchQueue.main.async {
        completion()
      }
    } else {
      observer = bookmarksAPI.add(
        BookmarkModelStateObserver({ [weak self] in
          if case .modelLoaded = $0 {
            self?.observer?.destroy()
            self?.observer = nil

            DispatchQueue.main.async {
              completion()
            }
          }
        })
      )
    }
  }
}
