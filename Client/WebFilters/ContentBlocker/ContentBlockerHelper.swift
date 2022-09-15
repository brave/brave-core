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
  weak var tab: Tab?

  var isEnabled: Bool {
    return tab != nil
  }
  
  /// The rule types and source types that are currently loaded in this tab
  private(set) var loadedRuleTypeWithSourceTypes: Set<ContentBlockerManager.RuleTypeWithSourceType> = []
  /// The rule types with their source types that should be  loaded in this tab
  var ruleListTypes: Set<ContentBlockerManager.RuleTypeWithSourceType> = [] {
    didSet { reloadNeededRuleLists() }
  }

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
  
  private func reloadNeededRuleLists() {
    // Remove old values that were previously added
    for loadedRuleTypeWithSourceType in loadedRuleTypeWithSourceTypes {
      // Check if should be removed or if the source type doesn't match, otherwise don't do anything
      guard !ruleListTypes.contains(loadedRuleTypeWithSourceType) else {
        continue
      }
      
      guard let ruleList = ContentBlockerManager.shared.cachedRuleList(for: loadedRuleTypeWithSourceType.ruleType) else {
        // For some reason the rule is not cached. Shouldn't happen.
        // But if it does we have to remove all the rule lists
        // We will add back all the necessary ones below
        tab?.webView?.configuration.userContentController.removeAllContentRuleLists()
        loadedRuleTypeWithSourceTypes = []
        assertionFailure("This shouldn't happen!")
        break
      }
      
      // Since either it shouldn't be included or the source type doesn't match, we remove it
      tab?.webView?.configuration.userContentController.remove(ruleList)
      loadedRuleTypeWithSourceTypes.remove(loadedRuleTypeWithSourceType)
    }
    
    // Add new values that are not yet added (or were removed above because the source type didn't match)
    for ruleTypeWithSourceType in ruleListTypes {
      // Only add rule lists that are missing
      guard !loadedRuleTypeWithSourceTypes.contains(ruleTypeWithSourceType) else { continue }
      guard let ruleList = ContentBlockerManager.shared.cachedRuleList(for: ruleTypeWithSourceType.ruleType) else { continue }
      tab?.webView?.configuration.userContentController.add(ruleList)
      loadedRuleTypeWithSourceTypes.insert(ruleTypeWithSourceType)
    }
    
    #if DEBUG
    let rulesString = loadedRuleTypeWithSourceTypes.map { ruleTypeWithSourceType -> String in
      let ruleTypeString: String
      
      switch ruleTypeWithSourceType.ruleType {
      case .general(let type):
        ruleTypeString = type.rawValue
      case .filterList(let uuid):
        ruleTypeString = "filterList(\(uuid))"
      }
      
      let rulesDebugString =
      """
      {
      ruleType: \(ruleTypeString)
      sourceType: \(ruleTypeWithSourceType.sourceType)
      }
      """
      
      return rulesDebugString
    }
    
    log.debug("ContentBlockerHelper")
    log.debug("loaded \(self.loadedRuleTypeWithSourceTypes.count, privacy: .public) tab rules:\n\(rulesString, privacy: .public)")
    #endif
  }
}
