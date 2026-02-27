// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import FaviconModels
import Foundation
import UIKit
import WebKit

/// A implementation of TabState that can be used in unit tests or SwiftUI Previews.
///
/// It for the most part doesn't do anything other than set up its `TabDataValues` containers to
/// allow attaching tab helpers and expose some mutable state for testing purposes
public final class FakeTabState: TabState {
  public var id: ID = .init()
  public var profile: any Profile = FakeProfile()
  public var data: TabDataValues = .init()
  public let view: UIView = .init()
  public var opener: (any TabState)?
  public var isVisible: Bool = false {
    didSet {
      if isVisible {
        lastActiveTime = .now
      }
    }
  }
  public var lastActiveTime: Date? = .now
  public var webViewProxy: WebViewProxy? { nil }
  public var isWebViewCreated: Bool { false }
  public func createWebView() {}
  public func deleteWebView() {}

  // MARK: - Restoration

  public var sessionData: Data? { nil }
  public func restore(using sessionData: Data) throws {}
  public var isRestoring: Bool { false }

  // MARK: - Delegates, Observation and Policy Decisions

  public var delegate: TabDelegate?
  public var downloadDelegate: TabDownloadDelegate?
  public func addPolicyDecider(_ policyDecider: some TabPolicyDecider) {}
  public func removePolicyDecider(_ policyDecider: some TabPolicyDecider) {}
  public func addObserver(_ observer: some TabObserver) {}
  public func removeObserver(_ observer: some TabObserver) {}

  // MARK: - State

  public var visibleSecureContentState: SecureContentState { .unknown }
  public var serverTrust: SecTrust? { nil }
  public var favicon: Favicon?
  public var url: URL? { visibleURL }
  public var visibleURL: URL?
  public var lastCommittedURL: URL?
  public var previousCommittedURL: URL? { nil }
  public var contentsMimeType: String? { nil }
  public var title: String?
  public var isLoading: Bool { false }
  public var estimatedProgress: Double { 0 }
  public var canGoBack: Bool { false }
  public var canGoForward: Bool { false }
  public var backForwardList: (any BackForwardListProxy)? { nil }
  public var redirectChain: [URL] = []
  public var currentInitialURL: URL? { nil }
  public var currentUserAgentType: UserAgentType { .automatic }

  // MARK: - Interaction

  public func loadRequest(_ request: URLRequest) {}
  public func setVirtualURL(_ url: URL?) { visibleURL = url }
  public func reload() {}
  public func reloadWithUserAgentType(_ userAgentType: UserAgentType) {}
  public func stopLoading() {}
  public func goBack() {}
  public func goForward() {}
  public func goToBackForwardListItem(_ item: any BackForwardListItemProxy) {}
  public var canTakeSnapshot: Bool { false }
  public func takeSnapshot(rect: CGRect, handler: @escaping (UIImage?) -> Void) { handler(nil) }
  public func createFullPagePDF() async throws -> Data? { nil }
  public func presentFindInteraction(with text: String) {}
  public func dismissFindInteraction() {}
  public func evaluateJavaScriptUnsafe(_ javascript: String) {}

  // MARK: - WebKit specific

  public func loadHTMLString(_ htmlString: String, baseURL: URL?) {}
  @MainActor public func evaluateJavaScript(
    functionName: String,
    args: [Any],
    frame: WKFrameInfo?,
    contentWorld: WKContentWorld,
    escapeArgs: Bool,
    asFunction: Bool
  ) async throws -> Any? { nil }
  public func callAsyncJavaScript(
    _ functionBody: String,
    arguments: [String: Any],
    in frame: WKFrameInfo?,
    contentWorld: WKContentWorld
  ) async throws -> Any? { nil }
  public var configuration: WKWebViewConfiguration? { nil }
  public var viewPrintFormatter: UIViewPrintFormatter? { nil }
  public var dataForDisplayedPDF: Data? { nil }
  public var sampledPageTopColor: UIColor? { nil }
  public var viewScale: CGFloat = 1.0
  public func clearBackForwardList() {}

  // MARK: - Chromium specific

  public func updateScripts() {}

  public init() {}
}
