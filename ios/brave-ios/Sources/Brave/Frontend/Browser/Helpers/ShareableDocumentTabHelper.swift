// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Web

extension TabDataValues {
  private struct ShareableDocumentTabHelperKey: TabDataKey {
    static var defaultValue: ShareableDocumentTabHelper?
  }

  var shareableDocumentHelper: ShareableDocumentTabHelper? {
    get { self[ShareableDocumentTabHelperKey.self] }
    set { self[ShareableDocumentTabHelperKey.self] = newValue }
  }
}

/// Tracks the shareable document (such as a PDF) for a tab's committed main frame navigation.
///
/// The document is replaced on each main frame navigation commit and only downloaded on-demand
/// when its file is requested.
@MainActor
final class ShareableDocumentTabHelper: TabObserver, TabPolicyDecider {
  private weak var tab: (any TabState)?

  /// The last main frame response received, used to obtain the `Content-Disposition` for the
  /// document once its navigation commits
  private var pendingMainFrameResponse: URLResponse?

  /// The shareable document for the tab's last committed page, or `nil` if the page is not a
  /// shareable document (e.g. an HTML page)
  private(set) var document: ShareableDocument?

  init(tab: some TabState) {
    self.tab = tab
    tab.addObserver(self)
    tab.addPolicyDecider(self)
    updateDocument(for: tab)
  }

  isolated deinit {
    tab?.removeObserver(self)
    tab?.removePolicyDecider(self)
  }

  private func updateDocument(for tab: some TabState) {
    let response = pendingMainFrameResponse
    pendingMainFrameResponse = nil

    // Each committed navigation may load different contents, even for the same URL
    document?.cancel()
    document = nil

    guard ShareableDocument.isShareable(tab), let url = tab.lastCommittedURL else {
      return
    }
    var contentDisposition: String?
    if let response = response as? HTTPURLResponse, response.url == url {
      contentDisposition = response.value(forHTTPHeaderField: "Content-Disposition")
    }
    document = ShareableDocument(tab: tab, url: url, contentDisposition: contentDisposition)
  }

  // MARK: - TabPolicyDecider

  @MainActor
  func tab(
    _ tab: some TabState,
    shouldAllowResponse response: URLResponse,
    responseInfo: WebResponseInfo
  ) async -> WebPolicyDecision {
    if responseInfo.isForMainFrame {
      pendingMainFrameResponse = response
    }
    return .allow
  }

  // MARK: - TabObserver

  func tabDidCommitNavigation(_ tab: some TabState) {
    updateDocument(for: tab)
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    document?.cancel()
    document = nil
    pendingMainFrameResponse = nil
    tab.removeObserver(self)
    tab.removePolicyDecider(self)
  }
}
