// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import UIKit
import WebKit
import CryptoKit
@testable import Brave

class MockScriptsViewController: UIViewController {
  enum LoadError: Error {
    case timedOut
  }
  
  typealias LoadCallback = (Error?) -> Void?
  
  let webView: WKWebView
  let scriptFactory: ScriptFactory
  
  private let userScriptManager = UserScriptManager()
  private var loadCallback: LoadCallback?
  
  init() {
    let configuration = WKWebViewConfiguration()
    self.webView = WKWebView(frame: CGRect(width: 10, height: 10), configuration: configuration)
    self.scriptFactory = ScriptFactory()
    super.init(nibName: nil, bundle: nil)
    
    webView.navigationDelegate = self
  }
  
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    // Will load some base scripts into this webview
    userScriptManager.loadScripts(into: webView, scripts: [])
    
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
  
  func loadHTMLString(_ htmlString: String, timeout: TimeInterval = 10) async throws {
    return try await withCheckedThrowingContinuation { continuation in
      self.webView.loadHTMLString(htmlString, baseURL: URL(string: "https://example.com"))
      // Add a timeout incase something is wrong (so we don't the test hang for ever)
      let timer = Timer.scheduledTimer(withTimeInterval: timeout, repeats: false, block: { _ in
        continuation.resume(throwing: LoadError.timedOut)
        
        Task { @MainActor in
          self.loadCallback = nil
        }
      })
      
      self.loadCallback = { error in
        self.loadCallback = nil
        timer.invalidate()
        continuation.resume()
      }
    }
  }
  
  // MARK: - MockMessageHandler
  
  func attachScriptHandler(contentWorld: WKContentWorld, name: String, timeout: TimeInterval = 10) -> AsyncStream<WKScriptMessage> {
    return AsyncStream { continuation in
      let userContentController = webView.configuration.userContentController
      // Add a timeout in case the script is bad and the handler never triggers
      // (so we don't the test hang for ever)
      let timer = Timer.scheduledTimer(withTimeInterval: timeout, repeats: false, block: { _ in
        continuation.finish()
      })
      
      continuation.onTermination = { @Sendable _ in
        userContentController.removeScriptMessageHandler(forName: name)
      }
      
      userContentController.addScriptMessageHandler(
        MockMessageHandler { message in
          timer.invalidate()
          continuation.yield(message)
          return nil
        },
        contentWorld: contentWorld,
        name: name
      )
    }
  }
  
  @discardableResult
  func attachScriptHandler(contentWorld: WKContentWorld, name: String, messageHandler: MockMessageHandler) -> MockMessageHandler {
    webView.configuration.userContentController.addScriptMessageHandler(
      messageHandler,
      contentWorld: contentWorld,
      name: name
    )
    
    return messageHandler
  }
}

// MARK: - WKNavigationDelegate

extension MockScriptsViewController: WKNavigationDelegate {
  func webView(_ webView: WKWebView, didStartProvisionalNavigation navigation: WKNavigation!) {
  }
  
  func webView(_ webView: WKWebView, didFinish navigation: WKNavigation!) {
    loadCallback?(nil)
  }
  
  func webView(_ webView: WKWebView, decidePolicyFor navigationAction: WKNavigationAction, preferences: WKWebpagePreferences, decisionHandler: @escaping (WKNavigationActionPolicy, WKWebpagePreferences) -> Void) {
    decisionHandler(.allow, preferences)
  }
  
  func webView(_ webView: WKWebView, didFail navigation: WKNavigation!, withError error: Error) {
    loadCallback?(error)
  }
}
