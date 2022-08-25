/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import BraveCore

class SendableTabInfoDataSource {

  /// The information  related with tab to be sent to alist of devices
  private let deviceList: [SendTabTargetDevice]
  let displayTitle: String
  let sendableURL: URL
  var selectedIndex = 0
  
  // MARK: Lifecycle

  init(with deviceList: [SendTabTargetDevice], displayTitle: String, sendableURL: URL) {
    self.deviceList = deviceList
    self.displayTitle = displayTitle
    self.sendableURL = sendableURL
  }

  // MARK: Internal

  func deviceInformation(for indexPath: IndexPath) -> SendTabTargetDevice? {
    return deviceList[safe: indexPath.row]
  }
  
  func deviceCacheID() -> String? {
    return deviceList[safe: selectedIndex]?.cacheId
  }
  
  func numberOfDevices() -> Int {
    return deviceList.count
  }
}
