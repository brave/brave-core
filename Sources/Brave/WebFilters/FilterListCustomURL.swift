// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Data
import BraveCore

struct FilterListCustomURL: Identifiable, Equatable {
  enum DownloadStatus: Equatable {
    case pending
    case failure
    case downloaded(Date)
  }
  
  var id: ObjectIdentifier {
    return setting.id
  }
  
  var setting: CustomFilterListSetting
  var downloadStatus: DownloadStatus = .pending
  
  @MainActor var title: String {
    let lastPathComponent = setting.externalURL.lastPathComponent
    guard !lastPathComponent.isEmpty else {
      return URLFormatter.formatURLOrigin(
        forDisplayOmitSchemePathAndTrivialSubdomains: setting.externalURL.absoluteDisplayString
      )
    }
    return lastPathComponent
  }
  
  public init(setting: CustomFilterListSetting, downloadStatus: DownloadStatus = .pending) {
    self.setting = setting
    self.downloadStatus = downloadStatus
  }
  
  @MainActor public init(externalURL: URL, isEnabled: Bool, inMemory: Bool) {
    let setting = CustomFilterListSetting.create(
      externalURL: externalURL, isEnabled: isEnabled,
      inMemory: inMemory
    )
    
    self.init(setting: setting)
  }
}
