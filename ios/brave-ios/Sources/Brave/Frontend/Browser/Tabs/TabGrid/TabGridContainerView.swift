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
  func didMoveTab(_ tab: any TabState, to index: Int)
}

/// A view that can display tabs in a grid formation
class TabGridContainerView: UIView {
  let collectionView: UICollectionView
  var isPrivateBrowsing: Bool
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
    collectionView.dragDelegate = self
    collectionView.dropDelegate = self
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
    var adjustedInsets = maskInsets
    adjustedInsets.top += TabGridSearchBar.defaultHeight + TabGridSearchBar.padding
    collectionView.contentInset = adjustedInsets
    collectionView.scrollIndicatorInsets = maskInsets
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
  func scrollViewWillEndDragging(
    _ scrollView: UIScrollView,
    withVelocity velocity: CGPoint,
    targetContentOffset: UnsafeMutablePointer<CGPoint>
  ) {
    let searchBarContainerHeight = TabGridSearchBar.defaultHeight + TabGridSearchBar.padding
    var proposedContentOffset = targetContentOffset.pointee
    let searchStart = -scrollView.contentInset.top
    let contentStart = -(scrollView.contentInset.top - searchBarContainerHeight)
    let yOffset = proposedContentOffset.y + scrollView.contentInset.top - searchBarContainerHeight
    if proposedContentOffset.y < -(scrollView.contentInset.top - searchBarContainerHeight) {
      proposedContentOffset.y =
        ((-yOffset - TabGridSearchBar.padding) / TabGridSearchBar.defaultHeight) > 0.3
        ? searchStart : contentStart
    }
    targetContentOffset.pointee = proposedContentOffset
  }

  func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
    guard let tab = tabs[safe: indexPath.item] else { return }
    delegate?.didSelectTab(tab, source: .tappedCell)
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

    UIImpactFeedbackGenerator(style: .medium).vibrate()

    let dragItem = UIDragItem(itemProvider: NSItemProvider())
    dragItem.localObject = tabID
    return [dragItem]
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
    guard let dragItem = coordinator.items.first?.dragItem,
      let tabID = dragItem.localObject as? TabState.ID,
      let tab = tabs.first(where: { $0.id == tabID }),
      let destinationIndexPath = coordinator.destinationIndexPath
    else { return }

    delegate?.didMoveTab(tab, to: destinationIndexPath.item)
    coordinator.drop(dragItem, toItemAt: destinationIndexPath)
  }

  func collectionView(
    _ collectionView: UICollectionView,
    dropSessionDidUpdate session: UIDropSession,
    withDestinationIndexPath destinationIndexPath: IndexPath?
  ) -> UICollectionViewDropProposal {

    guard let localDragSession = session.localDragSession,
      let item = localDragSession.items.first,
      let tabID = item.localObject as? TabState.ID
    else {
      return .init(operation: .forbidden)
    }

    if dataSource.indexPath(for: tabID) == nil {
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

  func makeUIView(context: Context) -> TabGridContainerView {
    let view =
      self.containerView ?? TabGridContainerView(isPrivateBrowsing: isPrivateBrowsing)
    view.delegate = context.coordinator
    return view
  }

  func updateUIView(_ uiView: TabGridContainerView, context: Context) {
    context.coordinator.dismissAction = context.environment.dismiss
    context.coordinator.containerView = uiView
    uiView.isPrivateBrowsing = isPrivateBrowsing
    uiView.updateTabs(tabs, transaction: context.transaction)
    uiView.maskInsets = .init(
      top: insets.top,
      left: 0,
      bottom: insets.bottom,
      right: 0
    )
  }

  func makeCoordinator() -> Coordinator {
    .init(viewModel: viewModel)
  }

  @MainActor
  class Coordinator: TabCollectionViewDelegate {
    var viewModel: TabGridViewModel
    var dismissAction: DismissAction?
    var containerView: TabGridContainerView?

    init(viewModel: TabGridViewModel) {
      self.viewModel = viewModel
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

    func didMoveTab(_ tab: any TabState, to index: Int) {
      viewModel.moveTab(tab, to: index)
      containerView?.updateTabs(viewModel.tabs, transaction: .init(animation: .default))
    }
  }
}
