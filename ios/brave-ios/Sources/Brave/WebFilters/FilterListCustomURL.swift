// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Data
import Foundation

struct FilterListCustomURL: Identifiable, Equatable {
  enum DownloadStatus: Equatable {
    case pending
    case failure
    case downloaded(Date)
  }

  var id: ObjectIdentifier {
    return setting.id
  }

  let title: String
  var setting: CustomFilterListSetting
  var downloadStatus: DownloadStatus = .pending

  @MainActor public init(
    setting: CustomFilterListSetting,
    downloadStatus: DownloadStatus = .pending
  ) {
    self.setting = setting
    self.downloadStatus = downloadStatus
    self.title = Self.createTitle(from: setting.externalURL)
  }

  @MainActor public init(externalURL: URL, isEnabled: Bool, inMemory: Bool) {
    let setting = CustomFilterListSetting.create(
      externalURL: externalURL,
      isEnabled: isEnabled,
      inMemory: inMemory
    )

    self.init(setting: setting)
  }

  private static func createTitle(from externalURL: URL) -> String {
    let lastPathComponent = externalURL.lastPathComponent
    guard !lastPathComponent.isEmpty else {
      return URLFormatter.formatURLOrigin(
        forDisplayOmitSchemePathAndTrivialSubdomains: externalURL.absoluteDisplayString
      )
    }
    return lastPathComponent
  }
}
