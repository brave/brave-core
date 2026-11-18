// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the Mozilla Public
// License, v. 2.0. See https://mozilla.org/MPL/2.0/.
import Preferences
import Foundation
import Web
import WebKit

@MainActor
enum TorOnionPolicy {
  private static var compilation: Task<WKContentRuleList, Error>?

  /// Install before allowing navigation, covering subresources as well as frames.
  static func install(on tab: some TabState) async throws {
    guard !tab.isTor else { return }
    if compilation == nil {
      guard Preferences.Privacy.onlyOnionInTor.value else { return }
      compilation = Task {
        try await withCheckedThrowingContinuation { continuation in
          WKContentRuleListStore.default().compileContentRuleList(
            forIdentifier: "Brave.OnionOnlyInTor", encodedContentRuleList: rulesJSON
          ) { list, error in
            if let list { continuation.resume(returning: list) }
            else { continuation.resume(throwing: error ?? URLError(.cannotLoadFromNetwork)) }
          }
        }
      }
    }
    guard let compilation else { return }
    let list = try await compilation.value
    guard let controller = tab.configuration?.userContentController else { return }
    controller.remove(list)
    if Preferences.Privacy.onlyOnionInTor.value { controller.add(list) }
  }
  // WebKit's URL-filter parser does not support regex alternation.
  static let rulesJSON = """
    [
      {"trigger":{"url-filter":"^https?://[^/]*\\\\.onion[:/]","url-filter-is-case-sensitive":false},"action":{"type":"block"}},
      {"trigger":{"url-filter":"^https?://[^/]*\\\\.onion$","url-filter-is-case-sensitive":false},"action":{"type":"block"}},
      {"trigger":{"url-filter":"^wss?://[^/]*\\\\.onion[:/]","url-filter-is-case-sensitive":false},"action":{"type":"block"}},
      {"trigger":{"url-filter":"^wss?://[^/]*\\\\.onion$","url-filter-is-case-sensitive":false},"action":{"type":"block"}}
    ]
    """
}
