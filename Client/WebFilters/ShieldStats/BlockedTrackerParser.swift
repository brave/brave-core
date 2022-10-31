// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveShared
import Onboarding

public enum AdBlockTrackerType: String, CaseIterable {
  case google
  case facebook
  case amazon
}

class BlockedTrackerParser {
  static let entityList = OnboardingDisconnectList.loadFromFile()

  static func parse(url: URL, fallbackToDomainURL: Bool) -> String? {
    guard let list = entityList else { return nil }

    let domain = url.baseDomain ?? url.host ?? url.schemelessAbsoluteString

    for entity in list.entities {
      let resources = entity.value.resources.filter({ $0 == domain })

      if !resources.isEmpty {
        return entity.key
      }
    }

    return fallbackToDomainURL ? domain : nil
  }
  
  static func parse(blockedRequestURLs: Set<URL>, selectedTabURL: URL) -> (displayTrackers: [AdBlockTrackerType], trackerCount: Int)? {
    guard let list = entityList else { return nil }
    
    var trackers = [String: [String]]()
    
    for entity in list.entities {
      for url in blockedRequestURLs {
        let domain = url.baseDomain ?? url.host ?? url.schemelessAbsoluteString
        let resources = entity.value.resources.filter({ $0 == domain })

        if !resources.isEmpty {
          trackers[entity.key] = resources
        } else {
          trackers[domain] = [domain]
        }
      }
    }
    
    let firstTracker = trackers.popFirst()
    let trackerCount = ((firstTracker?.value.count ?? 0) - 1) + trackers.reduce(0, { res, values in
        res + values.value.count
    })
    
    if trackerCount >= 10, !selectedTabURL.isSearchEngineURL {
      let displayTrackers = fetchBigTechAdBlockTrackers(trackers: trackers)
      
      return (displayTrackers, trackerCount)
    }

    return  nil
  }
  
  private static func fetchBigTechAdBlockTrackers(trackers: [String: [String]]) -> [AdBlockTrackerType] {
    var existingBigTechTrackers: [AdBlockTrackerType] = []
    
    for adBlockTracker in AdBlockTrackerType.allCases {
      let bigTechTrackerKey = trackers.first(where: { return $0.key.lowercased().contains(adBlockTracker.rawValue) })
      
      if bigTechTrackerKey != nil {
        existingBigTechTrackers.append(adBlockTracker)
      }
    }
    
    return existingBigTechTrackers
  }
}
