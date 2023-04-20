/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import UIKit
import SnapKit
import Shared
import Preferences
import Combine

protocol TabsBarViewControllerDelegate: AnyObject {
  func tabsBarDidSelectTab(_ tabsBarController: TabsBarViewController, _ tab: Tab)
  func tabsBarDidLongPressAddTab(_ tabsBarController: TabsBarViewController, button: UIButton)
  func tabsBarDidSelectAddNewTab(_ isPrivate: Bool)
  func tabsBarDidChangeReaderModeVisibility(_ isHidden: Bool)

}

class TabsBarViewController: UIViewController {
  private let leftOverflowIndicator = CAGradientLayer()
  private let rightOverflowIndicator = CAGradientLayer()

  weak var delegate: TabsBarViewControllerDelegate?
  private var cancellables: Set<AnyCancellable> = []

  private lazy var plusButton: UIButton = {
    let button = UIButton()
    button.setImage(UIImage(named: "add_tab", in: .module, compatibleWith: nil)!.template, for: .normal)
    button.imageEdgeInsets = UIEdgeInsets(top: 6, left: 10, bottom: 6, right: 10)
    button.tintColor = .braveLabel
    button.contentMode = .scaleAspectFit
    button.addTarget(self, action: #selector(addTabPressed), for: .touchUpInside)
    button.addGestureRecognizer(UILongPressGestureRecognizer(target: self, action: #selector(didLongPressAddTab(_:))))
    button.backgroundColor = .clear
    return button
  }()

  fileprivate lazy var collectionView: UICollectionView = {
    let layout = UICollectionViewFlowLayout()
    layout.scrollDirection = .horizontal
    layout.minimumInteritemSpacing = 0
    layout.minimumLineSpacing = 0

    let view = UICollectionView(frame: CGRect.zero, collectionViewLayout: layout)
    view.showsHorizontalScrollIndicator = false
    view.bounces = false
    view.delegate = self
    view.dataSource = self
    view.allowsSelection = true
    view.decelerationRate = UIScrollView.DecelerationRate.normal
    view.register(TabBarCell.self, forCellWithReuseIdentifier: "TabCell")
    return view
  }()

  private weak var tabManager: TabManager?
  private var tabList = WeakList<Tab>()

  init(tabManager: TabManager) {
    self.tabManager = tabManager
    super.init(nibName: nil, bundle: nil)
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    view.backgroundColor = Preferences.General.nightModeEnabled.value ? .nightModeBackground : .urlBarBackground
    collectionView.backgroundColor = view.backgroundColor

    tabManager?.addDelegate(self)

    // Can't get view.frame inside of lazy property, need to put this code here.
    collectionView.frame = view.frame
    (collectionView.collectionViewLayout as? UICollectionViewFlowLayout)?.itemSize = CGSize(width: UX.TabsBar.minimumWidth, height: view.frame.height)
    view.addSubview(collectionView)

    let longPressGesture = UILongPressGestureRecognizer(target: self, action: #selector(handleLongGesture(gesture:)))
    longPressGesture.minimumPressDuration = 0.2
    collectionView.addGestureRecognizer(longPressGesture)

    NotificationCenter.default.addObserver(self, selector: #selector(orientationChanged), name: UIDevice.orientationDidChangeNotification, object: nil)

    if UIDevice.current.userInterfaceIdiom == .pad {
      view.addSubview(plusButton)

      plusButton.snp.makeConstraints { make in
        make.right.top.bottom.equalTo(view)
        make.width.equalTo(UX.TabsBar.buttonWidth)
      }
    }

    collectionView.snp.makeConstraints { make in
      make.bottom.top.left.equalTo(view)
      make.right.equalTo(view).inset(UX.TabsBar.buttonWidth)
    }

    var newTabMenu: [UIAction] = []

    if !PrivateBrowsingManager.shared.isPrivateBrowsing {
      let openNewPrivateTab = UIAction(
        title: Strings.Hotkey.newPrivateTabTitle,
        image: UIImage(systemName: "plus.square.fill.on.square.fill"),
        handler: UIAction.deferredActionHandler { [unowned self] _ in
          self.delegate?.tabsBarDidSelectAddNewTab(true)
        })

      newTabMenu.append(openNewPrivateTab)
    }

    let openNewTab = UIAction(
      title: PrivateBrowsingManager.shared.isPrivateBrowsing ? Strings.Hotkey.newPrivateTabTitle : Strings.Hotkey.newTabTitle,
      image: PrivateBrowsingManager.shared.isPrivateBrowsing ? UIImage(systemName: "plus.square.fill.on.square.fill") : UIImage(systemName: "plus.square.on.square"),
      handler: UIAction.deferredActionHandler { [unowned self] _ in
        self.delegate?.tabsBarDidSelectAddNewTab(PrivateBrowsingManager.shared.isPrivateBrowsing)
      })

    newTabMenu.append(openNewTab)

    plusButton.menu = UIMenu(title: "", identifier: nil, children: newTabMenu)
    privateModeCancellable = PrivateBrowsingManager.shared
      .$isPrivateBrowsing
      .removeDuplicates()
      .sink(receiveValue: { [weak self] isPrivateBrowsing in
        self?.updateColors(isPrivateBrowsing)
      })
    
    Preferences.General.nightModeEnabled.objectWillChange
      .receive(on: RunLoop.main)
      .sink { [weak self] _ in
        self?.updateColors(PrivateBrowsingManager.shared.isPrivateBrowsing)
      }
      .store(in: &cancellables)
  }
  
  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)
    
    updateOverflowIndicatorsLayout(addButtonIncluded: true)
  }

  private var privateModeCancellable: AnyCancellable?
  private func updateColors(_ isPrivateBrowsing: Bool) {
    let backgroundColor: UIColor
    if isPrivateBrowsing {
      backgroundColor = .privateModeBackground
    } else {
      backgroundColor = Preferences.General.nightModeEnabled.value ? .nightModeBackground : .urlBarBackground
    }
    view.backgroundColor = backgroundColor
    collectionView.backgroundColor = view.backgroundColor
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)

    // Updating tabs here is especially handy when tabs are reordered from the tab tray.
    updateData()
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()

    updateOverflowIndicatorsLayout()
  }

  deinit {
    NotificationCenter.default.removeObserver(self)
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    // Updates overflow colors which use CGColor's
    overflowIndicators()
  }

  override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
    super.viewWillTransition(to: size, with: coordinator)

    // ensure the selected tab is visible after rotations
    if let index = tabManager?.currentDisplayedIndex {
      let indexPath = IndexPath(item: index, section: 0)
      // since bouncing is disabled, centering horizontally
      // will not overshoot the edges for the bookend tabs
      collectionView.scrollToItem(at: indexPath, at: .centeredHorizontally, animated: false)
    }

    DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
      self.reloadDataAndRestoreSelectedTab()
    }
  }

  @objc func orientationChanged() {
    overflowIndicators()
  }

  @objc func addTabPressed() {
    tabManager?.addTabAndSelect(isPrivate: PrivateBrowsingManager.shared.isPrivateBrowsing)
  }

  @objc private func didLongPressAddTab(_ longPress: UILongPressGestureRecognizer) {
    if longPress.state == .began {
      delegate?.tabsBarDidLongPressAddTab(self, button: plusButton)
    }
  }

  func updateData() {
    tabList = WeakList<Tab>()

    tabManager?.tabsForCurrentMode.forEach {
      tabList.insert($0)
    }

    overflowIndicators()
    reloadDataAndRestoreSelectedTab()
  }

  func updateSelectedTabTitle() {
    guard let selectedTabIndex = selectedTabIndexPath,
      let tab = tabList[selectedTabIndex.row]
    else { return }
    if let cell = collectionView.cellForItem(at: selectedTabIndex) as? TabBarCell {
      cell.titleLabel.text = tab.displayTitle
    }
  }

  func reloadDataAndRestoreSelectedTab() {
    collectionView.reloadData()

    guard let tabManager = tabManager, let selectedTabIndex = selectedTabIndexPath else {
      return
    }

    if selectedTabIndex.row < tabList.count() {
      collectionView.selectItem(
        at: selectedTabIndex,
        animated: (!tabManager.isRestoring),
        scrollPosition: .centeredHorizontally)
    }
  }

  private var selectedTabIndexPath: IndexPath? {
    guard let tabManager = tabManager, let selectedTab = tabManager.selectedTab,
      let selectedIndex = tabList.index(of: selectedTab)
    else { return nil }

    return IndexPath(row: selectedIndex, section: 0)
  }

  @objc func handleLongGesture(gesture: UILongPressGestureRecognizer) {
    switch gesture.state {
    case .began:
      guard let selectedIndexPath = self.collectionView.indexPathForItem(at: gesture.location(in: self.collectionView)) else {
        break
      }
      collectionView.beginInteractiveMovementForItem(at: selectedIndexPath)
    case .changed:
      if let gestureView = gesture.view {
        var location = gesture.location(in: gestureView)
        location.y = gestureView.center.y  // Lock y
        collectionView.updateInteractiveMovementTargetPosition(location)
      }
    case .ended:
      collectionView.endInteractiveMovement()
    default:
      collectionView.cancelInteractiveMovement()
    }
  }

  private func tabOverflowWidth(_ tabCount: Int) -> CGFloat {
    let overflow = CGFloat(tabCount) * UX.TabsBar.minimumWidth - collectionView.frame.width
    return max(overflow, 0)
  }

  private func updateOverflowIndicatorsLayout(addButtonIncluded: Bool = false) {
    let offset = Float(collectionView.contentOffset.x)
    let startFade = Float(30)
    leftOverflowIndicator.opacity = min(1, offset / startFade)

    // all the way scrolled right
    var offsetFromRight = collectionView.contentSize.width - CGFloat(offset) - collectionView.frame.width
    if addButtonIncluded {
      offsetFromRight -= plusButton.frame.width
    }
    rightOverflowIndicator.opacity = min(1, Float(offsetFromRight) / startFade)
  }

  fileprivate func overflowIndicators() {
    addScrollHint(for: .leftSide, maskLayer: leftOverflowIndicator)
    addScrollHint(for: .rightSide, maskLayer: rightOverflowIndicator)
    updateOverflowIndicatorsLayout()
  }

  private enum HintSide { case leftSide, rightSide }

  private func addScrollHint(for side: HintSide, maskLayer: CAGradientLayer) {
    maskLayer.removeFromSuperlayer()

    guard let barsColor = collectionView.backgroundColor?.resolvedColor(with: traitCollection) ?? view.backgroundColor else {
      // If not setup now, will be at some point, and then this can be flushed
      return
    }
    let colors = [barsColor.withAlphaComponent(0).cgColor, barsColor.cgColor]

    let locations = [0.9, 1.0]
    maskLayer.startPoint = CGPoint(x: side == .rightSide ? 0 : 1.0, y: 0.5)
    maskLayer.endPoint = CGPoint(x: side == .rightSide ? 1.0 : 0, y: 0.5)
    maskLayer.opacity = 0
    maskLayer.colors = colors
    maskLayer.locations = locations as [NSNumber]
    maskLayer.bounds = CGRect(x: 0, y: 0, width: collectionView.frame.width, height: UX.TabsBar.height)
    maskLayer.anchorPoint = CGPoint.zero
    // you must add the mask to the root view, not the scrollView, otherwise the masks will move as the user scrolls!
    view.layer.addSublayer(maskLayer)
  }
}

// MARK: - UIScrollViewDelegate
extension TabsBarViewController: UIScrollViewDelegate {
  func scrollViewDidScroll(_ scrollView: UIScrollView) {
    updateOverflowIndicatorsLayout()
  }
}

// MARK: - UICollectionViewDelegate
extension TabsBarViewController: UICollectionViewDelegate {
  func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
    if let tab = tabList[indexPath.row] {
      delegate?.tabsBarDidSelectTab(self, tab)
    }
  }
}

// MARK: - UICollectionViewDelegateFlowLayout
extension TabsBarViewController: UICollectionViewDelegateFlowLayout {
  func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, sizeForItemAt indexPath: IndexPath) -> CGSize {
    let tabCount = CGFloat(tabList.count())

    if tabCount < 1 { return CGSize.zero }
    if tabCount == 1 { return collectionView.frame.size }

    let tabsAndButtonWidth = tabCount * UX.TabsBar.minimumWidth
    if tabsAndButtonWidth < collectionView.frame.width {
      let maxWidth = collectionView.frame.width / tabCount
      return CGSize(width: maxWidth, height: view.frame.height)
    }

    return CGSize(width: UX.TabsBar.minimumWidth, height: view.frame.height)
  }
}

// MARK: - UICollectionViewDataSource
extension TabsBarViewController: UICollectionViewDataSource {

  func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
    return tabList.count()
  }

  func collectionView(_ collectionView: UICollectionView, canMoveItemAt indexPath: IndexPath) -> Bool {
    return true
  }

  func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
    guard let cell = collectionView.dequeueReusableCell(withReuseIdentifier: "TabCell", for: indexPath) as? TabBarCell
    else { return UICollectionViewCell() }
    guard let tab = tabList[indexPath.row] else { return cell }

    cell.tabManager = tabManager
    cell.tab = tab
    cell.titleLabel.text = tab.displayTitle
    cell.currentIndex = indexPath.row
    cell.separatorLineRight.isHidden = (indexPath.row != tabList.count() - 1)
    cell.configure()

    cell.closeTabCallback = { [weak self] tab in
      guard let self = self,
              let tabManager = self.tabManager,
              let previousIndex = self.tabList.index(of: tab) else {
        return
      }
      
      self.delegate?.tabsBarDidChangeReaderModeVisibility(true)
      
      // Add the tab information to recently closed before removing
      tabManager.addTabToRecentlyClosed(tab)
      tabManager.removeTab(tab)
      
      self.updateData()

      let previousOrNext = max(0, previousIndex - 1)
      tabManager.selectTab(self.tabList[previousOrNext])
      
      self.collectionView.selectItem(at: IndexPath(row: previousOrNext, section: 0), animated: false, scrollPosition: .centeredHorizontally)
      self.updateOverflowIndicatorsLayout()
      
    }

    return cell
  }

  func collectionView(_ collectionView: UICollectionView, moveItemAt sourceIndexPath: IndexPath, to destinationIndexPath: IndexPath) {
    guard let manager = tabManager, let fromTab = tabList[sourceIndexPath.row],
      let toTab = tabList[destinationIndexPath.row]
    else { return }

    // Find original from/to index... we need to target the full list not partial.
    let tabs = manager.tabsForCurrentMode
    guard let to = tabs.firstIndex(where: { $0 === toTab }) else { return }

    manager.moveTab(fromTab, toIndex: to)
    updateData()

    guard let selectedTab = tabList[destinationIndexPath.row] else { return }
    manager.selectTab(selectedTab)
  }
}

// MARK: - TabManagerDelegate
extension TabsBarViewController: TabManagerDelegate {
  func tabManager(_ tabManager: TabManager, didSelectedTabChange selected: Tab?, previous: Tab?) {
    assert(Thread.current.isMainThread)
    updateData()
    delegate?.tabsBarDidChangeReaderModeVisibility(false)
  }

  func tabManager(_ tabManager: TabManager, didAddTab tab: Tab) {
    updateData()
  }

  func tabManager(_ tabManager: TabManager, didRemoveTab tab: Tab) {
    assert(Thread.current.isMainThread)
    updateData()
  }

  func tabManagerDidRestoreTabs(_ tabManager: TabManager) {
    assert(Thread.current.isMainThread)
    updateData()
  }
}
