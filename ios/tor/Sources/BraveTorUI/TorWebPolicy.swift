// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. See https://mozilla.org/MPL/2.0/.
#if os(iOS)
import BraveTor
import Foundation
import Network
import TorWebKit
import WebKit

@MainActor
public enum TorWebPolicy {
  public static var inWebViewNavigationPolicy: WKNavigationActionPolicy {
    BraveTorInWebViewNavigationPolicy()
  }

  public static let supportsMediaPlayback = BraveTorSupportsExclusiveMediaLoading()

  // Onion Browser's Mysk mitigation uses the ping resource type for DNS prefetch.
  static var contentRules: String {
    let types = supportsMediaPlayback ? #"["ping"]"# : #"["ping","media"]"#
    return """
      [{"trigger":{"url-filter":".*","resource-type":\(types)},
        "action":{"type":"block"}}]
      """
  }

  public static func allowsResponseMIMEType(_ mimeType: String) -> Bool {
    if supportsMediaPlayback { return true }
    let mime = mimeType.lowercased()
    return !mime.hasPrefix("audio/") && !mime.hasPrefix("video/")
      && mime != "application/vnd.apple.mpegurl" && mime != "application/x-mpegurl"
  }

  public static func compileRules() async throws -> WKContentRuleList {
    let ruleID = supportsMediaPlayback ? "brave-tor-dns-containment-v2" : "brave-tor-dns-containment-v1"
    // This local compilation has no cancellable WebKit API. Callers check their
    // lifecycle generation after the callback and before creating any web view.
    return try await withCheckedThrowingContinuation { continuation in
      WKContentRuleListStore.default().compileContentRuleList(
        forIdentifier: ruleID, encodedContentRuleList: contentRules
      ) { ruleList, error in
        if let error {
          continuation.resume(throwing: error)
        } else if let ruleList {
          continuation.resume(returning: ruleList)
        } else {
          continuation.resume(throwing: TorError.unavailable)
        }
      }
    }
  }

  public static func configuration(
    endpoint: TorEndpoint, rules: WKContentRuleList, isolationID: String,
    javaScriptEnabled: Bool
  ) throws -> WKWebViewConfiguration {
    try configuration(
      websiteDataStore: dataStore(endpoint: endpoint, isolationID: isolationID),
      rules: rules, javaScriptEnabled: javaScriptEnabled
    )
  }

  static func dataStore(endpoint: TorEndpoint, isolationID: String) -> WKWebsiteDataStore {
    let store = WKWebsiteDataStore.nonPersistent()
    var proxy = ProxyConfiguration(
      socksv5Proxy: .hostPort(host: .ipv4(.loopback), port: NWEndpoint.Port(rawValue: endpoint.port)!)
    )
    proxy.allowFailover = false
    proxy.applyCredential(username: isolationID, password: isolationID)
    store.proxyConfigurations = [proxy]
    return store
  }

  static func configuration(
    websiteDataStore store: WKWebsiteDataStore, rules: WKContentRuleList,
    javaScriptEnabled: Bool
  ) throws -> WKWebViewConfiguration {
    guard !store.isPersistent, !store.proxyConfigurations.isEmpty,
      store.proxyConfigurations.allSatisfy({ !$0.allowFailover }),
      let processPool = BraveTorUTCProcessPool()
    else { throw TorError.unavailable }
    let configuration = WKWebViewConfiguration()
    configuration.processPool = processPool
    configuration.websiteDataStore = store
    configuration.userContentController.add(rules)
    applyPagePreferences(
      configuration.defaultWebpagePreferences, javaScriptEnabled: javaScriptEnabled
    )
    configuration.preferences.javaScriptCanOpenWindowsAutomatically = false
    configuration.preferences.isTextInteractionEnabled = true
    configuration.allowsInlineMediaPlayback = supportsMediaPlayback
    configuration.allowsAirPlayForMediaPlayback = false
    configuration.allowsPictureInPictureMediaPlayback = false
    configuration.mediaTypesRequiringUserActionForPlayback = .all
    return configuration
  }

  public static func applyPagePreferences(
    _ preferences: WKWebpagePreferences, javaScriptEnabled: Bool
  ) {
    preferences.allowsContentJavaScript = javaScriptEnabled
    preferences.isLockdownModeEnabled = true
  }
}
#endif
