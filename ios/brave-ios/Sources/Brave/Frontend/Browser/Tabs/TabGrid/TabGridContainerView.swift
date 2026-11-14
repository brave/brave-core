// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Favicon
import Foundation
import SwiftUI
import UIKit
import Web

enum TabCollectionViewSelectSource {
  case tappedCell
  case tappedBackground
  case keyboardShortcut
}

@MainActor
protocol TabCollectionViewDelegate: AnyObject {
  func didSelectTab(_ tab: any TabState, source: TabCollectionViewSelectSource)
  func didRemoveTab(_ tab: any TabState)
  func didMoveTab(_ tabIDs: [TabState.ID], to index: Int)
  func selectedTabListChanged(_ tabs: Set<TabState.ID>)
  func contextMenuForTabs(_ tabs: [any TabState]) -> UIMenu?
  func contextMenuForGroup(_ group: TabGroup) -> UIMenu?
  func didSelectGroup(_ group: TabGroup)
  func didRequestUngroup(_ group: TabGroup)
  func didRequestCloseTabs(in group: TabGroup)
}

/// A view that can display tabs in a grid formation
class TabGridContainerView: UIView {
  let collectionView: UICollectionView
  var isPrivateBrowsing: Bool
  var isEditing: Bool = false {
    didSet {
      collectionView.isEditing = isEditing
      updateDragInteraction()
    }
  }
  /// Whether tabs can be reordered by dragging them. The drop index is resolved against the
  /// tab grid's items, so it is off for a grid that shows a subset of them, like a group's tabs.
  var isReorderingEnabled: Bool = true {
    didSet {
      updateDragInteraction()
    }
  }
  var maskInsets: UIEdgeInsets = .zero {
    didSet {
      updateContentInsets()
    }
  }
  private var items: [TabGridItem] = []

  func updateItems(_ items: [TabGridItem], transaction: Transaction) {
    self.items = items
    applyItemsSnapshot(animated: transaction.animation != nil)
  }

  weak var delegate: TabCollectionViewDelegate?

  private(set) var dataSource: UICollectionViewDiffableDataSource<Int, TabGridItem.ID>!

  // At the moment there seems to be a bug in iOS/UICollectionView which prohibits multi-item drag
  // within from working correctly. It will stop live updating and then not call the drop delegate
  // `performDrop` method despite returning a move operation with the intent to insert at the
  // destination index path.
  private let supportsMultiSelectMoves = false

  init(isPrivateBrowsing: Bool) {
    self.isPrivateBrowsing = isPrivateBrowsing
    self.collectionView = .init(frame: .zero, collectionViewLayout: .tabGridLayout)

    super.init(frame: .zero)

    let tabRegistration: UICollectionView.CellRegistration<TabGridItemCell, any TabState> =
      .init { [unowned self] cell, _, item in
        cell.isPrivateBrowsing = self.isPrivateBrowsing
        cell.actionHandler = { [unowned self, weak item] action in
          guard let item else { return }
          handleAction(action, for: item)
        }
        cell.configure(with: item)
      }
    let groupRegistration:
      UICollectionView.CellRegistration<TabGridGroupCell, (TabGroup, [any TabState])> =
        .init { [unowned self] cell, _, item in
          cell.isPrivateBrowsing = self.isPrivateBrowsing
          cell.actionHandler = { [unowned self] action in
            handleGroupAction(action, for: item.0)
          }
          cell.configure(with: item.0, tabs: item.1)
        }
    self.dataSource = UICollectionViewDiffableDataSource<Int, TabGridItem.ID>(
      collectionView: collectionView,
      cellProvider: { [weak self] collectionView, indexPath, _ in
        guard let self,
          let item = items[safe: indexPath.item]
        else {
          return .init()
        }
        switch item {
        case .tab(let tab):
          return collectionView.dequeueConfiguredReusableCell(
            using: tabRegistration,
            for: indexPath,
            item: tab
          )
        case .group(let group, let tabs):
          return collectionView.dequeueConfiguredReusableCell(
            using: groupRegistration,
            for: indexPath,
            item: (group, tabs)
          )
        }
      }
    )

    prepareCollectionView()
    updateContentInsets()
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  private var scrolledToInitialTab: Bool = false

  override func willMove(toSuperview newSuperview: UIView?) {
    super.willMove(toSuperview: newSuperview)
    if newSuperview == nil {
      scrolledToInitialTab = false
    }
  }

  override func didMoveToSuperview() {
    super.didMoveToSuperview()
    if superview != nil {
      becomeFirstResponder()
    }
  }

  override func layoutSubviews() {
    super.layoutSubviews()

    if !scrolledToInitialTab {
      scrolledToInitialTab = true
      scrollToSelectedItem(animated: false)
    }

    if #available(iOS 26.0, *) {
      updateScrollEdgeViews()
    }
  }

  override var canBecomeFirstResponder: Bool {
    return true
  }

  override var keyCommands: [UIKeyCommand]? {
    let arrowKeys: [String] = [
      UIKeyCommand.inputLeftArrow,
      UIKeyCommand.inputRightArrow,
      UIKeyCommand.inputDownArrow,
      UIKeyCommand.inputUpArrow,
    ]
    return arrowKeys.map {
      let cmd = UIKeyCommand(input: $0, modifierFlags: [], action: #selector(handleArrowKeyCommand))
      cmd.wantsPriorityOverSystemBehavior = true
      return cmd
    }
  }

  // MARK: -

  /// The index path of the cell standing for the tab: its own, or its group's
  func indexPath(for tab: some TabState) -> IndexPath? {
    guard let item = items.first(where: { $0.tabs.contains(where: { $0 === tab }) }) else {
      return nil
    }
    return dataSource.indexPath(for: item.id)
  }

  /// The frame of the group's item in window coordinates, if it's in the grid
  func windowFrame(of group: TabGroup) -> CGRect? {
    guard let indexPath = dataSource.indexPath(for: .group(group.id)),
      let attributes = collectionView.layoutAttributesForItem(at: indexPath)
    else { return nil }
    return collectionView.convert(attributes.frame, to: nil)
  }

  fileprivate func scrollToSelectedItem(animated: Bool) {
    guard let item = items.first(where: \.containsVisibleTab),
      let indexPath = dataSource.indexPath(for: item.id)
    else { return }
    collectionView.scrollToItem(
      at: indexPath,
      at: .bottom,
      animated: animated
    )
  }

  // iOS 26+ views needed to support the `UIScrollEdgeElementContainerInteraction`
  private var topEdgeView: UIView?
  private var bottomEdgeView: UIView?

  private func updateDragInteraction() {
    collectionView.dragInteractionEnabled =
      isReorderingEnabled && (!isEditing || supportsMultiSelectMoves)
  }

  private func prepareCollectionView() {
    collectionView.alwaysBounceVertical = true
    collectionView.backgroundColor = .clear
    collectionView.dataSource = dataSource
    collectionView.delegate = self
    updateDragInteraction()
    collectionView.dragDelegate = self
    collectionView.dropDelegate = self
    collectionView.allowsSelectionDuringEditing = true
    collectionView.allowsMultipleSelectionDuringEditing = true
    collectionView.backgroundView = {
      let view = UIView()
      view.backgroundColor = .clear
      view.addGestureRecognizer(
        UITapGestureRecognizer(target: self, action: #selector(tappedCollectionViewBackground))
      )
      return view
    }()
    addSubview(collectionView)
    collectionView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    if #available(iOS 26.0, *) {
      // In order to support scroll edge effects in this view we must create scroll edge container
      // interactions, however this interaction does not work unless the view its added to contains
      // a glass view hierarchy in it (UIGlassEffect/UIGlassContainerEffect). We dont render the
      // header/footer bars in this representable, so instead we need to create temporary views
      // that will be added to the container view and laid out using the insets we already pass in.
      let topEdgeView = UIVisualEffectView(effect: UIGlassEffect())
      let topEdgeContainerInteraction = UIScrollEdgeElementContainerInteraction()
      topEdgeContainerInteraction.scrollView = collectionView
      topEdgeContainerInteraction.edge = .top
      topEdgeView.addInteraction(topEdgeContainerInteraction)
      addSubview(topEdgeView)
      self.topEdgeView = topEdgeView

      let bottomEdgeView = UIVisualEffectView(effect: UIGlassEffect())
      let bottomEdgeContainerInteraction = UIScrollEdgeElementContainerInteraction()
      bottomEdgeContainerInteraction.scrollView = collectionView
      bottomEdgeContainerInteraction.edge = .bottom
      bottomEdgeView.addInteraction(bottomEdgeContainerInteraction)
      addSubview(bottomEdgeView)
      self.bottomEdgeView = bottomEdgeView

      collectionView.topEdgeEffect.style = .soft
      collectionView.bottomEdgeEffect.style = .soft
    }
  }

  @available(iOS 26.0, *)
  private func updateScrollEdgeViews() {
    // width is set to `CGFloat.leastNormalMagnitude` for both of these so that they dont appear
    // in the actual view. The views frames MUST intersect with the scroll view and they cannot
    // be hidden/have an alpha of 0 or `UIScrollEdgeElementContainerInteraction` does not work
    topEdgeView?.frame = .init(
      x: 0,
      y: 0,
      width: CGFloat.leastNormalMagnitude,
      height: maskInsets.top + safeAreaInsets.top
    )
    bottomEdgeView?.frame = .init(
      x: 0,
      y: bounds.height - maskInsets.bottom - safeAreaInsets.bottom,
      width: CGFloat.leastNormalMagnitude,
      height: maskInsets.bottom + safeAreaInsets.bottom
    )
  }

  private func updateContentInsets() {
    let oldValue = collectionView.contentInset
    let isScrolledToTop =
      collectionView.contentOffset.y == -collectionView.contentInset.top
    let animations = { [self] in
      collectionView.contentInset = maskInsets
      collectionView.scrollIndicatorInsets = maskInsets
      // Only interested in animating between explicit mask insets
      if isScrolledToTop, oldValue != .zero {
        collectionView.contentOffset.y = -maskInsets.top
      }
    }

    if #available(iOS 26.0, *) {
      updateScrollEdgeViews()
    }

    // iOS 18+'s SwiftUI animation sharing feature is broken and causes weird jumps so we can't use
    // Animation.toolbarsSizeAnimation directly
    let animator = UIViewPropertyAnimator(duration: 0.3, dampingRatio: 0.825)
    animator.addAnimations(animations)
    animator.startAnimation()
  }

  private func applyItemsSnapshot(animated: Bool) {
    var snapshot: NSDiffableDataSourceSnapshot<Int, TabGridItem.ID> = .init()
    snapshot.appendSections([0])
    snapshot.appendItems(items.map(\.id))
    // A group keeps its identity as its title and tabs change, so its cell must be refreshed
    // explicitly. Tab cells observe their tab instead.
    snapshot.reconfigureItems(items.filter(\.isGroup).map(\.id))
    dataSource.apply(snapshot, animatingDifferences: animated)
  }

  private func handleAction(_ action: TabGridItemView.Action, for tab: some TabState) {
    switch action {
    case .closedTab:
      delegate?.didRemoveTab(tab)
    }
  }

  private func handleGroupAction(_ action: TabGridGroupItemView.Action, for group: TabGroup) {
    switch action {
    case .ungroup:
      delegate?.didRequestUngroup(group)
    case .closeAllTabs:
      delegate?.didRequestCloseTabs(in: group)
    }
  }

  @objc private func handleArrowKeyCommand(_ sender: UIKeyCommand) {
    guard let currentSelectedItemIndex = items.firstIndex(where: \.containsVisibleTab) else {
      return
    }
    var step: Int = 0
    switch sender.input {
    case UIKeyCommand.inputLeftArrow, UIKeyCommand.inputUpArrow:
      step -= 1
    case UIKeyCommand.inputRightArrow, UIKeyCommand.inputDownArrow:
      step += 1
    default:
      break
    }
    if let newSelectedTab = items[safe: currentSelectedItemIndex + step]?.representativeTab {
      delegate?.didSelectTab(newSelectedTab, source: .keyboardShortcut)
    }
  }

  @objc private func tappedCollectionViewBackground() {
    guard let selectedTab = items.lazy.flatMap(\.tabs).first(where: \.isVisible),
      traitCollection.horizontalSizeClass == .compact
    else { return }
    delegate?.didSelectTab(selectedTab, source: .tappedBackground)
  }
}

extension TabGridContainerView: UICollectionViewDelegate {
  func collectionView(
    _ collectionView: UICollectionView,
    canPerformPrimaryActionForItemAt indexPath: IndexPath
  ) -> Bool {
    return !collectionView.isEditing && !collectionView.hasActiveDrag
  }

  func collectionView(
    _ collectionView: UICollectionView,
    performPrimaryActionForItemAt indexPath: IndexPath
  ) {
    switch items[safe: indexPath.item] {
    case .tab(let tab):
      delegate?.didSelectTab(tab, source: .tappedCell)
    case .group(let group, _):
      delegate?.didSelectGroup(group)
    case nil:
      break
    }
  }

  func collectionView(
    _ collectionView: UICollectionView,
    shouldSelectItemAt indexPath: IndexPath
  ) -> Bool {
    // Only tabs can be multi-selected
    return items[safe: indexPath.item]?.isGroup == false
  }

  func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
    notifySelectedTabsChanged()
  }

  func collectionView(_ collectionView: UICollectionView, didDeselectItemAt indexPath: IndexPath) {
    notifySelectedTabsChanged()
  }

  private func notifySelectedTabsChanged() {
    let selectedIndexPaths = collectionView.indexPathsForSelectedItems ?? []
    let selectedTabIDs = selectedIndexPaths.compactMap { indexPath -> TabState.ID? in
      guard case .tab(let id) = dataSource.itemIdentifier(for: indexPath) else { return nil }
      return id
    }
    delegate?.selectedTabListChanged(Set(selectedTabIDs))
  }

  func collectionView(
    _ collectionView: UICollectionView,
    contextMenuConfigurationForItemsAt indexPaths: [IndexPath],
    point: CGPoint
  ) -> UIContextMenuConfiguration? {
    let items = indexPaths.compactMap({ self.items[safe: $0.item] })
    if case .group(let group, _) = items.first, items.count == 1 {
      return .init(actionProvider: { _ in
        return self.delegate?.contextMenuForGroup(group)
      })
    }
    let tabs = items.compactMap { item -> (any TabState)? in
      if case .tab(let tab) = item { tab } else { nil }
    }
    if tabs.isEmpty {
      return nil
    }
    return .init(actionProvider: { _ in
      return self.delegate?.contextMenuForTabs(tabs)
    })
  }

  func collectionView(
    _ collectionView: UICollectionView,
    contextMenuConfiguration configuration: UIContextMenuConfiguration,
    highlightPreviewForItemAt indexPath: IndexPath
  ) -> UITargetedPreview? {
    guard let cell = collectionView.cellForItem(at: indexPath),
      let parameters = dragPreviewParameters(collectionView: collectionView, indexPath: indexPath)
    else { return nil }
    return UITargetedPreview(view: cell, parameters: parameters)
  }

  func collectionView(
    _ collectionView: UICollectionView,
    contextMenuConfiguration configuration: UIContextMenuConfiguration,
    dismissalPreviewForItemAt indexPath: IndexPath
  ) -> UITargetedPreview? {
    guard let cell = collectionView.cellForItem(at: indexPath),
      let parameters = dragPreviewParameters(collectionView: collectionView, indexPath: indexPath)
    else { return nil }
    return UITargetedPreview(view: cell, parameters: parameters)
  }
}

extension TabGridContainerView: UICollectionViewDragDelegate, UICollectionViewDropDelegate {
  func collectionView(
    _ collectionView: UICollectionView,
    dragSessionIsRestrictedToDraggingApplication session: any UIDragSession
  ) -> Bool {
    return true
  }

  private func dragPreviewParameters(
    collectionView: UICollectionView,
    indexPath: IndexPath
  ) -> UIDragPreviewParameters? {
    guard let cell = collectionView.cellForItem(at: indexPath) else { return nil }
    let parameters = UIDragPreviewParameters()
    parameters.visiblePath = .init(roundedRect: cell.bounds, cornerRadius: 16)
    return parameters
  }

  func collectionView(
    _ collectionView: UICollectionView,
    dragPreviewParametersForItemAt indexPath: IndexPath
  ) -> UIDragPreviewParameters? {
    return dragPreviewParameters(collectionView: collectionView, indexPath: indexPath)
  }

  func collectionView(
    _ collectionView: UICollectionView,
    itemsForBeginning session: UIDragSession,
    at indexPath: IndexPath
  ) -> [UIDragItem] {
    // Groups can't be dragged
    guard case .tab(let tabID) = dataSource.itemIdentifier(for: indexPath) else { return [] }
    let dragItem = UIDragItem(itemProvider: NSItemProvider())
    dragItem.localObject = tabID
    return [dragItem]
  }

  func collectionView(
    _ collectionView: UICollectionView,
    itemsForAddingTo session: any UIDragSession,
    at indexPath: IndexPath,
    point: CGPoint
  ) -> [UIDragItem] {
    if !supportsMultiSelectMoves {
      return []
    }
    guard case .tab(let tabID) = dataSource.itemIdentifier(for: indexPath) else { return [] }
    let dragItem = UIDragItem(itemProvider: NSItemProvider())
    dragItem.localObject = tabID
    return [dragItem]
  }

  func collectionView(
    _ collectionView: UICollectionView,
    dragSessionWillBegin session: any UIDragSession
  ) {
    UIImpactFeedbackGenerator(style: .medium).vibrate()
  }

  func collectionView(
    _ collectionView: UICollectionView,
    dropPreviewParametersForItemAt indexPath: IndexPath
  ) -> UIDragPreviewParameters? {
    return dragPreviewParameters(collectionView: collectionView, indexPath: indexPath)
  }

  func collectionView(
    _ collectionView: UICollectionView,
    performDropWith coordinator: UICollectionViewDropCoordinator
  ) {
    guard
      case let tabIDs = coordinator.items.compactMap({ $0.dragItem.localObject as? TabState.ID }),
      !tabIDs.isEmpty,
      let destinationIndexPath = coordinator.destinationIndexPath
    else { return }

    delegate?.didMoveTab(tabIDs, to: destinationIndexPath.item)
    for item in coordinator.items {
      coordinator.drop(item.dragItem, toItemAt: destinationIndexPath)
    }
  }

  func collectionView(
    _ collectionView: UICollectionView,
    dropSessionDidUpdate session: UIDropSession,
    withDestinationIndexPath destinationIndexPath: IndexPath?
  ) -> UICollectionViewDropProposal {
    guard let localDragSession = session.localDragSession,
      case let tabIDs = localDragSession.items.compactMap({ $0.localObject as? TabState.ID }),
      !tabIDs.isEmpty
    else {
      return .init(operation: .forbidden)
    }

    let indexPaths = tabIDs.compactMap { dataSource.indexPath(for: .tab($0)) }
    if tabIDs.count != indexPaths.count {
      return .init(operation: .cancel)
    }

    return .init(operation: .move, intent: .insertAtDestinationIndexPath)
  }
}

struct TabGridContainerViewRepresentable: UIViewRepresentable {
  var viewModel: TabGridViewModel
  var containerView: TabGridContainerView?
  var items: [TabGridItem]
  var isPrivateBrowsing: Bool
  var insets: EdgeInsets
  var contextMenuForTabs: ([any TabState]) -> UIMenu?
  var contextMenuForGroup: (TabGroup) -> UIMenu?
  var scrollsOnModeSwitch: Bool = true
  var isReorderingEnabled: Bool = true
  var isGroupDetail: Bool = false
  @Binding var selectedTabList: Set<TabState.ID>

  func makeUIView(context: Context) -> TabGridContainerView {
    let view =
      self.containerView ?? TabGridContainerView(isPrivateBrowsing: isPrivateBrowsing)
    view.delegate = context.coordinator
    return view
  }

  func updateUIView(_ uiView: TabGridContainerView, context: Context) {
    let isEditing = context.environment.editMode?.wrappedValue == .active
    context.coordinator.dismissAction = context.environment.dismiss
    context.coordinator.containerView = uiView
    if isGroupDetail {
      viewModel.openGroupContainerView = uiView
    }
    uiView.isPrivateBrowsing = isPrivateBrowsing
    uiView.isEditing = isEditing
    uiView.isReorderingEnabled = isReorderingEnabled
    uiView.updateItems(items, transaction: context.transaction)
    if isEditing {
      let indexPaths = selectedTabList.compactMap { uiView.dataSource.indexPath(for: .tab($0)) }
      let indexPathsToDeselect = Set(uiView.collectionView.indexPathsForSelectedItems ?? [])
        .subtracting(indexPaths)
      for indexPath in indexPaths {
        uiView.collectionView.selectItem(at: indexPath, animated: false, scrollPosition: [])
      }
      for indexPath in indexPathsToDeselect {
        uiView.collectionView.deselectItem(at: indexPath, animated: false)
      }
    }
    uiView.maskInsets = .init(
      top: insets.top,
      left: 0,
      bottom: insets.bottom,
      right: 0
    )
  }

  func makeCoordinator() -> Coordinator {
    .init(
      viewModel: viewModel,
      contextMenu: contextMenuForTabs,
      groupContextMenu: contextMenuForGroup,
      scrollsOnModeSwitch: scrollsOnModeSwitch,
      selectedTabList: $selectedTabList
    )
  }

  @MainActor
  class Coordinator: TabCollectionViewDelegate {
    var viewModel: TabGridViewModel
    var dismissAction: DismissAction?
    var containerView: TabGridContainerView?
    var contextMenu: ([any TabState]) -> UIMenu?
    var groupContextMenu: (TabGroup) -> UIMenu?
    @Binding var selectedTabList: Set<TabState.ID>

    init(
      viewModel: TabGridViewModel,
      contextMenu: @escaping ([any TabState]) -> UIMenu?,
      groupContextMenu: @escaping (TabGroup) -> UIMenu?,
      scrollsOnModeSwitch: Bool,
      selectedTabList: Binding<Set<TabState.ID>>
    ) {
      self.viewModel = viewModel
      self.contextMenu = contextMenu
      self.groupContextMenu = groupContextMenu
      self._selectedTabList = selectedTabList

      // Set up scroll handler to be called after mode switch completes
      if scrollsOnModeSwitch {
        viewModel.onModeSwitchComplete = { [weak self] in
          guard let self, let containerView = self.containerView else { return }
          containerView.collectionView.layoutIfNeeded()
          containerView.scrollToSelectedItem(animated: false)
        }
      }
    }

    func didSelectTab(_ tab: any TabState, source: TabCollectionViewSelectSource) {
      if source == .tappedBackground && viewModel.isSearching {
        return
      }
      viewModel.selectTab(tab)
      if source != .keyboardShortcut {
        dismissAction?()
      }
    }

    func didRemoveTab(_ tab: any TabState) {
      let shouldDismissAfterClose = viewModel.tabs.count == 1
      viewModel.closeTab(tab)
      if shouldDismissAfterClose {
        dismissAction?()
      }
    }

    func didMoveTab(_ tabs: [TabState.ID], to index: Int) {
      viewModel.moveTabs(tabs, toItemIndex: index)
      containerView?.updateItems(viewModel.items, transaction: .init(animation: .default))
    }

    func selectedTabListChanged(_ tabs: Set<TabState.ID>) {
      selectedTabList = tabs
    }

    func contextMenuForTabs(_ tabs: [any TabState]) -> UIMenu? {
      self.contextMenu(tabs)
    }

    func contextMenuForGroup(_ group: TabGroup) -> UIMenu? {
      self.groupContextMenu(group)
    }

    func didSelectGroup(_ group: TabGroup) {
      // The group view grows out of the group's item
      viewModel.showGroup(group, from: containerView?.windowFrame(of: group))
    }

    func didRequestUngroup(_ group: TabGroup) {
      viewModel.ungroup(group)
    }

    func didRequestCloseTabs(in group: TabGroup) {
      let shouldDismissAfterClose = viewModel.closesAllTabs(in: group)
      viewModel.closeTabs(in: group)
      if shouldDismissAfterClose {
        dismissAction?()
      }
    }
  }
}
