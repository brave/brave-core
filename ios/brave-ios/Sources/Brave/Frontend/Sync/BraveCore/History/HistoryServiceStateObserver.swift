// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

class HistoryServiceStateObserver: BraveServiceStateObserver, HistoryServiceObserver {
  private let listener: (StateChange) -> Void

  enum StateChange {
    case serviceLoaded
    case serviceDeleted
    case historyVisited(HistoryNode)
    case historyModified(_ nodeList: [HistoryNode])
    case historyDeleted(_ nodeList: [HistoryNode], isAllHistory: Bool)
  }

  init(_ listener: @escaping (StateChange) -> Void) {
    self.listener = listener
  }

  func historyServiceLoaded() {
    listener(.serviceLoaded)

    postServiceLoadedNotification()
  }

  func historyServiceBeingDeleted() {
    listener(.serviceDeleted)
  }

  func historyNodeVisited(_ historyNode: HistoryNode) {
    listener(.historyVisited(historyNode))
  }

  func historyNodesModified(_ historyNodeList: [HistoryNode]) {
    listener(.historyModified(historyNodeList))
  }

  func historyNodesDeleted(_ historyNodeList: [HistoryNode], isAllHistory: Bool) {
    listener(.historyDeleted(historyNodeList, isAllHistory: isAllHistory))
  }

}
