/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import WebKit
import Shared
import Data
import BraveShared
import Combine
import os.log

private let log = ContentBlockerManager.log

enum BlockerStatus: String {
  case Disabled
  case NoBlockedURLs  // When TP is enabled but nothing is being blocked
  case Whitelisted
  case Blocking
}

struct ContentBlockingConfig {
  struct Prefs {
    static let normalBrowsingEnabledKey = "prefkey.trackingprotection.normalbrowsing"
    static let privateBrowsingEnabledKey = "prefkey.trackingprotection.privatebrowsing"
  }

  struct Defaults {
    static let normalBrowsing = true
    static let privateBrowsing = true
  }
}

struct NoImageModeDefaults {
  static let script = "[{'trigger':{'url-filter':'.*','resource-type':['image']},'action':{'type':'block'}}]".replacingOccurrences(of: "'", with: "\"")
  static let scriptName = "images"
}

enum BlockingStrength: String {
  case basic
  case strict

  static let allOptions: [BlockingStrength] = [.basic, .strict]
}

class ContentBlockerHelper {
  private(set) weak var tab: Tab?
  
  /// The rule lists that are loaded into the current tab
  private var setRuleLists: Set<WKContentRuleList> = []

  var stats: TPPageStats = TPPageStats() {
    didSet {
      guard let tab = self.tab else { return }
      if stats.total <= 1 {
        TabEvent.post(.didChangeContentBlocking, for: tab)
      }
      statsDidChange?(stats)
    }
  }

  var statsDidChange: ((TPPageStats) -> Void)?
  var blockedRequests: Set<URL> = []

  init(tab: Tab) {
    self.tab = tab
  }
  
  @MainActor func set(ruleLists: Set<WKContentRuleList>) {
    guard ruleLists != setRuleLists else { return }
    #if DEBUG
    ContentBlockerManager.log.debug("Set rule lists:")
    #endif
    
    // Remove unwanted rule lists
    for ruleList in setRuleLists.subtracting(ruleLists) {
      // It's added but we don't want it. So we remove it.
      tab?.webView?.configuration.userContentController.remove(ruleList)
      setRuleLists.remove(ruleList)
      
      #if DEBUG
      ContentBlockerManager.log.debug(" - \(ruleList.identifier)")
      #endif
    }
    
    // Add missing rule lists
    for ruleList in ruleLists.subtracting(setRuleLists) {
      tab?.webView?.configuration.userContentController.add(ruleList)
      setRuleLists.insert(ruleList)
      
      #if DEBUG
      ContentBlockerManager.log.debug(" + \(ruleList.identifier)")
      #endif
    }
  }
}
