// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import UIKit

extension BrowserViewController: KeyboardHelperDelegate {
  public func keyboardHelper(
    _ keyboardHelper: KeyboardHelper,
    keyboardWillShowWithState state: KeyboardState
  ) {
    keyboardState = state
    // Only collapse the url bar take control of the toolbar if the reason why the keyboard will
    // show is because the active web content presented it.
    let isKeyboardActiveForWebContent =
      tabManager.selectedTab?.webViewProxy?.isKeyboardVisible == true
    let isBraveOriginatedKeyboard = state.isLocal
    if isKeyboardActiveForWebContent, isBraveOriginatedKeyboard, isUsingBottomBar {
      UIView.animate(withDuration: 0.1) { [self] in
        // We can't actually set the toolbar state to collapsed since bar collapsing/expanding is
        // based on many web view traits such as content size and such so we will just use the
        // collapsed bar view directly
        if toolbarVisibilityViewModel.toolbarState == .expanded {
          header.collapsedBarContainerView.alpha = 1
        }
        header.expandedBarStackView.alpha = 0
      }
      collapsedURLBarView.isKeyboardVisible = true
      toolbarVisibilityViewModel.isEnabled = false
    }
    updateTabsBarVisibility()
    updateViewConstraints()

    UIViewPropertyAnimator(duration: state.animationDuration, curve: state.animationCurve) {
      self.alertStackView.layoutIfNeeded()
      if self.isUsingBottomBar {
        self.header.superview?.layoutIfNeeded()
      }
    }
    .startAnimation()

    guard let tab = tabManager.selectedTab else { return }

    self.evaluateWebsiteSupportOpenSearchEngine(in: tab)
  }

  public func keyboardHelper(
    _ keyboardHelper: KeyboardHelper,
    keyboardWillHideWithState state: KeyboardState
  ) {
    keyboardState = nil
    // Always reset things after orientation change that may change bottom bar
    if isUsingBottomBar, !toolbarVisibilityViewModel.isEnabled {
      UIView.animate(withDuration: 0.1) { [self] in
        // We can't actually set the toolbar state to expanded since bar collapsing/expanding is
        // based on many web view traits such as content size and such so we will just use the
        // collapsed bar view directly
        if toolbarVisibilityViewModel.toolbarState == .expanded {
          header.collapsedBarContainerView.alpha = 0
        }
        header.expandedBarStackView.alpha = 1
        toolbarVisibilityViewModel.isEnabled = true
      }
      collapsedURLBarView.isKeyboardVisible = false
    }
    updateTabsBarVisibility()
    updateViewConstraints()

    customSearchBarButtonItemGroup?.barButtonItems.removeAll()
    customSearchBarButtonItemGroup = nil

    if customSearchEngineButton.superview != nil {
      customSearchEngineButton.removeFromSuperview()
    }

    UIViewPropertyAnimator(duration: state.animationDuration, curve: state.animationCurve) {
      self.alertStackView.layoutIfNeeded()
      if self.isUsingBottomBar {
        self.header.superview?.layoutIfNeeded()
      }
    }
    .startAnimation()
  }
}
