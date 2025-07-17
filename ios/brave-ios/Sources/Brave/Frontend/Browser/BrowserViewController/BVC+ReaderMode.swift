// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Preferences
import Shared
import Storage
import Web
import WebKit

// MARK: - ReaderModeDelegate

extension BrowserViewController: ReaderModeScriptHandlerDelegate {
  func readerMode(
    _ readerMode: ReaderModeScriptHandler,
    didChangeReaderModeState state: ReaderModeState,
    forTab tab: some TabState
  ) {
    // If this reader mode availability state change is for the tab that we currently show, then update
    // the button. Otherwise do nothing and the button will be updated when the tab is made active.
    if tabManager.selectedTab === tab {
      topToolbar.updateReaderModeState(state)
    }
  }

  func readerMode(
    _ readerMode: ReaderModeScriptHandler,
    didDisplayReaderizedContentForTab tab: some TabState
  ) {
    if tabManager.selectedTab !== tab { return }
    showReaderModeBar(animated: true)
    tab.showContent(true)
  }

  func readerMode(
    _ readerMode: ReaderModeScriptHandler,
    didParseReadabilityResult readabilityResult: ReadabilityResult,
    forTab tab: some TabState
  ) {}
}

// MARK: - ReaderModeStyleViewControllerDelegate

extension BrowserViewController: ReaderModeStyleViewControllerDelegate {
  func readerModeStyleViewController(
    _ readerModeStyleViewController: ReaderModeStyleViewController,
    didConfigureStyle style: ReaderModeStyle
  ) {
    // Persist the new style to the profile
    Preferences.ReaderMode.style.value = style.encode()
    // Change the reader mode style on all tabs that have reader mode active
    for tabIndex in 0..<tabManager.count {
      if let tab = tabManager[tabIndex] {
        if let readerMode = tab.browserData?.getContentScript(
          name: ReaderModeScriptHandler.scriptName
        )
          as? ReaderModeScriptHandler
        {
          if readerMode.state == ReaderModeState.active {
            readerMode.setStyle(style, in: tab)
          }
        }
      }
    }
  }
}

// MARK: - ReaderModeBarViewDelegate

extension BrowserViewController: ReaderModeBarViewDelegate {
  func readerModeSettingsTapped(_ view: UIView) {
    guard
      let readerMode = tabManager.selectedTab?.browserData?.getContentScript(
        name: ReaderModeScriptHandler.scriptName
      ) as? ReaderModeScriptHandler,
      readerMode.state == ReaderModeState.active
    else {
      return
    }

    var readerModeStyle = defaultReaderModeStyle
    if let encodedString = Preferences.ReaderMode.style.value {
      if let style = ReaderModeStyle(encodedString: encodedString) {
        readerModeStyle = style
      }
    }

    let readerModeStyleViewController = ReaderModeStyleViewController(
      selectedStyle: readerModeStyle
    )
    readerModeStyleViewController.delegate = self
    readerModeStyleViewController.modalPresentationStyle = .popover
    readerModeStyleViewController.presentationController?.delegate = self

    if let popoverPresentationController = readerModeStyleViewController
      .popoverPresentationController
    {
      popoverPresentationController.backgroundColor = .white
      popoverPresentationController.sourceView = view
      popoverPresentationController.sourceRect =
        CGRect(x: view.bounds.width / 2, y: UIConstants.toolbarHeight / 2, width: 0, height: 0)
      popoverPresentationController.permittedArrowDirections = .up
    }

    present(readerModeStyleViewController, animated: true, completion: nil)
  }
}

// MARK: - ReaderModeBarUpdate

extension BrowserViewController {

  func showReaderModeBar(animated: Bool) {
    if self.readerModeBar == nil {
      let readerModeBar = ReaderModeBarView(
        privateBrowsingManager: tabManager.privateBrowsingManager
      )
      readerModeBar.delegate = self
      view.insertSubview(readerModeBar, aboveSubview: webViewContainer)
      self.readerModeBar = readerModeBar
    }

    updateViewConstraints()
  }

  func hideReaderModeBar(animated: Bool) {
    if let readerModeBar = self.readerModeBar {
      readerModeBar.removeFromSuperview()
      self.readerModeBar = nil
      self.updateViewConstraints()
    }
  }

  /// There are two ways we can enable reader mode. In the simplest case we open a URL to our internal reader mode
  /// and be done with it. In the more complicated case, reader mode was already open for this page and we simply
  /// navigated away from it. So we look to the left and right in the BackForwardList to see if a readerized version
  /// of the current page is there. And if so, we go there.

  func enableReaderMode() {
    guard let tab = tabManager.selectedTab, let backForwardList = tab.backForwardList else {
      return
    }

    let backList = backForwardList.backList
    let forwardList = backForwardList.forwardList

    guard let currentURL = backForwardList.currentItem?.url,
      let headers = (tab.responses?[currentURL] as? HTTPURLResponse)?.allHeaderFields
        as? [String: String],
      let readerModeURL = currentURL.encodeEmbeddedInternalURL(for: .readermode, headers: headers)
    else { return }

    recordTimeBasedNumberReaderModeUsedP3A(activated: true)

    let playlistItem = tab.playlistItem
    let translationState = tab.translationState ?? .unavailable
    if backList.count > 1 && backList.last?.url == readerModeURL {
      tab.goToBackForwardListItem(backList.last!)
      PlaylistScriptHandler.updatePlaylistTab(tab: tab, item: playlistItem)
      self.updateTranslateURLBar(tab: tab, state: translationState)
    } else if !forwardList.isEmpty && forwardList.first?.url == readerModeURL {
      tab.goToBackForwardListItem(forwardList.first!)
      PlaylistScriptHandler.updatePlaylistTab(tab: tab, item: playlistItem)
      self.updateTranslateURLBar(tab: tab, state: translationState)
    } else {
      // Store the readability result in the cache and load it. This will later move to the ReadabilityHelper.
      tab.evaluateJavaScript(
        functionName: "\(readerModeNamespace).readerize",
        contentWorld: ReaderModeScriptHandler.scriptSandbox
      ) { (object, error) -> Void in
        if let readabilityResult = ReadabilityResult(object: object as AnyObject?) {
          let playlistItem = tab.playlistItem
          let translationState = tab.translationState ?? .unavailable
          Task { @MainActor in
            try? await self.readerModeCache.put(currentURL, readabilityResult)
            if tab.loadRequest(PrivilegedRequest(url: readerModeURL) as URLRequest) != nil {
              PlaylistScriptHandler.updatePlaylistTab(tab: tab, item: playlistItem)
              self.updateTranslateURLBar(tab: tab, state: translationState)
            }
          }
        }
      }
    }
  }

  /// Disabling reader mode can mean two things. In the simplest case we were opened from the reading list, which
  /// means that there is nothing in the BackForwardList except the internal url for the reader mode page. In that
  /// case we simply open a new page with the original url. In the more complicated page, the non-readerized version
  /// of the page is either to the left or right in the BackForwardList. If that is the case, we navigate there.

  func disableReaderMode() {
    if let tab = tabManager.selectedTab, let backForwardList = tab.backForwardList {
      let backList = backForwardList.backList
      let forwardList = backForwardList.forwardList

      if let currentURL = backForwardList.currentItem?.url {
        if let originalURL = currentURL.decodeEmbeddedInternalURL(for: .readermode) {
          let playlistItem = tab.playlistItem
          let translationState = tab.translationState ?? .unavailable
          if backList.count > 1 && backList.last?.url == originalURL {
            tab.goToBackForwardListItem(backList.last!)
            PlaylistScriptHandler.updatePlaylistTab(tab: tab, item: playlistItem)
            self.updateTranslateURLBar(tab: tab, state: translationState)
          } else if !forwardList.isEmpty && forwardList.first?.url == originalURL {
            tab.goToBackForwardListItem(forwardList.first!)
            PlaylistScriptHandler.updatePlaylistTab(tab: tab, item: playlistItem)
            self.updateTranslateURLBar(tab: tab, state: translationState)
          } else {
            if tab.loadRequest(URLRequest(url: originalURL)) != nil {
              PlaylistScriptHandler.updatePlaylistTab(tab: tab, item: playlistItem)
              self.updateTranslateURLBar(tab: tab, state: translationState)
            }
          }
        }
      }
    }
  }

  @objc func dynamicFontChanged(_ notification: Notification) {
    guard notification.name == .dynamicFontChanged else { return }

    var readerModeStyle = defaultReaderModeStyle
    if let encodedString = Preferences.ReaderMode.style.value {
      if let style = ReaderModeStyle(encodedString: encodedString) {
        readerModeStyle = style
      }
    }
    readerModeStyle.fontSize = ReaderModeFontSize.defaultSize
    self.readerModeStyleViewController(
      ReaderModeStyleViewController(selectedStyle: readerModeStyle),
      didConfigureStyle: readerModeStyle
    )
  }
}
