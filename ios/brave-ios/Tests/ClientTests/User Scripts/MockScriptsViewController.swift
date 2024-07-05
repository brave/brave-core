// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import CryptoKit
import UIKit
import WebKit
import XCTest

@testable import Brave

class MockScriptsViewController: UIViewController {
  let webView: WKWebView
  let scriptFactory: ScriptFactory

  private let userScriptManager = UserScriptManager()
  private let delegateStream = ScriptsNavigationDelegateStream()

  init() {
    let configuration = WKWebViewConfiguration()
    self.webView = WKWebView(frame: CGRect(width: 10, height: 10), configuration: configuration)
    self.scriptFactory = ScriptFactory()
    super.init(nibName: nil, bundle: nil)
    webView.navigationDelegate = delegateStream
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    // Will load some base scripts into this webview
    userScriptManager.loadScripts(into: webView.configuration.userContentController, scripts: [])

    self.view.addSubview(webView)
    webView.snp.makeConstraints { make in
      make.edges.equalToSuperview()
    }
  }

  func add(scripts: Set<UserScriptType>) {
    for script in scripts.sorted(by: { $0.order < $1.order }) {
      do {
        let userScript = try scriptFactory.makeScript(for: script)
        add(userScript: userScript)
      } catch {
        XCTFail(error.localizedDescription)
      }
    }
  }

  func add(userScript: WKUserScript) {
    webView.configuration.userContentController.addUserScript(userScript)
  }

  func loadHTMLString(_ htmlString: String) -> ScriptsNavigationDelegateStream {
    self.webView.loadHTMLString(htmlString, baseURL: URL(string: "https://example.com"))
    return delegateStream
  }

  func loadHTMLStringAndWait(_ htmlString: String) async throws {
    for try await result in loadHTMLString(htmlString) {
      if let error = result.error {
        throw error
      } else {
        // We only care about the first result because we know the page loaded
        break
      }
    }
  }

  @discardableResult
  func attachScriptHandler(
    contentWorld: WKContentWorld,
    name: String,
    messageHandler: MockMessageHandler
  ) -> MockMessageHandler {
    webView.configuration.userContentController.addScriptMessageHandler(
      messageHandler,
      contentWorld: contentWorld,
      name: name
    )

    return messageHandler
  }

  func attachScriptHandler(
    contentWorld: WKContentWorld,
    name: String,
    timeout: TimeInterval = 120
  ) -> MockMessageHandler {
    return attachScriptHandler(
      contentWorld: contentWorld,
      name: name,
      messageHandler: MockMessageHandler(timeout: timeout) { _ in
        return nil
      }
    )
  }
}

class ScriptsNavigationDelegateStream: NSObject, WKNavigationDelegate, AsyncSequence,
  AsyncIteratorProtocol
{
  typealias AsyncIterator = ScriptsNavigationDelegateStream
  typealias Element = (navigation: WKNavigation, error: Error?)
  private var messages: [Element] = []
  private let timeout: TimeInterval
  private var start: Date

  init(timeout: TimeInterval = 60) {
    self.timeout = timeout
    self.start = Date()
  }

  enum LoadError: Error {
    case timedOut
  }

  func webView(_ webView: WKWebView, didStartProvisionalNavigation navigation: WKNavigation!) {
  }

  func webView(_ webView: WKWebView, didFinish navigation: WKNavigation!) {
    messages.append((navigation, nil))
  }

  func webView(
    _ webView: WKWebView,
    decidePolicyFor navigationAction: WKNavigationAction,
    preferences: WKWebpagePreferences,
    decisionHandler: @escaping (WKNavigationActionPolicy, WKWebpagePreferences) -> Void
  ) {
    decisionHandler(.allow, preferences)
  }

  func webView(_ webView: WKWebView, didFail navigation: WKNavigation!, withError error: Error) {
    messages.append((navigation, error))
  }

  func next() async throws -> Element? {
    while Date().timeIntervalSince(start) < timeout {
      guard !messages.isEmpty else {
        try await Task.sleep(seconds: 0.5)
        continue
      }

      return messages.removeFirst()
    }

    throw LoadError.timedOut
  }

  func makeAsyncIterator() -> ScriptsNavigationDelegateStream {
    self.start = Date()
    return self
  }
}
