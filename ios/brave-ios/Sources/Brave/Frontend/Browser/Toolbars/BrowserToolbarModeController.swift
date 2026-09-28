// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import UIKit

/// A view controller that displays the toolbars for a single `BrowserToolbarLayout`.
///
/// The controller's view is displayed over the entire web view. Touches that land on the
/// controller's view itself rather than one of its subviews are passed through to the web content
/// underneath. Any geometry the browser needs to lay out content around the toolbars is published
/// through the layout guides and `metrics`.
@MainActor
protocol BrowserToolbarModeController: UIViewController {
  /// The object notified when the toolbar geometry changes
  var layoutDelegate: (any BrowserToolbarModeControllerLayoutDelegate)? { get set }

  /// The area between the toolbars where page overlays such as the new tab page, toasts and
  /// find-in-page are placed
  var pageOverlayLayoutGuide: UILayoutGuide { get }

  /// The area the web content is displayed in on OS versions where the web view is not laid out
  /// underneath the toolbars
  var webContentLayoutGuide: UILayoutGuide { get }

  /// The metrics used to adjust the web view's viewport and scroll based toolbar collapsing
  var metrics: BrowserToolbarLayoutMetrics { get }

  /// Applies the scroll driven visibility state of the toolbars
  ///
  /// - Parameters:
  ///   - state: The toolbar state that is transitioning or has transitioned
  ///   - progress: The interactive transition progress, or `nil` if the state should be applied in
  ///     full
  func applyToolbarVisibility(state: ToolbarVisibilityViewModel.ToolbarState, progress: CGFloat?)

  /// Sets the scroll view displayed underneath the toolbars, such as the selected tab's web view
  /// scroll view, or `nil` if no scroll view is displayed
  func observeScrollView(_ scrollView: UIScrollView?)
}

/// An object that is notified when a `BrowserToolbarModeController`'s geometry changes
@MainActor
protocol BrowserToolbarModeControllerLayoutDelegate: AnyObject {
  /// Called after the controller's published geometry changed
  func browserToolbarModeControllerDidUpdateLayout(_ controller: any BrowserToolbarModeController)
}

/// Metrics published by a toolbar layout that decide how the web view's viewport and scroll based
/// toolbar collapsing behave
struct BrowserToolbarLayoutMetrics: Equatable {
  /// The insets of the web view's viewport when the toolbars are collapsed
  var minimumViewportInset: UIEdgeInsets = .zero
  /// The insets of the web view's viewport when the toolbars are expanded
  var maximumViewportInset: UIEdgeInsets = .zero
  /// The distance the toolbars travel when transitioning between expanded and collapsed states
  var transitionDistance: CGFloat = 0
  /// The minimum height of the scrollable content required for the toolbars to collapse
  var minimumCollapsableContentHeight: CGFloat = 0
  /// The minimum distance the scrollable content must exceed its container by for the toolbars to
  /// collapse
  var minimumCollapsableTransitionDistance: CGFloat = 0
}

/// The dependencies available to toolbar layouts when building their controllers
struct BrowserToolbarViewContext {
  var privateBrowsingManager: PrivateBrowsingManager
  var toolbarState: BrowserToolbarState
  var toolbarVisibilityViewModel: ToolbarVisibilityViewModel
}
