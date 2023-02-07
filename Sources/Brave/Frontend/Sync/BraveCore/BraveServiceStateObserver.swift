// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

class BraveServiceStateObserver: NSObject {

  // MARK: Static

  static let coreServiceLoadedNotification: Notification.Name = .init(rawValue: "BraveServiceStateDidLoaded")

  static var isServiceLoadStatePosted = false

  // MARK: Private

  func postServiceLoadedNotification() {
    guard !BraveServiceStateObserver.isServiceLoadStatePosted else {
      return
    }

    NotificationCenter.default.post(
      name: BraveServiceStateObserver.coreServiceLoadedNotification,
      object: nil)

    BraveServiceStateObserver.isServiceLoadStatePosted = true
  }
}
