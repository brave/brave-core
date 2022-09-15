// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

class AdblockServicePublisher {
  /// Ad block service used for getting certain components
  private weak var adBlockService: AdblockService?
  
  /// The published results
  @Published var filterListResults: [String: (date: Date, result: Result<URL, Error>)]
  
  init() {
    filterListResults = [:]
  }
  
  /// Set the enabled filter lists. Should be called before starting the publisher
  func register(uuid: String) {
    do {
      let adBlockService = try getAdblockService()
      let filterList = try self.filterList(forUUID: uuid)
      
      adBlockService.registerFilterListComponent(filterList, componentReady: { [weak self] filterList, folderPath in
        guard let folderPath = folderPath else { return }
        let folderURL = URL(fileURLWithPath: folderPath)
        self?.filterListResults[filterList.uuid] = (Date(), .success(folderURL))
      })
    } catch {
      filterListResults[uuid] = (Date(), .failure(error))
    }
  }
  
  func unregister(uuid: String) {
    do {
      let adBlockService = try getAdblockService()
      let filterList = try self.filterList(forUUID: uuid)
      adBlockService.unregisterFilterListComponent(filterList)
    } catch {
      filterListResults[uuid] = (Date(), .failure(error))
    }
  }
  
  /// Start this publisher with the given `AdblockService` and initial UUIDs
  func start(adBlockService: AdblockService?) {
    self.adBlockService = adBlockService
  }
  
  private func getAdblockService() throws -> AdblockService {
    guard let adBlockService = adBlockService else {
      // Set an error so we have a result..otherwise nothing happens
      throw "AdblockService not available"
    }
    
    return adBlockService
  }
  
  private func filterList(forUUID uuid: String) throws -> AdblockFilterList {
    guard let filterList = adBlockService?.regionalFilterLists?.first(where: { $0.uuid == uuid }) else {
      throw "`AdblockFilterList` for uuid `\(uuid)` not found"
    }
    
    return filterList
  }
  
  /// Get the downloaded file URL for the filter list and resource type
  ///
  /// - Note: Returns nil if the file does not exist
  func downloadedURL(forUUID uuid: String) throws -> URL? {
    switch filterListResults[uuid]?.result {
    case .failure(let error): throw error
    case .success(let url): return url
    case .none: return nil
    }
  }
}
