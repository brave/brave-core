// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SnapKit
import UIKit

/// The edge of the content area a content accessory is attached to
enum BrowserToolbarContentAccessoryEdge {
  case top
  case bottom
}

/// An object notified when the displayed toolbar layout or its geometry changes
@MainActor
protocol BrowserToolbarsViewControllerDelegate: AnyObject {
  /// Called after the displayed layout changed, including when the resolved mode has no layout
  /// (`toolbarsViewController.isActive` is `false`)
  func browserToolbarsViewControllerDidChangeLayout(
    _ toolbarsViewController: BrowserToolbarsViewController
  )
  /// Called after the active layout's published geometry changed
  func browserToolbarsViewControllerDidUpdateMetrics(
    _ toolbarsViewController: BrowserToolbarsViewController
  )
}

/// Coordinates the toolbar layer displayed over the web view
///
/// The container resolves which `BrowserToolbarMode` should be displayed, owns the controller that
/// displays it and forwards the active controller's geometry through layout guides which stay
/// stable when the active controller is swapped.
///
/// When there is no active controller, either because no layout is registered for the resolved
/// mode or because the mode has not been resolved yet, no toolbars are displayed and the published
/// metrics and guides span the whole view.
///
/// Content accessories, such as the reader mode bar, are views attached to an edge of the content
/// area. They are placed by the container rather than the active controller so that every layout
/// treats them the same way: they move with the toolbars, remain visible while the toolbars are
/// collapsed and their footprint is excluded from the published guides and added to the published
/// metrics.
@MainActor
final class BrowserToolbarsViewController: UIViewController {
  weak var delegate: (any BrowserToolbarsViewControllerDelegate)?

  /// The area between the toolbars where page overlays are placed
  let pageOverlayLayoutGuide = UILayoutGuide()
  /// The area the web content is displayed in on OS versions where the web view is not laid out
  /// underneath the toolbars
  let webContentLayoutGuide = UILayoutGuide()

  private let context: BrowserToolbarViewContext
  private let rules: [BrowserToolbarModeRule]
  private let registry: BrowserToolbarLayoutRegistry

  private(set) var activeMode: BrowserToolbarMode?
  private var activeController: (any BrowserToolbarModeController)?
  private var guideConstraints: [Constraint] = []

  init(
    context: BrowserToolbarViewContext,
    rules: [BrowserToolbarModeRule] = BrowserToolbarModeRule.defaultRules,
    registry: BrowserToolbarLayoutRegistry = .default
  ) {
    self.context = context
    self.rules = rules
    self.registry = registry
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
    remakeGuideConstraints()
  }

  // MARK: - Active Layout

  /// Whether or not a layout is currently displayed
  var isActive: Bool {
    activeController != nil
  }

  /// The metrics of the active layout including the footprint of any content accessories, or
  /// `.init()` if there isn't an active layout
  var metrics: BrowserToolbarLayoutMetrics {
    var metrics = activeController?.metrics ?? .init()
    if contentAccessories.isEmpty {
      return metrics
    }
    view.layoutIfNeeded()
    let topFootprint = contentAccessoryFootprint(edge: .top)
    let bottomFootprint = contentAccessoryFootprint(edge: .bottom)
    metrics.minimumViewportInset.top += topFootprint
    metrics.maximumViewportInset.top += topFootprint
    metrics.minimumViewportInset.bottom += bottomFootprint
    metrics.maximumViewportInset.bottom += bottomFootprint
    return metrics
  }

  /// Resolves the mode for the given inputs and displays its layout if it has changed.
  func updateMode(
    traitCollection: UITraitCollection,
    userInterfaceIdiom: UIUserInterfaceIdiom,
    isUsingBottomBarPreference: Bool
  ) {
    let mode = BrowserToolbarModeRule.resolveMode(
      for: .init(
        traitCollection: traitCollection,
        userInterfaceIdiom: userInterfaceIdiom,
        isUsingBottomBarPreference: isUsingBottomBarPreference
      ),
      rules: rules
    )
    guard mode != activeMode else { return }
    activeMode = mode
    setActiveController(registry.layout(for: mode)?.makeController(context: context))
  }

  private func setActiveController(_ controller: (any BrowserToolbarModeController)?) {
    if let activeController {
      activeController.layoutDelegate = nil
      activeController.willMove(toParent: nil)
      activeController.view.removeFromSuperview()
      activeController.removeFromParent()
    }
    activeController = controller
    if let controller {
      controller.layoutDelegate = self
      addChild(controller)
      // Content accessories are always displayed above the toolbars
      view.insertSubview(controller.view, at: 0)
      controller.view.snp.makeConstraints {
        $0.edges.equalToSuperview()
      }
      controller.didMove(toParent: self)
      controller.observeScrollView(scrollView)
    }
    remakeGuideConstraints()
    delegate?.browserToolbarsViewControllerDidChangeLayout(self)
  }

  // MARK: - Forwarding

  private weak var scrollView: UIScrollView?

  func observeScrollView(_ scrollView: UIScrollView?) {
    self.scrollView = scrollView
    activeController?.observeScrollView(scrollView)
  }

  /// Applies the scroll driven visibility state of the toolbars.
  ///
  /// Final states are animated, while interactive transitions (where `progress` is non-`nil`) are
  /// applied immediately.
  func applyToolbarVisibility(
    state: ToolbarVisibilityViewModel.ToolbarState,
    progress: CGFloat?
  ) {
    guard let activeController else { return }
    guard progress == nil else {
      activeController.applyToolbarVisibility(state: state, progress: progress)
      parent?.view.setNeedsLayout()
      return
    }
    let animator = context.toolbarVisibilityViewModel.toolbarChangePropertyAnimator
    animator.addAnimations { [self] in
      activeController.applyToolbarVisibility(state: state, progress: nil)
      // Layout is driven by the browser's view as its content is constrained to the layout guides
      parent?.view.layoutIfNeeded()
    }
    animator.startAnimation()
  }

  // MARK: - Content Accessories

  private var contentAccessories: [BrowserToolbarContentAccessoryEdge: UIView] = [:]

  /// The spacing between a content accessory and the toolbars & the sides of the view
  private var contentAccessoryInsets: UIEdgeInsets {
    if #available(iOS 26, *) {
      return UIEdgeInsets(equalInset: 8)
    }
    return .zero
  }

  /// Attaches a view to an edge of the content area, replacing any view previously attached to that
  /// edge, or removes it if `accessory` is `nil`.
  ///
  /// The container positions the accessory horizontally and vertically, the accessory is
  /// responsible for defining its own height.
  func setContentAccessory(_ accessory: UIView?, edge: BrowserToolbarContentAccessoryEdge) {
    loadViewIfNeeded()
    let previousAccessory = contentAccessories[edge]
    guard previousAccessory !== accessory else { return }
    previousAccessory?.removeFromSuperview()
    contentAccessories[edge] = accessory
    if let accessory {
      view.addSubview(accessory)
    }
    remakeGuideConstraints()
    delegate?.browserToolbarsViewControllerDidUpdateMetrics(self)
  }

  /// The distance between the edge of the content area and the far side of the content accessory
  /// attached to it, or 0 if there is no accessory attached to the edge
  private func contentAccessoryFootprint(edge: BrowserToolbarContentAccessoryEdge) -> CGFloat {
    guard let accessory = contentAccessories[edge] else { return 0 }
    let contentFrame = activeController?.pageOverlayLayoutGuide.layoutFrame ?? view.bounds
    switch edge {
    case .top:
      return max(0, accessory.frame.maxY - contentFrame.minY)
    case .bottom:
      return max(0, contentFrame.maxY - accessory.frame.minY)
    }
  }

  // MARK: - Layout Guides

  /// Pins the container's guides and content accessories to the active controller's guides so that
  /// anything constrained to them doesn't need to be remade when the controller changes.
  private func remakeGuideConstraints() {
    guideConstraints.forEach { $0.deactivate() }
    guideConstraints = []

    // The edges of the content area before any content accessories are placed
    let overlayTop = activeController?.pageOverlayLayoutGuide.snp.top ?? view.snp.top
    let overlayBottom = activeController?.pageOverlayLayoutGuide.snp.bottom ?? view.snp.bottom
    let webContentTop = activeController?.webContentLayoutGuide.snp.top ?? view.snp.top
    let webContentBottom = activeController?.webContentLayoutGuide.snp.bottom ?? view.snp.bottom

    let insets = contentAccessoryInsets
    let topAccessory = contentAccessories[.top]
    let bottomAccessory = contentAccessories[.bottom]
    topAccessory?.snp.makeConstraints {
      if activeController != nil {
        guideConstraints.append($0.top.equalTo(overlayTop).offset(insets.top).constraint)
      } else {
        guideConstraints.append(
          $0.top.equalTo(view.safeAreaLayoutGuide).offset(insets.top).constraint
        )
      }
      guideConstraints.append($0.leading.trailing.equalToSuperview().inset(insets).constraint)
    }
    bottomAccessory?.snp.makeConstraints {
      if activeController != nil {
        guideConstraints.append($0.bottom.equalTo(overlayBottom).offset(-insets.bottom).constraint)
      } else {
        guideConstraints.append(
          $0.bottom.equalTo(view.safeAreaLayoutGuide).offset(-insets.bottom).constraint
        )
      }
      guideConstraints.append($0.leading.trailing.equalToSuperview().inset(insets).constraint)
    }

    pageOverlayLayoutGuide.snp.makeConstraints {
      guideConstraints.append($0.leading.trailing.equalToSuperview().constraint)
      guideConstraints.append($0.top.equalTo(topAccessory?.snp.bottom ?? overlayTop).constraint)
      guideConstraints.append(
        $0.bottom.equalTo(bottomAccessory?.snp.top ?? overlayBottom).constraint
      )
    }
    webContentLayoutGuide.snp.makeConstraints {
      guideConstraints.append($0.leading.trailing.equalToSuperview().constraint)
      guideConstraints.append($0.top.equalTo(topAccessory?.snp.bottom ?? webContentTop).constraint)
      guideConstraints.append(
        $0.bottom.equalTo(bottomAccessory?.snp.top ?? webContentBottom).constraint
      )
    }
  }
}

extension BrowserToolbarsViewController: BrowserToolbarModeControllerLayoutDelegate {
  func browserToolbarModeControllerDidUpdateLayout(_ controller: any BrowserToolbarModeController) {
    guard controller === activeController else { return }
    delegate?.browserToolbarsViewControllerDidUpdateMetrics(self)
  }
}

/// A view which only receives touches that land on one of its subviews
final class PassthroughView: UIView {
  override func hitTest(_ point: CGPoint, with event: UIEvent?) -> UIView? {
    let view = super.hitTest(point, with: event)
    return view === self ? nil : view
  }
}
