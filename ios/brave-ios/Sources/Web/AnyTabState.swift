// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import FaviconModels
import Foundation
import UIKit
import WebKit

/// A type erased TabState
public class AnyTabState: TabState {
  private var tab: any TabState

  public init(_ tab: some TabState) {
    self.tab = tab
  }

  @_disfavoredOverload
  public init(_ tab: any TabState) {
    self.tab = tab
  }

  public var id: ID { tab.id }
  public var isPrivate: Bool { tab.isPrivate }
  public var data: TabDataValues {
    get { tab.data }
    set { tab.data = newValue }
  }
  public var view: UIView { tab.view }
  public var opener: (any TabState)? {
    get { tab.opener }
    set { tab.opener = newValue }
  }
  public var isVisible: Bool {
    get { tab.isVisible }
    set { tab.isVisible = newValue }
  }
  public var lastActiveTime: Date? { tab.lastActiveTime }
  public var webViewProxy: (any WebViewProxy)? { tab.webViewProxy }
  public var isWebViewCreated: Bool { tab.isWebViewCreated }
  public func createWebView() { tab.createWebView() }
  public func deleteWebView() { tab.deleteWebView() }
  public var sessionData: Data? { tab.sessionData }
  public func restore(using sessionData: Data) throws { try tab.restore(using: sessionData) }
  public var isRestoring: Bool { tab.isRestoring }

  public var delegate: (any TabDelegate)? {
    get { tab.delegate }
    set { tab.delegate = newValue }
  }

  public var downloadDelegate: (any TabDownloadDelegate)? {
    get { tab.downloadDelegate }
    set { tab.downloadDelegate = newValue }
  }

  public func addPolicyDecider(_ policyDecider: some TabPolicyDecider) {
    tab.addPolicyDecider(policyDecider)
  }

  public func removePolicyDecider(_ policyDecider: some TabPolicyDecider) {
    tab.removePolicyDecider(policyDecider)
  }

  public func addObserver(_ observer: some TabObserver) {
    tab.addObserver(observer)
  }

  public func removeObserver(_ observer: some TabObserver) {
    tab.removeObserver(observer)
  }

  public var visibleSecureContentState: SecureContentState { tab.visibleSecureContentState }
  public var serverTrust: SecTrust? { tab.serverTrust }
  public var favicon: Favicon? {
    get { tab.favicon }
    set { tab.favicon = newValue }
  }

  public var url: URL? { tab.url }
  public var visibleURL: URL? { tab.visibleURL }
  public var lastCommittedURL: URL? { tab.lastCommittedURL }
  public var previousCommittedURL: URL? { tab.previousCommittedURL }
  public var contentsMimeType: String? { tab.contentsMimeType }
  public var title: String? { tab.title }
  public var isLoading: Bool { tab.isLoading }
  public var estimatedProgress: Double { tab.estimatedProgress }
  public var canGoBack: Bool { tab.canGoBack }
  public var canGoForward: Bool { tab.canGoForward }
  public var backForwardList: (any BackForwardListProxy)? { tab.backForwardList }
  public var redirectChain: [URL] { tab.redirectChain }
  public var currentInitialURL: URL? { tab.currentInitialURL }
  public var currentUserAgentType: UserAgentType { tab.currentUserAgentType }

  public func loadRequest(_ request: URLRequest) {
    tab.loadRequest(request)
  }

  public func setVirtualURL(_ url: URL?) {
    tab.setVirtualURL(url)
  }

  public func reload() {
    tab.reload()
  }

  public func reloadWithUserAgentType(_ userAgentType: UserAgentType) {
    tab.reloadWithUserAgentType(userAgentType)
  }

  public func stopLoading() {
    tab.stopLoading()
  }

  public func goBack() {
    tab.goBack()
  }

  public func goForward() {
    tab.goForward()
  }

  public func goToBackForwardListItem(_ item: any BackForwardListItemProxy) {
    tab.goToBackForwardListItem(item)
  }

  public func evaluateJavaScript(
    functionName: String,
    args: [Any],
    frame: WKFrameInfo?,
    contentWorld: WKContentWorld,
    escapeArgs: Bool,
    asFunction: Bool
  ) async throws -> Any? {
    try await tab.evaluateJavaScript(
      functionName: functionName,
      args: args,
      frame: frame,
      contentWorld: contentWorld,
      escapeArgs: escapeArgs,
      asFunction: asFunction
    )
  }

  public func callAsyncJavaScript(
    _ functionBody: String,
    arguments: [String: Any],
    in frame: WKFrameInfo?,
    contentWorld: WKContentWorld
  ) async throws -> Any? {
    try await tab.callAsyncJavaScript(
      functionBody,
      arguments: arguments,
      in: frame,
      contentWorld: contentWorld
    )
  }

  public var canTakeSnapshot: Bool {
    tab.canTakeSnapshot
  }

  public func takeSnapshot(rect: CGRect, handler: @escaping (UIImage?) -> Void) {
    tab.takeSnapshot(rect: rect, handler: handler)
  }

  public func createFullPagePDF() async throws -> Data? {
    try await tab.createFullPagePDF()
  }

  public func presentFindInteraction(with text: String) {
    tab.presentFindInteraction(with: text)
  }

  public func dismissFindInteraction() {
    tab.dismissFindInteraction()
  }

  public func evaluateJavaScriptUnsafe(_ javascript: String) {
    tab.evaluateJavaScriptUnsafe(javascript)
  }

  public func loadHTMLString(_ htmlString: String, baseURL: URL?) {
    tab.loadHTMLString(htmlString, baseURL: baseURL)
  }

  public var configuration: WKWebViewConfiguration {
    tab.configuration
  }

  public var viewPrintFormatter: UIViewPrintFormatter? {
    tab.viewPrintFormatter
  }

  public var dataForDisplayedPDF: Data? {
    tab.dataForDisplayedPDF
  }

  public var sampledPageTopColor: UIColor? {
    tab.sampledPageTopColor
  }

  public var viewScale: CGFloat {
    get { tab.viewScale }
    set { tab.viewScale = newValue }
  }

  public func clearBackForwardList() {
    tab.clearBackForwardList()
  }

  public func updateScripts() {
    tab.updateScripts()
  }
}
