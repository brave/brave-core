// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Combine
import Shared
import SnapKit
import SwiftUI
import UIKit

/// Displays the `SplitToolbarLayout`
///
/// The layout is made up of a top chrome, containing the tabs bar & location bar, with a
/// collapsed URL bar laid over its bottom edge, and a bottom chrome containing the action buttons.
/// The contents of the chrome are SwiftUI, while the chrome itself is positioned using
/// constraints so that the published layout guides always reflect the geometry of the toolbars
/// mid-collapse.
///
/// Collapsing moves the top chrome up by the difference between its expanded height and the
/// collapsed URL bar height, and moves the bottom chrome down by its full height.
final class SplitToolbarController: UIViewController, BrowserToolbarModeController {
  weak var layoutDelegate: (any BrowserToolbarModeControllerLayoutDelegate)?

  let pageOverlayLayoutGuide = UILayoutGuide()
  let webContentLayoutGuide = UILayoutGuide()

  private let context: BrowserToolbarViewContext

  // MARK: Top chrome

  private let topChrome = UIView()
  private let topStackView = UIStackView()
  private let topHairline = UIView()
  private let topHostingController: UIHostingController<SplitTopToolbarView>
  private let collapsedBar = UIControl()
  private let collapsedURLBarView: CollapsedURLBarView

  // MARK: Bottom chrome

  private let bottomChrome = UIView()
  private let bottomHairline = UIView()
  private let bottomHostingController: UIHostingController<SplitBottomToolbarView>

  private var topChromeOffsetConstraint: Constraint?
  private var bottomChromeOffsetConstraint: Constraint?
  private var bottomChromeHeightConstraint: Constraint?

  private var lastPublishedMetrics: BrowserToolbarLayoutMetrics?
  private var cancellables: Set<AnyCancellable> = []

  init(context: BrowserToolbarViewContext) {
    self.context = context
    self.topHostingController = UIHostingController(
      rootView: SplitTopToolbarView(
        toolbarState: context.toolbarState,
        privateBrowsingManager: context.privateBrowsingManager,
        speechRecognizer: context.speechRecognizer
      )
    )
    self.bottomHostingController = UIHostingController(
      rootView: SplitBottomToolbarView(
        toolbarState: context.toolbarState,
        privateBrowsingManager: context.privateBrowsingManager
      )
    )
    self.collapsedURLBarView = CollapsedURLBarView(toolbarState: context.toolbarState)
    super.init(nibName: nil, bundle: nil)
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError()
  }

  override func loadView() {
    view = PassthroughView()
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    view.addLayoutGuide(pageOverlayLayoutGuide)
    view.addLayoutGuide(webContentLayoutGuide)

    view.addSubview(bottomChrome)
    view.addSubview(topChrome)

    setupTopChrome()
    setupBottomChrome()
    setupContentConstraints()

    context.privateBrowsingManager.$isPrivateBrowsing
      .removeDuplicates()
      .receive(on: RunLoop.main)
      .sink { [weak self] _ in self?.updateColors() }
      .store(in: &cancellables)
    updateColors()
  }

  private func setupTopChrome() {
    topChrome.snp.makeConstraints {
      topChromeOffsetConstraint = $0.top.equalTo(view.safeAreaLayoutGuide.snp.top).constraint
      $0.leading.trailing.equalToSuperview()
    }

    topStackView.axis = .vertical
    topChrome.addSubview(topStackView)
    topStackView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    // The hosting view must not pick up a vertical safe area: the top chrome slides up underneath
    // the status bar while collapsing, which would otherwise be added to its height as padding.
    // Only the horizontal safe area is respected, through the wrapper's safe area layout guide.
    topHostingController.sizingOptions = .intrinsicContentSize
    topHostingController.safeAreaRegions = []
    topHostingController.view.backgroundColor = .clear
    addChild(topHostingController)
    let topHostingWrapper = UIView()
    topHostingWrapper.addSubview(topHostingController.view)
    topHostingController.view.snp.makeConstraints {
      $0.top.bottom.equalToSuperview()
      $0.leading.trailing.equalTo(topHostingWrapper.safeAreaLayoutGuide)
    }
    topStackView.addArrangedSubview(topHostingWrapper)
    topHostingController.didMove(toParent: self)

    adoptTabsBar()

    topChrome.addSubview(collapsedBar)
    collapsedBar.alpha = 0
    collapsedBar.addTarget(self, action: #selector(tappedCollapsedBar), for: .touchUpInside)
    collapsedBar.addSubview(collapsedURLBarView)
    collapsedBar.snp.makeConstraints {
      $0.bottom.equalToSuperview()
      $0.leading.trailing.equalTo(topChrome.safeAreaLayoutGuide)
    }
    collapsedURLBarView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    topChrome.addSubview(topHairline)
    topHairline.snp.makeConstraints {
      $0.top.equalTo(topChrome.snp.bottom)
      $0.leading.trailing.equalToSuperview()
      $0.height.equalTo(1.0 / UIScreen.main.scale)
    }
  }

  private func setupBottomChrome() {
    bottomChrome.snp.makeConstraints {
      bottomChromeOffsetConstraint = $0.bottom.equalToSuperview().constraint
      $0.leading.trailing.equalToSuperview()
      bottomChromeHeightConstraint = $0.height.equalTo(UIConstants.toolbarHeight).constraint
    }

    bottomHostingController.sizingOptions = []
    bottomHostingController.safeAreaRegions = []
    bottomHostingController.view.backgroundColor = .clear
    addChild(bottomHostingController)
    bottomChrome.addSubview(bottomHostingController.view)
    bottomHostingController.didMove(toParent: self)
    bottomHostingController.view.snp.makeConstraints {
      $0.top.leading.trailing.equalToSuperview()
      $0.height.equalTo(UIConstants.toolbarHeight)
    }

    bottomChrome.addSubview(bottomHairline)
    bottomHairline.snp.makeConstraints {
      $0.bottom.equalTo(bottomChrome.snp.top)
      $0.leading.trailing.equalToSuperview()
      $0.height.equalTo(1.0 / UIScreen.main.scale)
    }
  }

  /// Takes ownership of the tabs bar from whichever controller currently contains it and displays
  /// it above the location bar
  private func adoptTabsBar() {
    let tabsBar = context.tabsBarViewController
    if tabsBar.parent != nil {
      tabsBar.willMove(toParent: nil)
      tabsBar.view.removeFromSuperview()
      tabsBar.removeFromParent()
    }
    addChild(tabsBar)
    topStackView.insertArrangedSubview(tabsBar.view, at: 0)
    tabsBar.didMove(toParent: self)
  }

  override func willMove(toParent parent: UIViewController?) {
    super.willMove(toParent: parent)
    if parent == nil {
      let tabsBar = context.tabsBarViewController
      if tabsBar.parent === self {
        tabsBar.willMove(toParent: nil)
        tabsBar.view.removeFromSuperview()
        tabsBar.removeFromParent()
        tabsBar.view.alpha = 1
      }
    }
  }

  private func updateColors() {
    let browserColors = context.privateBrowsingManager.browserColors
    topChrome.backgroundColor = browserColors.chromeBackground
    bottomChrome.backgroundColor = browserColors.chromeBackground
    topHairline.backgroundColor = browserColors.dividerSubtle
    bottomHairline.backgroundColor = browserColors.dividerStrong
    collapsedURLBarView.browserColors = browserColors
    topHostingController.view.overrideUserInterfaceStyle =
      context.privateBrowsingManager.isPrivateBrowsing ? .dark : .unspecified
  }

  @objc private func tappedCollapsedBar() {
    context.toolbarVisibilityViewModel.toolbarState = .expanded
  }

  // MARK: - Content constraints

  /// The web content & page overlays are placed between the bottom of the top chrome and the top of
  /// the bottom chrome
  private func setupContentConstraints() {
    pageOverlayLayoutGuide.snp.makeConstraints {
      $0.top.equalTo(topChrome.snp.bottom)
      $0.bottom.equalTo(bottomChrome.snp.top)
      $0.leading.trailing.equalToSuperview()
    }
    webContentLayoutGuide.snp.makeConstraints {
      $0.top.equalTo(topChrome.snp.bottom)
      $0.bottom.equalTo(bottomChrome.snp.top)
      $0.leading.trailing.equalToSuperview()
    }
  }

  // MARK: - Metrics

  var metrics: BrowserToolbarLayoutMetrics {
    // Chrome heights are only resolved once the chrome has been laid out
    view.layoutIfNeeded()
    topChrome.layoutIfNeeded()
    return computeMetrics()
  }

  private func computeMetrics() -> BrowserToolbarLayoutMetrics {
    let safeAreaInsets = view.safeAreaInsets
    let topChromeHeight = topChrome.bounds.height
    let collapsedHeight = collapsedBar.bounds.height
    let bottomChromeHeight = bottomChrome.bounds.height

    var minimumViewportInset = UIEdgeInsets()
    var maximumViewportInset = UIEdgeInsets()
    minimumViewportInset.top = safeAreaInsets.top + collapsedHeight
    maximumViewportInset.top = safeAreaInsets.top + topChromeHeight
    minimumViewportInset.bottom = bottomChromeHeight
    maximumViewportInset.bottom = bottomChromeHeight

    return .init(
      minimumViewportInset: minimumViewportInset,
      maximumViewportInset: maximumViewportInset,
      transitionDistance: topChromeHeight - collapsedHeight,
      // Since the height of the web view changes while collapsing a stable value is needed to
      // determine if the toolbars can collapse
      minimumCollapsableContentHeight: view.bounds.height - safeAreaInsets.top,
      minimumCollapsableTransitionDistance: topChromeHeight + bottomChromeHeight
    )
  }

  override func viewSafeAreaInsetsDidChange() {
    super.viewSafeAreaInsetsDidChange()
    // The height is constant regardless of where the bottom chrome is positioned
    bottomChromeHeightConstraint?.update(
      offset: UIConstants.toolbarHeight + view.safeAreaInsets.bottom
    )
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()

    topChrome.layoutIfNeeded()
    let metrics = computeMetrics()
    if lastPublishedMetrics != metrics {
      lastPublishedMetrics = metrics
      layoutDelegate?.browserToolbarModeControllerDidUpdateLayout(self)
    }
  }

  // MARK: - Visibility

  func applyToolbarVisibility(state: ToolbarVisibilityViewModel.ToolbarState, progress: CGFloat?) {
    let topDistance = topChrome.bounds.height - collapsedBar.bounds.height
    let bottomDistance = bottomChrome.bounds.height

    // Fraction of the way from expanded (0) to collapsed (1)
    let collapsedFraction: CGFloat
    // How visible the expanded content should be
    let expandedAlpha: CGFloat
    switch (state, progress) {
    case (.expanded, let progress?):
      collapsedFraction = min(1, max(0, progress))
      // Have it disappear a bit (1.5x) faster
      expandedAlpha = max(0, min(1, 1 - (progress * 1.5)))
    case (.collapsed, let progress?):
      collapsedFraction = min(1, max(0, 1 - progress))
      expandedAlpha = progress
    case (.expanded, nil):
      collapsedFraction = 0
      expandedAlpha = 1
    case (.collapsed, nil):
      collapsedFraction = 1
      expandedAlpha = 0
    }

    topChromeOffsetConstraint?.update(offset: -topDistance * collapsedFraction)
    bottomChromeOffsetConstraint?.update(offset: bottomDistance * collapsedFraction)

    topHostingController.view.alpha = expandedAlpha
    context.tabsBarViewController.view.alpha = expandedAlpha
    bottomHostingController.view.alpha = expandedAlpha
    bottomHairline.alpha = expandedAlpha
    collapsedBar.alpha = 1 - expandedAlpha
  }

  func observeScrollView(_ scrollView: UIScrollView?) {
    // Collapsing is driven by `ToolbarVisibilityViewModel`
  }
}
