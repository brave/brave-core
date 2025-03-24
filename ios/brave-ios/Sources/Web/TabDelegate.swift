// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import WebKit

/// A protocol that tells an object about web UI related events happening
///
/// `WKWebView` specific things should not be accessed from these methods, if you need to access
/// the underlying web view, you should only access it via `Tab`
public protocol TabDelegate: AnyObject {
  func tabWebViewDidClose(_ tab: Tab)
  func tab(
    _ tab: Tab,
    contextMenuConfigurationForLinkURL linkURL: URL?
  ) async -> UIContextMenuConfiguration?
  func tab(
    _ tab: Tab,
    requestMediaCapturePermissionsFor type: WebMediaCaptureType
  ) async -> WebPermissionDecision
  func tab(_ tab: Tab, runJavaScriptAlertPanelWithMessage message: String, pageURL: URL) async
  func tab(
    _ tab: Tab,
    runJavaScriptConfirmPanelWithMessage message: String,
    pageURL: URL
  ) async -> Bool
  func tab(
    _ tab: Tab,
    runJavaScriptConfirmPanelWithPrompt prompt: String,
    defaultText: String?,
    pageURL: URL
  ) async -> String?
  func tab(
    _ tab: Tab,
    didRequestHTTPAuthFor protectionSpace: URLProtectionSpace,
    proposedCredential credential: URLCredential?,
    previousFailureCount: Int
  ) async -> URLCredential?
  func tab(
    _ tab: Tab,
    createNewTabWithRequest request: URLRequest
  ) -> Tab?
  func tab(_ tab: Tab, shouldBlockJavaScriptForRequest request: URLRequest) -> Bool
  func tab(_ tab: Tab, shouldBlockUniversalLinksForRequest request: URLRequest) -> Bool
  func tab(_ tab: Tab, buildEditMenuWithBuilder builder: any UIMenuBuilder)
}

/// Media device capture types that a web page may request
public enum WebMediaCaptureType {
  case camera
  case microphone
  case cameraAndMicrophone
}

/// Permission decisions for responding to various permission prompts
public enum WebPermissionDecision {
  case prompt
  case grant
  case deny
}

extension TabDelegate {
  public func tab(
    _ tab: Tab,
    createNewTabWithRequest request: URLRequest
  ) -> Tab? {
    return nil
  }

  public func tabWebViewDidClose(_ tab: Tab) {}

  public func tab(
    _ tab: Tab,
    contextMenuConfigurationForLinkURL linkURL: URL?
  ) async -> UIContextMenuConfiguration? {
    return nil
  }

  public func tab(
    _ tab: Tab,
    requestMediaCapturePermissionsFor type: WebMediaCaptureType
  ) async -> WebPermissionDecision {
    return .prompt
  }

  public func tab(
    _ tab: Tab,
    runJavaScriptAlertPanelWithMessage message: String,
    pageURL: URL
  ) async {}

  public func tab(
    _ tab: Tab,
    runJavaScriptConfirmPanelWithMessage message: String,
    pageURL: URL
  ) async -> Bool {
    return false
  }

  public func tab(
    _ tab: Tab,
    runJavaScriptConfirmPanelWithPrompt prompt: String,
    defaultText: String?,
    pageURL: URL
  ) async -> String? {
    return nil
  }

  public func tab(
    _ tab: Tab,
    didRequestHTTPAuthFor protectionSpace: URLProtectionSpace,
    proposedCredential credential: URLCredential?,
    previousFailureCount: Int
  ) async -> URLCredential? {
    return nil
  }

  public func tab(_ tab: Tab, shouldBlockJavaScriptForRequest request: URLRequest) -> Bool {
    return false
  }

  public func tab(_ tab: Tab, shouldBlockUniversalLinksForRequest request: URLRequest) -> Bool {
    return false
  }
}
