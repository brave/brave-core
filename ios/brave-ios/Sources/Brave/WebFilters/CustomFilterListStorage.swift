// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Data

@MainActor class CustomFilterListStorage: ObservableObject {
  static let shared = CustomFilterListStorage(persistChanges: true)
  /// Wether or not to store the data into disk or into memory
  let persistChanges: Bool
  /// A list of filter list URLs and their enabled statuses
  @Published var filterListsURLs: [FilterListCustomURL]
  
  init(persistChanges: Bool) {
    self.persistChanges = persistChanges
    self.filterListsURLs = []
  }
  
  func loadCachedFilterLists() {
    let settings = CustomFilterListSetting.loadAllSettings(fromMemory: !persistChanges)
    
    self.filterListsURLs = settings.map { setting in
      let resource = setting.resource
      let date = try? resource.creationDate()
      
      if let date = date {
        return FilterListCustomURL(setting: setting, downloadStatus: .downloaded(date))
      } else {
        return FilterListCustomURL(setting: setting, downloadStatus: .pending)
      }
    }
  }
  
  /// Ensures that the settings for a filter list are stored
  /// - Parameters:
  ///   - uuid: The uuid of the filter list to update
  ///   - isEnabled: A boolean indicating if the filter list is enabled or not
  public func ensureFilterList(for uuid: String, isEnabled: Bool) {
    // Enable the setting
    if let index = filterListsURLs.firstIndex(where: { $0.setting.uuid == uuid }) {
      filterListsURLs[index].setting.isEnabled = isEnabled
    }
  }

  func update(filterListId id: ObjectIdentifier, with result: Result<Date, Error>) {
    guard let index = filterListsURLs.firstIndex(where: { $0.id == id }) else {
      return
    }
    
    switch result {
    case .failure(let error):
      #if DEBUG
      let externalURL = filterListsURLs[index].setting.externalURL.absoluteString
      ContentBlockerManager.log.error(
        "Failed to download resource \(externalURL): \(String(describing: error))"
      )
      #endif
      
      filterListsURLs[index].downloadStatus = .failure
    case .success(let date):
      filterListsURLs[index].downloadStatus = .downloaded(date)
    }
  }
}
