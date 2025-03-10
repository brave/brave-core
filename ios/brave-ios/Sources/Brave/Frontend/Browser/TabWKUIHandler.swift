// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import WebKit

class TabWKUIHandler: NSObject, WKUIDelegate {
  weak var tab: Tab?

  init(tab: Tab) {
    self.tab = tab
  }

  func webView(
    _ webView: WKWebView,
    createWebViewWith configuration: WKWebViewConfiguration,
    for navigationAction: WKNavigationAction,
    windowFeatures: WKWindowFeatures
  ) -> WKWebView? {
    guard let tab,
      let childTab = tab.webDelegate?.tab(
        tab,
        createNewTabWithRequest: navigationAction.request,
        configuration: configuration
      )
    else { return nil }
    return childTab.webView
  }

  func webViewDidClose(_ webView: WKWebView) {
    guard let tab else { return }
    tab.webDelegate?.tabWebViewDidClose(tab)
  }

  func webView(
    _ webView: WKWebView,
    requestMediaCapturePermissionFor origin: WKSecurityOrigin,
    initiatedByFrame frame: WKFrameInfo,
    type: WKMediaCaptureType,
    decisionHandler: @escaping (WKPermissionDecision) -> Void
  ) {
    guard let tab, let captureType = WebMediaCaptureType(type), let delegate = tab.webDelegate
    else {
      decisionHandler(.deny)
      return
    }

    if frame.securityOrigin.protocol.isEmpty || frame.securityOrigin.host.isEmpty {
      decisionHandler(.deny)
      return
    }

    let requestMediaPermissions: () -> Void = {
      Task {
        let permission = await delegate.tab(tab, requestMediaCapturePermissionsFor: captureType)
        decisionHandler(.init(permission))
      }
    }

    if webView.fullscreenState == .inFullscreen || webView.fullscreenState == .enteringFullscreen {
      webView.closeAllMediaPresentations {
        requestMediaPermissions()
      }
    } else {
      requestMediaPermissions()
    }
  }

  func webView(
    _ webView: WKWebView,
    runJavaScriptAlertPanelWithMessage message: String,
    initiatedByFrame frame: WKFrameInfo,
    completionHandler: @escaping () -> Void
  ) {
    guard let tab, let url = frame.origin?.url, let delegate = tab.webDelegate else {
      completionHandler()
      return
    }
    Task {
      await delegate.tab(tab, runJavaScriptAlertPanelWithMessage: message, pageURL: url)
      completionHandler()
    }
  }

  func webView(
    _ webView: WKWebView,
    runJavaScriptConfirmPanelWithMessage message: String,
    initiatedByFrame frame: WKFrameInfo,
    completionHandler: @escaping (Bool) -> Void
  ) {
    guard let tab, let url = frame.origin?.url, let delegate = tab.webDelegate else {
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
    _ webView: WKWebView,
    runJavaScriptTextInputPanelWithPrompt prompt: String,
    defaultText: String?,
    initiatedByFrame frame: WKFrameInfo,
    completionHandler: @escaping (String?) -> Void
  ) {
    guard let tab, let url = frame.origin?.url, let delegate = tab.webDelegate else {
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

  @MainActor func webView(
    _ webView: WKWebView,
    contextMenuConfigurationFor elementInfo: WKContextMenuElementInfo
  ) async -> UIContextMenuConfiguration? {
    guard let tab, let delegate = tab.webDelegate else { return nil }
    return await delegate.tab(
      tab,
      contextMenuConfigurationForLinkURL: elementInfo.linkURL
    )
  }

  func webView(
    _ webView: WKWebView,
    contextMenuForElement elementInfo: WKContextMenuElementInfo,
    willCommitWithAnimator animator: UIContextMenuInteractionCommitAnimating
  ) {
    guard let url = elementInfo.linkURL else { return }
    webView.load(URLRequest(url: url))
  }
}

extension WebMediaCaptureType {
  fileprivate init?(_ captureType: WKMediaCaptureType) {
    switch captureType {
    case .camera:
      self = .camera
    case .microphone:
      self = .microphone
    case .cameraAndMicrophone:
      self = .cameraAndMicrophone
    @unknown default:
      return nil
    }
  }
}

extension WKPermissionDecision {
  fileprivate init(_ permission: WebPermissionDecision) {
    switch permission {
    case .prompt:
      self = .prompt
    case .grant:
      self = .grant
    case .deny:
      self = .deny
    }
  }
}

extension WKFrameInfo {
  fileprivate var origin: URLOrigin? {
    let origin = URLOrigin(wkSecurityOrigin: securityOrigin)
    return !origin.isOpaque ? origin : nil
  }
}
