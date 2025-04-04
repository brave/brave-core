// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore

class TabCWVUIHandler: NSObject, BraveWebViewUIDelegate {
  private weak var tab: ChromiumTabState?

  init(tab: ChromiumTabState) {
    self.tab = tab
  }

  func webView(
    _ webView: CWVWebView,
    createWebViewWith configuration: CWVWebViewConfiguration,
    for action: CWVNavigationAction
  ) -> CWVWebView? {
    guard let tab,
      let childTab = tab.delegate?.tab(
        tab,
        createNewTabWithRequest: action.request
      ) as? ChromiumTabState
    else { return nil }
    childTab.cwvConfiguration = configuration
    // Child CWVWebView's must be created without a WKWebViewConfiguration to ensure it uses the
    // correct configuration passed by WebKit's delegate method
    childTab.wkConfiguration = nil
    childTab.createWebView()
    return childTab.webView
  }

  func webViewDidCreateNewWebView(_ webView: CWVWebView) {
    guard let tab else { return }
    tab.didCreateWebView()
  }

  func webViewDidClose(_ webView: CWVWebView) {
    guard let tab else { return }
    tab.delegate?.tabWebViewDidClose(tab)
  }

  func webView(
    _ webView: CWVWebView,
    contextMenuConfigurationFor element: CWVHTMLElement,
    completionHandler: @escaping (UIContextMenuConfiguration?) -> Void
  ) {
    guard let tab else {
      completionHandler(nil)
      return
    }
    Task {
      let configuration = await tab.delegate?.tab(
        tab,
        contextMenuConfigurationForLinkURL: element.hyperlink
      )
      completionHandler(configuration)
    }
  }

  func webView(
    _ webView: CWVWebView,
    requestMediaCapturePermissionFor type: CWVMediaCaptureType,
    decisionHandler: @escaping (CWVPermissionDecision) -> Void
  ) {
    guard let tab, let captureType = WebMediaCaptureType(type), let delegate = tab.delegate
    else {
      decisionHandler(.prompt)
      return
    }
    Task {
      let permission = await delegate.tab(tab, requestMediaCapturePermissionsFor: captureType)
      decisionHandler(.init(permission))
    }
  }

  func webView(
    _ webView: CWVWebView,
    runJavaScriptAlertPanelWithMessage message: String,
    pageURL url: URL,
    completionHandler: @escaping () -> Void
  ) {
    guard let tab else {
      completionHandler()
      return
    }
    Task {
      await tab.delegate?.tab(tab, runJavaScriptAlertPanelWithMessage: message, pageURL: url)
      completionHandler()
    }
  }

  func webView(
    _ webView: CWVWebView,
    runJavaScriptConfirmPanelWithMessage message: String,
    pageURL url: URL,
    completionHandler: @escaping (Bool) -> Void
  ) {
    guard let tab, let delegate = tab.delegate else {
      completionHandler(false)
      return
    }
    Task {
      let result = await delegate.tab(
        tab,
        runJavaScriptConfirmPanelWithMessage: message,
        pageURL: url
      )
      completionHandler(result)
    }
  }

  func webView(
    _ webView: CWVWebView,
    runJavaScriptTextInputPanelWithPrompt prompt: String,
    defaultText: String,
    pageURL url: URL,
    completionHandler: @escaping (String?) -> Void
  ) {
    guard let tab, let delegate = tab.delegate else {
      completionHandler(nil)
      return
    }
    Task {
      let result = await delegate.tab(
        tab,
        runJavaScriptConfirmPanelWithPrompt: prompt,
        defaultText: defaultText,
        pageURL: url
      )
      completionHandler(result)
    }
  }

  func webView(_ webView: CWVWebView, buildEditMenuWith builder: any UIMenuBuilder) {
    guard let tab, let delegate = tab.delegate else { return }
    delegate.tab(tab, buildEditMenuWithBuilder: builder)
  }
}

extension WebMediaCaptureType {
  init?(_ mediaCaptureType: CWVMediaCaptureType) {
    switch mediaCaptureType {
    case .microphone:
      self = .microphone
    case .camera:
      self = .camera
    case .cameraAndMicrophone:
      self = .cameraAndMicrophone
    default:
      return nil
    }
  }
}

extension CWVPermissionDecision {
  init(_ decision: WebPermissionDecision) {
    switch decision {
    case .prompt:
      self = .prompt
    case .grant:
      self = .grant
    case .deny:
      self = .deny
    }
  }
}
