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
}

/// A view that can display tabs in a grid formation
class TabGridContainerView: UIView {
  let collectionView: UICollectionView
  var isPrivateBrowsing: Bool
  var isEditing: Bool = false {
    didSet {
      collectionView.isEditing = isEditing
      collectionView.dragInteractionEnabled = !isEditing || supportsMultiSelectMoves
    }
  }
  var maskInsets: UIEdgeInsets = .zero {
    didSet {
      updateContentInsets()
    }
  }
  private var tabs: [any TabState] = []

  func updateTabs(_ tabs: [any TabState], transaction: Transaction) {
    self.tabs = tabs
    applyTabsSnapshot(animated: transaction.animation != nil)
  }

  weak var delegate: TabCollectionViewDelegate?

  private(set) var dataSource: UICollectionViewDiffableDataSource<Int, TabState.ID>!

  // At the moment there seems to be a bug in iOS/UICollectionView which prohibits multi-item drag
  // within from working correctly. It will stop live updating and then not call the drop delegate
  // `performDrop` method despite returning a move operation with the intent to insert at the
  // destination index path.
  private let supportsMultiSelectMoves = false

  init(isPrivateBrowsing: Bool) {
    self.isPrivateBrowsing = isPrivateBrowsing
    self.collectionView = .init(frame: .zero, collectionViewLayout: .tabGridLayout)

    super.init(frame: .zero)

    let registration: UICollectionView.CellRegistration<TabGridItemCell, any TabState> =
      .init { [unowned self] cell, _, item in
        cell.isPrivateBrowsing = self.isPrivateBrowsing
        cell.actionHandler = { [unowned self, weak item] action in
          guard let item else { return }
          handleAction(action, for: item)
        }
        cell.configure(with: item)
      }
    self.dataSource = UICollectionViewDiffableDataSource<Int, TabState.ID>(
      collectionView: collectionView,
      cellProvider: { [weak self] collectionView, indexPath, _ in
        guard let self,
          let tab = tabs[safe: indexPath.item]
        else {
          return .init()
        }
        return collectionView.dequeueConfiguredReusableCell(
          using: registration,
          for: indexPath,
          item: tab
        )
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

  private func scrollToSelectedItem(animated: Bool) {
    guard let tab = tabs.first(where: \.isVisible),
      let indexPath = dataSource.indexPath(for: tab.id)
    else { return }
    collectionView.scrollToItem(
      at: indexPath,
      at: .bottom,
      animated: animated
    )
  }

  private func prepareCollectionView() {
    collectionView.alwaysBounceVertical = true
    collectionView.backgroundColor = .clear
    collectionView.dataSource = dataSource
    collectionView.delegate = self
    collectionView.dragInteractionEnabled = true
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
    // iOS 18+'s SwiftUI animation sharing feature is broken and causes weird jumps so we can't use
    // Animation.toolbarsSizeAnimation directly
    let animator = UIViewPropertyAnimator(duration: 0.3, dampingRatio: 0.825)
    animator.addAnimations(animations)
    animator.startAnimation()
  }

  private func applyTabsSnapshot(animated: Bool) {
    var snapshot: NSDiffableDataSourceSnapshot<Int, TabState.ID> = .init()
    snapshot.appendSections([0])
    snapshot.appendItems(tabs.map(\.id))
    dataSource.apply(snapshot, animatingDifferences: animated)
  }

  private func handleAction(_ action: TabGridItemView.Action, for tab: some TabState) {
    switch action {
    case .closedTab:
      delegate?.didRemoveTab(tab)
    }
  }

  @objc private func handleArrowKeyCommand(_ sender: UIKeyCommand) {
    guard let currentSelectedTabIndex = tabs.firstIndex(where: \.isVisible) else {
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
    if let newSelectedTab = tabs[safe: currentSelectedTabIndex + step] {
      delegate?.didSelectTab(newSelectedTab, source: .keyboardShortcut)
    }
  }

  @objc private func tappedCollectionViewBackground() {
    guard let selectedTab = tabs.first(where: \.isVisible),
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
    guard let tab = tabs[safe: indexPath.item] else { return }
    delegate?.didSelectTab(tab, source: .tappedCell)
  }

  func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
    let selectedIndexPaths = collectionView.indexPathsForSelectedItems ?? []
    delegate?.selectedTabListChanged(
      Set(selectedIndexPaths.compactMap(dataSource.itemIdentifier(for:)))
    )
  }

  func collectionView(_ collectionView: UICollectionView, didDeselectItemAt indexPath: IndexPath) {
    let selectedIndexPaths = collectionView.indexPathsForSelectedItems ?? []
    delegate?.selectedTabListChanged(
      Set(selectedIndexPaths.compactMap(dataSource.itemIdentifier(for:)))
    )
  }

  func collectionView(
    _ collectionView: UICollectionView,
    contextMenuConfigurationForItemsAt indexPaths: [IndexPath],
    point: CGPoint
  ) -> UIContextMenuConfiguration? {
    let tabs = indexPaths.compactMap({ self.tabs[safe: $0.item] })
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
    guard let tabID = dataSource.itemIdentifier(for: indexPath) else { return [] }
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
    guard let tabID = dataSource.itemIdentifier(for: indexPath) else { return [] }
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

    let indexPaths = tabIDs.compactMap(dataSource.indexPath(for:))
    if tabIDs.count != indexPaths.count {
      return .init(operation: .cancel)
    }

    return .init(operation: .move, intent: .insertAtDestinationIndexPath)
  }
}

struct TabGridContainerViewRepresentable: UIViewRepresentable {
  var viewModel: TabGridViewModel
  var containerView: TabGridContainerView?
  var tabs: [any TabState]
  var isPrivateBrowsing: Bool
  var insets: EdgeInsets
  var contextMenuForTabs: ([any TabState]) -> UIMenu?
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
    uiView.isPrivateBrowsing = isPrivateBrowsing
    uiView.isEditing = isEditing
    uiView.updateTabs(tabs, transaction: context.transaction)
    if isEditing {
      let indexPaths = selectedTabList.compactMap { uiView.dataSource.indexPath(for: $0) }
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
      selectedTabList: $selectedTabList
    )
  }

  @MainActor
  class Coordinator: TabCollectionViewDelegate {
    var viewModel: TabGridViewModel
    var dismissAction: DismissAction?
    var containerView: TabGridContainerView?
    var contextMenu: ([any TabState]) -> UIMenu?
    @Binding var selectedTabList: Set<TabState.ID>

    init(
      viewModel: TabGridViewModel,
      contextMenu: @escaping ([any TabState]) -> UIMenu?,
      selectedTabList: Binding<Set<TabState.ID>>
    ) {
      self.viewModel = viewModel
      self.contextMenu = contextMenu
      self._selectedTabList = selectedTabList
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
      viewModel.moveTabs(tabs, to: index)
      containerView?.updateTabs(viewModel.tabs, transaction: .init(animation: .default))
    }

    func selectedTabListChanged(_ tabs: Set<TabState.ID>) {
      selectedTabList = tabs
    }

    func contextMenuForTabs(_ tabs: [any TabState]) -> UIMenu? {
      self.contextMenu(tabs)
    }
  }
}
