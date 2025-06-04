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
  func tabWebViewDidClose(_ tab: some TabState)
  func tab(
    _ tab: some TabState,
    contextMenuConfigurationForLinkURL linkURL: URL?
  ) async -> UIContextMenuConfiguration?
  func tab(
    _ tab: some TabState,
    contextMenuWithLinkURL linkURL: URL?,
    willCommitWithAnimator animator: UIContextMenuInteractionCommitAnimating?
  )
  func tab(
    _ tab: some TabState,
    requestMediaCapturePermissionsFor type: WebMediaCaptureType
  ) async -> WebPermissionDecision
  func tab(
    _ tab: some TabState,
    runJavaScriptAlertPanelWithMessage message: String,
    pageURL: URL
  ) async
  func tab(
    _ tab: some TabState,
    runJavaScriptConfirmPanelWithMessage message: String,
    pageURL: URL
  ) async -> Bool
  func tab(
    _ tab: some TabState,
    runJavaScriptConfirmPanelWithPrompt prompt: String,
    defaultText: String?,
    pageURL: URL
  ) async -> String?
  func tab(
    _ tab: some TabState,
    didRequestHTTPAuthFor protectionSpace: URLProtectionSpace,
    proposedCredential credential: URLCredential?,
    previousFailureCount: Int
  ) async -> URLCredential?
  func tab(
    _ tab: some TabState,
    createNewTabWithRequest request: URLRequest,
    isUserInitiated: Bool
  ) -> (any TabState)?
  func tab(_ tab: some TabState, shouldBlockJavaScriptForRequest request: URLRequest) -> Bool
  func tab(_ tab: some TabState, shouldBlockUniversalLinksForRequest request: URLRequest) -> Bool
  func tab(_ tab: some TabState, buildEditMenuWithBuilder builder: any UIMenuBuilder)
  func tab(_ tab: some TabState, defaultUserAgentTypeForURL url: URL) -> UserAgentType
  func tab(
    _ tab: some TabState,
    userAgentForType type: UserAgentType,
    request: URLRequest
  ) -> String?
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
    _ tab: some TabState,
    createNewTabWithRequest request: URLRequest,
    isUserInitiated: Bool
  ) -> (any TabState)? {
    return nil
  }

  public func tabWebViewDidClose(_ tab: some TabState) {}

  public func tab(
    _ tab: some TabState,
    contextMenuConfigurationForLinkURL linkURL: URL?
  ) async -> UIContextMenuConfiguration? {
    return nil
  }

  func tab(
    _ tab: some TabState,
    contextMenuWithLinkURL linkURL: URL?,
    willCommitWithAnimator animator: UIContextMenuInteractionCommitAnimating?
  ) {}

  public func tab(
    _ tab: some TabState,
    requestMediaCapturePermissionsFor type: WebMediaCaptureType
  ) async -> WebPermissionDecision {
    return .prompt
  }

  public func tab(
    _ tab: some TabState,
    runJavaScriptAlertPanelWithMessage message: String,
    pageURL: URL
  ) async {}

  public func tab(
    _ tab: some TabState,
    runJavaScriptConfirmPanelWithMessage message: String,
    pageURL: URL
  ) async -> Bool {
    return false
  }

  public func tab(
    _ tab: some TabState,
    runJavaScriptConfirmPanelWithPrompt prompt: String,
    defaultText: String?,
    pageURL: URL
  ) async -> String? {
    return nil
  }

  public func tab(
    _ tab: some TabState,
    didRequestHTTPAuthFor protectionSpace: URLProtectionSpace,
    proposedCredential credential: URLCredential?,
    previousFailureCount: Int
  ) async -> URLCredential? {
    return nil
  }

  public func tab(_ tab: some TabState, shouldBlockJavaScriptForRequest request: URLRequest) -> Bool
  {
    return false
  }

  public func tab(
    _ tab: some TabState,
    shouldBlockUniversalLinksForRequest request: URLRequest
  ) -> Bool {
    return false
  }

  func tab(_ tab: some TabState, defaultUserAgentTypeForURL url: URL) -> UserAgentType {
    return .mobile
  }

  func tab(
    _ tab: some TabState,
    userAgentForType type: UserAgentType,
    request: URLRequest
  ) -> String? {
    return nil
  }
}
