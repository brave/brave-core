// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Shared
import Strings
import SwiftUI
import UIKit
import Web

enum QuickViewActionButton {
  case shield
  case playlist
  case readerMode
  case translate
  case refresh
  case close
  case back
  case forward
  case share
  case openTab
  case sslStatus
}

@Observable
class QuickViewToolbarModel {
  var url: URL {
    didSet { updateDisplayInfo() }
  }
  var displayURL: String = ""
  var isURLLeftToRight: Bool = false
  var secureContentState: SecureContentState = .unknown
  var secondaryTopButton: QuickViewActionButton? {
    if readerModeState != .unavailable { return .readerMode }
    return nil
  }
  var isPrivate: Bool = false
  var canGoBack: Bool = false
  var canGoForward: Bool = false
  var isShieldEnabled: Bool = true
  var readerModeState: ReaderModeState = .unavailable
  var isPlaylistEnabled: Bool = false
  var isTranslateEnabled: Bool = false
  var isLoading: Bool = true
  var loadingProgress: Double = 0.0
  var onActionButton: ((QuickViewActionButton) -> Void)?
  var collapseProgress: CGFloat = 0.0
  private var loadingCompletionTask: Task<Void, Never>?

  init(
    url: URL,
    isPrivate: Bool,
    onActionButton: ((QuickViewActionButton) -> Void)? = nil
  ) {
    self.url = url
    self.isPrivate = isPrivate
    self.onActionButton = onActionButton
    updateDisplayInfo()
  }

  private func updateDisplayInfo() {
    var url = self.url.strippingBlobURLAuth

    if let internalURL = InternalURL(url), internalURL.isBasicAuthURL {
      displayURL = Strings.PageSecurityView.signIntoWebsiteURLBarTitle
      isURLLeftToRight = true
      return
    }

    if URLOrigin(url: url).url == nil && URIFixup.getURL(url.absoluteString) == nil {
      if url.scheme == "about" {
        url = URL(string: "about:blank")!
      }
    }

    var urlString = url.absoluteString
    let isWebPage = url.isWebPage(includeDataURIs: false) || url.scheme == "blob"
    if isWebPage, let origin = url.origin.url?.absoluteString {
      urlString = origin
    }

    displayURL = URLFormatter.formatURL(
      urlString,
      formatTypes: [.omitDefaults, .trimAfterHost, .omitHTTPS, .omitTrivialSubdomains],
      unescapeOptions: .normal
    )

    let isRenderedLeftToRight = url.isRenderedLeftToRight
    let isMixedCharset = !url.isUnidirectional
    var isLTR = isRenderedLeftToRight && !isMixedCharset
    if isMixedCharset { isLTR = true }

    isURLLeftToRight = !isWebPage || !isLTR
  }
}

extension QuickViewToolbarModel: TabObserver {
  func tabDidUpdateURL(_ tab: some TabState) {
    self.url = tab.visibleURL?.displayURL ?? tab.visibleURL ?? url
    secureContentState = tab.visibleSecureContentState
  }

  func tabDidStartLoading(_ tab: some TabState) {
    loadingCompletionTask?.cancel()
    loadingCompletionTask = nil
    isLoading = true
  }

  func tabDidStopLoading(_ tab: some TabState) {
    loadingCompletionTask = Task { @MainActor [weak self] in
      guard let self else { return }
      self.loadingProgress = 1.0
      try? await Task.sleep(for: .milliseconds(300))
      guard !Task.isCancelled else { return }
      self.isLoading = false
    }
  }

  func tabDidChangeLoadProgress(_ tab: some TabState) {
    loadingProgress = tab.estimatedProgress
  }

  func tabDidChangeBackForwardState(_ tab: some TabState) {
    let forwardIsReaderMode =
      tab.backForwardList?.forwardList.first?.url.isInternalURL(for: .readermode) == true
    canGoForward = tab.canGoForward && !forwardIsReaderMode
    canGoBack = tab.canGoBack
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }

  func tabDidChangeVisibleSecurityState(_ tab: some TabState) {
    secureContentState = tab.visibleSecureContentState
  }
}
