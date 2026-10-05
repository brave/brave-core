// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Preferences
import SnapKit
import UIKit

// MARK: - Toolbar layer

extension BrowserViewController {
  /// Whether or not the toolbar layer is responsible for displaying the toolbars in place of the
  /// legacy `header` & `footer`.
  ///
  /// This is the case whenever the layer exists, even for modes it has no layout for, in which
  /// case no toolbars are displayed at all.
  var isUsingToolbarsLayer: Bool {
    toolbarsViewController != nil
  }

  /// Hides the legacy toolbars when the toolbar layer is in use, and the toolbar layer until the
  /// toolbar UI is ready
  func updateLegacyToolbarsVisibility() {
    let isLegacyHidden = !isToolbarUIReady || isUsingToolbarsLayer
    header.isHidden = isLegacyHidden
    footer.isHidden = isLegacyHidden
    toolbarsViewController?.view.isHidden = !isToolbarUIReady
  }

  /// Resolves the layout the toolbar layer should display for the given traits
  func updateToolbarsLayer(using traitCollection: UITraitCollection) {
    toolbarsViewController?.updateMode(
      traitCollection: traitCollection,
      userInterfaceIdiom: traitCollection.userInterfaceIdiom,
      isUsingBottomBarPreference: Preferences.General.isUsingBottomBar.value
    )
  }

  /// Constrains the content of the browser to the geometry published by the toolbar layer
  func remakeConstraintsForToolbarsLayer() {
    guard let toolbarsViewController else { return }

    if let screenTimeViewController = screenTimeViewController,
      screenTimeViewController.parent != nil
    {
      screenTimeViewController.view.snp.remakeConstraints {
        $0.edges.equalTo(webViewContainer)
      }
    }

    if #unavailable(iOS 26.0) {
      webViewContainer.snp.remakeConstraints { make in
        make.left.right.equalTo(view)
        make.top.bottom.equalTo(toolbarsViewController.webContentLayoutGuide)
      }
    }

    pageOverlayLayoutGuide.snp.remakeConstraints { make in
      make.left.right.equalTo(view)
      make.top.bottom.equalTo(toolbarsViewController.pageOverlayLayoutGuide)
    }

    alertStackView.snp.remakeConstraints { make in
      make.centerX.equalTo(view)
      make.width.equalTo(view.safeArea.width)

      if let keyboardHeight = keyboardState?.intersectionHeightForView(view), keyboardHeight > 0 {
        make.bottom.equalTo(view).offset(-keyboardHeight)
      } else {
        make.bottom.lessThanOrEqualTo(toolbarsViewController.pageOverlayLayoutGuide.snp.bottom)
        make.bottom.lessThanOrEqualTo(view.safeArea.bottom)
      }
    }
  }
}

extension BrowserViewController: BrowserToolbarsViewControllerDelegate {
  func browserToolbarsViewControllerDidChangeLayout(
    _ toolbarsViewController: BrowserToolbarsViewController
  ) {
    updateTabsBarVisibility()
    view.setNeedsUpdateConstraints()
    view.setNeedsLayout()
  }

  func browserToolbarsViewControllerDidUpdateMetrics(
    _ toolbarsViewController: BrowserToolbarsViewController
  ) {
    view.setNeedsLayout()
  }
}
