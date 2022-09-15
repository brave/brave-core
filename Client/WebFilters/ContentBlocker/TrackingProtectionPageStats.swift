/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// This file is largely verbatim from Focus iOS (Blockzilla/Lib/TrackingProtection).
// The preload and postload js files are unmodified from Focus.

import Shared
import BraveShared
import Data
import Foundation
import BraveCore

struct TPPageStats {
  let adCount: Int
  let trackerCount: Int
  let scriptCount: Int
  let fingerprintingCount: Int
  let httpsCount: Int

  var total: Int { return adCount + trackerCount + scriptCount + fingerprintingCount + httpsCount }

  init(adCount: Int = 0, trackerCount: Int = 0, scriptCount: Int = 0, fingerprintingCount: Int = 0, httpsCount: Int = 0) {
    self.adCount = adCount
    self.trackerCount = trackerCount
    self.scriptCount = scriptCount
    self.fingerprintingCount = fingerprintingCount
    self.httpsCount = httpsCount
  }
  
  func adding(adCount: Int = 0, trackerCount: Int = 0, scriptCount: Int = 0, fingerprintingCount: Int = 0, httpsCount: Int = 0) -> TPPageStats {
    TPPageStats(
      adCount: self.adCount + adCount,
      trackerCount: self.trackerCount + trackerCount,
      scriptCount: self.scriptCount + scriptCount,
      fingerprintingCount: self.fingerprintingCount + fingerprintingCount,
      httpsCount: self.httpsCount + httpsCount)
  }
}

class TPStatsBlocklistChecker {
  static let shared = TPStatsBlocklistChecker()
  private let adblockSerialQueue = AdBlockStats.adblockSerialQueue
  
  enum BlockedType {
    case image
    case ad
    case http
  }

  func isBlocked(requestURL: URL, sourceURL: URL, loadedRuleTypes: Set<ContentBlockerManager.BlocklistRuleType>, resourceType: AdblockEngine.ResourceType, callback: @escaping (BlockedType?) -> Void) {
    guard let host = requestURL.host, !host.isEmpty else {
      // TP Stats init isn't complete yet
      callback(nil)
      return
    }

    if resourceType == .image && Preferences.Shields.blockImages.value {
      callback(.image)
    }

    adblockSerialQueue.async {
      if (loadedRuleTypes.contains(.general(.blockAds)) || loadedRuleTypes.contains(.general(.blockTrackers)))
          && AdBlockStats.shared.shouldBlock(requestURL: requestURL, sourceURL: sourceURL, resourceType: resourceType) {
        DispatchQueue.main.async {
          callback(.ad)
        }
        
        return
      }

      // TODO: Downgrade to 14.5 once api becomes available.
      if #unavailable(iOS 15.0) {
        HttpsEverywhereStats.shared.shouldUpgrade(requestURL) { shouldUpgrade in
          DispatchQueue.main.async {
            if loadedRuleTypes.contains(.general(.upgradeHTTP)) && shouldUpgrade {
              callback(.http)
            } else {
              callback(nil)
            }
          }
        }
      } else {
        DispatchQueue.main.async {
          callback(nil)
        }
      }
    }
  }
}
