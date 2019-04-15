/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import UIKit
import SnapKit
import Shared
import BraveShared

protocol TabsBarViewControllerDelegate: class {
    func tabsBarDidSelectTab(_ tabsBarController: TabsBarViewController, _ tab: Tab)
    func tabsBarDidLongPressAddTab(_ tabsBarController: TabsBarViewController, button: UIButton)
}

class TabsBarViewController: UIViewController {
    private let leftOverflowIndicator = CAGradientLayer()
    private let rightOverflowIndicator = CAGradientLayer()
    
    weak var delegate: TabsBarViewControllerDelegate?
    
    private lazy var plusButton: UIButton = {
        let button = UIButton()
        button.setImage(#imageLiteral(resourceName: "add_tab").template, for: .normal)
        button.imageEdgeInsets = UIEdgeInsets(top: 6, left: 10, bottom: 6, right: 10)
        button.tintColor = UIColor.black
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
    
    private let bottomLine = UIView()
    
    fileprivate weak var tabManager: TabManager?
    fileprivate var tabList = WeakList<Tab>()
    
    init(tabManager: TabManager) {
        self.tabManager = tabManager
        super.init(nibName: nil, bundle: nil)
    }
    
    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
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
        
        view.addSubview(bottomLine)
        
        collectionView.snp.makeConstraints { make in
            make.bottom.top.left.equalTo(view)
            make.right.equalTo(view).inset(UX.TabsBar.buttonWidth)
        }
        
        bottomLine.snp.makeConstraints { make in
            make.height.equalTo(1.0 / UIScreen.main.scale)
            make.top.equalTo(view.snp.bottom)
            make.left.right.equalTo(view)
        }
    }
    
    override func viewWillAppear(_ animated: Bool) {
        // Updating tabs here is especially handy when tabs are reordered from the tab tray.
        updateData()
    }
    
    deinit {
        NotificationCenter.default.removeObserver(self)
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
    
    func reloadDataAndRestoreSelectedTab() {
        collectionView.reloadData()
        
        if let tabManager = tabManager, let selectedTab = tabManager.selectedTab {
            let selectedIndex = tabList.index(of: selectedTab) ?? 0
            if selectedIndex < tabList.count() {
                collectionView.selectItem(at: IndexPath(row: selectedIndex, section: 0), animated: (!tabManager.isRestoring), scrollPosition: .centeredHorizontally)
            }
        }
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
                location.y = gestureView.center.y // Lock y
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
    
    fileprivate func overflowIndicators() {
        addScrollHint(for: .leftSide, maskLayer: leftOverflowIndicator)
        addScrollHint(for: .rightSide, maskLayer: rightOverflowIndicator)
        
        let offset = Float(collectionView.contentOffset.x)
        let startFade = Float(30)
        leftOverflowIndicator.opacity = min(1, offset / startFade)
        
        // all the way scrolled right
        let offsetFromRight = collectionView.contentSize.width - CGFloat(offset) - collectionView.frame.width
        rightOverflowIndicator.opacity = min(1, Float(offsetFromRight) / startFade)
    }
    
    private enum HintSide { case leftSide, rightSide }
    
    private func addScrollHint(for side: HintSide, maskLayer: CAGradientLayer) {
        maskLayer.removeFromSuperlayer()
        
        let barsColor = PrivateBrowsingManager.shared.isPrivateBrowsing ?
            UX.barsDarkBackgroundSolidColor : UX.barsBackgroundSolidColor
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
        overflowIndicators()
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
        
        cell.closeTabCallback = { [weak self] tab in
            guard let strongSelf = self, let tabManager = strongSelf.tabManager, let previousIndex = strongSelf.tabList.index(of: tab) else { return }
            
            tabManager.removeTab(tab)
            strongSelf.updateData()
            
            let previousOrNext = max(0, previousIndex - 1)
            tabManager.selectTab(strongSelf.tabList[previousOrNext])
            
            strongSelf.collectionView.selectItem(at: IndexPath(row: previousOrNext, section: 0), animated: true, scrollPosition: .centeredHorizontally)
        }
        
        return cell
    }
    
    func collectionView(_ collectionView: UICollectionView, moveItemAt sourceIndexPath: IndexPath, to destinationIndexPath: IndexPath) {
        guard let manager = tabManager, let fromTab = tabList[sourceIndexPath.row],
            let toTab = tabList[destinationIndexPath.row] else { return }
        
        // Find original from/to index... we need to target the full list not partial.
        let tabs = manager.tabsForCurrentMode
        guard let to = tabs.index(where: {$0 === toTab}) else { return }
        
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

extension TabsBarViewController: Themeable {
    func applyTheme(_ theme: Theme) {
        switch theme {
        case .regular:
            view.backgroundColor = BraveUX.GreyB
            plusButton.tintColor = BraveUX.GreyI
            bottomLine.backgroundColor = UIColor(white: 0.0, alpha: 0.2)
        case .private:
            view.backgroundColor = BraveUX.Black
            plusButton.tintColor = UIColor.white
            bottomLine.backgroundColor = UIColor(white: 1.0, alpha: 0.2)
        }

        collectionView.backgroundColor = view.backgroundColor
    }
}
