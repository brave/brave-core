// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI
import UIKit

/// A tool that will allow you to control the collapsing of toolbars surrounding a scroll view similarly to
/// mobile Safari.
///
/// A combination of ``toolbarState`` and ``interactiveTransitionProgress`` should be used to determine
/// how your UI adapts to scrolling behaviours.
///
/// To setup this, ensure you set ``transitionDistance`` based on the height difference of expanded and
/// collapsed states, _or_ the full height of your expanded view. TBD: When handling bottom bar, this will
/// be set based on the bottom safe area.
///
/// The behaviour of how toolbars collapse expand can be broken down into a few rules based on bar location
/// and user actions:
///
/// **Top Bar**
///
/// 1. If the user drags from the top of the scroll view, the bar will always interactively collapse/expand
///    to match the top edge of the contents.
/// 2. If the current toolbar state is collapsed, when the user ends a drag the toolbar can expand if the
///    direction of their scroll is upwards and meets a distance criteria based on velocity.
/// 3. If the current toolbar state is expanded, when the user scrolls downwards the toolbars will
///    interactively collapse until fully transitioned if there is space to do so. When a transition is
///    complete, the toolbar state is flipped and no further drag will cause interactive expansion.
/// 4. Tapping the status bar will expand the toolbars if collapsed. In this case of mobile Safari, tapping
///    the collapsed URL bar also expands the toolbars but this will be up to the UI to handle by setting
///    ``toolbarState`` itself if some collapsed URL bar UI exists.
///
/// **Bottom Bar**
///
/// 1. If the user drags, toolbars will always interactively collapse/expand until the ``transitionDistance``
///    is met, at which point it will immediately collapse/expand. This is true for both scroll directions and
///    is much simpler than top bar behaviors.
/// 2. In the case of mobile Safari, tapping the collapsed URL bar will expand the toolbars. This will be up
///    to the UI to handle by setting ``toolbarState`` itself if some collapsed URL bar UI exists.
@MainActor class ToolbarVisibilityViewModel: ObservableObject {

  /// Creates a view model with an estimation of the travel distance.
  ///
  /// Update this value when you have a correct value by setting ``transitionDistance``
  init(estimatedTransitionDistance: CGFloat) {
    self.transitionDistance = estimatedTransitionDistance
  }

  /// The display state of toolbars surrounding some observed scroll view
  enum ToolbarState {
    /// Any toolbars are expanded and fully visible. Scrolling to the top is available
    case expanded
    /// Any toolbars are collapsed
    case collapsed

    /// The inverse of the current state
    var inversed: ToolbarState {
      switch self {
      case .expanded: return .collapsed
      case .collapsed: return .expanded
      }
    }

    /// Inverses the current toolbar state
    mutating func inverse() {
      self = inversed
    }
  }

  /// Whether or not actions should be handled
  var isEnabled: Bool = true

  /// The current toolbar state based on previous actions sent
  @Published var toolbarState: ToolbarState = .expanded {
    didSet {
      // Explicitly setting the state means progress should be reset
      interactiveTransitionProgress = nil
    }
  }

  /// The SwiftUI animation to use when manually updating the toolbar state.
  ///
  /// This animation will be used by default when user actions end up inversing the state of the toolbar
  var toolbarChangeAnimation: Animation {
    .spring(response: 0.25, dampingFraction: 0.8, blendDuration: 0)
  }

  /// A UIKit animation to use when manually updating the toolbar state
  var toolbarChangePropertyAnimator: UIViewPropertyAnimator {
    .init(duration: 0.25, dampingRatio: 1)
  }

  /// The travel distance between the ``ToolbarState/expanded`` and ``ToolbarState/collapsed`` toolbar states
  ///
  /// This is how much the user has to pan before the current toolbar state inverses.
  ///
  /// This should be set to the height of the expanded toolbar, minus the height of a collapsed variant if
  /// available. For bottom bar this should be the drag distance before a full state change (to match Safari
  /// this will be the bottom safe area height)
  var transitionDistance: CGFloat

  /// The progress between the current state and its inverse during an interactive transition such as
  /// panning the scroll view
  @Published private(set) var interactiveTransitionProgress: CGFloat?

  /// A minimum content height that can be set to stop the toolbar from collapsing.
  ///
  /// When this is set to `nil`, the current scroll view's content height and ``transitionDistance`` will
  /// determine if the toolbar can collapse.
  var minimumCollapsableContentHeight: CGFloat?

  /// A snapshot in time for a scrolling container.
  ///
  /// Contains some basic info required to handle toolbar visibility
  struct ScrollViewSnapshot {
    var contentOffset: CGPoint
    var contentInset: UIEdgeInsets
    var contentHeight: CGFloat
    var frameHeight: CGFloat
    var isDecelerating: Bool
  }

  /// Vertical axis related information during a pan on a scroll view
  struct PanState {
    var yTranslation: CGFloat
    var yVelocity: CGFloat
  }

  /// Whether or not scrolling to the top of a scroll view is allowed when tapping the status bar
  var isScrollToTopAllowed: Bool {
    toolbarState == .expanded
  }

  /// An user action that may alter the current toolbar state
  enum Action {
    /// The user dragged/scrolled the scroll view
    case dragged(snapshot: ScrollViewSnapshot, panData: PanState)
    /// The user stopped dragging and the scroll view may start decelerating
    case endedDrag(snapshot: ScrollViewSnapshot, panData: PanState)
    /// The content size of the scroll view changed, by zooming for instance
    case contentSizeChanged(snapshot: ScrollViewSnapshot)
    /// The user tapped the status bar to scroll to the top.
    case tappedStatusBar
  }

  func send(action: Action) {
    if ProcessInfo.processInfo.isiOSAppOnVisionOS {
      // Trying to expand a collapsed URL bar on vision OS is difficult, until further changes
      // can be made to the UI itself, we will disable collapsing it on VisionOS
      return
    }
    if !isEnabled { return }
    switch action {
    case .dragged(let snapshot, let panData):
      if initialSnapshot == nil {
        initialSnapshot = snapshot
      }
      dragged(snapshot, pan: panData)
    case .endedDrag(let snapshot, let panData):
      guard let initialSnapshot = initialSnapshot else {
        assertionFailure("You must send a `dragged` action before sending `endedDrag`")
        return
      }
      endedDrag(snapshot, initialSnapshot: initialSnapshot, pan: panData)
      self.initialSnapshot = nil
    case .contentSizeChanged(let snapshot):
      if toolbarState == .collapsed, !isContentHeightSufficientForCollapse(snapshot) {
        withAnimation(toolbarChangeAnimation) {
          toolbarState = .expanded
        }
      }
    case .tappedStatusBar:
      if !isScrollToTopAllowed {
        withAnimation(toolbarChangeAnimation) {
          toolbarState = .expanded
        }
      }
    }
  }

  /// Distance travelled after decelerating to zero velocity at a constant rate
  private func project(initialVelocity: CGFloat, decelerationRate: CGFloat) -> CGFloat {
    (initialVelocity / 1000.0) * decelerationRate / (1.0 - decelerationRate)
  }

  private var initialSnapshot: ScrollViewSnapshot?

  private func dragged(_ snapshot: ScrollViewSnapshot, pan: PanState) {
    if !isContentHeightSufficientForCollapse(snapshot) || transitionDistance.isZero {
      if toolbarState == .collapsed {
        toolbarState = .expanded
      }
      return
    }

    let ty = pan.yTranslation
    let normalizedOffset = snapshot.contentOffset.y + snapshot.contentInset.top
    let isRubberBandingBottomEdge =
      snapshot.contentOffset.y + snapshot.frameHeight > snapshot.contentHeight
    // If we're not starting from 0 and are expanded then we actually want to handle it the same way as from
    // further down the page
    if normalizedOffset < transitionDistance,
      normalizedOffset - ty == 0 || toolbarState == .collapsed
    {
      // content offset of scroll view: 0 -> transitionDistance is always interactive
      var progress = max(0.0, min(1.0, normalizedOffset / transitionDistance))
      if toolbarState == .collapsed {
        progress = 1 - progress
      }
      interactiveTransitionProgress = progress
    } else if toolbarState == .expanded {
      // if expanded: collapsing when scrolling down
      //   - interactively collapses on the way down and up based on y-translation
      //   - once it has fully collapsed though scrolling back up does nothing until touch up
      //   - don't shrink if we're near the bottom and don't have enough space to collapse
      let startOffset = normalizedOffset + ty
      if startOffset + snapshot.frameHeight <= snapshot.contentHeight - transitionDistance,
        !isRubberBandingBottomEdge
      {
        let progress = max(0.0, min(1.0, -ty / transitionDistance))
        interactiveTransitionProgress = progress
      }
    }

    if interactiveTransitionProgress == 1 {
      toolbarState.inverse()
    }
  }

  private func endedDrag(
    _ snapshot: ScrollViewSnapshot,
    initialSnapshot: ScrollViewSnapshot,
    pan: PanState
  ) {
    if !isContentHeightSufficientForCollapse(snapshot) || transitionDistance.isZero {
      if interactiveTransitionProgress != nil || toolbarState == .collapsed {
        // Cancel the transition
        toolbarState = .expanded
      }
      return
    }

    let normalizedOffset = snapshot.contentOffset.y + snapshot.contentInset.top
    let velocity = pan.yVelocity
    let projectedDelta = project(
      initialVelocity: velocity,
      decelerationRate: UIScrollView.DecelerationRate.normal.rawValue
    )
    let projectedOffset = normalizedOffset - projectedDelta
    let isScrollingDown = snapshot.contentOffset.y > initialSnapshot.contentOffset.y
    let isRubberBandingBottomEdge =
      initialSnapshot.contentOffset.y + snapshot.frameHeight >= snapshot.contentHeight
      && isScrollingDown
    var resolvedState = toolbarState
    switch toolbarState {
    case .collapsed:
      let isScrollingIntoSafeArea =
        (snapshot.isDecelerating ? projectedOffset : normalizedOffset) < transitionDistance
      let isScrollingUpWithForce =
        snapshot.isDecelerating && velocity > 0 && projectedDelta > transitionDistance
      if isScrollingIntoSafeArea || isScrollingUpWithForce
        || (isRubberBandingBottomEdge && velocity < -200)
      {
        resolvedState = .expanded
      }
    case .expanded:
      if snapshot.isDecelerating, velocity < 0, abs(projectedOffset) > transitionDistance,
        !isRubberBandingBottomEdge
      {
        resolvedState = .collapsed
      }
    }
    withAnimation(toolbarChangeAnimation) {
      toolbarState = resolvedState
    }
  }

  /// Whether or not the scroll view's content size is large enough to support collapsing toolbars
  private func isContentHeightSufficientForCollapse(_ snapshot: ScrollViewSnapshot) -> Bool {
    if let minimumContentHeightThatAllowsCollapsing = minimumCollapsableContentHeight {
      return snapshot.contentHeight > minimumContentHeightThatAllowsCollapsing
    }
    return snapshot.contentHeight > snapshot.frameHeight + transitionDistance
  }

  // MARK: - UIScrollView setup

  private lazy var coordinator: ScrollViewCoordinator = .init(viewModel: self)
  private var scrollViewObservation: NSKeyValueObservation?

  /// Sets up the `delegate` on the passed in `UIScrollView` to automatically handle scrolling behaviours
  /// from users.
  ///
  /// If you need to control the delegate yourself, consider calling actions yourself instead
  func beginObservingScrollView(_ scrollView: UIScrollView) {
    scrollView.panGestureRecognizer.addTarget(self, action: #selector(pannedScrollView(_:)))
    scrollViewObservation = scrollView.observe(
      \.contentSize,
      options: [.old, .new],
      changeHandler: { [weak self] scrollView, change in
        guard change.oldValue != change.newValue else { return }
        DispatchQueue.main.async { [self] in
          self?.send(action: .contentSizeChanged(snapshot: Self.snapshotData(from: scrollView)))
        }
      }
    )
    scrollView.delegate = coordinator
  }

  func endScrollViewObservation(_ scrollView: UIScrollView) {
    scrollView.panGestureRecognizer.removeTarget(self, action: nil)
    scrollViewObservation?.invalidate()
    scrollViewObservation = nil
    scrollView.delegate = nil
  }
}

extension ToolbarVisibilityViewModel {
  @objc private func pannedScrollView(_ pan: UIPanGestureRecognizer) {
    guard let scrollView = pan.view as? UIScrollView else { return }

    let snapshot: ScrollViewSnapshot = Self.snapshotData(from: scrollView)
    let panData: PanState = .init(
      yTranslation: pan.translation(in: scrollView).y,
      yVelocity: pan.velocity(in: scrollView).y
    )

    send(action: .dragged(snapshot: snapshot, panData: panData))

    if pan.state == .ended {
      send(action: .endedDrag(snapshot: snapshot, panData: panData))
    }
  }

  private class ScrollViewCoordinator: NSObject, UIScrollViewDelegate {
    private weak var viewModel: ToolbarVisibilityViewModel?
    init(viewModel: ToolbarVisibilityViewModel) {
      self.viewModel = viewModel
    }

    func scrollViewShouldScrollToTop(_ scrollView: UIScrollView) -> Bool {
      guard let viewModel = viewModel else {
        return true
      }
      let isScrollToTopAllowed = viewModel.isScrollToTopAllowed
      viewModel.send(action: .tappedStatusBar)
      return isScrollToTopAllowed
    }
  }

  private static func snapshotData(from scrollView: UIScrollView) -> ScrollViewSnapshot {
    .init(
      contentOffset: scrollView.contentOffset,
      contentInset: scrollView.contentInset,
      contentHeight: scrollView.contentSize.height,
      frameHeight: scrollView.frame.height,
      isDecelerating: scrollView.isDecelerating
    )
  }
}
