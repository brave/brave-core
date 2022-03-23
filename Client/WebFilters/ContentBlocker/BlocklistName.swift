// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import WebKit
import Shared
import Data
import BraveShared

private let log = Logger.browserLogger

class BlocklistName: CustomStringConvertible, ContentBlocker {

  static let ad = BlocklistName(filename: "block-ads")
  static let tracker = BlocklistName(filename: "block-trackers")
  static let https = BlocklistName(filename: "upgrade-http")
  static let image = BlocklistName(filename: "block-images")
  static let cookie = BlocklistName(filename: "block-cookies")

  /// List of all bundled content blockers.
  /// Regional lists are downloaded on fly and not included here.
  static var allLists: Set<BlocklistName> {
    // TODO: Downgrade to 14.5 once api becomes available.
    if #available(iOS 15, *) {
      return [.ad, .tracker, .image]
    } else {
      return [.ad, .tracker, .https, .image]
    }
  }

  let filename: String
  var rule: WKContentRuleList?

  init(filename: String) {
    self.filename = filename
  }

  var description: String {
    return "<\(type(of: self)): \(self.filename)>"
  }

  private static let blocklistFileVersionMap: [BlocklistName: Preferences.Option<String?>] = [
    BlocklistName.ad: Preferences.BlockFileVersion.adblock,
    BlocklistName.https: Preferences.BlockFileVersion.httpse,
  ]

  lazy var fileVersionPref: Preferences.Option<String?>? = {
    return BlocklistName.blocklistFileVersionMap[self]
  }()

  lazy var fileVersion: String? = {
    guard let _ = BlocklistName.blocklistFileVersionMap[self] else { return nil }
    return Bundle.main.object(forInfoDictionaryKey: "CFBundleVersion") as? String
  }()

  static func blocklists(forDomain domain: Domain, locale: String? = Locale.current.languageCode) -> (on: Set<BlocklistName>, off: Set<BlocklistName>) {
    let regionalBlocker = ContentBlockerRegion.with(localeCode: locale)

    if domain.shield_allOff == 1 {
      var offList = allLists
      // Make sure to consider the regional list which needs to be disabled as well
      if let regionalBlocker = regionalBlocker {
        offList.insert(regionalBlocker)
      }
      return ([], offList)
    }

    var onList = Set<BlocklistName>()

    if domain.isShieldExpected(.AdblockAndTp, considerAllShieldsOption: true) {
      onList.formUnion([.ad, .tracker])

      if Preferences.Shields.useRegionAdBlock.value, let regionalBlocker = regionalBlocker {
        onList.insert(regionalBlocker)
      }
    }

    // For lists not implemented, always return exclude from `onList` to prevent accidental execution

    // TODO #159: Setup image shield

    var offList = allLists.subtracting(onList)
    // Make sure to consider the regional list since the user may disable it globally
    if let regionalBlocker = regionalBlocker, !onList.contains(regionalBlocker) {
      offList.insert(regionalBlocker)
    }

    return (onList, offList)
  }

  static func compileBundledRules(ruleStore: WKContentRuleListStore) async {
    await withTaskGroup(of: Void.self) { group in
      BlocklistName.allLists.forEach { list in
        group.addTask { await list.buildRule(ruleStore: ruleStore) }
      }

      // Compile block-cookie additionally
      group.addTask { await BlocklistName.cookie.buildRule(ruleStore: ruleStore) }
      await group.waitForAll()
    }
  }

  func compile(
    data: Data?,
    ruleStore: WKContentRuleListStore = ContentBlockerHelper.ruleStore
  ) async {

    guard let data = data, let dataString = String(data: data, encoding: .utf8) else {
      log.error("Could not read data for content blocker compilation.")
      return
    }

    do {
      let rule = try await ruleStore.compileContentRuleList(forIdentifier: filename, encodedContentRuleList: dataString)

      assert(rule != nil)
      self.rule = rule
    } catch {
      // TODO #382: Potential telemetry location
      log.error("Content blocker '\(self.filename)' errored: \(error.localizedDescription)")
    }
  }
}
