// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Algorithms
import Foundation
import SnapKit
import SwiftUI

/// A view which displays playlist content in up to two columns depending on the orientation &
/// size classes.
///
/// When the user is using an iPhone or an iPad in portrait (or split view), the sidebar
/// will be displayed as a draggable bottom sheet. The sidebar header will be laid out on top of the
/// `sidebar` and be pinned to the top. It will always be draggable by the user to adjust the
/// visiblity of the bottom sheet.
struct PlaylistSplitView<Sidebar: View, SidebarHeader: View, Content: View, Toolbar: View>: View {
  @Environment(\.interfaceOrientation) private var interfaceOrientation
  @Environment(\.horizontalSizeClass) private var horizontalSizeClass
  @Environment(\.isFullScreen) private var isFullScreen

  @Binding var selectedDetent: PlaylistSheetDetent
  var sidebar: Sidebar
  var sidebarHeader: SidebarHeader
  var content: Content
  var toolbar: Toolbar

  init(
    selectedDetent: Binding<PlaylistSheetDetent>,
    @ViewBuilder sidebar: () -> Sidebar,
    @ViewBuilder sidebarHeader: () -> SidebarHeader,
    @ViewBuilder content: () -> Content,
    @ViewBuilder toolbar: () -> Toolbar
  ) {
    self._selectedDetent = selectedDetent
    self.sidebar = sidebar()
    self.sidebarHeader = sidebarHeader()
    self.content = content()
    self.toolbar = toolbar()
  }

  private enum SidebarLayoutMode {
    /// Only the `content` is shown, sidebar content is hidden completely
    case none
    /// Shows `sidebar` as a sidebar on the leading edge of the screen
    case sidebar
    /// Shows `sidebar` inside of a bottom sheet
    case bottomSheet
  }

  private var sidebarLayoutMode: SidebarLayoutMode {
    // By default being in full screen mode means hiding the sidebar entirely
    if isFullScreen {
      return .none
    }
    // Whether or not the sidebar is visible
    //
    // This is not actually an inverse of whether or not the bottom drawer is visible, since
    // on iPhone in landscape, we show neither sidebar or drawer.
    if horizontalSizeClass == .regular && interfaceOrientation.isLandscape {
      return .sidebar
    }
    // Whether or not the sidebar is rendered inside of a bottom sheet
    //
    // On iPhone:
    //    - Potrait only; Landscape orientation will enter fullscreen mode
    // On iPad (Fullscreen):
    //    - Hide when sidebar is visible, based on horizontal size class being regular
    // On iPad (Split View):
    //    - Compact horizontal size class is iPhone layout, even if we're landscape orientation
    let idiom = UIDevice.current.userInterfaceIdiom
    if interfaceOrientation.isPortrait || (idiom == .pad && horizontalSizeClass == .compact) {
      return .bottomSheet
    }
    return .none
  }

  private var sidebarContents: some View {
    PlaylistDrawerScrollView(dragState: $sidebarScrollViewDragState) {
      sidebar
    }
    .safeAreaInset(edge: .top, spacing: 0) {
      VStack(spacing: 0) {
        if sidebarLayoutMode == .bottomSheet {
          Capsule()
            .fill(Color.primary.opacity(0.2))
            .frame(width: 32, height: 4)
            .padding(.vertical, 8)
        }
        sidebarHeader
        Divider()
      }
      .frame(maxWidth: .infinity)
      .background(Color(braveSystemName: .neutral10), ignoresSafeAreaEdges: .bottom)
      .contentShape(.rect)
      // We still need a separate gesture for the header (which is always draggable) since
      // `safeAreaInset(edge:spacing:)` doesn't actually get placed inside of a custom UIScrollView
      // representable.
      //
      // Maybe one day we'll be able to control ScrollView's drag gesture and can remove the
      // UIKit bridge
      .gesture(sidebarLayoutMode == .bottomSheet ? sidebarHeaderDragGesture : nil)
    }
  }

  // MARK: - Bottom Sheet Handling

  private var detentContext: PlaylistSheetDetent.Context {
    .init(maxDetentValue: maxDetentHeight, anchors: detentAnchors)
  }

  private func computeDetentHeights() {
    detentHeights = detents.map({ $0.heightInContext(detentContext) })
      .filter { !$0.isZero && $0.isFinite }
  }

  private func handleBottomSheetDragGestureChanged(translation: CGSize) {
    if detents.isEmpty { return }
    var dragState = activeSheetDragState ?? .init()
    if activeSheetDragState == nil {
      // Drag just started, setup initial height for calculations
      dragState.initialHeight = min(maxDetentHeight, selectedDetent.heightInContext(detentContext))
    }

    let heights = detentHeights
    let proposedHeight = (dragState.initialHeight - translation.height)
    let clampedHeight = max(
      heights.min() ?? 0,
      min(heights.max() ?? maxDetentHeight, proposedHeight)
    )
    let distance = abs(clampedHeight - proposedHeight)
    let sign = clampedHeight > proposedHeight ? -1.0 : 1.0
    // Since we want a more extreme resistant quicker we use 44 (the height of the toolbar) as the
    // dimension value.
    dragState.activeHeight =
      clampedHeight + sign
      * _computedOffsetBasedOnRubberBandingResistance(distance: distance, dimension: 44)
    activeSheetDragState = dragState
  }

  private func handleBottomSheetDragGestureEnded(predictedEndTranslation: CGSize) {
    guard !detents.isEmpty, let dragState = activeSheetDragState else { return }
    // Compute the heights for all active detents and sort them in decending order (since we
    // compare in descending order)
    let heights: [(detent: PlaylistSheetDetent, height: CGFloat)] =
      detents
      .map { ($0, $0.heightInContext(detentContext)) }
      .filter { !$0.1.isZero && $0.1.isFinite }
      .sorted(using: KeyPathComparator(\.height, order: .reverse))
    let predictedEndHeight = dragState.initialHeight - predictedEndTranslation.height
    let restingDetent = {
      // Its actually the mid-points that determine the detent to rest on, for example if we had 2
      // detents, `medium` (50%) and `large` (100%), then if your predicted end location ended up
      // passing the 75% point, we would rest on large.  Similarly, if we had 3 detents, where one
      // was a fractional height of 20%, and medium/large like usual, then we'd have an additional
      // check to rest on medium if we exceed 35%.
      for pair in heights.adjacentPairs() {
        let midpoint = pair.1.height + ((pair.0.height - pair.1.height) / 2.0)
        if predictedEndHeight > midpoint {
          return pair.0.detent
        }
      }
      return heights.last!.detent
    }()
    withAnimation(.snappy) {
      selectedDetent = restingDetent
      activeSheetDragState = nil
    }
  }

  @State private var isDraggingHeaderVertically: Bool?
  private var sidebarHeaderDragGesture: some Gesture {
    DragGesture(coordinateSpace: .global)
      .onChanged { value in
        // SwiftUI gestures don't have the ability to _not_ start a gesture based on some condition
        // like you can do with UIGestureRecognizerDelegate, so we have to only react to changes
        // to a gesture that started valid.
        if isDraggingHeaderVertically == nil {
          isDraggingHeaderVertically = abs(value.velocity.height) > abs(value.velocity.width)
        }
        if isDraggingHeaderVertically == true {
          handleBottomSheetDragGestureChanged(translation: value.translation)
        }
      }
      .onEnded { value in
        if isDraggingHeaderVertically == true {
          handleBottomSheetDragGestureEnded(predictedEndTranslation: value.predictedEndTranslation)
        }
        isDraggingHeaderVertically = nil
      }
  }

  private func _computedOffsetBasedOnRubberBandingResistance(
    distance x: CGFloat,
    constant c: CGFloat = 0.55,
    dimension d: CGFloat
  ) -> CGFloat {
    // f(x, d, c) = (x * d * c) / (d + c * x)
    //
    // where,
    // x – distance from the edge
    // c – constant (UIScrollView uses 0.55)
    // d – dimension, either width or height
    return (x * d * c) / (d + c * x)
  }

  private struct SheetDragState {
    var activeHeight: CGFloat = 0
    var initialHeight: CGFloat = 0
  }

  @State private var activeSheetDragState: SheetDragState?
  @State private var maxDetentHeight: CGFloat = 0
  @State private var sidebarScrollViewDragState: UIKitDragGestureValue = .empty
  @State private var detents: Set<PlaylistSheetDetent> = []
  @State private var detentHeights: [CGFloat] = []
  @State private var detentAnchors: [PlaylistSheetDetent.DetentAnchorID: CGRect] = [:]

  var body: some View {
    HStack(spacing: 0) {
      if sidebarLayoutMode == .sidebar {
        sidebarContents
          .frame(width: 320)
          // FIXME: Would be nice to remove this explicit color and keyboard avoidance from the split view and into sidebar
          .background(Color(braveSystemName: .neutral10))
          // FIXME: Keyboard avoidance still seems oddly busted in landscape iPad
          // - when hardware keyboard is up the bottom safe are seems to shift up even with this
          // - when software keyboard is up the toolbar gets messed up
          .ignoresSafeArea(.keyboard, edges: .bottom)
          .transition(.move(edge: .leading).combined(with: .opacity))
          .zIndex(1)
      }
      content
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background {
          GeometryReader { proxy in
            Color.clear
              .onAppear {
                maxDetentHeight = proxy.size.height
              }
              .onChange(of: proxy.size.height) { newValue in
                maxDetentHeight = newValue
              }
          }
        }
        .coordinateSpace(name: "PlaylistSplitView.Content")
        .simultaneousGesture(sidebarLayoutMode == .bottomSheet ? sidebarHeaderDragGesture : nil)
    }
    .background(Color(braveSystemName: .pageBackground))
    .safeAreaInset(edge: .top, spacing: 0) {
      if !isFullScreen {
        HStack {
          toolbar
        }
        .padding(.horizontal)
        .tint(Color.primary)
        // Mimic an actual navigation bar re: sizing/layout
        .dynamicTypeSize(...DynamicTypeSize.accessibility1)
        .frame(height: 44)
        .contentShape(.rect)
        .zIndex(2)
        // --
      }
    }
    .overlay(alignment: .bottom) {
      if sidebarLayoutMode == .bottomSheet {
        VStack(spacing: 0) {
          sidebarContents
            .onChange(of: sidebarScrollViewDragState) { value in
              switch value.state {
              case .changed:
                handleBottomSheetDragGestureChanged(translation: value.translation)
              case .ended, .cancelled:
                handleBottomSheetDragGestureEnded(
                  predictedEndTranslation: value.predictedEndTranslation
                )
              default:
                break
              }
            }
            .ignoresSafeArea(edges: .bottom)
        }
        .frame(
          height: activeSheetDragState?.activeHeight
            ?? min(maxDetentHeight, selectedDetent.heightInContext(detentContext)),
          alignment: .top
        )
        .background(Color(braveSystemName: .neutral10), ignoresSafeAreaEdges: .bottom)
        .containerShape(
          UnevenRoundedRectangle(
            cornerRadii: .init(
              topLeading: 10,
              bottomLeading: 0,
              bottomTrailing: 0,
              topTrailing: 10
            )
          )
        )
        .ignoresSafeArea(edges: .bottom)
        .transition(.move(edge: .bottom).combined(with: .opacity))
      }
    }
    // Have to apply this to the root unfortunately or the drawer still respects the keyboard
    // safe area.
    .ignoresSafeArea(sidebarLayoutMode == .bottomSheet ? .keyboard : [], edges: .bottom)
    // Detent handlers
    .onPreferenceChange(PlaylistSheetDetentPreferenceKey.self) { value in
      detents = value
      computeDetentHeights()
    }
    .onPreferenceChange(PlaylistSheetDetentAnchorPreferenceKey.self) { value in
      // Keep old values of detent anchors around for seamless adjustments when the underlying
      // `detents` change (like switching views), which will allow us to update the `selectedDetent`
      // even if an old anchor detent is not present in the new set of detents.
      detentAnchors.merge(with: value)
      computeDetentHeights()
    }
    .onChange(of: maxDetentHeight) { _ in
      computeDetentHeights()
    }
    // FIXME: Figure out what to do in AX sizes
    // XXXL may even have issues with DisplayZoom on
    .dynamicTypeSize(.xSmall...DynamicTypeSize.xxxLarge)
  }
}

/// A copy of DragGesture.Value set up from UIPanGestureRecognizer instead
private struct UIKitDragGestureValue: Equatable {
  var state: UIPanGestureRecognizer.State
  var location: CGPoint
  var startLocation: CGPoint
  var velocity: CGPoint
  var translation: CGSize {
    .init(width: location.x - startLocation.x, height: location.y - startLocation.y)
  }
  var predictedEndLocation: CGPoint {
    // In SwiftUI's DragGesture.Value, they compute the velocity with the following:
    //     velocity = 4 * (predictedEndLocation - location)
    // Since we have the velocity, we instead compute the predictedEndLocation with:
    //     predictedEndLocation = (velocity / 4) + location
    return .init(
      x: (velocity.x / 4.0) + location.x,
      y: (velocity.y / 4.0) + location.y
    )
  }
  var predictedEndTranslation: CGSize {
    let endLocation = predictedEndLocation
    return .init(width: endLocation.x - startLocation.x, height: endLocation.y - startLocation.y)
  }
  static var empty: UIKitDragGestureValue {
    .init(state: .possible, location: .zero, startLocation: .zero, velocity: .zero)
  }
}

/// Embeds a SwiftUI view hiearchy inside of a special `UIScrollView` subclass which can control
/// when not to actually scroll based on the current offset and height.
///
/// This is currently used because there is no way in SwiftUI right now to control a ScrollView's
/// built-in pan gesture (aside from introspection to grab the actual underlying UIScrollView, but
/// even at that point you can't set the delegate since it has to be the UIScrollView)
private struct PlaylistDrawerScrollView<Content: View>: UIViewControllerRepresentable {
  @Binding var dragState: UIKitDragGestureValue
  var content: Content

  init(
    dragState: Binding<UIKitDragGestureValue>,
    @ViewBuilder content: () -> Content
  ) {
    self._dragState = dragState
    self.content = content()
  }

  func makeUIViewController(context: Context) -> PlaylistSidebarScrollViewRepresentable<Content> {
    PlaylistSidebarScrollViewRepresentable(rootView: content, dragState: $dragState)
  }

  func updateUIViewController(
    _ uiViewController: PlaylistSidebarScrollViewRepresentable<Content>,
    context: Context
  ) {
    uiViewController.rootView = content
  }

  class PlaylistSidebarScrollViewRepresentable<Root: View>: UIViewController {
    @Binding var dragState: UIKitDragGestureValue
    private let hostingController: UIHostingController<Root>
    private let scrollView = PlaylistSidebarScrollView()

    var rootView: Root {
      get { hostingController.rootView }
      set {
        hostingController.rootView = newValue
        hostingController.view.setNeedsLayout()
        hostingController.view.setNeedsUpdateConstraints()
      }
    }

    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }

    init(rootView: Root, dragState: Binding<UIKitDragGestureValue>) {
      self._dragState = dragState
      hostingController = .init(rootView: rootView)
      super.init(nibName: nil, bundle: nil)
    }

    override func viewDidLoad() {
      super.viewDidLoad()

      view.addSubview(scrollView)
      scrollView.addSubview(hostingController.view)
      addChild(hostingController)
      hostingController.didMove(toParent: self)

      view.backgroundColor = .clear
      scrollView.backgroundColor = .clear
      scrollView.automaticallyAdjustsScrollIndicatorInsets = false
      scrollView.alwaysBounceVertical = true
      hostingController.view.backgroundColor = .clear

      scrollView.snp.makeConstraints {
        $0.edges.equalToSuperview()
      }
      scrollView.contentLayoutGuide.snp.makeConstraints {
        $0.width.equalTo(view)
        $0.top.bottom.equalTo(hostingController.view)
      }
      hostingController.view.snp.makeConstraints {
        $0.leading.trailing.equalTo(view)
      }

      let pan = UIPanGestureRecognizer(target: self, action: #selector(drawerPan(_:)))
      pan.delegate = scrollView
      scrollView.addGestureRecognizer(pan)
      pan.require(toFail: scrollView.panGestureRecognizer)
    }

    private var startLocation: CGPoint = .zero
    @objc private func drawerPan(_ pan: UIPanGestureRecognizer) {
      // We get the velocity & location based on the window coordinate system since the local
      // coordinate system will be shifted based on this gesture
      let location = pan.location(in: scrollView.window)
      if pan.state == .began {
        startLocation = location
        scrollView.showsVerticalScrollIndicator = false
      }
      dragState = .init(
        state: pan.state,
        location: location,
        startLocation: startLocation,
        velocity: pan.velocity(in: scrollView.window)
      )
      if pan.state == .ended || pan.state == .cancelled {
        scrollView.showsVerticalScrollIndicator = true
      }
    }
  }

  class PlaylistSidebarScrollView: UIScrollView, UIGestureRecognizerDelegate {
    override func safeAreaInsetsDidChange() {
      super.safeAreaInsetsDidChange()
      // For some reason the vertical idicator insets are messed up when the
      // `automaticallyAdjustsScrollIndicatorInsets` property is true so this manually updates
      // them to the correct value
      verticalScrollIndicatorInsets = safeAreaInsets
    }

    override func gestureRecognizerShouldBegin(_ gestureRecognizer: UIGestureRecognizer) -> Bool {
      guard let pan = gestureRecognizer as? UIPanGestureRecognizer,
        let scrollView = pan.view as? UIScrollView,
        pan === scrollView.panGestureRecognizer
      else {
        return true
      }
      // This will stop the regular scroll view pan from scrolling when the drawer is fully
      // collapsed (smallest "detent") and when you pan downwards from the top (offset = 0) so that
      // the secondary pan can activate and we can shift the drawer up and down.
      if scrollView.frame.height < 200 {
        return false
      }
      let yVelocity = pan.velocity(in: scrollView).y
      // Allow dragging from the top of the scroll view back down to collapse
      if yVelocity > 0, scrollView.contentOffset.y == scrollView.contentInset.top {
        return false
      }
      // Allow dragging at the bottom of the scroll view to expand further when there aren't
      // enough items to fill out the scroll view
      if yVelocity < 0, scrollView.frame.height > scrollView.contentSize.height,
        scrollView.contentOffset.y + scrollView.frame.height >= scrollView.contentSize.height
      {
        return false
      }
      return true
    }
  }
}

/// A type that represents a height where a PlaylistSplitView's bottom sheet naturally rests
struct PlaylistSheetDetent: Hashable {
  /// Information that you use to calculate the sheets height.
  fileprivate struct Context {
    /// The height that the presentation appears in.
    var maxDetentValue: CGFloat
    /// The bounds of all anchors currently set up in the view hierarchy
    var anchors: [DetentAnchorID: CGRect] = [:]
  }
  /// An ID that will be used in `anchor` detents and the associated View via
  /// `playlistSheetDetentAnchor`
  struct DetentAnchorID: Hashable {
    var id: AnyHashable
  }

  // Would be nice for this to be `any Hashable`, but would have to open existential for Equatable
  // and Hashable conformance
  fileprivate var id: AnyHashable = UUID().uuidString
  fileprivate var heightInContext: (Context) -> CGFloat

  /// A custom detent with the specified fractional height.
  static func fraction(_ fraction: CGFloat) -> PlaylistSheetDetent {
    .init { $0.maxDetentValue * fraction }
  }

  /// A custom detent with the specified height.
  static func height(_ height: CGFloat) -> PlaylistSheetDetent {
    .init { _ in height }
  }

  /// A custom detent that be associated with a View with a matching `playlistSheetDetentAnchor`
  /// modifier attached to it.
  static func anchor(_ id: DetentAnchorID) -> PlaylistSheetDetent {
    .init(
      id: id.id,
      heightInContext: { context in
        guard let anchorFrame = context.anchors[id] else { return 0 }
        return context.maxDetentValue - anchorFrame.height - anchorFrame.minY
      }
    )
  }

  /// The detent for a sheet at full height.
  static let large = PlaylistSheetDetent(id: "large") { context in
    context.maxDetentValue
  }

  /// The detent for a sheet that's approximately half the height of the screen
  static let medium = PlaylistSheetDetent(id: "medium") { context in
    context.maxDetentValue / 2
  }

  /// The detent for a sheet at a very small height.
  static let small = PlaylistSheetDetent(id: "small") { _ in
    120
  }

  static func == (lhs: PlaylistSheetDetent, rhs: PlaylistSheetDetent) -> Bool {
    lhs.id == rhs.id
  }

  func hash(into hasher: inout Hasher) {
    hasher.combine(id)
  }
}

private struct PlaylistSheetDetentPreferenceKey: PreferenceKey {
  static var defaultValue: Set<PlaylistSheetDetent> = [.large]
  static func reduce(
    value: inout Set<PlaylistSheetDetent>,
    nextValue: () -> Set<PlaylistSheetDetent>
  ) {
    value.formUnion(nextValue())
  }
}

private struct PlaylistSheetDetentAnchorPreferenceKey: PreferenceKey {
  static var defaultValue: [PlaylistSheetDetent.DetentAnchorID: CGRect] = [:]
  static func reduce(
    value: inout [PlaylistSheetDetent.DetentAnchorID: CGRect],
    nextValue: () -> [PlaylistSheetDetent.DetentAnchorID: CGRect]
  ) {
    value.merge(with: nextValue())
  }
}

extension View {
  /// Sets the available detents when sidebar is being displayed as a bottom sheet
  func playlistSheetDetents(
    _ detents: Set<PlaylistSheetDetent>
  ) -> some View {
    self.preference(key: PlaylistSheetDetentPreferenceKey.self, value: detents)
  }

  /// Associates this View as an anchor to a `PlaylistSheetDetent.anchor(_:)` detent with the same
  /// provided id.
  ///
  /// Anchor detents will be dynamically rest at the bottom of the view associated with it.
  func playlistSheetDetentAnchor(id: PlaylistSheetDetent.DetentAnchorID) -> some View {
    self.background {
      GeometryReader { proxy in
        Color.clear
          .preference(
            key: PlaylistSheetDetentAnchorPreferenceKey.self,
            value: [id: proxy.frame(in: .named("PlaylistSplitView.Content"))]
          )
      }
    }
  }
}
