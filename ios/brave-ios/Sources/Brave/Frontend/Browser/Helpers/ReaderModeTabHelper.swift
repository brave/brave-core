// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Growth
import Shared
@_spi(ChromiumWebViewAccess) import Web

extension TabDataValues {
  private struct ReaderModeTabHelperKey: TabDataKey {
    static var defaultValue: ReaderModeTabHelper?
  }
  var readerMode: ReaderModeTabHelper? {
    get { self[ReaderModeTabHelperKey.self] }
    set { self[ReaderModeTabHelperKey.self] = newValue }
  }
}

class ReaderModeTabHelper {
  private weak var tab: (any TabState)?
  private let readerModeCache: any ReaderModeCache
  private(set) var readabilityResult: ReadabilityResult?

  var onStateChanged: (() -> Void)?
  var onReaderModeDisplayed: (() -> Void)?
  var updateTranslateURLBar:
    ((_ tab: TabState, _ state: TranslateURLBarButton.TranslateState) -> Void)?

  init?(tab: some TabState, readerModeCache: any ReaderModeCache) {
    if !tab.isChromiumTab {
      return nil
    }
    self.tab = tab
    self.readerModeCache = readerModeCache
    tab.addObserver(self)
  }

  deinit {
    tab?.removeObserver(self)
  }

  var state: ReaderModeState {
    if let url = tab?.visibleURL.flatMap(InternalURL.init), url.isReaderModePage {
      return .active
    }
    return readabilityResult != nil ? .available : .unavailable
  }

  /// Checks the web views readability then assigns `readabilityResult`
  @MainActor
  func checkReadability() async {
    guard let tab, let url = tab.lastCommittedURL, url.isWebPage(includeDataURIs: false)
    else {
      readabilityResult = nil
      onStateChanged?()
      return
    }

    if FeatureList.kUseProfileWebViewConfiguration.enabled {
      guard let webView = BraveWebView.from(tab: tab)
      else {
        readabilityResult = nil
        onStateChanged?()
        return
      }
      guard let result = await webView.checkReadability() else {
        // No Reader Mode
        readabilityResult = nil
        onStateChanged?()
        return
      }
      do {
        let jsonObject = try JSONSerialization.jsonObject(with: Data(result.utf8)) as AnyObject
        readabilityResult = ReadabilityResult(object: jsonObject)
        onStateChanged?()
      } catch {
        readabilityResult = nil
        onStateChanged?()
      }
    } else {
      do {
        try await tab.evaluateJavaScript(
          functionName: "\(readerModeNamespace).checkReadability",
          contentWorld: ReaderModeScriptHandler.scriptSandbox
        )
        // onStateChanged?() will gets fired in delegate method
      } catch {
        readabilityResult = nil
        onStateChanged?()
      }
    }
  }

  @MainActor func setStyle(_ style: ReaderModeStyle) {
    guard let tab, state == .active else { return }
    if FeatureList.kUseProfileWebViewConfiguration.enabled,
      let webView = BraveWebView.from(tab: tab)
    {
      webView.setReaderModeTheme(
        style.theme.rawValue,
        fontType: style.fontType.rawValue,
        fontSize: style.fontSize.rawValue
      )
    } else {
      tab.evaluateJavaScript(
        functionName: "\(readerModeNamespace).setStyle",
        args: [style.encode()],
        contentWorld: ReaderModeScriptHandler.scriptSandbox,
        escapeArgs: false
      ) { _, _ in }
    }
  }

  func toggleReaderMode() {
    switch state {
    case .available:
      enableReaderMode()
    case .active:
      disableReaderMode()
    case .unavailable:
      break
    }
  }

  /// There are two ways we can enable reader mode. In the simplest case we open a URL to our internal reader mode
  /// and be done with it. In the more complicated case, reader mode was already open for this page and we simply
  /// navigated away from it. So we look to the left and right in the BackForwardList to see if a readerized version
  /// of the current page is there. And if so, we go there.

  private func enableReaderMode() {
    guard let tab, let backForwardList = tab.backForwardList else {
      return
    }

    let backList = backForwardList.backList
    let forwardList = backForwardList.forwardList

    guard let currentURL = backForwardList.currentItem?.url,
      let headers = (tab.responses?[currentURL] as? HTTPURLResponse)?.allHeaderFields
        as? [String: String],
      let readerModeURL = currentURL.encodeEmbeddedInternalURL(for: .readermode, headers: headers)
    else { return }

    Self.recordTimeBasedNumberReaderModeUsedP3A(activated: true)

    let playlistItem = tab.playlistItem
    let translationState = tab.translationState
    if backList.count > 1 && backList.last?.url == readerModeURL {
      tab.goToBackForwardListItem(backList.last!)
      PlaylistScriptHandler.updatePlaylistTab(tab: tab, item: playlistItem)
      self.updateTranslateURLBar?(tab, translationState)
    } else if !forwardList.isEmpty && forwardList.first?.url == readerModeURL {
      tab.goToBackForwardListItem(forwardList.first!)
      PlaylistScriptHandler.updatePlaylistTab(tab: tab, item: playlistItem)
      self.updateTranslateURLBar?(tab, translationState)
    } else {
      if let readabilityResult {
        Task { @MainActor in
          try? await readerModeCache.put(currentURL, readabilityResult)
          tab.loadRequest(PrivilegedRequest(url: readerModeURL) as URLRequest)
          PlaylistScriptHandler.updatePlaylistTab(tab: tab, item: playlistItem)
          self.updateTranslateURLBar?(tab, translationState)
        }
      }
    }
  }

  /// Disabling reader mode can mean two things. In the simplest case we were opened from the reading list, which
  /// means that there is nothing in the BackForwardList except the internal url for the reader mode page. In that
  /// case we simply open a new page with the original url. In the more complicated page, the non-readerized version
  /// of the page is either to the left or right in the BackForwardList. If that is the case, we navigate there.

  private func disableReaderMode() {
    if let tab, let backForwardList = tab.backForwardList {
      let backList = backForwardList.backList
      let forwardList = backForwardList.forwardList

      if let currentURL = backForwardList.currentItem?.url {
        if let originalURL = currentURL.decodeEmbeddedInternalURL(for: .readermode) {
          let playlistItem = tab.playlistItem
          let translationState = tab.translationState
          if backList.count > 1 && backList.last?.url == originalURL {
            tab.goToBackForwardListItem(backList.last!)
            PlaylistScriptHandler.updatePlaylistTab(tab: tab, item: playlistItem)
            self.updateTranslateURLBar?(tab, translationState)
          } else if !forwardList.isEmpty && forwardList.first?.url == originalURL {
            tab.goToBackForwardListItem(forwardList.first!)
            PlaylistScriptHandler.updatePlaylistTab(tab: tab, item: playlistItem)
            self.updateTranslateURLBar?(tab, translationState)
          } else {
            tab.loadRequest(URLRequest(url: originalURL))
            PlaylistScriptHandler.updatePlaylistTab(tab: tab, item: playlistItem)
            self.updateTranslateURLBar?(tab, translationState)
          }
        }
      }
    }
  }
}

// MARK: - TabObserver

extension ReaderModeTabHelper: TabObserver {
  func tabDidCreateWebView(_ tab: some TabState) {
    if !FeatureList.kUseProfileWebViewConfiguration.enabled {
      // add content script for legacy reader mode only
      guard let browserData = tab.browserData else { return }
      let handler = ReaderModeScriptHandler()
      browserData.addContentScript(
        handler,
        name: ReaderModeScriptHandler.scriptName,
        contentWorld: ReaderModeScriptHandler.scriptSandbox
      )
      handler.delegate = self
    }
  }

  func tabDidStartNavigation(_ tab: some TabState) {
    // Mirror BVC+TabObserver behavior: don't clear state when already on a reader mode page,
    // since the reader mode page itself can trigger sub-navigations after activation.
    if let url = tab.visibleURL, url.isInternalURL(for: .readermode) {
      return
    }
    readabilityResult = nil
  }

  func tabDidFinishNavigation(_ tab: some TabState) {
    Task { @MainActor in
      await checkReadability()
    }
  }

  func tabDidChangeTitle(_ tab: some TabState) {
    Task { @MainActor in
      await checkReadability()
    }
  }

  func tabDidUpdateURL(_ tab: some TabState) {
    Task { @MainActor in
      await checkReadability()
    }
  }

  func tab(_ tab: some TabState, frameDidBecomeAvailable frame: WebFrame) {
    Task { @MainActor in
      await checkReadability()
    }
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }
}

// MARK: - ReaderModeScriptHandlerDelegate

extension ReaderModeTabHelper: ReaderModeScriptHandlerDelegate {
  func readerMode(
    _ readerMode: ReaderModeScriptHandler,
    didChangeReaderModeState state: ReaderModeState,
    forTab tab: some TabState
  ) {
    onStateChanged?()
  }

  func readerMode(
    _ readerMode: ReaderModeScriptHandler,
    didDisplayReaderizedContentForTab tab: some TabState
  ) {
    onReaderModeDisplayed?()
  }

  func readerMode(
    _ readerMode: ReaderModeScriptHandler,
    didParseReadabilityResult readabilityResult: ReadabilityResult,
    forTab tab: some TabState
  ) {
    self.readabilityResult = readabilityResult
  }
}

// MARK: - P3A

extension ReaderModeTabHelper {
  static func recordTimeBasedNumberReaderModeUsedP3A(activated: Bool) {
    var storage = P3ATimedStorage<Int>.readerModeActivated
    if activated {
      storage.add(value: 1, to: Date())
    }

    // Q102- How many times did you use reader mode in the last 7 days?
    UmaHistogramRecordValueToBucket(
      "Brave.ReaderMode.NumberReaderModeActivated",
      buckets: [
        0,
        .r(1...5),
        .r(5...20),
        .r(20...50),
        .r(51...),
      ],
      value: storage.combinedValue
    )
  }
}
