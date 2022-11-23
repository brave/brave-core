// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import Brave

@MainActor class TopBarToolbarVisibilityViewModelTests: XCTestCase {

   func testDefaultState() {
    let viewModel = ToolbarVisibilityViewModel(estimatedTransitionDistance: 32)
    XCTAssertEqual(viewModel.toolbarState, .expanded)
    XCTAssertNil(viewModel.interactiveTransitionProgress)
    XCTAssertEqual(viewModel.transitionDistance, 32)
    XCTAssertTrue(viewModel.isEnabled)
  }
  
  func testDisablingActions() {
    let viewModel = ToolbarVisibilityViewModel(estimatedTransitionDistance: 100)
    let initialState = viewModel.toolbarState
    viewModel.isEnabled = false
    var snapshot: ToolbarVisibilityViewModel.ScrollViewSnapshot = .init(
      contentOffset: .init(x: 0, y: 50),
      contentInset: .zero,
      contentHeight: 960,
      frameHeight: 480,
      isDecelerating: false
    )
    viewModel.send(action: .dragged(snapshot: snapshot, panData: .init(yTranslation: -50, yVelocity: 0)))
    XCTAssertNil(viewModel.interactiveTransitionProgress)
    XCTAssertEqual(viewModel.toolbarState, initialState)
    
    snapshot.contentOffset = .init(x: 0, y: 100)
    viewModel.send(action: .dragged(snapshot: snapshot, panData: .init(yTranslation: -100, yVelocity: 0)))
    XCTAssertNil(viewModel.interactiveTransitionProgress)
    XCTAssertEqual(viewModel.toolbarState, initialState)
  }
  
  /// Tests pages that are too small to collapse should not collapse regardless of passing the
  /// transition distance threshold on scroll
  func testSmallContentNoCollapseAllowed() {
    let viewModel = ToolbarVisibilityViewModel(estimatedTransitionDistance: 32)
    let snapshot: ToolbarVisibilityViewModel.ScrollViewSnapshot = .init(
      contentOffset: .init(x: 0, y: 128),
      contentInset: .zero,
      contentHeight: 320, // content height < frame height + transition distance
      frameHeight: 480,
      isDecelerating: false
    )
    viewModel.send(action: .dragged(snapshot: snapshot, panData: .init(yTranslation: -128, yVelocity: 0)))
    XCTAssertEqual(viewModel.toolbarState, .expanded)
    XCTAssertNil(viewModel.interactiveTransitionProgress)
  }
  
  /// Tests that a when a minimum content height requirement is set that scrolling a page with a smaller
  /// content height doesn't begin to collapse at any point
  func testCustomMinimumContentHeightRequirementToCollapse() {
    let viewModel = ToolbarVisibilityViewModel(estimatedTransitionDistance: 100)
    viewModel.minimumCollapsableContentHeight = 480
    var snapshot: ToolbarVisibilityViewModel.ScrollViewSnapshot = .init(
      contentOffset: .init(x: 0, y: 128),
      contentInset: .zero,
      contentHeight: 320, // content height < minimumCollapsableContentHeight
      frameHeight: 480,
      isDecelerating: false
    )
    viewModel.send(action: .dragged(snapshot: snapshot, panData: .init(yTranslation: -50, yVelocity: 0)))
    XCTAssertEqual(viewModel.toolbarState, .expanded)
    XCTAssertNil(viewModel.interactiveTransitionProgress)
    viewModel.send(action: .endedDrag(snapshot: snapshot, panData: .init(yTranslation: -100, yVelocity: -1000)))
    XCTAssertEqual(viewModel.toolbarState, .expanded)
    
    snapshot.contentHeight = 960
    viewModel.send(action: .dragged(snapshot: snapshot, panData: .init(yTranslation: -100, yVelocity: 0)))
    XCTAssertEqual(viewModel.toolbarState, .collapsed)
  }
  
  /// Tests a continuous drag from the top of the scroll view
  func testExpandedToCollapsedFromTop() {
    let viewModel = ToolbarVisibilityViewModel(estimatedTransitionDistance: 100)
    var snapshot: ToolbarVisibilityViewModel.ScrollViewSnapshot = .init(
      contentOffset: .init(x: 0, y: 50),
      contentInset: .zero,
      contentHeight: 960,
      frameHeight: 480,
      isDecelerating: false
    )
    viewModel.send(action: .dragged(snapshot: snapshot, panData: .init(yTranslation: -50, yVelocity: 0)))
    XCTAssertEqual(viewModel.interactiveTransitionProgress, 0.5)
    XCTAssertEqual(viewModel.toolbarState, .expanded)
    
    snapshot.contentOffset = .init(x: 0, y: 75)
    viewModel.send(action: .dragged(snapshot: snapshot, panData: .init(yTranslation: -75, yVelocity: 0)))
    XCTAssertEqual(viewModel.interactiveTransitionProgress, 0.75)
    XCTAssertEqual(viewModel.toolbarState, .expanded)
    
    snapshot.contentOffset = .init(x: 0, y: 100)
    viewModel.send(action: .dragged(snapshot: snapshot, panData: .init(yTranslation: -100, yVelocity: 0)))
    XCTAssertEqual(viewModel.toolbarState, .collapsed)
    XCTAssertNil(viewModel.interactiveTransitionProgress)
  }
  
  func testExpandedToCollapsedFromMidPage() {
    let viewModel = ToolbarVisibilityViewModel(estimatedTransitionDistance: 100)
    var snapshot: ToolbarVisibilityViewModel.ScrollViewSnapshot = .init(
      contentOffset: .init(x: 0, y: 240),
      contentInset: .zero,
      contentHeight: 960,
      frameHeight: 480,
      isDecelerating: false
    )
    viewModel.send(action: .dragged(snapshot: snapshot, panData: .init(yTranslation: -50, yVelocity: 0)))
    XCTAssertEqual(viewModel.interactiveTransitionProgress, 0.5)
    XCTAssertEqual(viewModel.toolbarState, .expanded)
    
    snapshot.contentOffset = .init(x: 0, y: 75)
    viewModel.send(action: .dragged(snapshot: snapshot, panData: .init(yTranslation: -75, yVelocity: 0)))
    XCTAssertEqual(viewModel.interactiveTransitionProgress, 0.75)
    XCTAssertEqual(viewModel.toolbarState, .expanded)
    
    snapshot.contentOffset = .init(x: 0, y: 100)
    viewModel.send(action: .dragged(snapshot: snapshot, panData: .init(yTranslation: -100, yVelocity: 0)))
    XCTAssertEqual(viewModel.toolbarState, .collapsed)
    XCTAssertNil(viewModel.interactiveTransitionProgress)
  }
  
  /// Tests if a user scrolled less than the transition distance provided from the top
  func testExpandedToIncompleteCollapseFromTop() {
    let viewModel = ToolbarVisibilityViewModel(estimatedTransitionDistance: 100)
    let snapshot: ToolbarVisibilityViewModel.ScrollViewSnapshot = .init(
      contentOffset: .init(x: 0, y: 50),
      contentInset: .zero,
      contentHeight: 960,
      frameHeight: 480,
      isDecelerating: false
    )
    // User moved 50 points upwards
    viewModel.send(action: .dragged(snapshot: snapshot, panData: .init(yTranslation: -50, yVelocity: 0)))
    XCTAssertEqual(viewModel.interactiveTransitionProgress, 0.5)
    XCTAssertEqual(viewModel.toolbarState, .expanded)
    // User ended drag at that same point
    viewModel.send(action: .endedDrag(snapshot: snapshot, panData: .init(yTranslation: -50, yVelocity: 0)))
    XCTAssertNil(viewModel.interactiveTransitionProgress)
    XCTAssertEqual(viewModel.toolbarState, .expanded)
  }
  
  /// Tests when a does not complete a collapse (e.g. ``testExpandedToIncompleteCollapseFromTop``) but then
  /// begins a drag again which should be treated the same as the ``testExpandedToCollapsedFromMidPage`` test
  func testExpandedToCollapseWithinTransitionDistanceOffset() {
    let viewModel = ToolbarVisibilityViewModel(estimatedTransitionDistance: 100)
    var snapshot: ToolbarVisibilityViewModel.ScrollViewSnapshot = .init(
      contentOffset: .init(x: 0, y: 50),
      contentInset: .zero,
      contentHeight: 960,
      frameHeight: 480,
      isDecelerating: false
    )
    viewModel.send(action: .dragged(snapshot: snapshot, panData: .init(yTranslation: -50, yVelocity: 0)))
    viewModel.send(action: .endedDrag(snapshot: snapshot, panData: .init(yTranslation: -50, yVelocity: 0)))
    snapshot.contentOffset.y = 150
    viewModel.send(action: .dragged(snapshot: snapshot, panData: .init(yTranslation: -100, yVelocity: 0)))
    viewModel.send(action: .endedDrag(snapshot: snapshot, panData: .init(yTranslation: -100, yVelocity: 0)))
    XCTAssertNil(viewModel.interactiveTransitionProgress)
    XCTAssertEqual(viewModel.toolbarState, .collapsed)
  }
  
  /// Tests interactive expansion when you are dragging into the transition distance threshold
  func testCollapsedToExpandedFromTop() {
    let viewModel = ToolbarVisibilityViewModel(estimatedTransitionDistance: 100)
    viewModel.toolbarState = .collapsed
    let snapshot: ToolbarVisibilityViewModel.ScrollViewSnapshot = .init(
      contentOffset: .init(x: 0, y: 50),
      contentInset: .zero,
      contentHeight: 960,
      frameHeight: 480,
      isDecelerating: false
    )
    viewModel.send(action: .dragged(snapshot: snapshot, panData: .init(yTranslation: 50, yVelocity: 0)))
    XCTAssertEqual(viewModel.interactiveTransitionProgress, 0.5)
    XCTAssertEqual(viewModel.toolbarState, .collapsed) // Has not expanded yet
    viewModel.send(action: .endedDrag(snapshot: snapshot, panData: .init(yTranslation: 50, yVelocity: 100)))
    XCTAssertNil(viewModel.interactiveTransitionProgress)
    XCTAssertEqual(viewModel.toolbarState, .expanded) // Let go inside transition distance threshold = expanded
  }
  
  /// Tests scrolling upwards with toolbars already collapsed but not decelerating when letting go of the drag
  func testCollapsedScrollUpFromMidPageNoDeceleration() {
    let viewModel = ToolbarVisibilityViewModel(estimatedTransitionDistance: 100)
    viewModel.toolbarState = .collapsed
    let snapshot: ToolbarVisibilityViewModel.ScrollViewSnapshot = .init(
      contentOffset: .init(x: 0, y: 240),
      contentInset: .zero,
      contentHeight: 960,
      frameHeight: 480,
      isDecelerating: false
    )
    viewModel.send(action: .dragged(snapshot: snapshot, panData: .init(yTranslation: 50, yVelocity: 0)))
    XCTAssertNil(viewModel.interactiveTransitionProgress)
    XCTAssertEqual(viewModel.toolbarState, .collapsed)
    viewModel.send(action: .endedDrag(snapshot: snapshot, panData: .init(yTranslation: 50, yVelocity: 0)))
    XCTAssertNil(viewModel.interactiveTransitionProgress)
    XCTAssertEqual(viewModel.toolbarState, .collapsed) // No velocity/deceleration so not enough to expand
  }
  
  /// Tests scrolling upwards with toolbars already collapsed but not decelerating when letting go of the drag
  func testCollapsedScrollUpFromMidPageWithDeceleration() {
    let viewModel = ToolbarVisibilityViewModel(estimatedTransitionDistance: 100)
    viewModel.toolbarState = .collapsed
    var snapshot: ToolbarVisibilityViewModel.ScrollViewSnapshot = .init(
      contentOffset: .init(x: 0, y: 480),
      contentInset: .zero,
      contentHeight: 960,
      frameHeight: 480,
      isDecelerating: false
    )
    viewModel.send(action: .dragged(snapshot: snapshot, panData: .init(yTranslation: 50, yVelocity: 0)))
    XCTAssertNil(viewModel.interactiveTransitionProgress)
    XCTAssertEqual(viewModel.toolbarState, .collapsed)
    snapshot.isDecelerating = true
    viewModel.send(action: .endedDrag(snapshot: snapshot, panData: .init(yTranslation: 50, yVelocity: 400)))
    XCTAssertNil(viewModel.interactiveTransitionProgress)
    XCTAssertEqual(viewModel.toolbarState, .expanded) // No velocity/deceleration so not enough to expand
  }
  
  /// Tests rubber banding against the bottom of the scroll view and ending the drag with low velocity
  func testRubberBandingBottomEdgeLowVelocity() {
    let viewModel = ToolbarVisibilityViewModel(estimatedTransitionDistance: 100)
    viewModel.toolbarState = .collapsed
    var snapshot: ToolbarVisibilityViewModel.ScrollViewSnapshot = .init(
      contentOffset: .init(x: 0, y: 480),
      contentInset: .zero,
      contentHeight: 960,
      frameHeight: 480,
      isDecelerating: false
    )
    viewModel.send(action: .dragged(snapshot: snapshot, panData: .init(yTranslation: 0, yVelocity: 0)))
    XCTAssertNil(viewModel.interactiveTransitionProgress)
    XCTAssertEqual(viewModel.toolbarState, .collapsed)
    snapshot.contentOffset.y += 50
    viewModel.send(action: .dragged(snapshot: snapshot, panData: .init(yTranslation: -50, yVelocity: 0)))
    snapshot.isDecelerating = true
    viewModel.send(action: .endedDrag(snapshot: snapshot, panData: .init(yTranslation: -50, yVelocity: -100)))
    XCTAssertNil(viewModel.interactiveTransitionProgress)
    XCTAssertEqual(viewModel.toolbarState, .collapsed) // No velocity/deceleration so not enough to expand
  }
  
  /// Tests rubber banding against the bottom of the scroll view
  func testRubberBandingBottomEdge() {
    let viewModel = ToolbarVisibilityViewModel(estimatedTransitionDistance: 100)
    viewModel.toolbarState = .collapsed
    var snapshot: ToolbarVisibilityViewModel.ScrollViewSnapshot = .init(
      contentOffset: .init(x: 0, y: 480),
      contentInset: .zero,
      contentHeight: 960,
      frameHeight: 480,
      isDecelerating: false
    )
    viewModel.send(action: .dragged(snapshot: snapshot, panData: .init(yTranslation: 0, yVelocity: 0)))
    XCTAssertNil(viewModel.interactiveTransitionProgress)
    XCTAssertEqual(viewModel.toolbarState, .collapsed)
    snapshot.contentOffset.y += 50
    viewModel.send(action: .dragged(snapshot: snapshot, panData: .init(yTranslation: -50, yVelocity: 0)))
    snapshot.isDecelerating = true
    viewModel.send(action: .endedDrag(snapshot: snapshot, panData: .init(yTranslation: -50, yVelocity: -250)))
    XCTAssertNil(viewModel.interactiveTransitionProgress)
    XCTAssertEqual(viewModel.toolbarState, .expanded)
  }
  
  /// Tests that if you drag near the bottom with enough space to collapse that subseqently dragging into
  /// the threshold still continues to collapse
  func testExpandedDragNearBottomEdgeAllowedToCollapse() {
    let viewModel = ToolbarVisibilityViewModel(estimatedTransitionDistance: 100)
    var snapshot: ToolbarVisibilityViewModel.ScrollViewSnapshot = .init(
      contentOffset: .init(x: 0, y: 370), // Minimum required content offset to collapse is 960 - 480 - 100
      contentInset: .zero,
      contentHeight: 960,
      frameHeight: 480,
      isDecelerating: false
    )
    viewModel.send(action: .dragged(snapshot: snapshot, panData: .init(yTranslation: 0, yVelocity: 0)))
    XCTAssertEqual(viewModel.interactiveTransitionProgress, 0.0)
    snapshot.contentOffset.y += 50
    viewModel.send(action: .dragged(snapshot: snapshot, panData: .init(yTranslation: -50, yVelocity: 0)))
    XCTAssertEqual(viewModel.interactiveTransitionProgress, 0.5)
    snapshot.contentOffset.y += 50
    viewModel.send(action: .dragged(snapshot: snapshot, panData: .init(yTranslation: -100, yVelocity: 0)))
    viewModel.send(action: .endedDrag(snapshot: snapshot, panData: .init(yTranslation: -100, yVelocity: 0)))
    XCTAssertNil(viewModel.interactiveTransitionProgress)
    XCTAssertEqual(viewModel.toolbarState, .collapsed)
  }
  
  /// Tests that if you drag near the bottom without enough space for the bars to collapse that it doesn't
  /// interactively collapse at all
  func testExpandedDragNearBottomEdgeNotEnoughSpaceToCollapse() {
    let viewModel = ToolbarVisibilityViewModel(estimatedTransitionDistance: 100)
    var snapshot: ToolbarVisibilityViewModel.ScrollViewSnapshot = .init(
      contentOffset: .init(x: 0, y: 460), // Minimum required content offset to collapse is 960 - 480 - 100
      contentInset: .zero,
      contentHeight: 960,
      frameHeight: 480,
      isDecelerating: false
    )
    viewModel.send(action: .dragged(snapshot: snapshot, panData: .init(yTranslation: 0, yVelocity: 0)))
    XCTAssertNil(viewModel.interactiveTransitionProgress)
    snapshot.contentOffset.y += 50
    viewModel.send(action: .dragged(snapshot: snapshot, panData: .init(yTranslation: -50, yVelocity: 0)))
    XCTAssertNil(viewModel.interactiveTransitionProgress)
  }
  
  /// Tests that tapping the status bar does not scroll to top when collapsed and expands the bar when tapped
  /// instead.
  func testCollapsedScrollToTop() {
    let viewModel = ToolbarVisibilityViewModel(estimatedTransitionDistance: 100)
    viewModel.toolbarState = .collapsed
    XCTAssertFalse(viewModel.isScrollToTopAllowed)
    
    viewModel.send(action: .tappedStatusBar)
    XCTAssertEqual(viewModel.toolbarState, .expanded)
    XCTAssertTrue(viewModel.isScrollToTopAllowed)
  }
  
  /// Tests zooming into a page (which makes its content size bigger and allows for toolbar collapse), then
  /// zooming out back to regular sizes to ensure toolbars are expanded again if they aren't allowed at that
  /// new size.
  func testZoomingSmallPage() {
    let viewModel = ToolbarVisibilityViewModel(estimatedTransitionDistance: 100)
    var snapshot: ToolbarVisibilityViewModel.ScrollViewSnapshot = .init(
      contentOffset: .init(x: 0, y: 150),
      contentInset: .zero,
      contentHeight: 320,
      frameHeight: 480,
      isDecelerating: false
    )
    viewModel.send(action: .dragged(snapshot: snapshot, panData: .init(yTranslation: -50, yVelocity: 0)))
    XCTAssertEqual(viewModel.toolbarState, .expanded)
    // zoomed in 3x then scroll
    snapshot.contentHeight *= 3
    snapshot.contentOffset.y += 150
    viewModel.send(action: .contentSizeChanged(snapshot: snapshot))
    viewModel.send(action: .dragged(snapshot: snapshot, panData: .init(yTranslation: -150, yVelocity: 0)))
    viewModel.send(action: .endedDrag(snapshot: snapshot, panData: .init(yTranslation: -150, yVelocity: 0)))
    // Now possible for it to collapse
    XCTAssertEqual(viewModel.toolbarState, .collapsed)
    snapshot.contentHeight /= 3
    snapshot.contentOffset.y -= 150
    viewModel.send(action: .contentSizeChanged(snapshot: snapshot))
    // No longer valid after zooming back to normal content size to be collapsed. Automatically expanded
    // based on content size change
    XCTAssertEqual(viewModel.toolbarState, .expanded)
  }
  
  /// Tests when a page would become ineligable to collapse mid-scroll due to the overall size of the web view
  /// becoming larger.
  func testCollapseAllowedToNoCollapseAllowed() {
    let viewModel = ToolbarVisibilityViewModel(estimatedTransitionDistance: 100)
    var snapshot: ToolbarVisibilityViewModel.ScrollViewSnapshot = .init(
      contentOffset: .init(x: 0, y: 150),
      contentInset: .zero,
      contentHeight: 960,
      frameHeight: 480,
      isDecelerating: false
    )
    viewModel.send(action: .dragged(snapshot: snapshot, panData: .init(yTranslation: -50, yVelocity: 0)))
    XCTAssertEqual(viewModel.interactiveTransitionProgress, 0.5)
    snapshot.contentHeight = 360 // Now ineligible
    viewModel.send(action: .contentSizeChanged(snapshot: snapshot))
    // The next drag will do nothing as we are no longer ineligable to collapse
    viewModel.send(action: .dragged(snapshot: snapshot, panData: .init(yTranslation: -100, yVelocity: 0)))
    XCTAssertEqual(viewModel.interactiveTransitionProgress, 0.5)
    viewModel.send(action: .endedDrag(snapshot: snapshot, panData: .init(yTranslation: -100, yVelocity: 0)))
    // Upon letting go the bar will revert to expanded
    XCTAssertEqual(viewModel.toolbarState, .expanded)
    XCTAssertNil(viewModel.interactiveTransitionProgress)
  }
  
  /// Tests when a page becomes ineligable to collapse after it fully collapsed (i.e. the page changed size
  /// after collapsing correctly)
  func testNoCollapsedAllowedAfterPreviouslyCollapsed() {
    let viewModel = ToolbarVisibilityViewModel(estimatedTransitionDistance: 100)
    var snapshot: ToolbarVisibilityViewModel.ScrollViewSnapshot = .init(
      contentOffset: .init(x: 0, y: 150),
      contentInset: .zero,
      contentHeight: 960,
      frameHeight: 480,
      isDecelerating: false
    )
    viewModel.send(action: .dragged(snapshot: snapshot, panData: .init(yTranslation: -100, yVelocity: 0)))
    viewModel.send(action: .endedDrag(snapshot: snapshot, panData: .init(yTranslation: -100, yVelocity: 0)))
    XCTAssertNil(viewModel.interactiveTransitionProgress)
    XCTAssertEqual(viewModel.toolbarState, .collapsed)
    snapshot.contentHeight = 360 // Now ineligible
    viewModel.send(action: .dragged(snapshot: snapshot, panData: .init(yTranslation: -50, yVelocity: 0)))
    XCTAssertEqual(viewModel.toolbarState, .expanded)
  }
}
