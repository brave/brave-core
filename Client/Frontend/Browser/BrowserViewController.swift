/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Photos
import UIKit
import WebKit
import Shared
import Storage
import SnapKit
import XCGLogger
import Alamofire
import MobileCoreServices
import SwiftyJSON
import Deferred
import Data
import BraveShared
import SwiftKeychainWrapper

private let log = Logger.browserLogger

private let KVOs: [KVOConstants] = [
    .estimatedProgress,
    .loading,
    .canGoBack,
    .canGoForward,
    .URL,
    .title,
    .hasOnlySecureContent,
    .serverTrust
]

private let ActionSheetTitleMaxLength = 120

private struct BrowserViewControllerUX {
    fileprivate static let BackgroundColor = UIConstants.AppBackgroundColor
    fileprivate static let ShowHeaderTapAreaHeight: CGFloat = 32
    fileprivate static let BookmarkStarAnimationDuration: Double = 0.5
    fileprivate static let BookmarkStarAnimationOffset: CGFloat = 80
}

class BrowserViewController: UIViewController {
    var favoritesViewController: FavoritesViewController?
    var webViewContainer: UIView!
    var urlBar: URLBarView!
    var tabsBar: TabsBarViewController!
    var clipboardBarDisplayHandler: ClipboardBarDisplayHandler?
    var readerModeBar: ReaderModeBarView?
    var readerModeCache: ReaderModeCache
    var statusBarOverlay: UIView!
    fileprivate(set) var toolbar: TabToolbar?
    var searchController: SearchViewController?
    fileprivate var screenshotHelper: ScreenshotHelper!
    fileprivate var homePanelIsInline = false
    fileprivate var searchLoader: SearchLoader?
    fileprivate let alertStackView = UIStackView() // All content that appears above the footer should be added to this view. (Find In Page/SnackBars)
    fileprivate var findInPageBar: FindInPageBar?

    lazy var mailtoLinkHandler: MailtoLinkHandler = MailtoLinkHandler()

    lazy fileprivate var customSearchEngineButton: UIButton = {
        let searchButton = UIButton()
        searchButton.setImage(#imageLiteral(resourceName: "AddSearch").template, for: [])
        searchButton.addTarget(self, action: #selector(addCustomSearchEngineForFocusedElement), for: .touchUpInside)
        searchButton.accessibilityIdentifier = "BrowserViewController.customSearchEngineButton"
        return searchButton
    }()

    fileprivate var customSearchBarButton: UIBarButtonItem?

    // popover rotation handling
    fileprivate var displayedPopoverController: UIViewController?
    fileprivate var updateDisplayedPopoverProperties: (() -> Void)?

    var openInHelper: OpenInHelper?

    // location label actions
    fileprivate var pasteGoAction: AccessibleAction!
    fileprivate var pasteAction: AccessibleAction!
    fileprivate var copyAddressAction: AccessibleAction!

    fileprivate weak var tabTrayController: TabTrayController!
    let profile: Profile
    let tabManager: TabManager
    
    /// Whether last session was a crash or not
    fileprivate let crashedLastSession: Bool

    // These views wrap the urlbar and toolbar to provide background effects on them
    var header: UIStackView!
    var footer: UIView!
    fileprivate var topTouchArea: UIButton!
    
    // These constraints allow to show/hide tabs bar
    var webViewContainerTopOffset: Constraint?
    
    // Backdrop used for displaying greyed background for private tabs
    var webViewContainerBackdrop: UIView!

    var scrollController = TabScrollingController()

    fileprivate var keyboardState: KeyboardState?

    var pendingToast: Toast? // A toast that might be waiting for BVC to appear before displaying
    var downloadToast: DownloadToast? // A toast that is showing the combined download progress

    // Tracking navigation items to record history types.
    // TODO: weak references?
    var ignoredNavigation = Set<WKNavigation>()
    var typedNavigation = [WKNavigation: VisitType]()
    var navigationToolbar: TabToolbarProtocol {
        return toolbar ?? urlBar
    }
    
    /// Keep track of the URL request that was upgraded so that we can add it to the HTTPS page stats
    var pendingHTTPUpgrades = [String: URLRequest]()

    // Keep track of allowed `URLRequest`s from `webView(_:decidePolicyFor:decisionHandler:)` so
    // that we can obtain the originating `URLRequest` when a `URLResponse` is received. This will
    // allow us to re-trigger the `URLRequest` if the user requests a file to be downloaded.
    var pendingRequests = [String: URLRequest]()

    // This is set when the user taps "Download Link" from the context menu. We then force a
    // download of the next request through the `WKNavigationDelegate` that matches this URL.
    var pendingDownloadURL: URL?

    let downloadQueue = DownloadQueue()
    
    fileprivate var contentBlockListDeferred: Deferred<()>?
    
    // Web filters
    
    let safeBrowsing: SafeBrowsing?

    init(profile: Profile, tabManager: TabManager, crashedLastSession: Bool,
         safeBrowsingManager: SafeBrowsing? = SafeBrowsing()) {
        self.profile = profile
        self.tabManager = tabManager
        self.readerModeCache = ReaderMode.cache(for: tabManager.selectedTab)
        self.crashedLastSession = crashedLastSession
        self.safeBrowsing = safeBrowsingManager
        super.init(nibName: nil, bundle: nil)
        didInit()
    }

    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    override var supportedInterfaceOrientations: UIInterfaceOrientationMask {
        if UIDevice.current.userInterfaceIdiom == .phone {
            return .allButUpsideDown
        } else {
            return .all
        }
    }

    override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
        super.viewWillTransition(to: size, with: coordinator)

        dismissVisibleMenus()

        coordinator.animate(alongsideTransition: { context in
            self.scrollController.updateMinimumZoom()
            if let popover = self.displayedPopoverController {
                self.updateDisplayedPopoverProperties?()
                self.present(popover, animated: true, completion: nil)
            }
        }, completion: { _ in
            self.scrollController.setMinimumZoom()
        })
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
    }

    fileprivate func didInit() {
        screenshotHelper = ScreenshotHelper(controller: self)
        tabManager.addDelegate(self)
        tabManager.addNavigationDelegate(self)
        downloadQueue.delegate = self
        
        // Observe some user preferences
        Preferences.Privacy.privateBrowsingOnly.observe(from: self)
        Preferences.General.tabBarVisibility.observe(from: self)
        Preferences.Shields.allShields.forEach { $0.observe(from: self) }
        Preferences.Privacy.blockAllCookies.observe(from: self)
        // Lists need to be compiled before attempting tab restoration
        contentBlockListDeferred = ContentBlockerHelper.compileBundledLists()
    }

    override var preferredStatusBarStyle: UIStatusBarStyle {
        switch Theme.of(tabManager.selectedTab) {
        case .regular:
            return .default
        case .private:
            return .lightContent
        }
    }

    func shouldShowFooterForTraitCollection(_ previousTraitCollection: UITraitCollection) -> Bool {
        return previousTraitCollection.verticalSizeClass != .compact && previousTraitCollection.horizontalSizeClass != .regular
    }

    func toggleSnackBarVisibility(show: Bool) {
        if show {
            UIView.animate(withDuration: 0.1, animations: { self.alertStackView.isHidden = false })
        } else {
            alertStackView.isHidden = true
        }
    }

    fileprivate func updateToolbarStateForTraitCollection(_ newCollection: UITraitCollection, withTransitionCoordinator coordinator: UIViewControllerTransitionCoordinator? = nil) {
        let showToolbar = shouldShowFooterForTraitCollection(newCollection)

        urlBar.setShowToolbar(!showToolbar)
        toolbar?.removeFromSuperview()
        toolbar?.tabToolbarDelegate = nil
        toolbar = nil

        if showToolbar {
            toolbar = TabToolbar()
            footer.addSubview(toolbar!)
            toolbar?.tabToolbarDelegate = self

            let theme = Theme.of(tabManager.selectedTab)
            toolbar?.applyTheme(theme)

            updateTabCountUsingTabManager(self.tabManager)
        }

        view.setNeedsUpdateConstraints()
        if let home = favoritesViewController {
            home.view.setNeedsUpdateConstraints()
        }

        if let tab = tabManager.selectedTab,
               let webView = tab.webView {
            updateURLBar()
            navigationToolbar.updateBackStatus(webView.canGoBack)
            navigationToolbar.updateForwardStatus(webView.canGoForward)
            urlBar.locationView.loading = tab.loading
        }
        
        updateTabsBarVisibility()
    }

    override func willTransition(to newCollection: UITraitCollection, with coordinator: UIViewControllerTransitionCoordinator) {
        super.willTransition(to: newCollection, with: coordinator)

        // During split screen launching on iPad, this callback gets fired before viewDidLoad gets a chance to
        // set things up. Make sure to only update the toolbar state if the view is ready for it.
        if isViewLoaded {
            updateToolbarStateForTraitCollection(newCollection, withTransitionCoordinator: coordinator)
        }

        displayedPopoverController?.dismiss(animated: true, completion: nil)
        coordinator.animate(alongsideTransition: { context in
            self.scrollController.showToolbars(animated: false)
            if self.isViewLoaded {
                self.statusBarOverlay.backgroundColor = self.urlBar.backgroundColor
                self.setNeedsStatusBarAppearanceUpdate()
            }
            }, completion: nil)
    }

    func dismissVisibleMenus() {
        displayedPopoverController?.dismiss(animated: true)
    }

    @objc func appDidEnterBackgroundNotification() {
        displayedPopoverController?.dismiss(animated: false) {
            self.displayedPopoverController = nil
        }
    }

    @objc func tappedTopArea() {
        scrollController.showToolbars(animated: true)
    }

    @objc func appWillResignActiveNotification() {
        // Dismiss any popovers that might be visible
        displayedPopoverController?.dismiss(animated: false) {
            self.displayedPopoverController = nil
        }

        // If we are displaying a private tab, hide any elements in the tab that we wouldn't want shown
        // when the app is in the home switcher
        if let tab = tabManager.selectedTab, tab.isPrivate {
            webViewContainerBackdrop.alpha = 1
            webViewContainer.alpha = 0
            urlBar.locationContainer.alpha = 0
            presentedViewController?.popoverPresentationController?.containerView?.alpha = 0
            presentedViewController?.view.alpha = 0
        }
    }

    @objc func appDidBecomeActiveNotification() {
        // Re-show any components that might have been hidden because they were being displayed
        // as part of a private mode tab
        UIView.animate(withDuration: 0.2, delay: 0, options: UIView.AnimationOptions(), animations: {
            self.webViewContainer.alpha = 1
            self.urlBar.locationContainer.alpha = 1
            self.presentedViewController?.popoverPresentationController?.containerView?.alpha = 1
            self.presentedViewController?.view.alpha = 1
            self.view.backgroundColor = UIColor.clear
        }, completion: { _ in
            self.webViewContainerBackdrop.alpha = 0
        })

        // Re-show toolbar which might have been hidden during scrolling (prior to app moving into the background)
        scrollController.showToolbars(animated: false)
    }

    override func viewDidLoad() {
        super.viewDidLoad()
        NotificationCenter.default.addObserver(self, selector: #selector(appWillResignActiveNotification), name: UIApplication.willResignActiveNotification, object: nil)
        NotificationCenter.default.addObserver(self, selector: #selector(appDidBecomeActiveNotification), name: UIApplication.didBecomeActiveNotification, object: nil)
        NotificationCenter.default.addObserver(self, selector: #selector(appDidEnterBackgroundNotification), name: UIApplication.didEnterBackgroundNotification, object: nil)
        KeyboardHelper.defaultHelper.addDelegate(self)

        webViewContainerBackdrop = UIView()
        webViewContainerBackdrop.backgroundColor = UIColor.Photon.Grey50
        webViewContainerBackdrop.alpha = 0
        view.addSubview(webViewContainerBackdrop)

        webViewContainer = UIView()
        view.addSubview(webViewContainer)

        // Temporary work around for covering the non-clipped web view content
        statusBarOverlay = UIView()
        view.addSubview(statusBarOverlay)

        topTouchArea = UIButton()
        topTouchArea.isAccessibilityElement = false
        topTouchArea.addTarget(self, action: #selector(tappedTopArea), for: .touchUpInside)
        view.addSubview(topTouchArea)

        // Setup the URL bar, wrapped in a view to get transparency effect
        urlBar = URLBarView()
        urlBar.translatesAutoresizingMaskIntoConstraints = false
        urlBar.delegate = self
        urlBar.tabToolbarDelegate = self
        header = UIStackView().then {
            $0.axis = .vertical
            $0.clipsToBounds = true
        }
        header.addArrangedSubview(urlBar)
        
        tabsBar = TabsBarViewController(tabManager: tabManager)
        tabsBar.delegate = self
        header.addArrangedSubview(tabsBar.view)
        
        view.addSubview(header)
        
        addChild(tabsBar)
        tabsBar.didMove(toParent: self)

        // UIAccessibilityCustomAction subclass holding an AccessibleAction instance does not work, thus unable to generate AccessibleActions and UIAccessibilityCustomActions "on-demand" and need to make them "persistent" e.g. by being stored in BVC
        pasteGoAction = AccessibleAction(name: Strings.PasteAndGoTitle, handler: { () -> Bool in
            if let pasteboardContents = UIPasteboard.general.string {
                self.urlBar(self.urlBar, didSubmitText: pasteboardContents)
                return true
            }
            return false
        })
        pasteAction = AccessibleAction(name: Strings.PasteTitle, handler: { () -> Bool in
            if let pasteboardContents = UIPasteboard.general.string {
                // Enter overlay mode and make the search controller appear.
                self.urlBar.enterOverlayMode(pasteboardContents, pasted: true, search: true)

                return true
            }
            return false
        })
        copyAddressAction = AccessibleAction(name: Strings.CopyAddressTitle, handler: { () -> Bool in
            if let url = self.urlBar.currentURL {
                UIPasteboard.general.url = url as URL
            }
            return true
        })

        view.addSubview(alertStackView)
        footer = UIView()
        view.addSubview(footer)
        alertStackView.axis = .vertical
        alertStackView.alignment = .center

        clipboardBarDisplayHandler = ClipboardBarDisplayHandler(tabManager: tabManager)
        clipboardBarDisplayHandler?.delegate = self
        
        scrollController.urlBar = urlBar
        scrollController.header = header
        scrollController.tabsBar = tabsBar
        scrollController.footer = footer
        scrollController.snackBars = alertStackView

        self.updateToolbarStateForTraitCollection(self.traitCollection)

        setupConstraints()

        // Setup UIDropInteraction to handle dragging and dropping
        // links into the view from other apps.
        let dropInteraction = UIDropInteraction(delegate: self)
        view.addInteraction(dropInteraction)
        
        initializeSyncWebView()
    }
    
    /// Initialize Sync without connecting. Sync webview needs to be in a "permanent" location
    /// to continue working predictably. If Sync is not in the view "hierarchy"
    /// it will behavior extremely unpredictably, often just dying in the middle of a promize chain.
    /// Added to keyWindow, since it can then be utilized from any VC (e.g. settings modal).
    /// This also inits sync at app launch.
    private func initializeSyncWebView() {
        let sync = Sync.shared
        
        #if NO_SYNC
        if sync.syncSeedArray == nil { return }
        
        let msg = """
            Sync has been disabled, as it will not be included in the next couple of production builds.
            Your iOS device has been auto-removed from any sync groups.
        """
        
        let alert = UIAlertController(title: "Sync Disabled", message: msg, preferredStyle: .alert)
        alert.addAction(UIAlertAction(title: "Ok", style: .default, handler: nil))
        present(alert, animated: true, completion: nil)
        #endif
        
        sync.webView.alpha = 0.01
        UIApplication.shared.keyWindow?.insertSubview(Sync.shared.webView, at: 0)
    }
    
    fileprivate func setupTabs() {
        let isPrivate = Preferences.Privacy.privateBrowsingOnly.value
        let noTabsAdded = tabManager.tabsForCurrentMode.isEmpty
        
        var tabToSelect: Tab?
        
        if noTabsAdded {
            // Two scenarios if there are no tabs in tabmanager:
            // 1. We have not restored tabs yet, attempt to restore or make a new tab if there is nothing.
            // 2. We are in private browsing mode and need to add a new private tab.
            tabToSelect = isPrivate ? tabManager.addTab(isPrivate: true) : tabManager.restoreAllTabs()
        } else {
            tabToSelect = tabManager.tabsForCurrentMode.last
        }
        
        contentBlockListDeferred?.uponQueue(.main) { _ in
            self.tabManager.selectTab(tabToSelect)
        }
    }

    fileprivate func setupConstraints() {
        header.snp.makeConstraints { make in
            scrollController.headerTopConstraint = make.top.equalTo(view.safeArea.top).constraint
            make.left.right.equalTo(self.view)
        }
        
        urlBar.snp.makeConstraints { make in
            make.height.equalTo(UIConstants.TopToolbarHeight)
        }
        
        tabsBar.view.snp.makeConstraints { make in
            make.height.equalTo(UX.TabsBar.height)
        }

        webViewContainerBackdrop.snp.makeConstraints { make in
            make.edges.equalTo(webViewContainer)
        }
    }

    override func viewDidLayoutSubviews() {
        super.viewDidLayoutSubviews()
        statusBarOverlay.snp.remakeConstraints { make in
            make.top.left.right.equalTo(self.view)
            make.bottom.equalTo(view.safeArea.top)
        }
    }

    override var canBecomeFirstResponder: Bool {
        return true
    }
    
    override func becomeFirstResponder() -> Bool {
        // Make the web view the first responder so that it can show the selection menu.
        return tabManager.selectedTab?.webView?.becomeFirstResponder() ?? false
    }

    func loadQueuedTabs(receivedURLs: [URL]? = nil) {
        // Chain off of a trivial deferred in order to run on the background queue.
        succeed().upon() { res in
            self.dequeueQueuedTabs(receivedURLs: receivedURLs ?? [])
        }
    }

    fileprivate func dequeueQueuedTabs(receivedURLs: [URL]) {
        assert(!Thread.current.isMainThread, "This must be called in the background.")
        self.profile.queue.getQueuedTabs() >>== { cursor in

            // This assumes that the DB returns rows in some kind of sane order.
            // It does in practice, so WFM.
            if cursor.count > 0 {

                // Filter out any tabs received by a push notification to prevent dupes.
                let urls = cursor.compactMap { $0?.url.asURL }.filter { !receivedURLs.contains($0) }
                if !urls.isEmpty {
                    DispatchQueue.main.async {
                        self.tabManager.addTabsForURLs(urls, zombie: false)
                    }
                }

                // Clear *after* making an attempt to open. We're making a bet that
                // it's better to run the risk of perhaps opening twice on a crash,
                // rather than losing data.
                self.profile.queue.clearQueuedTabs()
            }
                
            // Then, open any received URLs from push notifications.
            if !receivedURLs.isEmpty {
                DispatchQueue.main.async {
                    let unopenedReceivedURLs = receivedURLs.filter { self.tabManager.getTabForURL($0) == nil }

                    self.tabManager.addTabsForURLs(unopenedReceivedURLs, zombie: false)

                    if let lastURL = receivedURLs.last, let tab = self.tabManager.getTabForURL(lastURL) {
                        self.tabManager.selectTab(tab)
                    }
                }
            }
        }
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)

        checkCrashRestoration()
        
        updateTabCountUsingTabManager(tabManager)
        clipboardBarDisplayHandler?.checkIfShouldDisplayBar()
        favoritesViewController?.updateDuckDuckGoVisibility()
    }
    
    fileprivate lazy var checkCrashRestoration: () -> Void = {
        if crashedLastSession {
            showRestoreTabsAlert()
        } else {
            setupTabs()
        }
        return {}
    }()

    fileprivate func showRestoreTabsAlert() {
        guard canRestoreTabs() else {
            self.tabManager.addTabAndSelect()
            return
        }
        let alert = UIAlertController.restoreTabsAlert(
            okayCallback: { _ in
                self.setupTabs()
            },
            noCallback: { _ in
                TabMO.deleteAll()
                self.tabManager.addTabAndSelect()
            }
        )
        self.present(alert, animated: true, completion: nil)
    }

    fileprivate func canRestoreTabs() -> Bool {
        // Make sure there's at least one real tab open
        return !TabMO.getAll().compactMap({ $0.url }).isEmpty
    }

    override func viewDidAppear(_ animated: Bool) {
        screenshotHelper.viewIsVisible = true
        screenshotHelper.takePendingScreenshots(tabManager.allTabs)

        super.viewDidAppear(animated)

        if shouldShowWhatsNewTab() {
            // Only display if the SUMO topic has been configured in the Info.plist (present and not empty)
            if let whatsNewTopic = AppInfo.whatsNewTopic, whatsNewTopic != "" {
                if let whatsNewURL = SupportUtils.URLForTopic(whatsNewTopic) {
                    self.openURLInNewTab(whatsNewURL, isPrivileged: false)
                    profile.prefs.setString(AppInfo.appVersion, forKey: LatestAppVersionProfileKey)
                }
            }
        }

        if let toast = self.pendingToast {
            self.pendingToast = nil
            show(toast: toast, afterWaiting: ButtonToastUX.ToastDelay)
        }
        showQueuedAlertIfAvailable()
        
        if PrivateBrowsingManager.shared.isPrivateBrowsing {
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.2) {
                self.presentDuckDuckGoCallout()
            }
        }
    }

    // THe logic for shouldShowWhatsNewTab is as follows: If we do not have the LatestAppVersionProfileKey in
    // the profile, that means that this is a fresh install and we do not show the What's New. If we do have
    // that value, we compare it to the major version of the running app. If it is different then this is an
    // upgrade, downgrades are not possible, so we can show the What's New page.

    fileprivate func shouldShowWhatsNewTab() -> Bool {
        guard let latestMajorAppVersion = profile.prefs.stringForKey(LatestAppVersionProfileKey)?.components(separatedBy: ".").first else {
            return false // Clean install, never show What's New
        }

        return latestMajorAppVersion != AppInfo.majorAppVersion && DeviceInfo.hasConnectivity()
    }

    fileprivate func showQueuedAlertIfAvailable() {
        if let queuedAlertInfo = tabManager.selectedTab?.dequeueJavascriptAlertPrompt() {
            let alertController = queuedAlertInfo.alertController()
            alertController.delegate = self
            present(alertController, animated: true, completion: nil)
        }
    }

    override func viewWillDisappear(_ animated: Bool) {
        screenshotHelper.viewIsVisible = false
        super.viewWillDisappear(animated)
    }

    override func viewDidDisappear(_ animated: Bool) {
        super.viewDidDisappear(animated)
    }

    func resetBrowserChrome() {
        // animate and reset transform for tab chrome
        urlBar.updateAlphaForSubviews(1)
        footer.alpha = 1

        [header, footer, readerModeBar].forEach { view in
                view?.transform = .identity
        }
        statusBarOverlay.isHidden = false
    }

    override func updateViewConstraints() {
        super.updateViewConstraints()

        topTouchArea.snp.remakeConstraints { make in
            make.top.left.right.equalTo(self.view)
            make.height.equalTo(BrowserViewControllerUX.ShowHeaderTapAreaHeight)
        }

        readerModeBar?.snp.remakeConstraints { make in
            make.top.equalTo(self.header.snp.bottom)
            make.height.equalTo(UIConstants.ToolbarHeight)
            make.leading.trailing.equalTo(self.view)
        }

        webViewContainer.snp.remakeConstraints { make in
            make.left.right.equalTo(self.view)
            
            webViewContainerTopOffset = make.top.equalTo(readerModeBar?.snp.bottom ?? self.header.snp.bottom).constraint

            let findInPageHeight = (findInPageBar == nil) ? 0 : UIConstants.ToolbarHeight
            if let toolbar = self.toolbar {
                make.bottom.equalTo(toolbar.snp.top).offset(-findInPageHeight)
            } else {
                make.bottom.equalTo(self.view).offset(-findInPageHeight)
            }
        }

        // Setup the bottom toolbar
        toolbar?.snp.remakeConstraints { make in
            make.edges.equalTo(self.footer)
            make.height.equalTo(UIConstants.BottomToolbarHeight)
        }

        footer.snp.remakeConstraints { make in
            scrollController.footerBottomConstraint = make.bottom.equalTo(self.view.snp.bottom).constraint
            make.leading.trailing.equalTo(self.view)
        }

        urlBar.setNeedsUpdateConstraints()

        // Remake constraints even if we're already showing the home controller.
        // The home controller may change sizes if we tap the URL bar while on about:home.
        favoritesViewController?.view.snp.remakeConstraints { make in
            webViewContainerTopOffset = make.top.equalTo(readerModeBar?.snp.bottom ?? self.header.snp.bottom).constraint
            
            make.left.right.equalTo(self.view)
            if self.homePanelIsInline {
                make.bottom.equalTo(self.toolbar?.snp.top ?? self.view.snp.bottom)
            } else {
                make.bottom.equalTo(self.view.snp.bottom)
            }
        }

        alertStackView.snp.remakeConstraints { make in
            make.centerX.equalTo(self.view)
            make.width.equalTo(self.view.snp.width)
            if let keyboardHeight = keyboardState?.intersectionHeightForView(self.view), keyboardHeight > 0 {
                make.bottom.equalTo(self.view).offset(-keyboardHeight)
            } else if let toolbar = self.toolbar {
                make.bottom.equalTo(toolbar.snp.top)
            } else {
                make.bottom.equalTo(self.view)
            }
        }
    }

    fileprivate func showHomePanelController(inline: Bool) {
        homePanelIsInline = inline

        if favoritesViewController == nil {
            let homePanelController = FavoritesViewController(profile: profile)
            homePanelController.delegate = self
            homePanelController.view.alpha = 0

            self.favoritesViewController = homePanelController

            addChild(homePanelController)
            view.addSubview(homePanelController.view)
            homePanelController.didMove(toParent: self)
        }
        guard let homePanelController = self.favoritesViewController else {
            assertionFailure("homePanelController is still nil after assignment.")
            return
        }
        
        // We have to run this animation, even if the view is already showing because there may be a hide animation running
        // and we want to be sure to override its results.
        UIView.animate(withDuration: 0.2, animations: { () -> Void in
            homePanelController.view.alpha = 1
        }, completion: { finished in
            if finished {
                self.webViewContainer.accessibilityElementsHidden = true
                UIAccessibility.post(notification: .screenChanged, argument: nil)
            }
        })
        view.setNeedsUpdateConstraints()
    }

    fileprivate func hideHomePanelController() {
        if let controller = favoritesViewController {
            self.favoritesViewController = nil
            UIView.animate(withDuration: 0.2, delay: 0, options: .beginFromCurrentState, animations: { () -> Void in
                controller.view.alpha = 0
            }, completion: { _ in
                controller.willMove(toParent: nil)
                controller.view.removeFromSuperview()
                controller.removeFromParent()
                self.webViewContainer.accessibilityElementsHidden = false
                UIAccessibility.post(notification: .screenChanged, argument: nil)

                // Refresh the reading view toolbar since the article record may have changed
                if let readerMode = self.tabManager.selectedTab?.getContentScript(name: ReaderMode.name()) as? ReaderMode, readerMode.state == .active {
                    self.showReaderModeBar(animated: false)
                }
            })
        }
    }

    fileprivate func updateInContentHomePanel(_ url: URL?) {
        if !urlBar.inOverlayMode {
            guard let url = url else {
                hideHomePanelController()
                return
            }
            if url.isAboutHomeURL && !url.isErrorPageURL {
                showHomePanelController(inline: true)
            } else if !url.isLocalUtility || url.isReaderModeURL || url.isErrorPageURL {
                hideHomePanelController()
            }
        }
    }

    fileprivate func showSearchController() {
        if searchController != nil {
            return
        }

        let tabType = TabType.of(tabManager.selectedTab)
        searchController = SearchViewController(forTabType: tabType)
        searchController!.searchEngines = profile.searchEngines
        searchController!.searchDelegate = self
        searchController!.profile = self.profile

        searchLoader = SearchLoader(profile: profile, urlBar: urlBar)
        searchLoader?.addListener(searchController!)

        addChild(searchController!)
        view.addSubview(searchController!.view)
        searchController!.view.snp.makeConstraints { make in
            make.top.equalTo(self.urlBar.snp.bottom)
            make.left.right.bottom.equalTo(self.view)
            return
        }

        favoritesViewController?.view?.isHidden = true

        searchController!.didMove(toParent: self)
    }
    
    func updateTabsBarVisibility() {
        if tabManager.selectedTab == nil {
            tabsBar.view.isHidden = true
            return
        }
        
        func shouldShowTabBar() -> Bool {
            let tabCount = tabManager.tabsForCurrentMode.count
            guard let tabBarVisibility = TabBarVisibility(rawValue: Preferences.General.tabBarVisibility.value) else {
                // This should never happen
                assertionFailure("Invalid tab bar visibility preference: \(Preferences.General.tabBarVisibility.value).")
                return tabCount > 1
            }
            switch tabBarVisibility {
            case .always:
                return tabCount > 1 || UIDevice.current.userInterfaceIdiom == .pad
            case .landscapeOnly:
                return tabCount > 1 && UIDevice.current.orientation.isLandscape
            case .never:
                return false
            }
        }
        
        let isShowing = !tabsBar.view.isHidden
        let shouldShow = shouldShowTabBar()
        
        if isShowing != shouldShow && presentedViewController == nil {
            UIView.animate(withDuration: 0.1) {
                self.tabsBar.view.isHidden = !shouldShow
            }
        } else {
            tabsBar.view.isHidden = !shouldShow
        }
    }

    fileprivate func hideSearchController() {
        if let searchController = searchController {
            searchController.willMove(toParent: nil)
            searchController.view.removeFromSuperview()
            searchController.removeFromParent()
            self.searchController = nil
            favoritesViewController?.view?.isHidden = false
            searchLoader = nil
        }
    }

    func finishEditingAndSubmit(_ url: URL, visitType: VisitType) {
        urlBar.currentURL = url
        urlBar.leaveOverlayMode()

        guard let tab = tabManager.selectedTab else {
            return
        }

        if let webView = tab.webView {
            resetSpoofedUserAgentIfRequired(webView, newURL: url)
        }

        if let nav = tab.loadRequest(PrivilegedRequest(url: url) as URLRequest) {
            self.recordNavigationInTab(tab, navigation: nav, visitType: visitType)
        }
    }

    func addBookmark(_ tabState: TabState) {
        guard let url = tabState.url else { return }
        let absoluteString = url.absoluteString
        let shareItem = ShareItem(url: absoluteString, title: tabState.title, favicon: tabState.favicon)
        _ = profile.bookmarks.shareItem(shareItem)
    }

    override func accessibilityPerformEscape() -> Bool {
        if urlBar.inOverlayMode {
            urlBar.didClickCancel()
            return true
        } else if let selectedTab = tabManager.selectedTab, selectedTab.canGoBack {
            selectedTab.goBack()
            return true
        }
        return false
    }

    override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey: Any]?, context: UnsafeMutableRawPointer?) {
        guard let webView = object as? WKWebView, let tab = tabManager[webView], let kp = keyPath, let path = KVOConstants(rawValue: kp) else {
            assertionFailure("Unhandled KVO key: \(keyPath ?? "nil")")
            return
        }
        
        if let helper = tab.getContentScript(name: ContextMenuHelper.name()) as? ContextMenuHelper {
            // This is zero-cost if already installed. It needs to be checked frequently (hence every event here triggers this function), as when a new tab is created it requires multiple attempts to setup the handler correctly.
            helper.replaceGestureHandlerIfNeeded()
        }
        
        switch path {
        case .estimatedProgress:
            guard tab === tabManager.selectedTab,
                let progress = change?[.newKey] as? Float else { break }
            if !(webView.url?.isLocalUtility ?? false) {
                urlBar.updateProgressBar(progress)
            } else {
                urlBar.hideProgressBar()
            }
        case .loading:
            guard let loading = change?[.newKey] as? Bool else { break }
            
            if tab === tabManager.selectedTab {
                urlBar.locationView.loading = loading
                if !(webView.url?.isLocalUtility ?? false) {
                    if loading && urlBar.currentProgress() < URLBarView.psuedoProgressValue {
                        urlBar.updateProgressBar(URLBarView.psuedoProgressValue)
                    }
                } else {
                    urlBar.hideProgressBar()
                }
            }
            
            if !loading {
                runScriptsOnWebView(webView)
            }
        case .URL:
            guard let tab = tabManager[webView] else { break }
            
            // To prevent spoofing, only change the URL immediately if the new URL is on
            // the same origin as the current URL. Otherwise, do nothing and wait for
            // didCommitNavigation to confirm the page load.
            if tab.url?.origin == webView.url?.origin {
                tab.url = webView.url
                
                if tab === tabManager.selectedTab && !tab.restoring {
                    updateUIForReaderHomeStateForTab(tab)
                }
            }
        case .title:
            // Ensure that the tab title *actually* changed to prevent repeated calls
            // to navigateInTab(tab:).
            guard let title = (webView.title?.count == 0 ? webView.url?.absoluteString : webView.title) else { break }
            if !title.isEmpty && title != tab.lastTitle {
                navigateInTab(tab: tab)
            }
        case .canGoBack:
            guard tab === tabManager.selectedTab, let canGoBack = change?[.newKey] as? Bool else {
                break
            }
            
            navigationToolbar.updateBackStatus(canGoBack)
        case .canGoForward:
            guard tab === tabManager.selectedTab, let canGoForward = change?[.newKey] as? Bool else {
                break
            }
            
            navigationToolbar.updateForwardStatus(canGoForward)
        case .hasOnlySecureContent:
            guard let tab = tabManager[webView] else {
                break
            }
            
            if tab.contentIsSecure && !webView.hasOnlySecureContent {
                tab.contentIsSecure = false
            }
            
            urlBar.contentIsSecure = tab.contentIsSecure
        case .serverTrust:
            guard let tab = tabManager[webView] else {
                break
            }

            tab.contentIsSecure = false
            urlBar.contentIsSecure = tab.contentIsSecure

            guard let serverTrust = tab.webView?.serverTrust else {
                break
            }

            SecTrustEvaluateAsync(serverTrust, DispatchQueue.global()) { _, secTrustResult in
                switch secTrustResult {
                case .proceed, .unspecified:
                    tab.contentIsSecure = true
                default:
                    tab.contentIsSecure = false
                }

                DispatchQueue.main.async {
                    self.updateURLBar()
                }
            }
        default:
            assertionFailure("Unhandled KVO key: \(keyPath ?? "nil")")
        }
    }

    fileprivate func runScriptsOnWebView(_ webView: WKWebView) {
        guard let url = webView.url, url.isWebPage(), !url.isLocal else {
            return
        }
        if NoImageModeHelper.isActivated {
            webView.evaluateJavaScript("__firefox__.NoImageMode.setEnabled(true)", completionHandler: nil)
        }
    }

    func updateUIForReaderHomeStateForTab(_ tab: Tab) {
        updateURLBar()
        scrollController.showToolbars(animated: false)

        if let url = tab.url {
            if url.isReaderModeURL {
                showReaderModeBar(animated: false)
                NotificationCenter.default.addObserver(self, selector: #selector(dynamicFontChanged), name: .DynamicFontChanged, object: nil)
            } else {
                hideReaderModeBar(animated: false)
                NotificationCenter.default.removeObserver(self, name: .DynamicFontChanged, object: nil)
            }

            updateInContentHomePanel(url as URL)
        }
    }

    /// Updates the URL bar security, text and button states.
    fileprivate func updateURLBar() {
        guard let tab = tabManager.selectedTab else { return }
        
        urlBar.currentURL = tab.url?.displayURL
        
        urlBar.contentIsSecure = tab.contentIsSecure
        
        let isPage = tab.url?.displayURL?.isWebPage() ?? false
        toolbar?.updatePageStatus(isPage)
        urlBar.updatePageStatus(isPage)
    }

    // MARK: Opening New Tabs

    func switchToPrivacyMode(isPrivate: Bool ) {
        let tabTrayController = self.tabTrayController ?? TabTrayController(tabManager: tabManager, profile: profile, tabTrayDelegate: self)
        if tabTrayController.privateMode != isPrivate {
            tabTrayController.changePrivacyMode(isPrivate)
        }
        self.tabTrayController = tabTrayController
    }

    func switchToTabForURLOrOpen(_ url: URL, isPrivate: Bool = false, isPrivileged: Bool) {
        popToBVC()
        if let tab = tabManager.getTabForURL(url) {
            tabManager.selectTab(tab)
        } else {
            openURLInNewTab(url, isPrivate: isPrivate, isPrivileged: isPrivileged)
        }
    }

    func openURLInNewTab(_ url: URL?, isPrivate: Bool = false, isPrivileged: Bool) {
        urlBar.leaveOverlayMode(didCancel: true)

        if let selectedTab = tabManager.selectedTab {
            screenshotHelper.takeScreenshot(selectedTab)
        }
        let request: URLRequest?
        if let url = url {
            request = isPrivileged ? PrivilegedRequest(url: url) as URLRequest : URLRequest(url: url)
        } else {
            request = nil
        }

        switchToPrivacyMode(isPrivate: isPrivate)
        _ = tabManager.addTabAndSelect(request, isPrivate: isPrivate)
        if url == nil && NewTabAccessors.getNewTabPage() == .blankPage {
            urlBar.tabLocationViewDidTapLocation(urlBar.locationView)
        }
    }

    func openBlankNewTab(focusLocationField: Bool, isPrivate: Bool = false, searchFor searchText: String? = nil) {
        popToBVC()
        openURLInNewTab(nil, isPrivate: isPrivate, isPrivileged: true)
        let freshTab = tabManager.selectedTab
        
        if focusLocationField {
            DispatchQueue.main.asyncAfter(deadline: .now() + .milliseconds(300)) {
                // Without a delay, the text field fails to become first responder
                // Check that the newly created tab is still selected.
                // This let's the user spam the Cmd+T button without lots of responder changes.
                guard freshTab == self.tabManager.selectedTab else { return }
                self.urlBar.tabLocationViewDidTapLocation(self.urlBar.locationView)
                if let text = searchText {
                    self.urlBar.setLocation(text, search: true)
                }
            }
        }
    }

    fileprivate func popToBVC() {
        guard let currentViewController = navigationController?.topViewController else {
                return
        }
        currentViewController.dismiss(animated: true, completion: nil)
        if currentViewController != self {
            _ = self.navigationController?.popViewController(animated: true)
        } else if urlBar.inOverlayMode {
            urlBar.didClickCancel()
        }
    }

    // MARK: User Agent Spoofing

    func resetSpoofedUserAgentIfRequired(_ webView: WKWebView, newURL: URL) {
        // Reset the UA when a different domain is being loaded
        if webView.url?.host != newURL.host {
            webView.customUserAgent = nil
        }
    }

    func restoreSpoofedUserAgentIfRequired(_ webView: WKWebView, newRequest: URLRequest) {
        // Restore any non-default UA from the request's header
        let ua = newRequest.value(forHTTPHeaderField: "User-Agent")
        webView.customUserAgent = ua != UserAgent.defaultUserAgent() ? ua : nil
    }

    fileprivate func presentActivityViewController(_ url: URL, tab: Tab? = nil, sourceView: UIView?, sourceRect: CGRect, arrowDirection: UIPopoverArrowDirection) {
        let helper = ShareExtensionHelper(url: url, tab: tab)
        
        let findInPageActivity = FindInPageActivity() { [unowned self] in
            self.updateFindInPageVisibility(visible: true)
        }
        
        let requestDesktopSiteActivity = RequestDesktopSiteActivity(tab: tab) { [weak tab] in
            tab?.toggleDesktopSite()
        }
        
        var activities: [UIActivity] = [findInPageActivity]
        
        // These actions don't apply if we're sharing a temporary document
        if !url.isFileURL {
            // We don't allow to have 2 same favorites.
            if !FavoritesHelper.isAlreadyAdded(url) {
                let addToFavoritesActivity = AddToFavoritesActivity() { [weak tab] in
                    FavoritesHelper.add(url: url, title: tab?.displayTitle, color: nil)
                }
                activities.append(addToFavoritesActivity)
            }
            activities.append(requestDesktopSiteActivity)
        }
        
        let controller = helper.createActivityViewController(activities: activities, { [unowned self] completed, _ in
            // After dismissing, check to see if there were any prompts we queued up
            self.showQueuedAlertIfAvailable()

            // Usually the popover delegate would handle nil'ing out the references we have to it
            // on the BVC when displaying as a popover but the delegate method doesn't seem to be
            // invoked on iOS 10. See Bug 1297768 for additional details.
            self.displayedPopoverController = nil
            self.updateDisplayedPopoverProperties = nil
        })

        if let popoverPresentationController = controller.popoverPresentationController {
            popoverPresentationController.sourceView = sourceView
            popoverPresentationController.sourceRect = sourceRect
            popoverPresentationController.permittedArrowDirections = arrowDirection
            popoverPresentationController.delegate = self
        }

        present(controller, animated: true, completion: nil)
    }

    func updateFindInPageVisibility(visible: Bool, tab: Tab? = nil) {
        if visible {
            if findInPageBar == nil {
                let findInPageBar = FindInPageBar()
                self.findInPageBar = findInPageBar
                findInPageBar.delegate = self
                alertStackView.addArrangedSubview(findInPageBar)

                findInPageBar.snp.makeConstraints { make in
                    make.height.equalTo(UIConstants.ToolbarHeight)
                    make.leading.trailing.equalTo(alertStackView)
                }

                updateViewConstraints()

                // We make the find-in-page bar the first responder below, causing the keyboard delegates
                // to fire. This, in turn, will animate the Find in Page container since we use the same
                // delegate to slide the bar up and down with the keyboard. We don't want to animate the
                // constraints added above, however, so force a layout now to prevent these constraints
                // from being lumped in with the keyboard animation.
                findInPageBar.layoutIfNeeded()
            }

            self.findInPageBar?.becomeFirstResponder()
        } else if let findInPageBar = self.findInPageBar {
            findInPageBar.endEditing(true)
            let tab = tab ?? tabManager.selectedTab
            guard let webView = tab?.webView else { return }
            webView.evaluateJavaScript("__firefox__.findDone()", completionHandler: nil)
            findInPageBar.removeFromSuperview()
            self.findInPageBar = nil
            updateViewConstraints()
        }
    }
    
    fileprivate func postLocationChangeNotificationForTab(_ tab: Tab, navigation: WKNavigation?) {
        let notificationCenter = NotificationCenter.default
        var info = [AnyHashable: Any]()
        info["url"] = tab.url?.displayURL
        info["title"] = tab.title
        if let visitType = self.getVisitTypeForTab(tab, navigation: navigation)?.rawValue {
            info["visitType"] = visitType
        }
        info["tabType"] = tab.type
        notificationCenter.post(name: .OnLocationChange, object: self, userInfo: info)
    }

    func navigateInTab(tab: Tab, to navigation: WKNavigation? = nil) {
        tabManager.expireSnackbars()

        guard let webView = tab.webView else {
            print("Cannot navigate in tab without a webView")
            return
        }

        if let url = webView.url {
            if !url.isErrorPageURL, !url.isAboutHomeURL, !url.isFileURL {
                postLocationChangeNotificationForTab(tab, navigation: navigation)

                // Fire the readability check. This is here and not in the pageShow event handler in ReaderMode.js anymore
                // because that event wil not always fire due to unreliable page caching. This will either let us know that
                // the currently loaded page can be turned into reading mode or if the page already is in reading mode. We
                // ignore the result because we are being called back asynchronous when the readermode status changes.
                webView.evaluateJavaScript("\(ReaderModeNamespace).checkReadability()", completionHandler: nil)

                // Re-run additional scripts in webView to extract updated favicons and metadata.
                runScriptsOnWebView(webView)
                
                // Only add history of a url which is not a localhost url
                if !tab.isPrivate {
                    History.add(tab.title ?? "", url: url)
                }
            }

            TabEvent.post(.didChangeURL(url), for: tab)
        }

        if tab === tabManager.selectedTab {
            UIAccessibility.post(notification: .screenChanged, argument: nil)
            // must be followed by LayoutChanged, as ScreenChanged will make VoiceOver
            // cursor land on the correct initial element, but if not followed by LayoutChanged,
            // VoiceOver will sometimes be stuck on the element, not allowing user to move
            // forward/backward. Strange, but LayoutChanged fixes that.
            UIAccessibility.post(notification: .layoutChanged, argument: nil)
        } else if let webView = tab.webView {
            // To Screenshot a tab that is hidden we must add the webView,
            // then wait enough time for the webview to render.
            view.insertSubview(webView, at: 0)
            DispatchQueue.main.asyncAfter(deadline: .now() + .milliseconds(500)) {
                self.screenshotHelper.takeScreenshot(tab)
                if webView.superview == self.view {
                    webView.removeFromSuperview()
                }
            }
        }
        
        // Remember whether or not a desktop site was requested
        tab.desktopSite = webView.customUserAgent?.isEmpty == false
    }
    
    // MARK: - Browser PIN Callout
    
    private var isBrowserLockEnabled: Bool {
        return KeychainWrapper.sharedAppContainerKeychain.authenticationInfo() != nil
    }
    
    private var browserLockPopup: AlertPopupView?
    
    func presentBrowserLockCallout() {
        if isBrowserLockEnabled || Preferences.Popups.browserLock.value || browserLockPopup != nil { return }
        
        urlBar.leaveOverlayMode()
        
        let popup = AlertPopupView(image: #imageLiteral(resourceName: "browser_lock_popup"), title: Strings.Browser_lock_callout_title, message: Strings.Browser_lock_callout_message)
        popup.addButton(title: Strings.Browser_lock_callout_not_now) { () -> PopupViewDismissType in
            Preferences.Popups.browserLock.value = true
            self.browserLockPopup = nil
            return .flyDown
        }
        
        popup.addButton(title: Strings.Browser_lock_callout_enable, type: .primary) { [weak self] () -> PopupViewDismissType in
            Preferences.Popups.browserLock.value = true
            self?.browserLockPopup = nil
            
            let setupPasscodeController = SetupPasscodeViewController()
            let container = UINavigationController(rootViewController: setupPasscodeController)
            self?.present(container, animated: true)
            
            return .flyUp
        }
        browserLockPopup = popup
        popup.showWithType(showType: .flyUp)
    }
    
    // MARK: - DuckDuckGo Callout
    
    private var duckDuckGoPopup: AlertPopupView?
    func presentDuckDuckGoCallout(force: Bool = false) {
        // Don't show duplicate popups
        if duckDuckGoPopup != nil { return }
        
        // Check to see if its been presented already
        if !SearchEngines.shouldShowDuckDuckGoPromo || (Preferences.Popups.duckDuckGoPrivateSearch.value && !force) {
            presentBrowserLockCallout()
            return
        }
        
        // Do not show ddg popup if user already chose it for private browsing.
        if profile.searchEngines.defaultEngine(forType: .privateMode).shortName == OpenSearchEngine.EngineNames.duckDuckGo {
            presentBrowserLockCallout()
            return
        }
        
        urlBar.leaveOverlayMode()
        
        let popup = AlertPopupView(image: UIImage(named: "duckduckgo"), title: Strings.DDG_callout_title, message: Strings.DDG_callout_message)
        popup.dismissHandler = { [weak self] in
            self?.presentBrowserLockCallout()
        }
        popup.addButton(title: Strings.DDG_callout_no) {
            Preferences.Popups.duckDuckGoPrivateSearch.value = true
            self.duckDuckGoPopup = nil
            return .flyDown
        }
        popup.addButton(title: Strings.DDG_callout_enable, type: .primary) { [weak self] in
            self?.duckDuckGoPopup = nil
            
            if self?.profile == nil {
                return .flyUp
            }
            
            Preferences.Popups.duckDuckGoPrivateSearch.value = true
            self?.profile.searchEngines.setDefaultEngine(OpenSearchEngine.EngineNames.duckDuckGo, forType: .privateMode)
            
            self?.favoritesViewController?.updateDuckDuckGoVisibility()
            
            return .flyUp
        }
        duckDuckGoPopup = popup
        popup.showWithType(showType: .flyUp)
    }
}

extension BrowserViewController: ClipboardBarDisplayHandlerDelegate {
    func shouldDisplay(clipboardBar bar: ButtonToast) {
        show(toast: bar, duration: ClipboardBarToastUX.ToastDelay)
    }
}

extension BrowserViewController: QRCodeViewControllerDelegate {
    func didScanQRCodeWithURL(_ url: URL) {
        openBlankNewTab(focusLocationField: false)
        finishEditingAndSubmit(url, visitType: VisitType.typed)
    }

    func didScanQRCodeWithText(_ text: String) {
        openBlankNewTab(focusLocationField: false)
        submitSearchText(text)
    }
}

extension BrowserViewController: SettingsDelegate {
    func settingsOpenURLInNewTab(_ url: URL) {
        let forcedPrivate = PrivateBrowsingManager.shared.isPrivateBrowsing
        self.openURLInNewTab(url, isPrivate: forcedPrivate, isPrivileged: false)
    }
    
    func settingsOpenURLs(_ urls: [URL]) {
        let tabIsPrivate = TabType.of(tabManager.selectedTab).isPrivate
        self.tabManager.addTabsForURLs(urls, zombie: false, isPrivate: tabIsPrivate)
    }
    
    func settingsDidFinish(_ settingsViewController: SettingsViewController) {
        settingsViewController.dismiss(animated: true, completion: {
            // iPad doesn't receive a viewDidAppear because it displays settings as a floating modal window instead
            // of a fullscreen overlay.
            if UIDevice.isIpad && PrivateBrowsingManager.shared.isPrivateBrowsing {
                DispatchQueue.main.asyncAfter(deadline: .now() + 0.2) {
                    self.presentDuckDuckGoCallout()
                }
            }
        })
    }
}

extension BrowserViewController: PresentingModalViewControllerDelegate {
    func dismissPresentedModalViewController(_ modalViewController: UIViewController, animated: Bool) {
        self.dismiss(animated: animated, completion: nil)
    }
}

/**
 * History visit management.
 * TODO: this should be expanded to track various visit types; see Bug 1166084.
 */
extension BrowserViewController {
    func ignoreNavigationInTab(_ tab: Tab, navigation: WKNavigation) {
        self.ignoredNavigation.insert(navigation)
    }

    func recordNavigationInTab(_ tab: Tab, navigation: WKNavigation, visitType: VisitType) {
        self.typedNavigation[navigation] = visitType
    }

    /**
     * Untrack and do the right thing.
     */
    func getVisitTypeForTab(_ tab: Tab, navigation: WKNavigation?) -> VisitType? {
        guard let navigation = navigation else {
            // See https://github.com/WebKit/webkit/blob/master/Source/WebKit2/UIProcess/Cocoa/NavigationState.mm#L390
            return VisitType.link
        }

        if let _ = self.ignoredNavigation.remove(navigation) {
            return nil
        }

        return self.typedNavigation.removeValue(forKey: navigation) ?? VisitType.link
    }
}

extension BrowserViewController: URLBarDelegate {
    func showTabTray() {
        updateFindInPageVisibility(visible: false)
        
        let tabTrayController = TabTrayController(tabManager: tabManager, profile: profile, tabTrayDelegate: self)
        
        if let tab = tabManager.selectedTab {
            screenshotHelper.takeScreenshot(tab)
        }
        
        navigationController?.pushViewController(tabTrayController, animated: true)
        self.tabTrayController = tabTrayController
    }

    func urlBarDidPressReload(_ urlBar: URLBarView) {
        tabManager.selectedTab?.reload()
    }
    
    func urlBarDidPressQRButton(_ urlBar: URLBarView) {
        let qrCodeViewController = QRCodeViewController()
        qrCodeViewController.qrCodeDelegate = self
        let controller = QRCodeNavigationController(rootViewController: qrCodeViewController)
        self.present(controller, animated: true, completion: nil)
    }
    
    func urlBarDidPressStop(_ urlBar: URLBarView) {
        tabManager.selectedTab?.stop()
    }
    
    func urlBarDidLongPressReloadButton(_ urlBar: URLBarView, from button: UIButton) {
        guard let tab = tabManager.selectedTab else { return }
        let alert = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
        alert.addAction(UIAlertAction(title: Strings.CancelButtonTitle, style: .cancel, handler: nil))
        
        let toggleActionTitle = tab.desktopSite ? Strings.AppMenuViewMobileSiteTitleString : Strings.AppMenuViewDesktopSiteTitleString
        alert.addAction(UIAlertAction(title: toggleActionTitle, style: .default, handler: { _ in
            tab.toggleDesktopSite()
        }))
        
        let generator = UIImpactFeedbackGenerator(style: .heavy)
        generator.impactOccurred()
        if UIDevice.current.userInterfaceIdiom == .pad {
            alert.popoverPresentationController?.sourceView = self.view
            alert.popoverPresentationController?.sourceRect = self.view.convert(button.frame, from: button.superview)
            alert.popoverPresentationController?.permittedArrowDirections = [.up]
        }
        present(alert, animated: true)
    }

    func urlBarDidPressTabs(_ urlBar: URLBarView) {
        showTabTray()
    }

    func urlBarDidPressReaderMode(_ urlBar: URLBarView) {
        if let tab = tabManager.selectedTab {
            if let readerMode = tab.getContentScript(name: "ReaderMode") as? ReaderMode {
                switch readerMode.state {
                case .available:
                    enableReaderMode()
                case .active:
                    disableReaderMode()
                case .unavailable:
                    break
                }
            }
        }
    }

    func urlBarDidLongPressReaderMode(_ urlBar: URLBarView) -> Bool {
        // Maybe we want to add something here down the road
        return false
    }

    func locationActionsForURLBar(_ urlBar: URLBarView) -> [AccessibleAction] {
        if UIPasteboard.general.string != nil {
            return [pasteGoAction, pasteAction, copyAddressAction]
        } else {
            return [copyAddressAction]
        }
    }

    func urlBarDisplayTextForURL(_ url: URL?) -> (String?, Bool) {
        // use the initial value for the URL so we can do proper pattern matching with search URLs
        var searchURL = self.tabManager.selectedTab?.currentInitialURL
        if searchURL?.isErrorPageURL ?? true {
            searchURL = url
        }
        if let query = profile.searchEngines.queryForSearchURL(searchURL as URL?) {
            return (query, true)
        } else {
            return (url?.absoluteString, false)
        }
    }

    func urlBarDidLongPressLocation(_ urlBar: URLBarView) {
        let alert = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
        
        for action in locationActionsForURLBar(urlBar) {
            alert.addAction(action.alertAction(style: .default))
        }
        
        alert.addAction(UIAlertAction(title: Strings.CancelButtonTitle, style: .cancel, handler: nil))
        
        let setupPopover = { [unowned self] in
            if let popoverPresentationController = alert.popoverPresentationController {
                popoverPresentationController.sourceView = urlBar
                popoverPresentationController.sourceRect = urlBar.frame
                popoverPresentationController.permittedArrowDirections = .any
                popoverPresentationController.delegate = self
            }
        }
        
        setupPopover()
        
        if alert.popoverPresentationController != nil {
            displayedPopoverController = alert
            updateDisplayedPopoverProperties = setupPopover
        }
        
        self.present(alert, animated: true)
    }

    func urlBarDidPressScrollToTop(_ urlBar: URLBarView) {
        if let selectedTab = tabManager.selectedTab, favoritesViewController == nil {
            // Only scroll to top if we are not showing the home view controller
            selectedTab.webView?.scrollView.setContentOffset(CGPoint.zero, animated: true)
        }
    }

    func urlBarLocationAccessibilityActions(_ urlBar: URLBarView) -> [UIAccessibilityCustomAction]? {
        return locationActionsForURLBar(urlBar).map { $0.accessibilityCustomAction }
    }

    func urlBar(_ urlBar: URLBarView, didEnterText text: String) {
        if text.isEmpty {
            hideSearchController()
        } else {
            showSearchController()
            searchController?.searchQuery = text
            searchLoader?.query = text
        }
    }

    func urlBar(_ urlBar: URLBarView, didSubmitText text: String) {
        processAddressBar(text: text, visitType: nil)
    }

    func processAddressBar(text: String, visitType: VisitType?) {
        if let fixupURL = URIFixup.getURL(text) {
            // The user entered a URL, so use it.
            finishEditingAndSubmit(fixupURL, visitType: visitType ?? .typed)
            return
        }

        // We couldn't build a URL, so check for a matching search keyword.
        let trimmedText = text.trimmingCharacters(in: .whitespaces)
        guard let possibleKeywordQuerySeparatorSpace = trimmedText.index(of: " ") else {
            submitSearchText(text)
            return
        }

        let possibleKeyword = String(trimmedText[..<possibleKeywordQuerySeparatorSpace])
        let possibleQuery = String(trimmedText[trimmedText.index(after: possibleKeywordQuerySeparatorSpace)])

        profile.bookmarks.getURLForKeywordSearch(possibleKeyword).uponQueue(.main) { result in
            if var urlString = result.successValue,
                let escapedQuery = possibleQuery.addingPercentEncoding(withAllowedCharacters: NSCharacterSet.urlQueryAllowed),
                let range = urlString.range(of: "%s") {
                urlString.replaceSubrange(range, with: escapedQuery)

                if let url = URL(string: urlString) {
                    self.finishEditingAndSubmit(url, visitType: visitType ?? .typed)
                    return
                }
            }

            self.submitSearchText(text)
        }
    }

    fileprivate func submitSearchText(_ text: String) {
        let engine = profile.searchEngines.defaultEngine()

        if let searchURL = engine.searchURLForQuery(text) {
            // We couldn't find a matching search keyword, so do a search query.
            finishEditingAndSubmit(searchURL, visitType: VisitType.typed)
        } else {
            // We still don't have a valid URL, so something is broken. Give up.
            print("Error handling URL entry: \"\(text)\".")
            assertionFailure("Couldn't generate search URL: \(text)")
        }
    }

    func urlBarDidEnterOverlayMode(_ urlBar: URLBarView) {
        if .blankPage == NewTabAccessors.getNewTabPage() {
            UIAccessibility.post(notification: .screenChanged, argument: nil)
        } else {
            if let toast = clipboardBarDisplayHandler?.clipboardToast {
                toast.removeFromSuperview()
            }
            showHomePanelController(inline: false)
        }
    }

    func urlBarDidLeaveOverlayMode(_ urlBar: URLBarView) {
        hideSearchController()
        updateInContentHomePanel(tabManager.selectedTab?.url as URL?)
    }

    func urlBarDidBeginDragInteraction(_ urlBar: URLBarView) {
        dismissVisibleMenus()
    }
    
    func urlBarDidTapBraveShieldsButton(_ urlBar: URLBarView) {
        // BRAVE TODO: Use actual instance
        guard let selectedTab = tabManager.selectedTab else { return }
        let shields = ShieldsViewController(tab: selectedTab)
        shields.shieldsSettingsChanged = { [unowned self] _ in
            // Reload this tab. This will also trigger an update of the brave icon in `TabLocationView` if
            // the setting changed is the global `.AllOff` shield
            self.tabManager.selectedTab?.reload()
            
            // In 1.6 we "reload" the whole web view state, dumping caches, etc. (reload():BraveWebView.swift:495)
            // BRAVE TODO: Port over proper tab reloading with Shields
        }
        let popover = PopoverController(contentController: shields, contentSizeBehavior: .preferredContentSize)
        popover.present(from: urlBar.shieldsButton, on: self)
    }
    
    func urlBarDidTapMenuButton(_ urlBar: URLBarView) {
        guard let selectedTab = tabManager.selectedTab else { return }
        
        let homePanel = HomeMenuController(profile: profile, tabState: selectedTab.tabState)
        homePanel.preferredContentSize = CGSize(width: PopoverController.preferredPopoverWidth, height: 600.0)
        homePanel.delegate = self
        let popover = PopoverController(contentController: homePanel, contentSizeBehavior: .preferredContentSize)
        popover.present(from: urlBar.menuButton, on: self)
    }
}

extension BrowserViewController: TabToolbarDelegate {
    func tabToolbarDidPressBack(_ tabToolbar: TabToolbarProtocol, button: UIButton) {
        tabManager.selectedTab?.goBack()
    }

    func tabToolbarDidLongPressBack(_ tabToolbar: TabToolbarProtocol, button: UIButton) {
        let generator = UIImpactFeedbackGenerator(style: .heavy)
        generator.impactOccurred()
        showBackForwardList()
    }
    
    func tabToolbarDidPressForward(_ tabToolbar: TabToolbarProtocol, button: UIButton) {
        tabManager.selectedTab?.goForward()
    }
    
    func tabToolbarDidPressShare(_ tabToolbar: TabToolbarProtocol, button: UIButton) {
        func share(url: URL) {
            presentActivityViewController(
                url,
                tab: url.isFileURL ? nil : tabManager.selectedTab,
                sourceView: view,
                sourceRect: view.convert(urlBar.shareButton.frame, from: urlBar.shareButton.superview),
                arrowDirection: [.up]
            )
        }
        
        guard let tab = tabManager.selectedTab, let url = tab.url else { return }
        
        if let temporaryDocument = tab.temporaryDocument {
            temporaryDocument.getURL().uponQueue(.main, block: { tempDocURL in
                // If we successfully got a temp file URL, share it like a downloaded file,
                // otherwise present the ordinary share menu for the web URL.
                if tempDocURL.isFileURL {
                    share(url: tempDocURL)
                } else {
                    share(url: url)
                }
            })
        } else {
            share(url: url)
        }
    }
    
    func tabToolbarDidPressAddTab(_ tabToolbar: TabToolbarProtocol, button: UIButton) {
        self.openBlankNewTab(focusLocationField: true, isPrivate: PrivateBrowsingManager.shared.isPrivateBrowsing)
    }

    func tabToolbarDidLongPressAddTab(_ tabToolbar: TabToolbarProtocol, button: UIButton) {
        showAddTabContextMenu(sourceView: toolbar ?? urlBar, button: button)
    }
    
    private func addTabAlertActions() -> [UIAlertAction] {
        var actions: [UIAlertAction] = []
        if !PrivateBrowsingManager.shared.isPrivateBrowsing {
            let newPrivateTabAction = UIAlertAction(title: Strings.NewPrivateTabTitle, style: .default, handler: { [unowned self] _ in
                // BRAVE TODO: Add check for DuckDuckGo popup (and based on 1.6, whether the browser lock is enabled?)
                // before focusing on the url bar
                self.openBlankNewTab(focusLocationField: true, isPrivate: true)
            })
            actions.append(newPrivateTabAction)
        }
        actions.append(UIAlertAction(title: Strings.NewTabTitle, style: .default, handler: { [unowned self] _ in
            self.openBlankNewTab(focusLocationField: true, isPrivate: PrivateBrowsingManager.shared.isPrivateBrowsing)
        }))
        return actions
    }
    
    func showAddTabContextMenu(sourceView: UIView, button: UIButton) {
        let alertController = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
        alertController.addAction(UIAlertAction(title: Strings.CancelButtonTitle, style: .cancel, handler: nil))
        addTabAlertActions().forEach(alertController.addAction)
        alertController.popoverPresentationController?.sourceView = sourceView
        alertController.popoverPresentationController?.sourceRect = button.frame
        let generator = UIImpactFeedbackGenerator(style: .heavy)
        generator.impactOccurred()
        present(alertController, animated: true)
    }
    
    func tabToolbarDidLongPressForward(_ tabToolbar: TabToolbarProtocol, button: UIButton) {
        let generator = UIImpactFeedbackGenerator(style: .heavy)
        generator.impactOccurred()
        showBackForwardList()
    }

    func tabToolbarDidPressTabs(_ tabToolbar: TabToolbarProtocol, button: UIButton) {
        showTabTray()
    }
    
    func tabToolbarDidLongPressTabs(_ tabToolbar: TabToolbarProtocol, button: UIButton) {
        guard self.presentedViewController == nil else {
            return
        }
        let controller = AlertController(title: nil, message: nil, preferredStyle: .actionSheet)
        
        if (UIDevice.current.userInterfaceIdiom == .pad && tabsBar.view.isHidden) ||
            (UIDevice.current.userInterfaceIdiom == .phone && toolbar == nil) {
            addTabAlertActions().forEach(controller.addAction)
        }
        
        if tabManager.tabsForCurrentMode.count > 1 {
            controller.addAction(UIAlertAction(title: String(format: Strings.CloseAllTabsTitle, tabManager.tabsForCurrentMode.count), style: .destructive, handler: { _ in
                self.tabManager.removeAll()
            }), accessibilityIdentifier: "toolbarTabButtonLongPress.closeTab")
        }
        controller.addAction(UIAlertAction(title: Strings.CloseTabTitle, style: .destructive, handler: { _ in
            if let tab = self.tabManager.selectedTab {
                self.tabManager.removeTab(tab)
            }
        }), accessibilityIdentifier: "toolbarTabButtonLongPress.closeTab")
        controller.addAction(UIAlertAction(title: Strings.CancelButtonTitle, style: .cancel, handler: nil), accessibilityIdentifier: "toolbarTabButtonLongPress.cancel")
        controller.popoverPresentationController?.sourceView = toolbar ?? urlBar
        controller.popoverPresentationController?.sourceRect = button.frame
        let generator = UIImpactFeedbackGenerator(style: .heavy)
        generator.impactOccurred()
        present(controller, animated: true, completion: nil)
    }

    func showBackForwardList() {
        if let backForwardList = tabManager.selectedTab?.webView?.backForwardList {
            let backForwardViewController = BackForwardListViewController(profile: profile, backForwardList: backForwardList)
            backForwardViewController.tabManager = tabManager
            backForwardViewController.bvc = self
            backForwardViewController.modalPresentationStyle = .overCurrentContext
            backForwardViewController.backForwardTransitionDelegate = BackForwardListAnimator()
            self.present(backForwardViewController, animated: true, completion: nil)
        }
    }
    
    func tabToolbarDidSwipeToChangeTabs(_ tabToolbar: TabToolbarProtocol, direction: UISwipeGestureRecognizer.Direction) {
        let tabs = tabManager.tabsForCurrentMode
        guard let selectedTab = tabManager.selectedTab, let index = tabs.firstIndex(where: { $0 === selectedTab }) else { return }
        let newTabIndex = index + (direction == .left ? -1 : 1)
        if newTabIndex >= 0 && newTabIndex < tabs.count {
            tabManager.selectTab(tabs[newTabIndex])
        }
    }
}

extension BrowserViewController: TabsBarViewControllerDelegate {
    func tabsBarDidSelectTab(_ tabsBarController: TabsBarViewController, _ tab: Tab) {
        if tab == tabManager.selectedTab { return }
        urlBar.leaveOverlayMode(didCancel: true)
        tabManager.selectTab(tab)
    }
    
    func tabsBarDidLongPressAddTab(_ tabsBarController: TabsBarViewController, button: UIButton) {
        showAddTabContextMenu(sourceView: tabsBarController.view, button: button)
    }
}

extension BrowserViewController: TabDelegate {

    func tab(_ tab: Tab, didCreateWebView webView: WKWebView) {
        webView.frame = webViewContainer.frame
        // Observers that live as long as the tab. Make sure these are all cleared in willDeleteWebView below!
        KVOs.forEach { webView.addObserver(self, forKeyPath: $0.rawValue, options: .new, context: nil) }
        webView.scrollView.addObserver(self.scrollController, forKeyPath: KVOConstants.contentSize.rawValue, options: .new, context: nil)
        webView.uiDelegate = self

        let formPostHelper = FormPostHelper(tab: tab)
        tab.addContentScript(formPostHelper, name: FormPostHelper.name())

        let readerMode = ReaderMode(tab: tab)
        readerMode.delegate = self
        tab.addContentScript(readerMode, name: ReaderMode.name())

        // only add the logins helper if the tab is not a private browsing tab
        if !tab.isPrivate {
            let logins = LoginsHelper(tab: tab, profile: profile)
            tab.addContentScript(logins, name: LoginsHelper.name())
        }

        let contextMenuHelper = ContextMenuHelper(tab: tab)
        contextMenuHelper.delegate = self
        tab.addContentScript(contextMenuHelper, name: ContextMenuHelper.name())

        let errorHelper = ErrorPageHelper()
        tab.addContentScript(errorHelper, name: ErrorPageHelper.name())

        let sessionRestoreHelper = SessionRestoreHelper(tab: tab)
        sessionRestoreHelper.delegate = self
        tab.addContentScript(sessionRestoreHelper, name: SessionRestoreHelper.name())

        let findInPageHelper = FindInPageHelper(tab: tab)
        findInPageHelper.delegate = self
        tab.addContentScript(findInPageHelper, name: FindInPageHelper.name())

        let noImageModeHelper = NoImageModeHelper(tab: tab)
        tab.addContentScript(noImageModeHelper, name: NoImageModeHelper.name())
        
        let printHelper = PrintHelper(tab: tab)
        tab.addContentScript(printHelper, name: PrintHelper.name())

        let customSearchHelper = CustomSearchHelper(tab: tab)
        tab.addContentScript(customSearchHelper, name: CustomSearchHelper.name())

        let nightModeHelper = NightModeHelper(tab: tab)
        tab.addContentScript(nightModeHelper, name: NightModeHelper.name())

        // XXX: Bug 1390200 - Disable NSUserActivity/CoreSpotlight temporarily
        // let spotlightHelper = SpotlightHelper(tab: tab)
        // tab.addHelper(spotlightHelper, name: SpotlightHelper.name())

        tab.addContentScript(LocalRequestHelper(), name: LocalRequestHelper.name())

        let historyStateHelper = HistoryStateHelper(tab: tab)
        historyStateHelper.delegate = self
        tab.addContentScript(historyStateHelper, name: HistoryStateHelper.name())

        tab.contentBlocker.setupTabTrackingProtection()
        tab.addContentScript(tab.contentBlocker, name: ContentBlockerHelper.name())

        tab.addContentScript(FocusHelper(tab: tab), name: FocusHelper.name())
        
        tab.addContentScript(FingerprintingProtection(tab: tab), name: FingerprintingProtection.name())
    }

    func tab(_ tab: Tab, willDeleteWebView webView: WKWebView) {
        tab.cancelQueuedAlerts()
        KVOs.forEach { webView.removeObserver(self, forKeyPath: $0.rawValue) }
        webView.scrollView.removeObserver(self.scrollController, forKeyPath: KVOConstants.contentSize.rawValue)
        webView.uiDelegate = nil
        webView.scrollView.delegate = nil
        webView.removeFromSuperview()
    }

    fileprivate func findSnackbar(_ barToFind: SnackBar) -> Int? {
        let bars = alertStackView.arrangedSubviews
        for (index, bar) in bars.enumerated() where bar === barToFind {
            return index
        }
        return nil
    }

    func showBar(_ bar: SnackBar, animated: Bool) {
        view.layoutIfNeeded()
        UIView.animate(withDuration: animated ? 0.25 : 0, animations: {
            self.alertStackView.insertArrangedSubview(bar, at: 0)
            self.view.layoutIfNeeded()
        })
    }

    func removeBar(_ bar: SnackBar, animated: Bool) {
        UIView.animate(withDuration: animated ? 0.25 : 0, animations: {
            bar.removeFromSuperview()
        })
    }

    func removeAllBars() {
        alertStackView.arrangedSubviews.forEach { $0.removeFromSuperview() }
    }

    func tab(_ tab: Tab, didAddSnackbar bar: SnackBar) {
        showBar(bar, animated: true)
    }

    func tab(_ tab: Tab, didRemoveSnackbar bar: SnackBar) {
        removeBar(bar, animated: true)
    }

    func tab(_ tab: Tab, didSelectFindInPageForSelection selection: String) {
        updateFindInPageVisibility(visible: true)
        findInPageBar?.text = selection
    }
}

extension BrowserViewController: SearchViewControllerDelegate {
    func searchViewController(_ searchViewController: SearchViewController, didSelectURL url: URL) {
        finishEditingAndSubmit(url, visitType: VisitType.typed)
    }

    func searchViewController(_ searchViewController: SearchViewController, didLongPressSuggestion suggestion: String) {
        self.urlBar.setLocation(suggestion, search: true)
    }

    func presentSearchSettingsController() {
        let settingsNavigationController = SearchSettingsTableViewController()
        settingsNavigationController.model = self.profile.searchEngines
        settingsNavigationController.profile = self.profile
        let navController = ModalSettingsNavigationController(rootViewController: settingsNavigationController)

        self.present(navController, animated: true, completion: nil)
    }

    func searchViewController(_ searchViewController: SearchViewController, didHighlightText text: String, search: Bool) {
        self.urlBar.setLocation(text, search: search)
    }
}

extension BrowserViewController: TabManagerDelegate {
    func tabManager(_ tabManager: TabManager, didSelectedTabChange selected: Tab?, previous: Tab?) {
        // Remove the old accessibilityLabel. Since this webview shouldn't be visible, it doesn't need it
        // and having multiple views with the same label confuses tests.
        if let wv = previous?.webView {
            wv.endEditing(true)
            wv.accessibilityLabel = nil
            wv.accessibilityElementsHidden = true
            wv.accessibilityIdentifier = nil
            wv.removeFromSuperview()
        }

        if let tab = selected, let webView = tab.webView {
            updateURLBar()
            
            if let url = tab.url, !url.isLocalUtility, webView.isLoading {
                if webView.estimatedProgress > 0 {
                    urlBar.updateProgressBar(Float(webView.estimatedProgress))
                } else {
                    urlBar.updateProgressBar(URLBarView.psuedoProgressValue)
                }
            } else {
                urlBar.hideProgressBar()
            }

            if tab.type != previous?.type {
                let theme = Theme.of(tab)
                applyTheme(theme)
            }

            readerModeCache = ReaderMode.cache(for: tab)
            ReaderModeHandlers.readerModeCache = readerModeCache

            scrollController.tab = selected
            webViewContainer.addSubview(webView)
            webView.snp.makeConstraints { make in
                make.left.right.top.bottom.equalTo(self.webViewContainer)
            }
            webView.accessibilityLabel = Strings.WebContentAccessibilityLabel
            webView.accessibilityIdentifier = "contentView"
            webView.accessibilityElementsHidden = false

            if webView.url == nil {
                // The web view can go gray if it was zombified due to memory pressure.
                // When this happens, the URL is nil, so try restoring the page upon selection.
                tab.reload()
            }
        }

        if selected?.type != previous?.type {
            updateTabCountUsingTabManager(tabManager)
        }
        
        if PrivateBrowsingManager.shared.isPrivateBrowsing && presentedViewController == nil {
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.4) {
                self.presentDuckDuckGoCallout()
            }
        }

        removeAllBars()
        if let bars = selected?.bars {
            for bar in bars {
                showBar(bar, animated: true)
            }
        }

        updateFindInPageVisibility(visible: false, tab: previous)
        updateTabsBarVisibility()

        urlBar.locationView.loading = selected?.loading ?? false
        navigationToolbar.updateBackStatus(selected?.canGoBack ?? false)
        navigationToolbar.updateForwardStatus(selected?.canGoForward ?? false)
        
        if let readerMode = selected?.getContentScript(name: ReaderMode.name()) as? ReaderMode {
            urlBar.updateReaderModeState(readerMode.state)
            if readerMode.state == .active {
                showReaderModeBar(animated: false)
            } else {
                hideReaderModeBar(animated: false)
            }
        } else {
            urlBar.updateReaderModeState(ReaderModeState.unavailable)
        }

        updateInContentHomePanel(selected?.url as URL?)
    }

    func tabManager(_ tabManager: TabManager, willAddTab tab: Tab) {
    }

    func tabManager(_ tabManager: TabManager, didAddTab tab: Tab) {
        // If we are restoring tabs then we update the count once at the end
        if !tabManager.isRestoring {
            updateTabCountUsingTabManager(tabManager)
        }
        tab.tabDelegate = self
        updateTabsBarVisibility()
    }

    func tabManager(_ tabManager: TabManager, willRemoveTab tab: Tab) {
    }

    func tabManager(_ tabManager: TabManager, didRemoveTab tab: Tab) {
        updateTabCountUsingTabManager(tabManager)
        // tabDelegate is a weak ref (and the tab's webView may not be destroyed yet)
        // so we don't expcitly unset it.
        urlBar.leaveOverlayMode(didCancel: true)
        if let url = tab.url, !url.isAboutURL && !tab.isPrivate {
            profile.recentlyClosedTabs.addTab(url as URL, title: tab.title, faviconURL: tab.displayFavicon?.url)
        }
        updateTabsBarVisibility()
    }

    func tabManagerDidAddTabs(_ tabManager: TabManager) {
        updateTabCountUsingTabManager(tabManager)
    }

    func tabManagerDidRestoreTabs(_ tabManager: TabManager) {
        updateTabCountUsingTabManager(tabManager)
    }

    func show(toast: Toast, afterWaiting delay: DispatchTimeInterval = SimpleToastUX.ToastDelayBefore, duration: DispatchTimeInterval? = SimpleToastUX.ToastDismissAfter) {
        if let downloadToast = toast as? DownloadToast {
            self.downloadToast = downloadToast
        }

        // If BVC isnt visible hold on to this toast until viewDidAppear
        if self.view.window == nil {
            self.pendingToast = toast
            return
        }

        toast.showToast(viewController: self, delay: delay, duration: duration, makeConstraints: { make in
            make.left.right.equalTo(self.view)
            make.bottom.equalTo(self.webViewContainer)
        })
    }
    
    func tabManagerDidRemoveAllTabs(_ tabManager: TabManager, toast: ButtonToast?) {
        guard let toast = toast, !tabTrayController.privateMode else {
            return
        }
        show(toast: toast, afterWaiting: ButtonToastUX.ToastDelay)
    }

    fileprivate func updateTabCountUsingTabManager(_ tabManager: TabManager) {
        let count = tabManager.tabsForCurrentMode.count
        toolbar?.updateTabCount(count)
        urlBar.updateTabCount(count)
    }
}

/// List of schemes that are allowed to be opened in new tabs.
private let schemesAllowedToBeOpenedAsPopups = ["http", "https", "javascript", "about"]

extension BrowserViewController: WKUIDelegate {
    func webView(_ webView: WKWebView, createWebViewWith configuration: WKWebViewConfiguration, for navigationAction: WKNavigationAction, windowFeatures: WKWindowFeatures) -> WKWebView? {
        guard let parentTab = tabManager[webView] else { return nil }

        guard navigationAction.isAllowed, shouldRequestBeOpenedAsPopup(navigationAction.request) else {
            print("Denying popup from request: \(navigationAction.request)")
            return nil
        }

        if let currentTab = tabManager.selectedTab {
            screenshotHelper.takeScreenshot(currentTab)
        }

        // If the page uses `window.open()` or `[target="_blank"]`, open the page in a new tab.
        // IMPORTANT!!: WebKit will perform the `URLRequest` automatically!! Attempting to do
        // the request here manually leads to incorrect results!!
        let newTab = tabManager.addPopupForParentTab(parentTab, configuration: configuration)

        return newTab.webView
    }

    fileprivate func shouldRequestBeOpenedAsPopup(_ request: URLRequest) -> Bool {
        // Treat `window.open("")` the same as `window.open("about:blank")`.
        if request.url?.absoluteString.isEmpty ?? false {
            return true
        }

        if let scheme = request.url?.scheme?.lowercased(), schemesAllowedToBeOpenedAsPopups.contains(scheme) {
            return true
        }

        return false
    }

    fileprivate func shouldDisplayJSAlertForWebView(_ webView: WKWebView) -> Bool {
        // Only display a JS Alert if we are selected and there isn't anything being shown
        return ((tabManager.selectedTab == nil ? false : tabManager.selectedTab!.webView == webView)) && (self.presentedViewController == nil)
    }

    func webView(_ webView: WKWebView, runJavaScriptAlertPanelWithMessage message: String, initiatedByFrame frame: WKFrameInfo, completionHandler: @escaping () -> Void) {
        let messageAlert = MessageAlert(message: message, frame: frame, completionHandler: completionHandler)
        if shouldDisplayJSAlertForWebView(webView) {
            present(messageAlert.alertController(), animated: true, completion: nil)
        } else if let promptingTab = tabManager[webView] {
            promptingTab.queueJavascriptAlertPrompt(messageAlert)
        } else {
            // This should never happen since an alert needs to come from a web view but just in case call the handler
            // since not calling it will result in a runtime exception.
            completionHandler()
        }
    }

    func webView(_ webView: WKWebView, runJavaScriptConfirmPanelWithMessage message: String, initiatedByFrame frame: WKFrameInfo, completionHandler: @escaping (Bool) -> Void) {
        let confirmAlert = ConfirmPanelAlert(message: message, frame: frame, completionHandler: completionHandler)
        if shouldDisplayJSAlertForWebView(webView) {
            present(confirmAlert.alertController(), animated: true, completion: nil)
        } else if let promptingTab = tabManager[webView] {
            promptingTab.queueJavascriptAlertPrompt(confirmAlert)
        } else {
            completionHandler(false)
        }
    }

    func webView(_ webView: WKWebView, runJavaScriptTextInputPanelWithPrompt prompt: String, defaultText: String?, initiatedByFrame frame: WKFrameInfo, completionHandler: @escaping (String?) -> Void) {
        let textInputAlert = TextInputAlert(message: prompt, frame: frame, completionHandler: completionHandler, defaultText: defaultText)
        if shouldDisplayJSAlertForWebView(webView) {
            present(textInputAlert.alertController(), animated: true, completion: nil)
        } else if let promptingTab = tabManager[webView] {
            promptingTab.queueJavascriptAlertPrompt(textInputAlert)
        } else {
            completionHandler(nil)
        }
    }

    /// Invoked when an error occurs while starting to load data for the main frame.
    func webView(_ webView: WKWebView, didFailProvisionalNavigation navigation: WKNavigation!, withError error: Error) {
        // Ignore the "Frame load interrupted" error that is triggered when we cancel a request
        // to open an external application and hand it over to UIApplication.openURL(). The result
        // will be that we switch to the external app, for example the app store, while keeping the
        // original web page in the tab instead of replacing it with an error page.
        let error = error as NSError
        if error.domain == "WebKitErrorDomain" && error.code == 102 {
            return
        }

        if checkIfWebContentProcessHasCrashed(webView, error: error as NSError) {
            return
        }

        if error.code == Int(CFNetworkErrors.cfurlErrorCancelled.rawValue) {
            if let tab = tabManager[webView], tab === tabManager.selectedTab {
                urlBar.currentURL = tab.url?.displayURL
            }
            return
        }

        if let url = error.userInfo[NSURLErrorFailingURLErrorKey] as? URL {
            ErrorPageHelper().showPage(error, forUrl: url, inWebView: webView)

            // If the local web server isn't working for some reason (Firefox cellular data is
            // disabled in settings, for example), we'll fail to load the session restore URL.
            // We rely on loading that page to get the restore callback to reset the restoring
            // flag, so if we fail to load that page, reset it here.
            if url.aboutComponent == "sessionrestore" {
                tabManager.allTabs.filter { $0.webView == webView }.first?.restoring = false
            }
        }
    }

    fileprivate func checkIfWebContentProcessHasCrashed(_ webView: WKWebView, error: NSError) -> Bool {
        if error.code == WKError.webContentProcessTerminated.rawValue && error.domain == "WebKitErrorDomain" {
            print("WebContent process has crashed. Trying to reload to restart it.")
            webView.reload()
            return true
        }

        return false
    }

    func webViewDidClose(_ webView: WKWebView) {
        if let tab = tabManager[webView] {
            self.tabManager.removeTab(tab)
        }
    }
}

extension BrowserViewController: ReaderModeDelegate {
    func readerMode(_ readerMode: ReaderMode, didChangeReaderModeState state: ReaderModeState, forTab tab: Tab) {
        // If this reader mode availability state change is for the tab that we currently show, then update
        // the button. Otherwise do nothing and the button will be updated when the tab is made active.
        if tabManager.selectedTab === tab {
            urlBar.updateReaderModeState(state)
        }
    }

    func readerMode(_ readerMode: ReaderMode, didDisplayReaderizedContentForTab tab: Tab) {
        self.showReaderModeBar(animated: true)
        tab.showContent(true)
    }

    func readerMode(_ readerMode: ReaderMode, didParseReadabilityResult readabilityResult: ReadabilityResult, forTab tab: Tab) {
    }
}

// MARK: - UIPopoverPresentationControllerDelegate

extension BrowserViewController: UIPopoverPresentationControllerDelegate {
    func popoverPresentationControllerDidDismissPopover(_ popoverPresentationController: UIPopoverPresentationController) {
        displayedPopoverController = nil
        updateDisplayedPopoverProperties = nil
    }
}

extension BrowserViewController: UIAdaptivePresentationControllerDelegate {
    // Returning None here makes sure that the Popover is actually presented as a Popover and
    // not as a full-screen modal, which is the default on compact device classes.
    func adaptivePresentationStyle(for controller: UIPresentationController, traitCollection: UITraitCollection) -> UIModalPresentationStyle {
        return .none
    }
}

// MARK: - ReaderModeStyleViewControllerDelegate

extension BrowserViewController: ReaderModeStyleViewControllerDelegate {
    func readerModeStyleViewController(_ readerModeStyleViewController: ReaderModeStyleViewController, didConfigureStyle style: ReaderModeStyle) {
        // Persist the new style to the profile
        let encodedStyle: [String: Any] = style.encodeAsDictionary()
        profile.prefs.setObject(encodedStyle, forKey: ReaderModeProfileKeyStyle)
        // Change the reader mode style on all tabs that have reader mode active
        for tabIndex in 0..<tabManager.count {
            if let tab = tabManager[tabIndex] {
                if let readerMode = tab.getContentScript(name: "ReaderMode") as? ReaderMode {
                    if readerMode.state == ReaderModeState.active {
                        readerMode.style = style
                    }
                }
            }
        }
    }
}

extension BrowserViewController {
    func updateReaderModeBar() {
        if let readerModeBar = readerModeBar {
            let theme = Theme.of(tabManager.selectedTab)
            readerModeBar.applyTheme(theme)
        }
    }

    func showReaderModeBar(animated: Bool) {
        if self.readerModeBar == nil {
            let readerModeBar = ReaderModeBarView(frame: CGRect.zero)
            readerModeBar.delegate = self
            view.insertSubview(readerModeBar, belowSubview: header)
            self.readerModeBar = readerModeBar
        }

        updateReaderModeBar()

        self.updateViewConstraints()
    }

    func hideReaderModeBar(animated: Bool) {
        if let readerModeBar = self.readerModeBar {
            readerModeBar.removeFromSuperview()
            self.readerModeBar = nil
            self.updateViewConstraints()
        }
    }

    /// There are two ways we can enable reader mode. In the simplest case we open a URL to our internal reader mode
    /// and be done with it. In the more complicated case, reader mode was already open for this page and we simply
    /// navigated away from it. So we look to the left and right in the BackForwardList to see if a readerized version
    /// of the current page is there. And if so, we go there.

    func enableReaderMode() {
        guard let tab = tabManager.selectedTab, let webView = tab.webView else { return }

        let backList = webView.backForwardList.backList
        let forwardList = webView.backForwardList.forwardList

        guard let currentURL = webView.backForwardList.currentItem?.url, let readerModeURL = currentURL.encodeReaderModeURL(WebServer.sharedInstance.baseReaderModeURL()) else { return }

        if backList.count > 1 && backList.last?.url == readerModeURL {
            webView.go(to: backList.last!)
        } else if forwardList.count > 0 && forwardList.first?.url == readerModeURL {
            webView.go(to: forwardList.first!)
        } else {
            // Store the readability result in the cache and load it. This will later move to the ReadabilityHelper.
            webView.evaluateJavaScript("\(ReaderModeNamespace).readerize()", completionHandler: { (object, error) -> Void in
                if let readabilityResult = ReadabilityResult(object: object as AnyObject?) {
                    do {
                        try self.readerModeCache.put(currentURL, readabilityResult)
                    } catch _ {
                    }
                    if let nav = webView.load(PrivilegedRequest(url: readerModeURL) as URLRequest) {
                        self.ignoreNavigationInTab(tab, navigation: nav)
                    }
                }
            })
        }
    }

    /// Disabling reader mode can mean two things. In the simplest case we were opened from the reading list, which
    /// means that there is nothing in the BackForwardList except the internal url for the reader mode page. In that
    /// case we simply open a new page with the original url. In the more complicated page, the non-readerized version
    /// of the page is either to the left or right in the BackForwardList. If that is the case, we navigate there.

    func disableReaderMode() {
        if let tab = tabManager.selectedTab,
            let webView = tab.webView {
            let backList = webView.backForwardList.backList
            let forwardList = webView.backForwardList.forwardList

            if let currentURL = webView.backForwardList.currentItem?.url {
                if let originalURL = currentURL.decodeReaderModeURL {
                    if backList.count > 1 && backList.last?.url == originalURL {
                        webView.go(to: backList.last!)
                    } else if forwardList.count > 0 && forwardList.first?.url == originalURL {
                        webView.go(to: forwardList.first!)
                    } else {
                        if let nav = webView.load(URLRequest(url: originalURL)) {
                            self.ignoreNavigationInTab(tab, navigation: nav)
                        }
                    }
                }
            }
        }
    }

    @objc func dynamicFontChanged(_ notification: Notification) {
        guard notification.name == .DynamicFontChanged else { return }

        var readerModeStyle = DefaultReaderModeStyle
        if let dict = profile.prefs.dictionaryForKey(ReaderModeProfileKeyStyle) {
            if let style = ReaderModeStyle(dict: dict as [String: AnyObject]) {
                readerModeStyle = style
            }
        }
        readerModeStyle.fontSize = ReaderModeFontSize.defaultSize
        self.readerModeStyleViewController(ReaderModeStyleViewController(), didConfigureStyle: readerModeStyle)
    }
}

extension BrowserViewController: ReaderModeBarViewDelegate {
    func readerModeBar(_ readerModeBar: ReaderModeBarView, didSelectButton buttonType: ReaderModeBarButtonType) {
        switch buttonType {
        case .settings:
            if let readerMode = tabManager.selectedTab?.getContentScript(name: "ReaderMode") as? ReaderMode, readerMode.state == ReaderModeState.active {
                var readerModeStyle = DefaultReaderModeStyle
                if let dict = profile.prefs.dictionaryForKey(ReaderModeProfileKeyStyle) {
                    if let style = ReaderModeStyle(dict: dict as [String: AnyObject]) {
                        readerModeStyle = style
                    }
                }

                let readerModeStyleViewController = ReaderModeStyleViewController()
                readerModeStyleViewController.delegate = self
                readerModeStyleViewController.readerModeStyle = readerModeStyle
                readerModeStyleViewController.modalPresentationStyle = .popover

                let setupPopover = { [unowned self] in
                    if let popoverPresentationController = readerModeStyleViewController.popoverPresentationController {
                        popoverPresentationController.backgroundColor = UIColor.Photon.White100
                        popoverPresentationController.delegate = self
                        popoverPresentationController.sourceView = readerModeBar
                        popoverPresentationController.sourceRect = CGRect(x: readerModeBar.frame.width/2, y: UIConstants.ToolbarHeight, width: 1, height: 1)
                        popoverPresentationController.permittedArrowDirections = .up
                    }
                }

                setupPopover()

                if readerModeStyleViewController.popoverPresentationController != nil {
                    displayedPopoverController = readerModeStyleViewController
                    updateDisplayedPopoverProperties = setupPopover
                }

                self.present(readerModeStyleViewController, animated: true, completion: nil)
            }
        }
    }
}

extension BrowserViewController: ContextMenuHelperDelegate {
    func contextMenuHelper(_ contextMenuHelper: ContextMenuHelper, didLongPressElements elements: ContextMenuHelper.Elements, gestureRecognizer: UIGestureRecognizer) {
        // locationInView can return (0, 0) when the long press is triggered in an invalid page
        // state (e.g., long pressing a link before the document changes, then releasing after a
        // different page loads).
        let touchPoint = gestureRecognizer.location(in: view)
        guard touchPoint != CGPoint.zero else { return }

        let touchSize = CGSize(width: 0, height: 16)

        let actionSheetController = AlertController(title: nil, message: nil, preferredStyle: .actionSheet)
        var dialogTitle: String?

        if let url = elements.link, let currentTab = tabManager.selectedTab {
            dialogTitle = url.absoluteString
            let tabType = currentTab.type

            let addTab = { (rURL: URL, isPrivate: Bool) in
                let tab = self.tabManager.addTab(URLRequest(url: rURL as URL), afterTab: currentTab, isPrivate: isPrivate)
                if isPrivate && !PrivateBrowsingManager.shared.isPrivateBrowsing {
                    self.tabManager.selectTab(tab)
                } else {
                    // We're not showing the top tabs; show a toast to quick switch to the fresh new tab.
                    let toast = ButtonToast(labelText: Strings.ContextMenuButtonToastNewTabOpenedLabelText, buttonText: Strings.ContextMenuButtonToastNewTabOpenedButtonText, completion: { buttonPressed in
                        if buttonPressed {
                            self.tabManager.selectTab(tab)
                        }
                    })
                    self.show(toast: toast)
                }
                self.scrollController.showToolbars(animated: true)
            }

            if !tabType.isPrivate {
                let openNewTabAction =  UIAlertAction(title: Strings.OpenNewTabButtonTitle, style: .default) { _ in
                    addTab(url, false)
                }
                actionSheetController.addAction(openNewTabAction, accessibilityIdentifier: "linkContextMenu.openInNewTab")
            }
           
            let openNewPrivateTabAction =  UIAlertAction(title: Strings.OpenNewPrivateTabButtonTitle, style: .default) { _ in
                addTab(url, true)
            }
            actionSheetController.addAction(openNewPrivateTabAction, accessibilityIdentifier: "linkContextMenu.openInNewPrivateTab")

            let copyAction = UIAlertAction(title: Strings.CopyLinkActionTitle, style: .default) { _ in
                UIPasteboard.general.url = url as URL
            }
            actionSheetController.addAction(copyAction, accessibilityIdentifier: "linkContextMenu.copyLink")

            let shareAction = UIAlertAction(title: Strings.ShareLinkActionTitle, style: .default) { _ in
                self.presentActivityViewController(url as URL, sourceView: self.view, sourceRect: CGRect(origin: touchPoint, size: touchSize), arrowDirection: .any)
            }
            actionSheetController.addAction(shareAction, accessibilityIdentifier: "linkContextMenu.share")
        }

        if let url = elements.image {
            if dialogTitle == nil {
                dialogTitle = url.absoluteString
            }
            
            let openInNewTabAction = UIAlertAction(title: Strings.OpenImageInNewTabActionTitle, style: .default) { _ in
                let isPrivate = PrivateBrowsingManager.shared.isPrivateBrowsing
                self.tabManager.addTab(URLRequest(url: url), afterTab: self.tabManager.selectedTab, isPrivate: isPrivate)
                self.scrollController.showToolbars(animated: true)
            }
            actionSheetController.addAction(openInNewTabAction, accessibilityIdentifier: "linkContextMenu.openImageInNewTab")

            let photoAuthorizeStatus = PHPhotoLibrary.authorizationStatus()
            let saveImageAction = UIAlertAction(title: Strings.SaveImageActionTitle, style: .default) { _ in
                if photoAuthorizeStatus == .authorized || photoAuthorizeStatus == .notDetermined {
                    self.getData(url) { data in
                        PHPhotoLibrary.shared().performChanges({
                            PHAssetCreationRequest.forAsset().addResource(with: .photo, data: data, options: nil)
                        }, completionHandler: nil)
                    }
                } else {
                    let accessDenied = UIAlertController(title: Strings.AccessPhotoDeniedAlertTitle, message: Strings.AccessPhotoDeniedAlertMessage, preferredStyle: .alert)
                    let dismissAction = UIAlertAction(title: Strings.CancelButtonTitle, style: .default, handler: nil)
                    accessDenied.addAction(dismissAction)
                    let settingsAction = UIAlertAction(title: Strings.OpenPhoneSettingsActionTitle, style: .default ) { _ in
                        UIApplication.shared.open(URL(string: UIApplication.openSettingsURLString)!, options: [:])
                    }
                    accessDenied.addAction(settingsAction)
                    self.present(accessDenied, animated: true, completion: nil)
                }
            }
            actionSheetController.addAction(saveImageAction, accessibilityIdentifier: "linkContextMenu.saveImage")

            let copyAction = UIAlertAction(title: Strings.CopyImageActionTitle, style: .default) { _ in
                // put the actual image on the clipboard
                // do this asynchronously just in case we're in a low bandwidth situation
                let pasteboard = UIPasteboard.general
                pasteboard.url = url as URL
                let changeCount = pasteboard.changeCount
                let application = UIApplication.shared
                var taskId: UIBackgroundTaskIdentifier = UIBackgroundTaskIdentifier(rawValue: 0)
                taskId = application.beginBackgroundTask (expirationHandler: {
                    application.endBackgroundTask(taskId)
                })

                Alamofire.request(url).validate(statusCode: 200..<300).response { response in
                    // Only set the image onto the pasteboard if the pasteboard hasn't changed since
                    // fetching the image; otherwise, in low-bandwidth situations,
                    // we might be overwriting something that the user has subsequently added.
                    if changeCount == pasteboard.changeCount, let imageData = response.data, response.error == nil {
                        pasteboard.addImageWithData(imageData, forURL: url)
                    }

                    application.endBackgroundTask(taskId)
                }
            }
            actionSheetController.addAction(copyAction, accessibilityIdentifier: "linkContextMenu.copyImage")
        }

        // If we're showing an arrow popup, set the anchor to the long press location.
        if let popoverPresentationController = actionSheetController.popoverPresentationController {
            popoverPresentationController.sourceView = view
            popoverPresentationController.sourceRect = CGRect(origin: touchPoint, size: touchSize)
            popoverPresentationController.permittedArrowDirections = .any
            popoverPresentationController.delegate = self
        }

        if actionSheetController.popoverPresentationController != nil {
            displayedPopoverController = actionSheetController
        }

        actionSheetController.title = dialogTitle?.ellipsize(maxLength: ActionSheetTitleMaxLength)
        let cancelAction = UIAlertAction(title: Strings.CancelButtonTitle, style: .cancel, handler: nil)
        actionSheetController.addAction(cancelAction)
        self.present(actionSheetController, animated: true, completion: nil)
    }

    private func getData(_ url: URL, success: @escaping (Data) -> Void) {
        Alamofire.request(url).validate(statusCode: 200..<300).response { response in
            if let data = response.data {
                success(data)
            }
        }
    }

    func contextMenuHelper(_ contextMenuHelper: ContextMenuHelper, didCancelGestureRecognizer: UIGestureRecognizer) {
        displayedPopoverController?.dismiss(animated: true) {
            self.displayedPopoverController = nil
        }
    }
}

extension BrowserViewController: HistoryStateHelperDelegate {
    func historyStateHelper(_ historyStateHelper: HistoryStateHelper, didPushOrReplaceStateInTab tab: Tab) {
        navigateInTab(tab: tab)
        tabManager.saveTab(tab)
    }
}

/**
 A third party search engine Browser extension
**/
extension BrowserViewController {

    func addCustomSearchButtonToWebView(_ webView: WKWebView) {
        // For now we're going to just not add the custom search button to the web view
        // TODO: #586 Re-enable custom search engines button or remove entirely
        return
        
        /*
        //check if the search engine has already been added.
        let domain = webView.url?.domainURL.host
        let matches = self.profile.searchEngines.orderedEngines.filter {$0.shortName == domain}
        if !matches.isEmpty {
            self.customSearchEngineButton.tintColor = UIColor.Photon.Grey50
            self.customSearchEngineButton.isUserInteractionEnabled = false
        } else {
            self.customSearchEngineButton.tintColor = UIConstants.SystemBlueColor
            self.customSearchEngineButton.isUserInteractionEnabled = true
        }

        /*
         This is how we access hidden views in the WKContentView
         Using the public headers we can find the keyboard accessoryView which is not usually available.
         Specific values here are from the WKContentView headers.
         https://github.com/JaviSoto/iOS9-Runtime-Headers/blob/master/Frameworks/WebKit.framework/WKContentView.h
        */
        guard let webContentView = UIView.findSubViewWithFirstResponder(webView) else {
            /*
             In some cases the URL bar can trigger the keyboard notification. In that case the webview isnt the first responder
             and a search button should not be added.
             */
            return
        }

        guard let input = webContentView.perform(#selector(getter: UIResponder.inputAccessoryView)),
            let inputView = input.takeUnretainedValue() as? UIInputView,
            let nextButton = inputView.value(forKey: "_nextItem") as? UIBarButtonItem,
            let nextButtonView = nextButton.value(forKey: "view") as? UIView else {
                //failed to find the inputView instead lets use the inputAssistant
                addCustomSearchButtonToInputAssistant(webContentView)
                return
            }
            inputView.addSubview(self.customSearchEngineButton)
            self.customSearchEngineButton.snp.remakeConstraints { make in
                make.leading.equalTo(nextButtonView.snp.trailing).offset(20)
                make.width.equalTo(inputView.snp.height)
                make.top.equalTo(nextButtonView.snp.top)
                make.height.equalTo(inputView.snp.height)
            }
        */
    }

    /**
     This adds the customSearchButton to the inputAssistant
     for cases where the inputAccessoryView could not be found for example
     on the iPad where it does not exist. However this only works on iOS9
     **/
    func addCustomSearchButtonToInputAssistant(_ webContentView: UIView) {
        guard customSearchBarButton == nil else {
            return //The searchButton is already on the keyboard
        }
        let inputAssistant = webContentView.inputAssistantItem
        let item = UIBarButtonItem(customView: customSearchEngineButton)
        customSearchBarButton = item
        _ = Try(withTry: {
            inputAssistant.trailingBarButtonGroups.last?.barButtonItems.append(item)
        }) { exception in
            log.error("Failed adding custom search button to input assistant: \(String(describing: exception))")
        }
    }

    @objc func addCustomSearchEngineForFocusedElement() {
        guard let webView = tabManager.selectedTab?.webView else {
            return
        }
        webView.evaluateJavaScript("__firefox__.searchQueryForField()") { (result, _) in
            guard let searchQuery = result as? String, let favicon = self.tabManager.selectedTab!.displayFavicon else {
                //Javascript responded with an incorrectly formatted message. Show an error.
                let alert = ThirdPartySearchAlerts.failedToAddThirdPartySearch()
                self.present(alert, animated: true, completion: nil)
                return
            }
            self.addSearchEngine(searchQuery, favicon: favicon)
            self.customSearchEngineButton.tintColor = UIColor.Photon.Grey50
            self.customSearchEngineButton.isUserInteractionEnabled = false
        }
    }

    func addSearchEngine(_ searchQuery: String, favicon: Favicon) {
        guard searchQuery != "",
            let iconURL = URL(string: favicon.url),
            let url = URL(string: searchQuery.addingPercentEncoding(withAllowedCharacters: CharacterSet.urlFragmentAllowed)!),
            let shortName = url.domainURL.host else {
                let alert = ThirdPartySearchAlerts.failedToAddThirdPartySearch()
                self.present(alert, animated: true, completion: nil)
                return
        }

        let alert = ThirdPartySearchAlerts.addThirdPartySearchEngine { alert in
            self.customSearchEngineButton.tintColor = UIColor.Photon.Grey50
            self.customSearchEngineButton.isUserInteractionEnabled = false

            WebImageCacheManager.shared.load(from: iconURL) { (image, _, _, _, _) in
                guard let image = image else {
                    let alert = ThirdPartySearchAlerts.failedToAddThirdPartySearch()
                    self.present(alert, animated: true, completion: nil)
                    return
                }

                self.profile.searchEngines.addSearchEngine(OpenSearchEngine(engineID: nil, shortName: shortName, image: image, searchTemplate: searchQuery, suggestTemplate: nil, isCustomEngine: true))
                let Toast = SimpleToast()
                Toast.showAlertWithText(Strings.ThirdPartySearchEngineAdded, bottomContainer: self.webViewContainer)
            }
        }

        self.present(alert, animated: true, completion: {})
    }
}

extension BrowserViewController: KeyboardHelperDelegate {
    func keyboardHelper(_ keyboardHelper: KeyboardHelper, keyboardWillShowWithState state: KeyboardState) {
        keyboardState = state
        updateViewConstraints()

        UIView.animate(withDuration: state.animationDuration) {
            UIView.setAnimationCurve(state.animationCurve)
            self.alertStackView.layoutIfNeeded()
        }

        guard let webView = tabManager.selectedTab?.webView else {
            return
        }
        webView.evaluateJavaScript("__firefox__.searchQueryForField()") { (result, _) in
            guard let _ = result as? String else {
                return
            }
            self.addCustomSearchButtonToWebView(webView)
        }
    }

    func keyboardHelper(_ keyboardHelper: KeyboardHelper, keyboardDidShowWithState state: KeyboardState) {

    }

    func keyboardHelper(_ keyboardHelper: KeyboardHelper, keyboardWillHideWithState state: KeyboardState) {
        keyboardState = nil
        updateViewConstraints()
        //If the searchEngineButton exists remove it form the keyboard
        if let buttonGroup = customSearchBarButton?.buttonGroup {
            buttonGroup.barButtonItems = buttonGroup.barButtonItems.filter { $0 != customSearchBarButton }
            customSearchBarButton = nil
        }

        if self.customSearchEngineButton.superview != nil {
            self.customSearchEngineButton.removeFromSuperview()
        }

        UIView.animate(withDuration: state.animationDuration) {
            UIView.setAnimationCurve(state.animationCurve)
            self.alertStackView.layoutIfNeeded()
        }
    }
}

extension BrowserViewController: SessionRestoreHelperDelegate {
    func sessionRestoreHelper(_ helper: SessionRestoreHelper, didRestoreSessionForTab tab: Tab) {
        tab.restoring = false

        if let tab = tabManager.selectedTab, tab.webView === tab.webView {
            updateUIForReaderHomeStateForTab(tab)
        }
    }
}

extension BrowserViewController: TabTrayDelegate {
    // This function animates and resets the tab chrome transforms when
    // the tab tray dismisses.
    func tabTrayDidDismiss(_ tabTray: TabTrayController) {
        // BRAVE TODO: Add update tabs method?
        resetBrowserChrome()
    }

    func tabTrayDidAddTab(_ tabTray: TabTrayController, tab: Tab) {
        // BRAVE TODO: Add update tabs method?
    }

    func tabTrayDidAddBookmark(_ tab: Tab) {
        guard let url = tab.url?.absoluteString, !url.isEmpty else { return }
        self.addBookmark(tab.tabState)
    }

    func tabTrayRequestsPresentationOf(_ viewController: UIViewController) {
        self.present(viewController, animated: false, completion: nil)
    }
}

// MARK: Browser Chrome Theming
extension BrowserViewController: Themeable {

    func applyTheme(_ theme: Theme) {
        let ui: [Themeable?] = [urlBar, toolbar, readerModeBar, tabsBar, favoritesViewController]
        ui.forEach { $0?.applyTheme(theme) }
        statusBarOverlay.backgroundColor = urlBar.backgroundColor
        setNeedsStatusBarAppearanceUpdate()
    }
}

extension BrowserViewController: FindInPageBarDelegate, FindInPageHelperDelegate {
    func findInPage(_ findInPage: FindInPageBar, didTextChange text: String) {
        find(text, function: "find")
    }

    func findInPage(_ findInPage: FindInPageBar, didFindNextWithText text: String) {
        findInPageBar?.endEditing(true)
        find(text, function: "findNext")
    }

    func findInPage(_ findInPage: FindInPageBar, didFindPreviousWithText text: String) {
        findInPageBar?.endEditing(true)
        find(text, function: "findPrevious")
    }

    func findInPageDidPressClose(_ findInPage: FindInPageBar) {
        updateFindInPageVisibility(visible: false)
    }

    fileprivate func find(_ text: String, function: String) {
        guard let webView = tabManager.selectedTab?.webView else { return }
        let escaped = text.replacingOccurrences(of: "\\", with: "\\\\").replacingOccurrences(of: "\"", with: "\\\"")
        webView.evaluateJavaScript("__firefox__.\(function)(\"\(escaped)\")", completionHandler: nil)
    }

    func findInPageHelper(_ findInPageHelper: FindInPageHelper, didUpdateCurrentResult currentResult: Int) {
        findInPageBar?.currentResult = currentResult
    }

    func findInPageHelper(_ findInPageHelper: FindInPageHelper, didUpdateTotalResults totalResults: Int) {
        findInPageBar?.totalResults = totalResults
    }
}

extension BrowserViewController: JSPromptAlertControllerDelegate {
    func promptAlertControllerDidDismiss(_ alertController: JSPromptAlertController) {
        showQueuedAlertIfAvailable()
    }
}

extension BrowserViewController: HomeMenuControllerDelegate {
    
    func menuDidOpenSettings(_ menu: HomeMenuController) {
        menu.dismiss(animated: true) { [weak self] in
            guard let `self` = self else { return }
            let settingsController = SettingsViewController(profile: self.profile, tabManager: self.tabManager)
            settingsController.settingsDelegate = self
            let container = SettingsNavigationController(rootViewController: settingsController)
            container.modalPresentationStyle = .formSheet
            self.present(container, animated: true)
        }
    }
    
    func menuDidSelectURL(_ menu: HomeMenuController, url: URL, visitType: VisitType, action: MenuURLAction) {
        switch action {
        case .openInCurrentTab:
            menu.dismiss(animated: true)
            finishEditingAndSubmit(url, visitType: visitType)
            
        case .openInNewTab(let isPrivate):
            menu.dismiss(animated: true)
            
            let tab = self.tabManager.addTab(PrivilegedRequest(url: url) as URLRequest, afterTab: self.tabManager.selectedTab, isPrivate: isPrivate)
            if isPrivate && !PrivateBrowsingManager.shared.isPrivateBrowsing {
                self.tabManager.selectTab(tab)
            } else {
                // If we are showing toptabs a user can just use the top tab bar
                // If in overlay mode switching doesnt correctly dismiss the homepanels
                guard !self.urlBar.inOverlayMode else {
                    return
                }
                // We're not showing the top tabs; show a toast to quick switch to the fresh new tab.
                let toast = ButtonToast(labelText: Strings.ContextMenuButtonToastNewTabOpenedLabelText, buttonText: Strings.ContextMenuButtonToastNewTabOpenedButtonText, completion: { buttonPressed in
                    if buttonPressed {
                        self.tabManager.selectTab(tab)
                    }
                })
                self.show(toast: toast)
            }
            
        case .copy:
            UIPasteboard.general.url = url
        case .share:
            menu.dismiss(animated: true) {
                self.presentActivityViewController(
                    url,
                    sourceView: self.view,
                    sourceRect: self.view.convert(self.urlBar.shareButton.frame, from: self.urlBar.shareButton.superview),
                    arrowDirection: [.up]
                )
            }
        }
    }
    
    func menuDidBatchOpenURLs(_ menu: HomeMenuController, urls: [URL]) {
        let tabIsPrivate = TabType.of(tabManager.selectedTab).isPrivate
        self.tabManager.addTabsForURLs(urls, zombie: false, isPrivate: tabIsPrivate)
    }
}

extension BrowserViewController: TopSitesDelegate {
    
    func didSelect(input: String) {
        processAddressBar(text: input, visitType: .bookmark)
    }
    
    func didTapDuckDuckGoCallout() {
        presentDuckDuckGoCallout(force: true)
    }
}

extension BrowserViewController: PreferencesObserver {
    func preferencesDidChange(for key: String) {
        switch key {
        case Preferences.General.tabBarVisibility.key:
            updateTabsBarVisibility()
        case Preferences.Privacy.privateBrowsingOnly.key:
            let isPrivate = Preferences.Privacy.privateBrowsingOnly.value
            switchToPrivacyMode(isPrivate: isPrivate)
            PrivateBrowsingManager.shared.isPrivateBrowsing = isPrivate
            setupTabs()
            updateTabsBarVisibility()
        case Preferences.Shields.blockAdsAndTracking.key,
             Preferences.Shields.httpsEverywhere.key,
             Preferences.Shields.blockScripts.key,
             Preferences.Shields.blockPhishingAndMalware.key,
             Preferences.Shields.blockImages.key,
             Preferences.Shields.fingerprintingProtection.key,
             Preferences.Shields.useRegionAdBlock.key:
            tabManager.allTabs.forEach { $0.webView?.reload() }
        case Preferences.Privacy.blockAllCookies.key:
            // All `block all cookies` toggle requires a hard reset of Webkit configuration.
            tabManager.reset()
            if !Preferences.Privacy.blockAllCookies.value {
                tabManager.allTabs.forEach {
                    if let url: URL = $0.webView?.url {
                        $0.loadRequest(PrivilegedRequest(url: url) as URLRequest)
                    }
                }
            }
        default:
            log.debug("Received a preference change for an unknown key: \(key) on \(type(of: self))")
            break
        }
    }
}

