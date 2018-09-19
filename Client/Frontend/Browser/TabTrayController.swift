/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import SnapKit
import Storage
import Shared
import BraveShared

struct TabTrayControllerUX {
    static let CornerRadius = CGFloat(6.0)
    static let DefaultBorderWidth = 1.0 / UIScreen.main.scale
    static let BackgroundColor = UIColor.TopTabs.Background
    static let CellBackgroundColor = UIColor.TopTabs.Background
    static let ToolbarFont = UIFont.systemFont(ofSize: 17.0)
    static let TextBoxHeight = CGFloat(32.0)
    static let FaviconSize = CGFloat(20)
    static let Margin = CGFloat(15)
    static let CloseButtonSize = CGFloat(32)
    static let CloseButtonMargin = CGFloat(6.0)
    static let CloseButtonEdgeInset = CGFloat(7)

    static let NumberOfColumnsThin = 1
    static let NumberOfColumnsWide = 3
    static let CompactNumberOfColumnsThin = 2

    static let MenuFixedWidth: CGFloat = 320
}

private struct LightTabCellUX {
    static let TabTitleTextColor = UIColor.black
}

private struct DarkTabCellUX {
    static let TabTitleTextColor = UIColor.Photon.White100
}

protocol TabCellDelegate: class {
    func tabCellDidClose(_ cell: TabCell)
}

class TabCell: UICollectionViewCell {
    static let Identifier = "TabCellIdentifier"
    static let BorderWidth: CGFloat = 3

    let backgroundHolder = UIView()
    let screenshotView = UIImageViewAligned()
    let titleBackgroundView = GradientView(
        colors: [UIColor(white: 1.0, alpha: 0.98), UIColor(white: 1.0, alpha: 0.9), UIColor(white: 1.0, alpha: 0.0)],
        positions: [0, 0.5, 1],
        startPoint: .zero,
        endPoint: CGPoint(x: 0, y: 1)
    )
    let titleText: UILabel
    let favicon: UIImageView = UIImageView()
    let closeButton: UIButton

    var animator: SwipeAnimator!

    weak var delegate: TabCellDelegate?

    // Changes depending on whether we're full-screen or not.
    var margin = CGFloat(0)

    override init(frame: CGRect) {
        self.backgroundHolder.backgroundColor = UIColor.Photon.White100
        self.backgroundHolder.layer.cornerRadius = TabTrayControllerUX.CornerRadius
        self.backgroundHolder.clipsToBounds = true

        self.screenshotView.contentMode = .scaleAspectFill
        self.screenshotView.clipsToBounds = true
        self.screenshotView.isUserInteractionEnabled = false
        self.screenshotView.alignLeft = true
        self.screenshotView.alignTop = true
        screenshotView.backgroundColor = UIConstants.AppBackgroundColor

        self.favicon.backgroundColor = UIColor.clear
        self.favicon.layer.cornerRadius = 2.0
        self.favicon.layer.masksToBounds = true

        self.titleText = UILabel()
        self.titleText.isUserInteractionEnabled = false
        self.titleText.numberOfLines = 1
        self.titleText.font = DynamicFontHelper.defaultHelper.DefaultSmallFontBold
        self.titleText.textColor = LightTabCellUX.TabTitleTextColor
        self.titleText.backgroundColor = .clear

        self.closeButton = UIButton()
        self.closeButton.setImage(#imageLiteral(resourceName: "tab_close"), for: [])
        self.closeButton.imageView?.contentMode = .scaleAspectFit
        self.closeButton.contentMode = .center
        self.closeButton.imageEdgeInsets = UIEdgeInsets(equalInset: TabTrayControllerUX.CloseButtonEdgeInset)

        super.init(frame: frame)
        
        self.animator = SwipeAnimator(animatingView: self)
        self.closeButton.addTarget(self, action: #selector(close), for: .touchUpInside)

        layer.borderColor = UIColor.Photon.Grey90A20.cgColor
        layer.borderWidth = TabTrayControllerUX.DefaultBorderWidth
        layer.cornerRadius = TabTrayControllerUX.CornerRadius
        
        contentView.addSubview(backgroundHolder)
        backgroundHolder.addSubview(self.screenshotView)
        backgroundHolder.addSubview(self.titleBackgroundView)
        
        titleBackgroundView.addSubview(self.closeButton)
        titleBackgroundView.addSubview(self.titleText)
        titleBackgroundView.addSubview(self.favicon)

        self.accessibilityCustomActions = [
            UIAccessibilityCustomAction(name: NSLocalizedString("Close", comment: "Accessibility label for action denoting closing a tab in tab list (tray)"), target: self.animator, selector: #selector(SwipeAnimator.closeWithoutGesture))
        ]
    }

    func setTabSelected(_ tab: Tab) {
        layer.shadowColor = UIConstants.SystemBlueColor.cgColor
        layer.shadowOpacity = 1
        layer.shadowRadius = 0 // A 0 radius creates a solid border instead of a gradient blur
        layer.masksToBounds = false
        // create a frame that is "BorderWidth" size bigger than the cell
        layer.shadowOffset = CGSize(width: -TabCell.BorderWidth, height: -TabCell.BorderWidth)
        let shadowPath = CGRect(width: layer.frame.width + (TabCell.BorderWidth * 2), height: layer.frame.height + (TabCell.BorderWidth * 2))
        layer.shadowPath = UIBezierPath(roundedRect: shadowPath, cornerRadius: TabTrayControllerUX.CornerRadius+TabCell.BorderWidth).cgPath
        layer.borderWidth = 0.0
    }

    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    override func layoutSubviews() {
        super.layoutSubviews()

        backgroundHolder.frame = CGRect(x: margin, y: margin, width: frame.width, height: frame.height)
        screenshotView.frame = CGRect(size: backgroundHolder.frame.size)

        titleBackgroundView.snp.makeConstraints { (make) in
            make.top.left.right.equalTo(backgroundHolder)
            make.height.equalTo(TabTrayControllerUX.TextBoxHeight + 15.0)
        }

        favicon.snp.makeConstraints { make in
            make.leading.equalTo(titleBackgroundView).offset(6)
            make.top.equalTo((TabTrayControllerUX.TextBoxHeight - TabTrayControllerUX.FaviconSize) / 2)
            make.size.equalTo(TabTrayControllerUX.FaviconSize)
        }

        titleText.snp.makeConstraints { (make) in
            make.leading.equalTo(favicon.snp.trailing).offset(6)
            make.trailing.equalTo(closeButton.snp.leading).offset(-6)
            make.centerY.equalTo(favicon)
        }

        closeButton.snp.makeConstraints { make in
            make.size.equalTo(TabTrayControllerUX.CloseButtonSize)
            make.trailing.equalTo(titleBackgroundView)
            make.centerY.equalTo(favicon)
        }

        let shadowPath = CGRect(width: layer.frame.width + (TabCell.BorderWidth * 2), height: layer.frame.height + (TabCell.BorderWidth * 2))
        layer.shadowPath = UIBezierPath(roundedRect: shadowPath, cornerRadius: TabTrayControllerUX.CornerRadius+TabCell.BorderWidth).cgPath
    }

    override func prepareForReuse() {
        // Reset any close animations.
        backgroundHolder.transform = .identity
        backgroundHolder.alpha = 1
        self.titleText.font = DynamicFontHelper.defaultHelper.DefaultSmallFontBold
        layer.shadowOffset = .zero
        layer.shadowPath = nil
        layer.shadowOpacity = 0
        layer.borderWidth = TabTrayControllerUX.DefaultBorderWidth
    }

    override func accessibilityScroll(_ direction: UIAccessibilityScrollDirection) -> Bool {
        var right: Bool
        switch direction {
        case .left:
            right = false
        case .right:
            right = true
        default:
            return false
        }
        animator.close(right: right)
        return true
    }

    @objc
    func close() {
        self.animator.closeWithoutGesture()
    }
}

struct PrivateModeStrings {
    static let toggleAccessibilityLabel = NSLocalizedString("Private Mode", tableName: "PrivateBrowsing", comment: "Accessibility label for toggling on/off private mode")
    static let toggleAccessibilityHint = NSLocalizedString("Turns private mode on or off", tableName: "PrivateBrowsing", comment: "Accessiblity hint for toggling on/off private mode")
    static let toggleAccessibilityValueOn = NSLocalizedString("On", tableName: "PrivateBrowsing", comment: "Toggled ON accessibility value")
    static let toggleAccessibilityValueOff = NSLocalizedString("Off", tableName: "PrivateBrowsing", comment: "Toggled OFF accessibility value")
}

protocol TabTrayDelegate: class {
    func tabTrayDidDismiss(_ tabTray: TabTrayController)
    func tabTrayDidAddTab(_ tabTray: TabTrayController, tab: Tab)
    func tabTrayDidAddBookmark(_ tab: Tab)
    func tabTrayRequestsPresentationOf(_ viewController: UIViewController)
}

class TabTrayController: UIViewController, Themeable {
    let tabManager: TabManager
    let profile: Profile
    weak var delegate: TabTrayDelegate?
    var otherBrowsingModeOffset: CGPoint

    var collectionView: UICollectionView!
    var draggedCell: TabCell?
    var dragOffset: CGPoint = .zero
    lazy var toolbar: TrayToolbar = {
        let toolbar = TrayToolbar()
        toolbar.addTabButton.addTarget(self, action: #selector(didClickAddTab), for: .touchUpInside)
        toolbar.privateModeButton.addTarget(self, action: #selector(didTogglePrivateMode), for: .touchUpInside)
        toolbar.doneButton.addTarget(self, action: #selector(didClickDone), for: .touchUpInside)
        return toolbar
    }()

    fileprivate(set) internal var privateMode: Bool = false {
        didSet {
            PrivateBrowsingManager.shared.isPrivateBrowsing = privateMode
            
            tabDataSource.tabs = tabsToDisplay
            applyTheme(privateMode ? .private : .regular)
            collectionView?.reloadData()
            setNeedsStatusBarAppearanceUpdate()
        }
    }

    fileprivate var tabsToDisplay: [Tab] {
        let tabType: TabType = privateMode ? .private : .regular
        return tabManager.tabs(withType: tabType)
    }

    fileprivate lazy var emptyPrivateTabsView: EmptyPrivateTabsView = {
        let emptyView = EmptyPrivateTabsView()
        emptyView.learnMoreButton.addTarget(self, action: #selector(didTapLearnMore), for: .touchUpInside)
        return emptyView
    }()

    fileprivate lazy var tabDataSource: TabManagerDataSource = {
        return TabManagerDataSource(tabs: self.tabsToDisplay, cellDelegate: self, tabManager: self.tabManager)
    }()

    fileprivate lazy var tabLayoutDelegate: TabLayoutDelegate = {
        let delegate = TabLayoutDelegate(profile: self.profile, traitCollection: self.traitCollection)
        delegate.tabSelectionDelegate = self
        return delegate
    }()

    var numberOfColumns: Int {
        return tabLayoutDelegate.numberOfColumns
    }

    var tabs: [Tab] {
        return tabDataSource.tabs
    }

    init(tabManager: TabManager, profile: Profile) {
        self.tabManager = tabManager
        self.profile = profile
        self.otherBrowsingModeOffset = CGPoint(x: 0.0, y: 0.0)
        super.init(nibName: nil, bundle: nil)

        tabManager.addDelegate(self)
    }

    convenience init(tabManager: TabManager, profile: Profile, tabTrayDelegate: TabTrayDelegate) {
        self.init(tabManager: tabManager, profile: profile)
        self.delegate = tabTrayDelegate
    }

    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    deinit {
        self.tabManager.removeDelegate(self)
    }

    @objc func dynamicFontChanged(_ notification: Notification) {
        guard notification.name == .DynamicFontChanged else { return }

        self.collectionView.reloadData()
    }

// MARK: View Controller Callbacks
    override func viewDidLoad() {
        super.viewDidLoad()

        view.accessibilityLabel = NSLocalizedString("Tabs Tray", comment: "Accessibility label for the Tabs Tray view.")
        
        collectionView = UICollectionView(frame: view.frame, collectionViewLayout: UICollectionViewFlowLayout())
        
        collectionView.dataSource = tabDataSource
        collectionView.delegate = tabLayoutDelegate
        collectionView.contentInset = UIEdgeInsets(top: 0, left: 0, bottom: UIConstants.BottomToolbarHeight, right: 0)
        collectionView.register(TabCell.self, forCellWithReuseIdentifier: TabCell.Identifier)

        collectionView.dragInteractionEnabled = true
        collectionView.dragDelegate = tabDataSource
        collectionView.dropDelegate = tabDataSource

        view.addSubview(collectionView)
        view.addSubview(toolbar)

        makeConstraints()

        view.insertSubview(emptyPrivateTabsView, aboveSubview: collectionView)
        emptyPrivateTabsView.snp.makeConstraints { make in
            make.top.left.right.equalTo(self.collectionView)
            make.bottom.equalTo(self.toolbar.snp.top)
        }

        if let tab = tabManager.selectedTab, tab.isPrivate {
            privateMode = true
        }

        // XXX: Bug 1447726 - Temporarily disable 3DT in tabs tray
        // register for previewing delegate to enable peek and pop if force touch feature available
        // if traitCollection.forceTouchCapability == .available {
        //     registerForPreviewing(with: self, sourceView: view)
        // }

        emptyPrivateTabsView.isHidden = !privateTabsAreEmpty()

        NotificationCenter.default.addObserver(self, selector: #selector(appWillResignActiveNotification), name: .UIApplicationWillResignActive, object: nil)
        NotificationCenter.default.addObserver(self, selector: #selector(appDidBecomeActiveNotification), name: .UIApplicationDidBecomeActive, object: nil)
        NotificationCenter.default.addObserver(self, selector: #selector(dynamicFontChanged), name: .DynamicFontChanged, object: nil)
        
        applyTheme(privateMode ? .private : .regular)
    }

    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
    }

    override func viewDidDisappear(_ animated: Bool) {
        super.viewDidDisappear(animated)
    }

    override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
        super.traitCollectionDidChange(previousTraitCollection)

        // Update the trait collection we reference in our layout delegate
        tabLayoutDelegate.traitCollection = traitCollection
        self.collectionView.collectionViewLayout.invalidateLayout()
    }

    fileprivate func cancelExistingGestures() {
        if let visibleCells = self.collectionView.visibleCells as? [TabCell] {
            for cell in visibleCells {
                cell.animator.cancelExistingGestures()
            }
        }
    }

    override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
        super.viewWillTransition(to: size, with: coordinator)
        coordinator.animate(alongsideTransition: { _ in
            self.collectionView.collectionViewLayout.invalidateLayout()
        }, completion: nil)
    }

    override var preferredStatusBarStyle: UIStatusBarStyle {
        return privateMode ? .lightContent : .default
    }

    fileprivate func makeConstraints() {
        collectionView.snp.makeConstraints { make in
            make.left.equalTo(view.safeArea.left)
            make.right.equalTo(view.safeArea.right)
            make.bottom.equalTo(view.safeArea.bottom)
            make.top.equalTo(view.safeArea.top)
        }

        toolbar.snp.makeConstraints { make in
            make.left.right.bottom.equalTo(view)
            make.height.equalTo(UIConstants.BottomToolbarHeight)
        }
    }
    
    func applyTheme(_ theme: Theme) {
        collectionView?.backgroundColor = TabTrayControllerUX.BackgroundColor.colorFor(theme)
        toolbar.applyTheme(theme)
    }

// MARK: Selectors
    @objc func didClickDone() {
        if tabDataSource.tabs.isEmpty {
            openNewTab()
        } else {
            if TabType.of(tabManager.selectedTab).isPrivate != privateMode {
                tabManager.selectTab(tabDataSource.tabs.first!)
            }
            self.navigationController?.popViewController(animated: true)
        }
    }

    @objc func didClickAddTab() {
        openNewTab()
    }

    @objc func didTapLearnMore() {
        let learnMoreRequest = URLRequest(url: URL(string: "https://github.com/brave/browser-laptop/wiki/What-a-Private-Tab-actually-does")!)
        openNewTab(learnMoreRequest)
    }

    @objc func didTogglePrivateMode() {
        let scaleDownTransform = CGAffineTransform(scaleX: 0.9, y: 0.9)

        let newOffset = CGPoint(x: 0.0, y: collectionView.contentOffset.y)
        collectionView.setContentOffset(self.otherBrowsingModeOffset, animated:false)
        self.otherBrowsingModeOffset = newOffset
        let fromView: UIView
        if !privateTabsAreEmpty(), let snapshot = collectionView.snapshotView(afterScreenUpdates: false) {
            snapshot.frame = collectionView.frame
            view.insertSubview(snapshot, aboveSubview: collectionView)
            fromView = snapshot
        } else {
            fromView = emptyPrivateTabsView
        }

        tabManager.willSwitchTabMode(leavingPBM: privateMode)
        privateMode = !privateMode
        // If we are exiting private mode and we have the close private tabs option selected, make sure
        // we clear out all of the private tabs
        let exitingPrivateMode = !privateMode && tabManager.shouldClearPrivateTabs()

        toolbar.privateModeButton.isSelected = privateMode
        collectionView.layoutSubviews()

        let toView: UIView
        if !privateTabsAreEmpty(), let newSnapshot = collectionView.snapshotView(afterScreenUpdates: !exitingPrivateMode) {
            emptyPrivateTabsView.isHidden = true
            //when exiting private mode don't screenshot the collectionview (causes the UI to hang)
            newSnapshot.frame = collectionView.frame
            view.insertSubview(newSnapshot, aboveSubview: fromView)
            collectionView.alpha = 0
            toView = newSnapshot
        } else {
            emptyPrivateTabsView.isHidden = false
            toView = emptyPrivateTabsView
        }
        toView.alpha = 0
        toView.transform = scaleDownTransform

        UIView.animate(withDuration: 0.2, delay: 0, options: [], animations: { () -> Void in
            fromView.transform = scaleDownTransform
            fromView.alpha = 0
            toView.transform = .identity
            toView.alpha = 1
        }) { finished in
            if fromView != self.emptyPrivateTabsView {
                fromView.removeFromSuperview()
            }
            if toView != self.emptyPrivateTabsView {
                toView.removeFromSuperview()
            }
            self.collectionView.alpha = 1
        }
    }

    fileprivate func privateTabsAreEmpty() -> Bool {
        let tabs = tabManager.tabs(withType: .private)
        return privateMode && tabs.isEmpty
    }

    func changePrivacyMode(_ isPrivate: Bool) {
        if isPrivate != privateMode {
            guard let _ = collectionView else {
                privateMode = isPrivate
                return
            }

            didTogglePrivateMode()
        }
    }

    func openNewTab() {
        openNewTab(nil)
    }

    fileprivate func openNewTab(_ request: URLRequest?) {
        toolbar.isUserInteractionEnabled = false

        // We're only doing one update here, but using a batch update lets us delay selecting the tab
        // until after its insert animation finishes.
        var tab: Tab?
        self.collectionView?.performBatchUpdates({
            tab = self.tabManager.addTab(request, isPrivate: self.privateMode)
        }, completion: { finished in
            // The addTab delegate method will pop to the BVC no need to do anything here.
            self.toolbar.isUserInteractionEnabled = true
            if finished, request == nil, NewTabAccessors.getNewTabPage(self.profile.prefs) == .blankPage,
                let appDelegate = UIApplication.shared.delegate as? AppDelegate,
                let bvc = appDelegate.browserViewController {
                DispatchQueue.main.asyncAfter(deadline: .now() + 0.6) {
                    bvc.urlBar.tabLocationViewDidTapLocation(bvc.urlBar.locationView)
                }
            }

            if let tab = tab {
                self.delegate?.tabTrayDidAddTab(self, tab: tab)
            }
        })
    }

    func closeTabsForCurrentTray() {
        tabManager.removeTabsWithUndoToast(tabsToDisplay)
        self.collectionView.reloadData()
    }
}

// MARK: - App Notifications
extension TabTrayController {
    @objc func appWillResignActiveNotification() {
        if privateMode {
            collectionView.alpha = 0
        }
    }

    @objc func appDidBecomeActiveNotification() {
        // Re-show any components that might have been hidden because they were being displayed
        // as part of a private mode tab
        UIView.animate(withDuration: 0.2, delay: 0, options: [], animations: {
            self.collectionView.alpha = 1
        },
        completion: nil)
    }
}

extension TabTrayController: TabSelectionDelegate {
    func didSelectTabAtIndex(_ index: Int) {
        let tab = tabsToDisplay[index]
        tabManager.selectTab(tab)
        _ = self.navigationController?.popViewController(animated: true)
    }
}

extension TabTrayController: PresentingModalViewControllerDelegate {
    func dismissPresentedModalViewController(_ modalViewController: UIViewController, animated: Bool) {
        dismiss(animated: animated, completion: { self.collectionView.reloadData() })
    }
}

extension TabTrayController: TabManagerDelegate {
    func tabManager(_ tabManager: TabManager, didSelectedTabChange selected: Tab?, previous: Tab?) {
        tabDataSource.isDragging = false

        // Redraw the cells representing the selected (and recently unselected) tabs.
        let tabs = tabDataSource.tabs

        // Only redraw if there is more than one tab in the tray.
        guard tabs.count > 1 else {
            return
        }

        let updated = [ selected, previous ]
            .compactMap { $0 }
            .compactMap { tabs.index(of: $0) }
            .map { IndexPath(item: $0, section: 0) }

        assertIsMainThread("Changing selected tab is on main thread")
        collectionView?.performBatchUpdates({
            self.collectionView.reloadItems(at: updated)

            if !updated.isEmpty {
                self.collectionView.scrollToItem(at: updated[0], at: [.centeredHorizontally, .centeredVertically], animated: true)
            }
        })
    }

    func tabManager(_ tabManager: TabManager, willAddTab tab: Tab) {
        tabDataSource.isDragging = false
    }

    func tabManager(_ tabManager: TabManager, willRemoveTab tab: Tab) {
        tabDataSource.isDragging = false
    }

    func tabManager(_ tabManager: TabManager, didAddTab tab: Tab) {
        // Get the index of the added tab from it's set (private or normal)
        guard let index = tabsToDisplay.index(of: tab) else { return }
        if !privateTabsAreEmpty() {
            emptyPrivateTabsView.isHidden = true
        }

        tabDataSource.addTab(tab)
        self.collectionView?.performBatchUpdates({
            self.collectionView.insertItems(at: [IndexPath(item: index, section: 0)])
        }, completion: { finished in
            if finished {
                tabManager.selectTab(tab)
                // don't pop the tab tray view controller if it is not in the foreground
                if self.presentedViewController == nil {
                    _ = self.navigationController?.popViewController(animated: true)
                }
            }
        })
    }

    func tabManager(_ tabManager: TabManager, didRemoveTab tab: Tab) {
        // it is possible that we are removing a tab that we are not currently displaying
        // through the Close All Tabs feature (which will close tabs that are not in our current privacy mode)
        // check this before removing the item from the collection
        let removedIndex = tabDataSource.removeTab(tab)
        if removedIndex > -1 {
            self.collectionView.performBatchUpdates({
                self.collectionView.deleteItems(at: [IndexPath(item: removedIndex, section: 0)])
            }, completion: { finished in
                guard finished && self.privateTabsAreEmpty() else { return }
                self.emptyPrivateTabsView.isHidden = false
            })
        }
    }

    func tabManagerDidAddTabs(_ tabManager: TabManager) {
    }

    func tabManagerDidRestoreTabs(_ tabManager: TabManager) {
    }
    
    func tabManagerDidRemoveAllTabs(_ tabManager: TabManager, toast: ButtonToast?) {
        guard privateMode else {
            return
        }

        if let toast = toast {
            view.addSubview(toast)
            toast.showToast(makeConstraints: { make in
                make.left.right.equalTo(self.view)
                make.bottom.equalTo(self.toolbar.snp.top)
            })
        }
    }
}

extension TabTrayController: UIScrollViewAccessibilityDelegate {
    func accessibilityScrollStatus(for scrollView: UIScrollView) -> String? {
        guard var visibleCells = collectionView.visibleCells as? [TabCell] else { return nil }
        var bounds = collectionView.bounds
        bounds = bounds.offsetBy(dx: collectionView.contentInset.left, dy: collectionView.contentInset.top)
        bounds.size.width -= collectionView.contentInset.left + collectionView.contentInset.right
        bounds.size.height -= collectionView.contentInset.top + collectionView.contentInset.bottom
        // visible cells do sometimes return also not visible cells when attempting to go past the last cell with VoiceOver right-flick gesture; so make sure we have only visible cells (yeah...)
        visibleCells = visibleCells.filter { !$0.frame.intersection(bounds).isEmpty }

        let cells = visibleCells.map { self.collectionView.indexPath(for: $0)! }
        let indexPaths = cells.sorted { (a: IndexPath, b: IndexPath) -> Bool in
            return a.section < b.section || (a.section == b.section && a.row < b.row)
        }

        if indexPaths.count == 0 {
            return NSLocalizedString("No tabs", comment: "Message spoken by VoiceOver to indicate that there are no tabs in the Tabs Tray")
        }

        let firstTab = indexPaths.first!.row + 1
        let lastTab = indexPaths.last!.row + 1
        let tabCount = collectionView.numberOfItems(inSection: 0)

        if firstTab == lastTab {
            let format = NSLocalizedString("Tab %@ of %@", comment: "Message spoken by VoiceOver saying the position of the single currently visible tab in Tabs Tray, along with the total number of tabs. E.g. \"Tab 2 of 5\" says that tab 2 is visible (and is the only visible tab), out of 5 tabs total.")
            return String(format: format, NSNumber(value: firstTab as Int), NSNumber(value: tabCount as Int))
        } else {
            let format = NSLocalizedString("Tabs %@ to %@ of %@", comment: "Message spoken by VoiceOver saying the range of tabs that are currently visible in Tabs Tray, along with the total number of tabs. E.g. \"Tabs 8 to 10 of 15\" says tabs 8, 9 and 10 are visible, out of 15 tabs total.")
            return String(format: format, NSNumber(value: firstTab as Int), NSNumber(value: lastTab as Int), NSNumber(value: tabCount as Int))
        }
    }
}

extension TabTrayController: SwipeAnimatorDelegate {
    func swipeAnimator(_ animator: SwipeAnimator, viewWillExitContainerBounds: UIView) {
        guard let tabCell = animator.animatingView as? TabCell, let indexPath = collectionView.indexPath(for: tabCell) else { return }

        let tab = tabsToDisplay[indexPath.item]
        tabManager.removeTab(tab)
        UIAccessibilityPostNotification(UIAccessibilityAnnouncementNotification, NSLocalizedString("Closing tab", comment: "Accessibility label (used by assistive technology) notifying the user that the tab is being closed."))
    }
}

extension TabTrayController: TabCellDelegate {
    func tabCellDidClose(_ cell: TabCell) {
        let indexPath = collectionView.indexPath(for: cell)!
        let tab = tabsToDisplay[indexPath.item]
        tabManager.removeTab(tab)
    }
}

fileprivate class TabManagerDataSource: NSObject, UICollectionViewDataSource {
    unowned var cellDelegate: TabCellDelegate & SwipeAnimatorDelegate
    fileprivate var tabs: [Tab]
    fileprivate var tabManager: TabManager
    fileprivate var isDragging = false

    init(tabs: [Tab], cellDelegate: TabCellDelegate & SwipeAnimatorDelegate, tabManager: TabManager) {
        self.cellDelegate = cellDelegate
        self.tabs = tabs
        self.tabManager = tabManager
        super.init()
    }

    /**
     Removes the given tab from the data source

     - parameter tab: Tab to remove

     - returns: The index of the removed tab, -1 if tab did not exist
     */
    func removeTab(_ tabToRemove: Tab) -> Int {
        var index: Int = -1
        for (i, tab) in tabs.enumerated() where tabToRemove === tab {
            index = i
            tabs.remove(at: index)
            break
        }
        return index
    }

    /**
     Adds the given tab to the data source

     - parameter tab: Tab to add
     */
    func addTab(_ tab: Tab) {
        tabs.append(tab)
    }

    @objc func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
        guard let tabCell = collectionView.dequeueReusableCell(withReuseIdentifier: TabCell.Identifier, for: indexPath) as? TabCell else { return UICollectionViewCell() }
        tabCell.animator.delegate = cellDelegate
        tabCell.delegate = cellDelegate

        let tab = tabs[indexPath.item]

        tabCell.titleText.text = tab.displayTitle

        if !tab.displayTitle.isEmpty {
            tabCell.accessibilityLabel = tab.displayTitle
        } else {
            tabCell.accessibilityLabel = tab.url?.aboutComponent ?? "" // If there is no title we are most likely on a home panel.
        }
        tabCell.isAccessibilityElement = true
        tabCell.accessibilityHint = NSLocalizedString("Swipe right or left with three fingers to close the tab.", comment: "Accessibility hint for tab tray's displayed tab.")

        if let favIcon = tab.displayFavicon, let url = URL(string: favIcon.url) {
            tabCell.favicon.sd_setImage(with: url, placeholderImage: #imageLiteral(resourceName: "defaultFavicon"), options: [], completed: nil)
        } else {
            tabCell.favicon.image = #imageLiteral(resourceName: "defaultFavicon")

            if tab.isPrivate {
                tabCell.favicon.tintColor = UIColor.Photon.White100
            }
        }

        if tab == tabManager.selectedTab {
            tabCell.setTabSelected(tab)
        }

        tabCell.screenshotView.image = tab.screenshot

        return tabCell
    }

    @objc func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
        return tabs.count
    }
}

@available(iOS 11.0, *)
extension TabManagerDataSource: UICollectionViewDragDelegate {
    func collectionView(_ collectionView: UICollectionView, dragSessionWillBegin session: UIDragSession) {
        isDragging = true
    }

    func collectionView(_ collectionView: UICollectionView, dragSessionDidEnd session: UIDragSession) {
        isDragging = false
    }

    func collectionView(_ collectionView: UICollectionView, itemsForBeginning session: UIDragSession, at indexPath: IndexPath) -> [UIDragItem] {
        let tab = tabs[indexPath.item]

        // Get the tab's current URL. If it is `nil`, check the `sessionData` since
        // it may be a tab that has not been restored yet.
        var url = tab.url
        if url == nil, let sessionData = tab.sessionData {
            let urls = sessionData.urls
            let index = sessionData.currentPage + urls.count - 1
            if index < urls.count {
                url = urls[index]
            }
        }

        // Ensure we actually have a URL for the tab being dragged and that the URL is not local.
        // If not, just create an empty `NSItemProvider` so we can create a drag item with the
        // `Tab` so that it can at still be re-ordered.
        var itemProvider: NSItemProvider
        if url != nil, !(url?.isLocal ?? true) {
            itemProvider = NSItemProvider(contentsOf: url) ?? NSItemProvider()
        } else {
            itemProvider = NSItemProvider()
        }

        let dragItem = UIDragItem(itemProvider: itemProvider)
        dragItem.localObject = tab
        return [dragItem]
    }
}

@available(iOS 11.0, *)
extension TabManagerDataSource: UICollectionViewDropDelegate {
    func collectionView(_ collectionView: UICollectionView, performDropWith coordinator: UICollectionViewDropCoordinator) {
        guard isDragging, let destinationIndexPath = coordinator.destinationIndexPath, let dragItem = coordinator.items.first?.dragItem, let tab = dragItem.localObject as? Tab, let sourceIndex = tabs.index(of: tab) else {
            return
        }

        coordinator.drop(dragItem, toItemAt: destinationIndexPath)
        isDragging = false

        let destinationIndex = destinationIndexPath.item
        tabManager.moveTab(fromIndex: sourceIndex, toIndex: destinationIndex)
        tabs.insert(tabs.remove(at: sourceIndex), at: destinationIndex)
        collectionView.moveItem(at: IndexPath(item: sourceIndex, section: 0), to: destinationIndexPath)
    }

    func collectionView(_ collectionView: UICollectionView, dropSessionDidUpdate session: UIDropSession, withDestinationIndexPath destinationIndexPath: IndexPath?) -> UICollectionViewDropProposal {
        guard let localDragSession = session.localDragSession, let item = localDragSession.items.first, let tab = item.localObject as? Tab else {
            return UICollectionViewDropProposal(operation: .forbidden)
        }

        // If the tab doesn't exist by the time we get here, we must return a
        // `.cancel` operation continuously until `isDragging` can be reset.
        guard isDragging, tabs.index(of: tab) != nil else {
            isDragging = false
            return UICollectionViewDropProposal(operation: .cancel)
        }

        return UICollectionViewDropProposal(operation: .move, intent: .insertAtDestinationIndexPath)
    }
}

@objc protocol TabSelectionDelegate: class {
    func didSelectTabAtIndex(_ index: Int)
}

fileprivate class TabLayoutDelegate: NSObject, UICollectionViewDelegateFlowLayout {
    weak var tabSelectionDelegate: TabSelectionDelegate?

    fileprivate var traitCollection: UITraitCollection
    fileprivate var profile: Profile
    fileprivate var numberOfColumns: Int {
        // iPhone 4-6+ portrait
        if traitCollection.horizontalSizeClass == .compact && traitCollection.verticalSizeClass == .regular {
            return TabTrayControllerUX.CompactNumberOfColumnsThin
        } else {
            return TabTrayControllerUX.NumberOfColumnsWide
        }
    }

    init(profile: Profile, traitCollection: UITraitCollection) {
        self.profile = profile
        self.traitCollection = traitCollection
        super.init()
    }

    fileprivate func cellHeightForCurrentDevice() -> CGFloat {
        let shortHeight = TabTrayControllerUX.TextBoxHeight * 6

        if self.traitCollection.verticalSizeClass == .compact {
            return shortHeight
        } else if self.traitCollection.horizontalSizeClass == .compact {
            return shortHeight
        } else {
            return TabTrayControllerUX.TextBoxHeight * 8
        }
    }

    @objc func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, minimumInteritemSpacingForSectionAt section: Int) -> CGFloat {
        return TabTrayControllerUX.Margin
    }

    @objc func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, sizeForItemAt indexPath: IndexPath) -> CGSize {
        let cellWidth = floor((collectionView.bounds.width - TabTrayControllerUX.Margin * CGFloat(numberOfColumns + 1)) / CGFloat(numberOfColumns))
        return CGSize(width: cellWidth, height: self.cellHeightForCurrentDevice())
    }

    @objc func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, insetForSectionAt section: Int) -> UIEdgeInsets {
        return UIEdgeInsets(equalInset: TabTrayControllerUX.Margin)
    }

    @objc func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, minimumLineSpacingForSectionAt section: Int) -> CGFloat {
        return TabTrayControllerUX.Margin
    }

    @objc func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
        tabSelectionDelegate?.didSelectTabAtIndex(indexPath.row)
    }
}

private struct EmptyPrivateTabsViewUX {
    static let TitleColor = UIColor.Photon.White100
    static let TitleFont = UIFont.systemFont(ofSize: 22, weight: UIFont.Weight.medium)
    static let DescriptionColor = UIColor.Photon.White100
    static let DescriptionFont = UIFont.systemFont(ofSize: 14)
    static let LearnMoreFont = UIFont.systemFont(ofSize: 17, weight: UIFont.Weight.medium)
    static let TextMargin: CGFloat = 18
    static let LearnMoreMargin: CGFloat = 15
    static let DescriptionMargin: CGFloat = 20
    static let MinBottomMargin: CGFloat = 10
}

// View we display when there are no private tabs created
fileprivate class EmptyPrivateTabsView: UIView {
    fileprivate lazy var titleLabel: UILabel = {
        let label = UILabel()
        label.textColor = EmptyPrivateTabsViewUX.TitleColor
        label.font = EmptyPrivateTabsViewUX.TitleFont
        label.textAlignment = .center
        return label
    }()

    fileprivate var descriptionLabel: UILabel = {
        let label = UILabel()
        label.textColor = EmptyPrivateTabsViewUX.DescriptionColor
        label.font = EmptyPrivateTabsViewUX.DescriptionFont
        label.textAlignment = .center
        label.numberOfLines = 0
        return label
    }()

    fileprivate var learnMoreButton: UIButton = {
        let button = UIButton(type: .system)
        button.setTitle(Strings.Private_Tab_Link, for: [])
        button.setTitleColor(UIConstants.PrivateModeTextHighlightColor, for: [])
        button.titleLabel?.font = EmptyPrivateTabsViewUX.LearnMoreFont
        return button
    }()

    fileprivate var iconImageView: UIImageView = {
        let imageView = UIImageView(image: #imageLiteral(resourceName: "private_glasses"))
        return imageView
    }()

    override init(frame: CGRect) {
        super.init(frame: frame)

        titleLabel.text =  Strings.Private_Tab_Title
        descriptionLabel.text = Strings.Private_Tab_Body

        addSubview(titleLabel)
        addSubview(descriptionLabel)
        addSubview(iconImageView)
        addSubview(learnMoreButton)

        titleLabel.snp.makeConstraints { make in
            make.center.equalTo(self)
        }

        iconImageView.snp.makeConstraints { make in
            make.bottom.equalTo(titleLabel.snp.top).offset(-EmptyPrivateTabsViewUX.TextMargin)
            make.centerX.equalTo(self)
        }

        descriptionLabel.snp.makeConstraints { make in
            make.left.right.equalTo(self).inset(EmptyPrivateTabsViewUX.DescriptionMargin)
            make.top.equalTo(titleLabel.snp.bottom).offset(EmptyPrivateTabsViewUX.TextMargin)
            make.centerX.equalTo(self)
        }

        learnMoreButton.snp.makeConstraints { (make) -> Void in
            make.top.equalTo(descriptionLabel.snp.bottom).offset(EmptyPrivateTabsViewUX.LearnMoreMargin).priority(10)
            make.bottom.lessThanOrEqualTo(self).offset(-EmptyPrivateTabsViewUX.MinBottomMargin).priority(1000)
            make.centerX.equalTo(self)
        }
    }

    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
}

extension TabTrayController: TabPeekDelegate {

    func tabPeekDidAddBookmark(_ tab: Tab) {
        delegate?.tabTrayDidAddBookmark(tab)
    }

    func tabPeekDidCloseTab(_ tab: Tab) {
        if let index = self.tabDataSource.tabs.index(of: tab),
            let cell = self.collectionView?.cellForItem(at: IndexPath(item: index, section: 0)) as? TabCell {
            cell.close()
        }
    }

    func tabPeekRequestsPresentationOf(_ viewController: UIViewController) {
        delegate?.tabTrayRequestsPresentationOf(viewController)
    }
}

extension TabTrayController: UIViewControllerPreviewingDelegate {

    func previewingContext(_ previewingContext: UIViewControllerPreviewing, viewControllerForLocation location: CGPoint) -> UIViewController? {
        guard let collectionView = collectionView else { return nil }

        let convertedLocation = self.view.convert(location, to: collectionView)

        guard let indexPath = collectionView.indexPathForItem(at: convertedLocation),
            let cell = collectionView.cellForItem(at: indexPath) else { return nil }

        let tab = tabDataSource.tabs[indexPath.row]
        let tabVC = TabPeekViewController(tab: tab, delegate: self)
        if let browserProfile = profile as? BrowserProfile {
            tabVC.setState(withProfile: browserProfile)
        }
        previewingContext.sourceRect = self.view.convert(cell.frame, from: collectionView)

        return tabVC
    }

    func previewingContext(_ previewingContext: UIViewControllerPreviewing, commit viewControllerToCommit: UIViewController) {
        guard let tpvc = viewControllerToCommit as? TabPeekViewController else { return }
        tabManager.selectTab(tpvc.tab)
        navigationController?.popViewController(animated: true)
        delegate?.tabTrayDidDismiss(self)
    }
}

extension TabTrayController: UIAdaptivePresentationControllerDelegate, UIPopoverPresentationControllerDelegate {
    // Returning None here makes sure that the Popover is actually presented as a Popover and
    // not as a full-screen modal, which is the default on compact device classes.
    func adaptivePresentationStyle(for controller: UIPresentationController, traitCollection: UITraitCollection) -> UIModalPresentationStyle {
        return .none
    }
}

// MARK: - Toolbar
class TrayToolbar: UIView {
    fileprivate let toolbarButtonSize = CGSize(width: 44, height: 44)

    let addTabButton = UIButton(type: .system).then {
        $0.setImage(#imageLiteral(resourceName: "add_tab").template, for: .normal)
        $0.accessibilityLabel = NSLocalizedString("Add Tab", comment: "Accessibility label for the Add Tab button in the Tab Tray.")
        $0.accessibilityIdentifier = "TabTrayController.addTabButton"
    }
    
    let doneButton = UIButton(type: .system).then {
        $0.setTitle(Strings.Done, for: .normal)
        $0.titleLabel?.font = TabTrayControllerUX.ToolbarFont
        $0.accessibilityLabel = Strings.Done
        $0.accessibilityIdentifier = "TabTrayController.doneButton"
    }

    let privateModeButton = PrivateModeButton().then {
        $0.titleLabel?.font = TabTrayControllerUX.ToolbarFont
        $0.setTitle(Strings.Private, for: .normal)
    }
    
    fileprivate let sideOffset: CGFloat = 22

    fileprivate override init(frame: CGRect) {
        super.init(frame: frame)
        backgroundColor = .white
        addSubview(addTabButton)
        addSubview(doneButton)

        var buttonToCenter: UIButton?
        buttonToCenter = addTabButton
        
        privateModeButton.accessibilityIdentifier = "TabTrayController.maskButton"

        buttonToCenter?.snp.makeConstraints { make in
            make.centerX.equalTo(self)
            make.top.equalTo(self)
            make.size.equalTo(toolbarButtonSize)
        }

        doneButton.snp.makeConstraints { make in
            make.centerY.equalTo(self)
            make.trailing.equalTo(self).offset(-sideOffset)
        }

        addSubview(privateModeButton)
        privateModeButton.snp.makeConstraints { make in
            make.centerY.equalTo(self)
            make.leading.equalTo(self).offset(sideOffset)
        }
    }

    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    fileprivate func applyTheme(_ theme: Theme) {
        UIApplication.shared.windows.first?.backgroundColor = TabTrayControllerUX.BackgroundColor.colorFor(theme)
        addTabButton.tintColor = UIColor.TabTray.ToolbarButtonTint.colorFor(theme) // Needs to be changed
        doneButton.tintColor = UIColor.TabTray.ToolbarButtonTint.colorFor(theme)
        backgroundColor = TabTrayControllerUX.BackgroundColor.colorFor(theme)
        privateModeButton.applyTheme(theme)
    }
}
