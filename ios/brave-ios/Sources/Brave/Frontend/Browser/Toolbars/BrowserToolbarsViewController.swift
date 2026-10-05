// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SnapKit
import UIKit

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

  /// The metrics of the active layout, or `.init()` if there isn't one
  var metrics: BrowserToolbarLayoutMetrics {
    activeController?.metrics ?? .init()
  }

  /// A bar displayed below the top toolbar. See `BrowserToolbarModeController.readerModeBar`
  var readerModeBar: UIView? {
    didSet {
      activeController?.readerModeBar = readerModeBar
    }
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
      activeController.readerModeBar = nil
      activeController.layoutDelegate = nil
      activeController.willMove(toParent: nil)
      activeController.view.removeFromSuperview()
      activeController.removeFromParent()
    }
    activeController = controller
    if let controller {
      controller.layoutDelegate = self
      addChild(controller)
      view.addSubview(controller.view)
      controller.view.snp.makeConstraints {
        $0.edges.equalToSuperview()
      }
      controller.didMove(toParent: self)
      controller.readerModeBar = readerModeBar
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

  // MARK: - Layout Guides

  /// Pins the container's guides to the active controller's guides so that anything constrained
  /// to them doesn't need to be remade when the controller changes.
  private func remakeGuideConstraints() {
    guideConstraints.forEach { $0.deactivate() }
    guideConstraints = []

    pageOverlayLayoutGuide.snp.makeConstraints {
      guideConstraints.append($0.leading.trailing.equalToSuperview().constraint)
      if let activeController {
        guideConstraints.append($0.top.equalTo(activeController.pageOverlayLayoutGuide).constraint)
        guideConstraints.append(
          $0.bottom.equalTo(activeController.pageOverlayLayoutGuide).constraint
        )
      } else {
        guideConstraints.append($0.top.bottom.equalToSuperview().constraint)
      }
    }
    webContentLayoutGuide.snp.makeConstraints {
      guideConstraints.append($0.leading.trailing.equalToSuperview().constraint)
      if let activeController {
        guideConstraints.append($0.top.equalTo(activeController.webContentLayoutGuide).constraint)
        guideConstraints.append(
          $0.bottom.equalTo(activeController.webContentLayoutGuide).constraint
        )
      } else {
        guideConstraints.append($0.top.bottom.equalToSuperview().constraint)
      }
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
