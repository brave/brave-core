// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Preferences
import Shared
import Storage
import Web
import WebKit

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
      tabManager[tabIndex]?.readerMode?.setStyle(style)
    }
  }
}

// MARK: - ReaderModeBarViewDelegate

extension BrowserViewController: ReaderModeBarViewDelegate {
  func readerModeSettingsTapped(_ view: UIView) {
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
