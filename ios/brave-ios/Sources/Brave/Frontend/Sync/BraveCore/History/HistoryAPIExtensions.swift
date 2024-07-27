// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import CoreData
import Data
import Foundation
import ScreenTime
import Shared

extension BraveHistoryAPI {

  // MARK: Internal

  func add(url: URL, title: String, dateAdded: Date) {
    let historyNode = HistoryNode(url: url, title: title, dateAdded: dateAdded)
    addHistory(historyNode)
  }

  func suffix(_ maxLength: Int, _ completion: @escaping ([HistoryNode]) -> Void) {
    let options = HistorySearchOptions(
      maxCount: UInt(max(20, maxLength)),
      duplicateHandling: .removePerDay
    )
    search(
      withQuery: nil,
      options: options,
      completion: { historyResults in
        completion(historyResults.map { $0 })
      }
    )
  }

  func byFrequency(query: String, completion: @escaping ([HistoryNode]) -> Void) {
    guard !query.isEmpty else {
      return
    }
    let options = HistorySearchOptions(
      maxCount: 200,
      duplicateHandling: query.isEmpty ? .removePerDay : .removeAll
    )
    search(
      withQuery: query,
      options: options,
      completion: { historyResults in
        completion(historyResults.map { $0 })
      }
    )
  }

  func update(_ historyNode: HistoryNode, customTitle: String?, dateAdded: Date?) {
    if let title = customTitle {
      historyNode.title = title
    }

    if let date = dateAdded {
      historyNode.dateAdded = date
    }
  }

  func deleteAll(completion: @escaping () -> Void) {
    var screenTimeHistory: STWebHistory?
    do {
      screenTimeHistory = try STWebHistory(bundleIdentifier: Bundle.main.bundleIdentifier!)
    } catch {
      assertionFailure("STWebHistory could not be initialized: \(error)")
    }

    DispatchQueue.main.async {
      self.removeAll {
        Domain.deleteNonBookmarkedAndClearSiteVisits {
          screenTimeHistory?.deleteAllHistory()
          completion()
        }
      }
    }
  }
}
