// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

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
}

enum QuickViewButtonState {
  case normal
  case active
  case disabled
}

@Observable
class QuickViewToolbarModel {
  var url: URL
  var secondaryTopButton: QuickViewActionButton?
  var buttonStates: [QuickViewActionButton: QuickViewButtonState] = [
    .shield: .normal,
    .back: .disabled,
    .forward: .disabled,
  ]
  var isLoading: Bool = true
  var loadingProgress: Double = 0.0
  var onActionButton: ((QuickViewActionButton) -> Void)?

  init(
    url: URL,
    secondaryTopButton: QuickViewActionButton? = nil,
    onActionButton: ((QuickViewActionButton) -> Void)? = nil
  ) {
    self.url = url
    self.secondaryTopButton = secondaryTopButton
    self.onActionButton = onActionButton
  }

  func state(for button: QuickViewActionButton) -> QuickViewButtonState {
    buttonStates[button] ?? .normal
  }
  
  func updateShieldingState(_ isEnabled: Bool) {
    buttonStates[.shield] = isEnabled ? .normal : .disabled
  }
}

extension QuickViewToolbarModel: TabObserver {
  func tabDidUpdateURL(_ tab: some TabState) {
    if let url = tab.visibleURL?.displayURL {
      self.url = url
    }
  }

  func tabDidStartLoading(_ tab: some TabState) {
    isLoading = true
  }

  func tabDidStopLoading(_ tab: some TabState) {
    isLoading = false
  }

  func tabDidChangeLoadProgress(_ tab: some TabState) {
    loadingProgress = tab.estimatedProgress
  }

  func tabDidChangeBackForwardState(_ tab: some TabState) {
    if let forwardListItem = tab.backForwardList?.forwardList.first,
      forwardListItem.url.isInternalURL(for: .readermode)
    {
      buttonStates[.forward] = .disabled
    } else {
      buttonStates[.forward] = tab.canGoForward ? .normal : .disabled
    }

    buttonStates[.back] = tab.canGoBack ? .normal : .disabled
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }
}

extension QuickViewToolbarModel: ReaderModeScriptHandlerDelegate {
  func readerMode(
    _ readerMode: ReaderModeScriptHandler,
    didChangeReaderModeState state: ReaderModeState,
    forTab tab: some TabState
  ) {
    updateReaderModeState(state)
  }

  func readerMode(
    _ readerMode: ReaderModeScriptHandler,
    didDisplayReaderizedContentForTab tab: some TabState
  ) {
  }

  func readerMode(
    _ readerMode: ReaderModeScriptHandler,
    didParseReadabilityResult readabilityResult: ReadabilityResult,
    forTab tab: some TabState
  ) {
  }

  func updateReaderModeState(_ state: ReaderModeState) {
    switch state {
    case .available:
      secondaryTopButton = .readerMode
      buttonStates[.readerMode] = .normal
    case .active:
      secondaryTopButton = .readerMode
      buttonStates[.readerMode] = .active
    case .unavailable:
      if secondaryTopButton == .readerMode {
        secondaryTopButton = nil
        buttonStates.removeValue(forKey: .readerMode)
      }
    }
  }
}
