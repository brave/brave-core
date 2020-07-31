/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import SnapKit
import Storage
import Shared
import BraveShared

struct TabTrayControllerUX {
    static let cornerRadius = CGFloat(6.0)
    static let defaultBorderWidth = 1.0 / UIScreen.main.scale
    static let toolbarFont = UIFont.systemFont(ofSize: 17.0, weight: .medium)
    static let textBoxHeight = CGFloat(32.0)
    static let faviconSize = CGFloat(20)
    static let margin = CGFloat(15)
    static let closeButtonSize = CGFloat(32)
    static let closeButtonMargin = CGFloat(6.0)
    static let closeButtonEdgeInset = CGFloat(7)

    static let numberOfColumnsThin = 1
    static let numberOfColumnsWide = 3
    static let compactNumberOfColumnsThin = 2

    static let menuFixedWidth: CGFloat = 320
}

private struct LightTabCellUX {
    static let tabTitleTextColor = UIColor.black
}

private struct DarkTabCellUX {
    static let tabTitleTextColor = UIColor.Photon.white100
}

protocol TabCellDelegate: class {
    func tabCellDidClose(_ cell: TabCell)
}

class TabCell: UICollectionViewCell, Themeable {
    static let identifier = "TabCellIdentifier"
    static let borderWidth: CGFloat = 3

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
        self.backgroundHolder.backgroundColor = .white
        self.backgroundHolder.layer.cornerRadius = TabTrayControllerUX.cornerRadius
        self.backgroundHolder.clipsToBounds = true

        self.screenshotView.contentMode = .scaleAspectFill
        self.screenshotView.clipsToBounds = true
        self.screenshotView.isUserInteractionEnabled = false
        self.screenshotView.alignLeft = true
        self.screenshotView.alignTop = true
        screenshotView.backgroundColor = UIConstants.appBackgroundColor

        self.favicon.backgroundColor = UIColor.clear
        self.favicon.layer.cornerRadius = 2.0
        self.favicon.layer.masksToBounds = true

        self.titleText = UILabel()
        self.titleText.isUserInteractionEnabled = false
        self.titleText.numberOfLines = 1
        self.titleText.font = DynamicFontHelper.defaultHelper.DefaultSmallFontBold
        self.titleText.textColor = LightTabCellUX.tabTitleTextColor
        self.titleText.backgroundColor = .clear

        self.closeButton = UIButton()
        self.closeButton.setImage(#imageLiteral(resourceName: "tab_close"), for: [])
        self.closeButton.imageView?.contentMode = .scaleAspectFit
        self.closeButton.contentMode = .center
        self.closeButton.imageEdgeInsets = UIEdgeInsets(equalInset: TabTrayControllerUX.closeButtonEdgeInset)

        super.init(frame: frame)
        
        self.animator = SwipeAnimator(animatingView: self)
        self.closeButton.addTarget(self, action: #selector(close), for: .touchUpInside)

        layer.borderColor = UIColor.Photon.grey90A20.cgColor
        layer.borderWidth = TabTrayControllerUX.defaultBorderWidth
        layer.cornerRadius = TabTrayControllerUX.cornerRadius
        
        contentView.addSubview(backgroundHolder)
        backgroundHolder.addSubview(self.screenshotView)
        backgroundHolder.addSubview(self.titleBackgroundView)
        
        titleBackgroundView.addSubview(self.closeButton)
        titleBackgroundView.addSubview(self.titleText)
        titleBackgroundView.addSubview(self.favicon)

        self.accessibilityCustomActions = [
            UIAccessibilityCustomAction(name: Strings.tabAccessibilityCloseActionLabel, target: self.animator, selector: #selector(SwipeAnimator.closeWithoutGesture))
        ]
    }

    func setTabSelected(_ tab: Tab) {
        layer.shadowColor = UIConstants.systemBlueColor.cgColor
        layer.shadowOpacity = 1
        layer.shadowRadius = 0 // A 0 radius creates a solid border instead of a gradient blur
        layer.masksToBounds = false
        // create a frame that is "BorderWidth" size bigger than the cell
        layer.shadowOffset = CGSize(width: -TabCell.borderWidth, height: -TabCell.borderWidth)
        let shadowPath = CGRect(width: layer.frame.width + (TabCell.borderWidth * 2), height: layer.frame.height + (TabCell.borderWidth * 2))
        layer.shadowPath = UIBezierPath(roundedRect: shadowPath, cornerRadius: TabTrayControllerUX.cornerRadius+TabCell.borderWidth).cgPath
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
            make.height.equalTo(TabTrayControllerUX.textBoxHeight + 15.0)
        }

        favicon.snp.makeConstraints { make in
            make.leading.equalTo(titleBackgroundView).offset(6)
            make.top.equalTo((TabTrayControllerUX.textBoxHeight - TabTrayControllerUX.faviconSize) / 2)
            make.size.equalTo(TabTrayControllerUX.faviconSize)
        }

        titleText.snp.makeConstraints { (make) in
            make.leading.equalTo(favicon.snp.trailing).offset(6)
            make.trailing.equalTo(closeButton.snp.leading).offset(-6)
            make.centerY.equalTo(favicon)
        }

        closeButton.snp.makeConstraints { make in
            make.size.equalTo(TabTrayControllerUX.closeButtonSize)
            make.trailing.equalTo(titleBackgroundView)
            make.centerY.equalTo(favicon)
        }

        let shadowPath = CGRect(width: layer.frame.width + (TabCell.borderWidth * 2), height: layer.frame.height + (TabCell.borderWidth * 2))
        layer.shadowPath = UIBezierPath(roundedRect: shadowPath, cornerRadius: TabTrayControllerUX.cornerRadius+TabCell.borderWidth).cgPath
    }

    override func prepareForReuse() {
        // Reset any close animations.
        backgroundHolder.transform = .identity
        backgroundHolder.alpha = 1
        self.titleText.font = DynamicFontHelper.defaultHelper.DefaultSmallFontBold
        layer.shadowOffset = .zero
        layer.shadowPath = nil
        layer.shadowOpacity = 0
        layer.borderWidth = TabTrayControllerUX.defaultBorderWidth
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
    
    func applyTheme(_ theme: Theme) {
        styleChildren(theme: theme)
        
        // TabCell doesn't use much theming atm, using non-themable values for some views here.
        titleText.appearanceTextColor = .black
        screenshotView.backgroundColor = backgroundHolder.backgroundColor
        favicon.tintColor = theme.colors.tints.home
    }
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
            // Should be set immediately before other logic executes
            PrivateBrowsingManager.shared.isPrivateBrowsing = privateMode
            tabDataSource.tabs = tabManager.tabsForCurrentMode
            
            // This is a little tricky since this menu is one of the only places inside of the appliation
            //  that has a state without gauranteeing a tab exists. Most UI elements should use `theme` or at the least
            //  the related tab's theme, here this is not possible, so using hardened values 😕😭
            applyTheme(Theme.of(nil))
            toolbar.privateModeButton.isSelected = privateMode
            collectionView?.reloadData()
            setNeedsStatusBarAppearanceUpdate()
        }
    }

    fileprivate lazy var emptyPrivateTabsView: EmptyPrivateTabsView = {
        let emptyView = EmptyPrivateTabsView()
        emptyView.learnMoreButton.addTarget(self, action: #selector(didTapLearnMore), for: .touchUpInside)
        return emptyView
    }()

    fileprivate lazy var tabDataSource: TabManagerDataSource = {
        return TabManagerDataSource(tabs: tabManager.tabsForCurrentMode, cellDelegate: self, tabManager: self.tabManager)
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
        
        Preferences.Privacy.privateBrowsingOnly.observe(from: self)
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
        guard notification.name == .dynamicFontChanged else { return }

        self.collectionView.reloadData()
    }

    // MARK: View Controller Callbacks
    override func viewDidLoad() {
        super.viewDidLoad()

        view.accessibilityLabel = Strings.tabTrayAccessibilityLabel
        
        collectionView = UICollectionView(frame: view.frame, collectionViewLayout: UICollectionViewFlowLayout())
        
        collectionView.dataSource = tabDataSource
        collectionView.delegate = tabLayoutDelegate
        collectionView.contentInset = UIEdgeInsets(top: 0, left: 0, bottom: UIConstants.bottomToolbarHeight, right: 0)
        collectionView.register(TabCell.self, forCellWithReuseIdentifier: TabCell.identifier)

        collectionView.dragInteractionEnabled = true
        collectionView.dragDelegate = tabDataSource
        collectionView.dropDelegate = tabDataSource

        view.addSubview(collectionView)
        view.addSubview(toolbar)
        
        updatePrivateModeButtonVisibility()

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

        NotificationCenter.default.addObserver(self, selector: #selector(appWillResignActiveNotification), name: UIApplication.willResignActiveNotification, object: nil)
        NotificationCenter.default.addObserver(self, selector: #selector(appDidBecomeActiveNotification), name: UIApplication.didBecomeActiveNotification, object: nil)
        NotificationCenter.default.addObserver(self, selector: #selector(dynamicFontChanged), name: .dynamicFontChanged, object: nil)
        
        if #available(iOS 13.0, *) {
            /// At this point in time the `UITraitCollection.current` is not correct so the theme
            /// that `Theme.of(_:)` gets does not get the current theme in automatic-theme mode
            let oldTraitCollection = UITraitCollection.current
            UITraitCollection.current = traitCollection
            applyTheme(Theme.of(tabManager.selectedTab))
            UITraitCollection.current = oldTraitCollection
        } else {
            applyTheme(Theme.of(tabManager.selectedTab))
        }
    }

    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
    }

    override func viewDidDisappear(_ animated: Bool) {
        super.viewDidDisappear(animated)
        if privateMode {
            resetEmptyPrivateBrowsingView()
        }
    }

    override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
        super.traitCollectionDidChange(previousTraitCollection)
        
        if #available(iOS 13.0, *) {
            if traitCollection.userInterfaceStyle != previousTraitCollection?.userInterfaceStyle {
                applyTheme(Theme.of(tabManager.selectedTab))
            }
        }

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
            make.left.right.equalTo(view.safeAreaLayoutGuide)
            make.bottom.equalTo(view)
            make.height.equalTo(UIConstants.bottomToolbarHeight)
        }
    }
    
    var themeableChildren: [Themeable?]? {
        let cells = collectionView?.visibleCells.compactMap({ $0 as? TabCell }) ?? []
        return [toolbar] + cells
    }
    
    func applyTheme(_ theme: Theme) {
        styleChildren(theme: theme)
        view.backgroundColor = theme.colors.home
        collectionView?.backgroundColor = theme.colors.home
    }
    
    /// Reset the empty private browsing state (hide the details, unhide the learn more button) if it was changed
    func resetEmptyPrivateBrowsingView() {
        emptyPrivateTabsView.detailsLabel.isHidden = true
        emptyPrivateTabsView.learnMoreButton.isHidden = false
    }

// MARK: Selectors
    @objc func didClickDone() {
        if tabDataSource.tabs.isEmpty {
            openNewTab()
        } else {
            if tabManager.selectedTab == nil || TabType.of(tabManager.selectedTab).isPrivate != privateMode {
                tabManager.selectTab(tabDataSource.tabs.first!)
            }
            self.navigationController?.popViewController(animated: true)
        }
    }

    @objc func didClickAddTab() {
        openNewTab()
    }

    @objc func didTapLearnMore() {
        self.emptyPrivateTabsView.detailsLabel.alpha = 0.0
        UIView.animate(withDuration: 0.15, animations: {
            self.emptyPrivateTabsView.learnMoreButton.isHidden = true
            self.emptyPrivateTabsView.detailsLabel.isHidden = false
        }, completion: { _ in
                UIView.animate(withDuration: 0.15) {
                    self.emptyPrivateTabsView.detailsLabel.alpha = 1.0
                }
        })
        // Needs to be done in a separate call so the stack view's height is updated properly based on the hidden
        // status of learnMoreButton/detailsLabel
        UIView.animate(withDuration: 0.2) {
            self.emptyPrivateTabsView.updateContentInset()
        }
    }

    @objc func didTogglePrivateMode() {
        let scaleDownTransform = CGAffineTransform(scaleX: 0.9, y: 0.9)

        let newOffset = CGPoint(x: 0.0, y: collectionView.contentOffset.y)
        collectionView.setContentOffset(self.otherBrowsingModeOffset, animated: false)
        self.otherBrowsingModeOffset = newOffset
        let fromView: UIView
        if !privateTabsAreEmpty(), let snapshot = collectionView.snapshotView(afterScreenUpdates: false) {
            snapshot.frame = collectionView.frame
            view.insertSubview(snapshot, aboveSubview: collectionView)
            fromView = snapshot
        } else {
            fromView = emptyPrivateTabsView
        }

        // If we are exiting private mode and we have the close private tabs option selected, make sure
        // we clear out all of the private tabs
        tabManager.willSwitchTabMode(leavingPBM: privateMode)
        privateMode = !privateMode
        
        // When we switch from Private => Regular make sure we reset _selectedIndex, fix for bug #888
        tabManager.resetSelectedIndex()
        
        collectionView.layoutSubviews()

        let toView: UIView
        if !privateTabsAreEmpty(), let newSnapshot = collectionView.snapshotView(afterScreenUpdates: true) {
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
        let isPrivate = PrivateBrowsingManager.shared.isPrivateBrowsing
        return isPrivate && tabManager.tabsForCurrentMode.isEmpty
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
            if finished, request == nil, NewTabAccessors.getNewTabPage() == .blankPage,
                let appDelegate = UIApplication.shared.delegate as? AppDelegate,
                let bvc = appDelegate.browserViewController {
                DispatchQueue.main.asyncAfter(deadline: .now() + 0.6) {
                    bvc.topToolbar.tabLocationViewDidTapLocation(bvc.topToolbar.locationView)
                }
            }

            if let tab = tab {
                self.delegate?.tabTrayDidAddTab(self, tab: tab)
            }
        })
    }

    func closeTabsForCurrentTray() {
        tabManager.removeTabsWithUndoToast(tabManager.tabsForCurrentMode)
        self.collectionView.reloadData()
    }
    
    func updatePrivateModeButtonVisibility() {
        toolbar.privateModeButton.isHidden = Preferences.Privacy.privateBrowsingOnly.value
    }
    
    private func updateApplicationShortcuts() {
        if let delegate = UIApplication.shared.delegate as? AppDelegate {
            delegate.updateShortcutItems(UIApplication.shared)
        }
    }
}

extension TabTrayController: PreferencesObserver {
    func preferencesDidChange(for key: String) {
        if key == Preferences.Privacy.privateBrowsingOnly.key {
            updatePrivateModeButtonVisibility()
            updateApplicationShortcuts()
        }
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
        let tab = tabManager.tabsForCurrentMode[index]
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
            .compactMap { tabs.firstIndex(of: $0) }
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
        guard let index = tabManager.tabsForCurrentMode.firstIndex(of: tab) else { return }
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
        if removedIndex > -1, let collectionView = self.collectionView {
            collectionView.performBatchUpdates({
                collectionView.deleteItems(at: [IndexPath(item: removedIndex, section: 0)])
            }, completion: { finished in
                guard self.privateTabsAreEmpty() else { return }
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
            return Strings.tabTrayEmptyVoiceOverText
        }

        let firstTab = indexPaths.first!.row + 1
        let lastTab = indexPaths.last!.row + 1
        let tabCount = collectionView.numberOfItems(inSection: 0)

        if firstTab == lastTab {
            return String(format: Strings.tabTraySingleTabPositionFormatVoiceOverText, NSNumber(value: firstTab as Int), NSNumber(value: tabCount as Int))
        } else {
            return String(format: Strings.tabTrayMultiTabPositionFormatVoiceOverText, NSNumber(value: firstTab as Int), NSNumber(value: lastTab as Int), NSNumber(value: tabCount as Int))
        }
    }
}

extension TabTrayController: SwipeAnimatorDelegate {
    func swipeAnimator(_ animator: SwipeAnimator, viewWillExitContainerBounds: UIView) {
        guard let tabCell = animator.animatingView as? TabCell, let indexPath = collectionView.indexPath(for: tabCell) else { return }

        let tab = tabManager.tabsForCurrentMode[indexPath.item]
        tabManager.removeTab(tab)
        UIAccessibility.post(notification: .announcement, argument: Strings.tabTrayClosingTabAccessibilityNotificationText)
    }
}

extension TabTrayController: TabCellDelegate {
    func tabCellDidClose(_ cell: TabCell) {
        let indexPath = collectionView.indexPath(for: cell)!
        let tab = tabManager.tabsForCurrentMode[indexPath.item]
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
        guard let tabCell = collectionView.dequeueReusableCell(withReuseIdentifier: TabCell.identifier, for: indexPath) as? TabCell else { return UICollectionViewCell() }
        tabCell.animator.delegate = cellDelegate
        tabCell.delegate = cellDelegate

        let tab = tabs[indexPath.item]

        tabCell.titleText.text = tab.displayTitle
        tabCell.favicon.image = #imageLiteral(resourceName: "defaultFavicon")

        if !tab.displayTitle.isEmpty {
            tabCell.accessibilityLabel = tab.displayTitle
        } else {
            tabCell.accessibilityLabel = tab.url?.aboutComponent ?? "" // If there is no title we are most likely on a home panel.
        }
        tabCell.isAccessibilityElement = true
        tabCell.accessibilityHint = Strings.tabTrayCellCloseAccessibilityHint

        tabCell.favicon.clearMonogramFavicon()
        
        // Tab may not be restored and so may not include a tab URL yet...
        if let favicon = tab.displayFavicon, let url = favicon.url.asURL {
            WebImageCacheManager.shared.load(from: url, completion: { image, _, _, _, loadedURL in
                guard url == loadedURL else { return }
                tabCell.favicon.image = image ?? FaviconFetcher.defaultFaviconImage
            })
        } else if let url = tab.url, !url.isLocal {
            tabCell.favicon.loadFavicon(for: url)
        } else {
            tabCell.favicon.image = #imageLiteral(resourceName: "defaultFavicon")
        }

        if tab == tabManager.selectedTab {
            tabCell.setTabSelected(tab)
        }

        tabCell.screenshotView.image = tab.screenshot
        tabCell.applyTheme(Theme.of(tab))
        
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
        guard isDragging, let destinationIndexPath = coordinator.destinationIndexPath, let dragItem = coordinator.items.first?.dragItem, let tab = dragItem.localObject as? Tab, let sourceIndex = tabs.firstIndex(of: tab) else {
            return
        }

        coordinator.drop(dragItem, toItemAt: destinationIndexPath)
        isDragging = false

        let destinationIndex = destinationIndexPath.item
        tabManager.moveTab(tab, toIndex: destinationIndex)
        tabs.insert(tabs.remove(at: sourceIndex), at: destinationIndex)
        collectionView.moveItem(at: IndexPath(item: sourceIndex, section: 0), to: destinationIndexPath)
    }

    func collectionView(_ collectionView: UICollectionView, dropSessionDidUpdate session: UIDropSession, withDestinationIndexPath destinationIndexPath: IndexPath?) -> UICollectionViewDropProposal {
        guard let localDragSession = session.localDragSession, let item = localDragSession.items.first, let tab = item.localObject as? Tab else {
            return UICollectionViewDropProposal(operation: .forbidden)
        }

        // If the tab doesn't exist by the time we get here, we must return a
        // `.cancel` operation continuously until `isDragging` can be reset.
        guard isDragging, tabs.firstIndex(of: tab) != nil else {
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
            return TabTrayControllerUX.compactNumberOfColumnsThin
        } else {
            return TabTrayControllerUX.numberOfColumnsWide
        }
    }

    init(profile: Profile, traitCollection: UITraitCollection) {
        self.profile = profile
        self.traitCollection = traitCollection
        super.init()
    }

    fileprivate func cellHeightForCurrentDevice() -> CGFloat {
        let shortHeight = TabTrayControllerUX.textBoxHeight * 6

        if self.traitCollection.verticalSizeClass == .compact {
            return shortHeight
        } else if self.traitCollection.horizontalSizeClass == .compact {
            return shortHeight
        } else {
            return TabTrayControllerUX.textBoxHeight * 8
        }
    }

    @objc func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, minimumInteritemSpacingForSectionAt section: Int) -> CGFloat {
        return TabTrayControllerUX.margin
    }

    @objc func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, sizeForItemAt indexPath: IndexPath) -> CGSize {
        let cellWidth = floor((collectionView.bounds.width - TabTrayControllerUX.margin * CGFloat(numberOfColumns + 1)) / CGFloat(numberOfColumns))
        return CGSize(width: cellWidth, height: self.cellHeightForCurrentDevice())
    }

    @objc func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, insetForSectionAt section: Int) -> UIEdgeInsets {
        return UIEdgeInsets(equalInset: TabTrayControllerUX.margin)
    }

    @objc func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, minimumLineSpacingForSectionAt section: Int) -> CGFloat {
        return TabTrayControllerUX.margin
    }

    @objc func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
        tabSelectionDelegate?.didSelectTabAtIndex(indexPath.row)
    }
}

private struct EmptyPrivateTabsViewUX {
    static let titleColor = UIColor.Photon.grey10
    static let titleFont = UIFont.systemFont(ofSize: 20, weight: .semibold)
    static let descriptionColor = UIColor.Photon.grey20
    static let descriptionFont = UIFont.systemFont(ofSize: 14)
    static let learnMoreFont = UIFont.systemFont(ofSize: 17, weight: .medium)
    static let textMargin: CGFloat = 40
    static let minBottomMargin: CGFloat = 15
    static let stackViewSpacing: CGFloat = 15.0
}

// View we display when there are no private tabs created
// Need access for iOS 12 appearance adjustments
/*fileprivate*/ class EmptyPrivateTabsView: UIView {
    
    let scrollView = UIScrollView().then {
        $0.alwaysBounceVertical = true
        $0.indicatorStyle = .white
    }
    
    let stackView = UIStackView().then {
        $0.axis = .vertical
        $0.spacing = EmptyPrivateTabsViewUX.stackViewSpacing
    }
    
    let titleLabel = UILabel().then {
        $0.textColor = EmptyPrivateTabsViewUX.titleColor
        $0.font = EmptyPrivateTabsViewUX.titleFont
        $0.textAlignment = .center
        $0.text = Strings.privateBrowsing
    }

    let descriptionLabel = UILabel().then {
        $0.textColor = EmptyPrivateTabsViewUX.descriptionColor
        $0.font = EmptyPrivateTabsViewUX.descriptionFont
        $0.text = Strings.privateTabBody
        $0.numberOfLines = 0
    }
    
    let detailsLabel = UILabel().then {
        $0.textColor = EmptyPrivateTabsViewUX.descriptionColor
        $0.font = EmptyPrivateTabsViewUX.descriptionFont
        $0.text = Strings.privateTabDetails
        $0.isHidden = true
        $0.numberOfLines = 0
    }

    let learnMoreButton = UIButton(type: .system).then {
        $0.setTitle(Strings.privateTabLink, for: [])
        $0.setTitleColor(UIConstants.privateModeTextHighlightColor, for: [])
        $0.titleLabel?.font = EmptyPrivateTabsViewUX.learnMoreFont
        $0.titleLabel?.numberOfLines = 0
    }

    let iconImageView = UIImageView(image: #imageLiteral(resourceName: "private_glasses")).then {
        $0.contentMode = .center
        $0.setContentHuggingPriority(.required, for: .vertical)
        $0.setContentCompressionResistancePriority(.defaultHigh, for: .vertical)
    }

    override init(frame: CGRect) {
        super.init(frame: frame)

        addSubview(scrollView)
        scrollView.addSubview(stackView)
        stackView.addArrangedSubview(iconImageView)
        stackView.addArrangedSubview(titleLabel)
        stackView.addArrangedSubview(descriptionLabel)
        stackView.addArrangedSubview(detailsLabel)
        stackView.addArrangedSubview(learnMoreButton)
        
        stackView.setCustomSpacing(EmptyPrivateTabsViewUX.stackViewSpacing * 2.0, after: iconImageView)
        
        scrollView.snp.makeConstraints {
            $0.edges.equalTo(self.snp.edges)
        }
        scrollView.contentLayoutGuide.snp.makeConstraints {
            $0.width.equalTo(self)
            $0.top.equalTo(self.stackView).offset(-EmptyPrivateTabsViewUX.minBottomMargin)
            $0.bottom.equalTo(self.stackView).offset(EmptyPrivateTabsViewUX.minBottomMargin)
        }
        stackView.snp.makeConstraints {
            $0.left.right.equalTo(self).inset(EmptyPrivateTabsViewUX.textMargin)
        }
    }
    
    func updateContentInset() {
        stackView.layoutIfNeeded()
        if stackView.bounds.height < bounds.height {
            // Center it in the container
            scrollView.contentInset.top = ceil((scrollView.frame.height - stackView.bounds.height) / 2.0)
        } else {
            scrollView.contentInset.top = 0
        }
    }
    
    override func layoutSubviews() {
        super.layoutSubviews()
        updateContentInset()
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
        if let index = self.tabDataSource.tabs.firstIndex(of: tab),
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
class TrayToolbar: UIView, Themeable {
    fileprivate let toolbarButtonSize = CGSize(width: 44, height: 44)

    let addTabButton = UIButton(type: .system).then {
        $0.setImage(#imageLiteral(resourceName: "add_tab").template, for: .normal)
        $0.accessibilityLabel = Strings.tabTrayAddTabAccessibilityLabel
        $0.accessibilityIdentifier = "TabTrayController.addTabButton"
    }
    
    let doneButton = UIButton(type: .system).then {
        $0.setTitle(Strings.done, for: .normal)
        $0.titleLabel?.font = TabTrayControllerUX.toolbarFont
        $0.accessibilityLabel = Strings.done
        $0.accessibilityIdentifier = "TabTrayController.doneButton"
    }

    let privateModeButton = PrivateModeButton().then {
        $0.titleLabel?.font = TabTrayControllerUX.toolbarFont
        $0.setTitle(Strings.private, for: .normal)
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
            make.centerY.equalTo(safeArea.centerY)
            make.trailing.equalTo(self).offset(-sideOffset)
        }

        addSubview(privateModeButton)
        privateModeButton.snp.makeConstraints { make in
            make.centerY.equalTo(safeArea.centerY)
            make.leading.equalTo(self).offset(sideOffset)
        }
    }

    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    var themeableChildren: [Themeable?]? {
        return [privateModeButton]
    }

    func applyTheme(_ theme: Theme) {
        styleChildren(theme: theme)
        
        backgroundColor = theme.colors.home
        
        addTabButton.tintColor = theme.colors.tints.footer
        doneButton.tintColor = addTabButton.tintColor
        
    }
}
