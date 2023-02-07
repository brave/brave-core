// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

class SendTabToSelfStateObserver: BraveServiceStateObserver, SendTabToSelfModelStateObserver {

  private let listener: (StateChange) -> Void

  enum StateChange {
    case sendTabToSelfModelLoaded
    case sendTabToSelfEntriesAddedRemotely(_ newEntries: [OpenDistantTab])
    case sendTabToSelfEntriesRemovedRemotely
    case sendTabToSelfEntriesOpenedRemotely
  }

  init(_ listener: @escaping (StateChange) -> Void) {
    self.listener = listener
  }

  func sendTabToSelfModelLoaded() {
    listener(.sendTabToSelfModelLoaded)
  }
  
  func sendTab(toSelfEntriesAddedRemotely newEntries: [OpenDistantTab]) {
    listener(.sendTabToSelfEntriesAddedRemotely(newEntries))
  }
  
  func sendTabToSelfEntriesRemovedRemotely() {
    listener(.sendTabToSelfEntriesRemovedRemotely)
  }
  
  func sendTabToSelfEntriesOpenedRemotely() {
    listener(.sendTabToSelfEntriesOpenedRemotely)
  }
}
