// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import Shared

extension BrowserViewController: KeyboardHelperDelegate {
  public func keyboardHelper(_ keyboardHelper: KeyboardHelper, keyboardWillShowWithState state: KeyboardState) {
    keyboardState = state
    if isUsingBottomBar && !topToolbar.inOverlayMode && presentedViewController == nil {
      UIView.animate(withDuration: 0.1) { [self] in
        // We can't actually set the toolbar state to collapsed since bar collapsing/expanding is based on
        // many web view traits such as content size and such so we will just use the collapsed bar view
        // directly
        if toolbarVisibilityViewModel.toolbarState == .expanded {
          header.collapsedBarContainerView.alpha = 1
        }
        header.expandedBarStackView.alpha = 0
        updateTabsBarVisibility()
      }
      collapsedURLBarView.isKeyboardVisible = true
      toolbarVisibilityViewModel.isEnabled = false
    }
    updateViewConstraints()
    
    UIViewPropertyAnimator(duration: state.animationDuration, curve: state.animationCurve) {
      self.alertStackView.layoutIfNeeded()
      if self.isUsingBottomBar {
        self.header.superview?.layoutIfNeeded()
      }
    }
    .startAnimation()
    
    guard let webView = tabManager.selectedTab?.webView else { return }
    
    self.evaluateWebsiteSupportOpenSearchEngine(webView)
  }
  
  public func keyboardHelper(_ keyboardHelper: KeyboardHelper, keyboardWillHideWithState state: KeyboardState) {
    keyboardState = nil
    if isUsingBottomBar && !topToolbar.inOverlayMode &&
        (presentedViewController == nil || collapsedURLBarView.isKeyboardVisible) /* Always reset things if collapsed url bar is visible */ ||
        !toolbarVisibilityViewModel.isEnabled /* Always reset things after orientation change that may change bottom bar */ {
      UIView.animate(withDuration: 0.1) { [self] in
        // We can't actually set the toolbar state to expanded since bar collapsing/expanding is based on
        // many web view traits such as content size and such so we will just use the collapsed bar view
        // directly
        if toolbarVisibilityViewModel.toolbarState == .expanded {
          header.collapsedBarContainerView.alpha = 0
        }
        header.expandedBarStackView.alpha = 1
        updateTabsBarVisibility()
        toolbarVisibilityViewModel.isEnabled = true
      }
      collapsedURLBarView.isKeyboardVisible = false
    }
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
