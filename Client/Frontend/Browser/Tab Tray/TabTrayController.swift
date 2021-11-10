// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared

protocol TabTrayDelegate: AnyObject {
    /// Notifies the delegate that order of tabs on tab tray has changed.
    /// This info can be used to update UI in other place, for example update order of tabs in tabs bar.
    func tabOrderChanged()
}

class TabTrayController: UIViewController {
    
    var tabTrayView: View {
        return view as! View // swiftlint:disable:this force_cast
    }
    
    override func loadView() {
        view = View()
    }
    
    enum TabTraySection {
      case main
    }
    
    typealias DataSource = UICollectionViewDiffableDataSource<TabTraySection, Tab>
    typealias Snapshot = NSDiffableDataSourceSnapshot<TabTraySection, Tab>
    
    let tabManager: TabManager
    weak var delegate: TabTrayDelegate?
    
    private(set) lazy var dataSource =
    DataSource(collectionView: tabTrayView.collectionView,
               cellProvider: { [weak self] collectionView, indexPath, tab -> UICollectionViewCell? in
        self?.cellProvider(collectionView: collectionView, indexPath: indexPath, tab: tab)
    })
    
    private(set) var privateMode: Bool = false {
        didSet {
            // Should be set immediately before other logic executes
            PrivateBrowsingManager.shared.isPrivateBrowsing = privateMode
            applySnapshot()
            
            tabTrayView.privateModeButton.isSelected = privateMode
        }
    }
    
    init(tabManager: TabManager) {
        self.tabManager = tabManager
        super.init(nibName: nil, bundle: nil)
    }
    
    @available(*, unavailable)
    required init?(coder: NSCoder) { fatalError() }
    
    deinit {
        tabManager.removeDelegate(self)
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        tabManager.addDelegate(self)
        
        tabTrayView.collectionView.do {
            $0.register(TabCell.self, forCellWithReuseIdentifier: TabCell.identifier)
            
            $0.dataSource = dataSource
            $0.delegate = self
            $0.dragDelegate = self
            $0.dropDelegate = self
            $0.dragInteractionEnabled = true
        }
        
        privateMode = tabManager.selectedTab?.isPrivate == true
        
        tabTrayView.do {
            $0.doneButton.addTarget(self, action: #selector(doneAction), for: .touchUpInside)
            $0.newTabButton.addTarget(self, action: #selector(newTabAction), for: .touchUpInside)
            $0.privateModeButton.addTarget(self, action: #selector(togglePrivateModeAction), for: .touchUpInside)
        }
    }
    
    private var initialScrollCompleted = false
    
    override func viewDidLayoutSubviews() {
        super.viewDidLayoutSubviews()
        
        // When user opens the tray for the first time, we scroll the collection view to selected tab.
        if initialScrollCompleted { return }
        
        if let selectedTab = tabManager.selectedTab,
            let selectedIndexPath = dataSource.indexPath(for: selectedTab) {
            DispatchQueue.main.async {
                self.tabTrayView.collectionView.scrollToItem(at: selectedIndexPath, at: .centeredVertically, animated: false)
            }
            
            initialScrollCompleted = true
        }
    }
    
    // MARK: Snapshot handling
    
    private func applySnapshot() {
        var snapshot = Snapshot()
        snapshot.appendSections([.main])
        snapshot.appendItems(tabManager.tabsForCurrentMode)
        dataSource.apply(snapshot, animatingDifferences: true)
    }
    
    /// Reload the data source even if no changes are present. Use with caution.
    func forceReload() {
        var snapshot = dataSource.snapshot()
        snapshot.reloadSections([.main])
        dataSource.apply(snapshot, animatingDifferences: false)
    }
    
    private func cellProvider(collectionView: UICollectionView,
                              indexPath: IndexPath,
                              tab: Tab) -> UICollectionViewCell? {
        guard let cell = collectionView
                .dequeueReusableCell(withReuseIdentifier: TabCell.identifier,
                                     for: indexPath) as? TabCell else { return UICollectionViewCell() }
        
        cell.configure(with: tab)
        
        if tab == tabManager.selectedTab {
            cell.setTabSelected(tab)
        }
        
        cell.closedTab = { [weak self] tab in
            self?.remove(tab: tab)
            UIAccessibility.post(notification: .announcement, argument: Strings.tabTrayClosingTabAccessibilityNotificationText)
        }
        
        return cell
    }
    
    // MARK: - Actions
    
    @objc func doneAction() {
        dismiss(animated: true)
    }
    
    @objc func newTabAction() {
        let isPrivateModeInfoShowing = tabTrayView.isPrivateModeInfoShowing
        
        if privateMode {
            tabTrayView.hidePrivateModeInfo()
        }
        
        // If private mode info is showing it means we already added one tab.
        // So when user taps on the 'new tab' button we do nothing, only dismiss the view.
        if isPrivateModeInfoShowing {
            dismiss(animated: true)
        } else {
            tabManager.addTabAndSelect(isPrivate: privateMode)
        }
    }
    
    @objc func togglePrivateModeAction() {
        tabManager.willSwitchTabMode(leavingPBM: privateMode)
        privateMode.toggle()
        // When we switch from Private => Regular make sure we reset _selectedIndex, fix for bug #888
        tabManager.resetSelectedIndex()
        if privateMode {
            tabTrayView.showPrivateModeInfo()
            // New private tab is created immediately to reflect changes on NTP.
            // If user drags the modal down or dismisses it, a new private tab will be ready.
            tabManager.addTabAndSelect(isPrivate: true)
        } else {
            tabTrayView.hidePrivateModeInfo()
            // When you go back from private mode, a first tab is selected.
            // So when you dismiss the modal, correct tab and url is showed.
            tabManager.selectTab(tabManager.tabsForCurrentMode.first)
        }
    }
    
    func remove(tab: Tab) {
        tabManager.removeTab(tab)
        applySnapshot()
    }
    
    func removeAllTabs() {
        tabManager.removeTabsWithUndoToast(tabManager.tabsForCurrentMode)
        applySnapshot()
    }
}

// MARK: - UICollectionViewDelegate
extension TabTrayController: UICollectionViewDelegate {
    func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
        guard let tab = dataSource.itemIdentifier(for: indexPath) else { return }
        tabManager.selectTab(tab)
        dismiss(animated: true)
    }
}

// MARK: - TabManagerDelegate
extension TabTrayController: TabManagerDelegate {
    func tabManager(_ tabManager: TabManager, didAddTab tab: Tab) {
        applySnapshot()
        
        // This check is mainly for entering private mode.
        // Then a first private tab is created, and we want to show an informational screen
        // instead of taking the user directly to new tab page.
        if tabManager.tabsForCurrentMode.count > 1 {
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.2) {
                self.dismiss(animated: true)
            }
        }
    }

    func tabManager(_ tabManager: TabManager, didRemoveTab tab: Tab) {
        // When user removes their last tab, a new one is created.
        // Until then, the view is dismissed and takes the user directly to that tab.
        if tabManager.tabsForCurrentMode.count < 1 {
            dismiss(animated: true)
        }
    }
    
    func tabManager(_ tabManager: TabManager, didSelectedTabChange selected: Tab?, previous: Tab?) { }
    func tabManager(_ tabManager: TabManager, willAddTab tab: Tab) { }
    func tabManager(_ tabManager: TabManager, willRemoveTab tab: Tab) { }
    func tabManagerDidAddTabs(_ tabManager: TabManager) { }
    func tabManagerDidRestoreTabs(_ tabManager: TabManager) { }
    func tabManagerDidRemoveAllTabs(_ tabManager: TabManager, toast: ButtonToast?) { }
}

// MARK: - Drag and Drop

// MARK: UICollectionViewDragDelegate
extension TabTrayController: UICollectionViewDragDelegate {
    func collectionView(_ collectionView: UICollectionView, itemsForBeginning session: UIDragSession, at indexPath: IndexPath) -> [UIDragItem] {
        
        guard let tab = dataSource.itemIdentifier(for: indexPath) else { return [] }
        
        UIImpactFeedbackGenerator(style: .medium).bzzt()
        
        let dragItem = UIDragItem(itemProvider: NSItemProvider())
        dragItem.localObject = tab
        return [dragItem]
    }
}

// MARK: UICollectionViewDropDelegate
extension TabTrayController: UICollectionViewDropDelegate {
    func collectionView(_ collectionView: UICollectionView,
                        performDropWith coordinator: UICollectionViewDropCoordinator) {
        
        guard let dragItem = coordinator.items.first?.dragItem,
              let tab = dragItem.localObject as? Tab,
              let destinationIndexPath = coordinator.destinationIndexPath else { return }
        
        coordinator.drop(dragItem, toItemAt: destinationIndexPath)
        tabManager.moveTab(tab, toIndex: destinationIndexPath.item)
        delegate?.tabOrderChanged()
        applySnapshot()
    }
    
    func collectionView(_ collectionView: UICollectionView, dropSessionDidUpdate session: UIDropSession, withDestinationIndexPath destinationIndexPath: IndexPath?) -> UICollectionViewDropProposal {
        
        guard let localDragSession = session.localDragSession,
              let item = localDragSession.items.first,
              let tab = item.localObject as? Tab else {
            return .init(operation: .forbidden)
        }
        
        if dataSource.indexPath(for: tab) == nil {
            return .init(operation: .cancel)
        }
        
        return .init(operation: .move, intent: .insertAtDestinationIndexPath)
    }
}

// MARK: - UIScrollViewAccessibilityDelegate

extension TabTrayController: UIScrollViewAccessibilityDelegate {
    func accessibilityScrollStatus(for scrollView: UIScrollView) -> String? {
        let collectionView = tabTrayView.collectionView
        
        guard var visibleCells = collectionView.visibleCells as? [TabCell] else { return nil }
        var bounds = collectionView.bounds
        bounds = bounds.offsetBy(dx: collectionView.contentInset.left, dy: collectionView.contentInset.top)
        bounds.size.width -= collectionView.contentInset.left + collectionView.contentInset.right
        bounds.size.height -= collectionView.contentInset.top + collectionView.contentInset.bottom
        // visible cells do sometimes return also not visible cells when attempting to go past the last cell with VoiceOver right-flick gesture; so make sure we have only visible cells (yeah...)
        visibleCells = visibleCells.filter { !$0.frame.intersection(bounds).isEmpty }

        let cells = visibleCells.compactMap { collectionView.indexPath(for: $0) }
        let indexPaths = cells.sorted { (a: IndexPath, b: IndexPath) -> Bool in
            return a.section < b.section || (a.section == b.section && a.row < b.row)
        }

        if indexPaths.isEmpty {
            return Strings.tabTrayEmptyVoiceOverText
        }
        
        guard let firstTab = indexPaths.first, let lastTab = indexPaths.last else {
            return nil
        }

        let firstTabRow = firstTab.row + 1
        let lastTabRow = lastTab.row + 1
        let tabCount = collectionView.numberOfItems(inSection: 0)

        if firstTabRow == lastTabRow {
            return String(format: Strings.tabTraySingleTabPositionFormatVoiceOverText,
                          NSNumber(value: firstTabRow), NSNumber(value: tabCount))
        } else {
            return String(format: Strings.tabTrayMultiTabPositionFormatVoiceOverText, NSNumber(value: firstTabRow as Int), NSNumber(value: lastTabRow), NSNumber(value: tabCount))
        }
    }
}
