// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import WebKit
import Shared
import Data
import BraveShared

private let log = Logger.browserLogger

protocol ContentBlocker: AnyObject, Hashable {
  // Make constant `let
  var filename: String { get }
  var rule: WKContentRuleList? { get set }
  var fileVersionPref: Preferences.Option<String?>? { get }
  var fileVersion: String? { get }
}

extension ContentBlocker {
  func buildRule(ruleStore: WKContentRuleListStore) async {
    guard await needsCompiling(ruleStore: ruleStore) else {
      return
    }

    if let jsonString = await BlocklistName.loadJsonFromBundle(forResource: filename) {
      do {
        let rule = try await ruleStore.compileContentRuleList(forIdentifier: filename, encodedContentRuleList: jsonString)

        assert(rule != nil)

        self.rule = rule
        self.fileVersionPref?.value = self.fileVersion
      } catch {
        // TODO #382: Potential telemetry location
        log.error("Content blocker '\(self.filename)' errored: \(error.localizedDescription)")
        assert(false)
      }
    }
  }

  private func needsCompiling(ruleStore: WKContentRuleListStore) async -> Bool {
    if fileVersionPref?.value != fileVersion {
      // New file, so we must update the lists, no need to check the store
      return true
    }

    do {
      rule = try await ruleStore.contentRuleList(forIdentifier: filename)
      return false
    } catch {
      log.error(error)
      return true
    }
  }

  private static func loadJsonFromBundle(forResource file: String) async -> String? {
    await Task.detached(priority: .userInitiated) {
      guard let path = Bundle.main.path(forResource: file, ofType: "json"),
        let source = try? String(contentsOfFile: path, encoding: .utf8)
      else {
        assert(false)
        return nil
      }

      return source
    }.value
  }

  public static func == (lhs: Self, rhs: Self) -> Bool {
    return lhs.filename == rhs.filename
  }

  public func hash(into hasher: inout Hasher) {
    hasher.combine(filename)
  }
}
