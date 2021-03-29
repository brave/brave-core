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
import MobileCoreServices
import SwiftyJSON
import Data
import BraveShared
import SwiftKeychainWrapper
import BraveRewards
import StoreKit
import SafariServices
import BraveUI
import NetworkExtension
import YubiKit
import FeedKit

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

private struct BrowserViewControllerUX {
    fileprivate static let showHeaderTapAreaHeight: CGFloat = 32
    fileprivate static let bookmarkStarAnimationDuration: Double = 0.5
    fileprivate static let bookmarkStarAnimationOffset: CGFloat = 80
}

class BrowserViewController: UIViewController {
    var webViewContainer: UIView!
    var topToolbar: TopToolbarView!
    var tabsBar: TabsBarViewController!
    var clipboardBarDisplayHandler: ClipboardBarDisplayHandler?
    var readerModeBar: ReaderModeBarView?
    var readerModeCache: ReaderModeCache
    var statusBarOverlay: UIView!
    fileprivate(set) var toolbar: BottomToolbarView?
    var searchController: SearchViewController?
    fileprivate var screenshotHelper: ScreenshotHelper!
    fileprivate var homePanelIsInline = false
    fileprivate var searchLoader: SearchLoader?
    let alertStackView = UIStackView() // All content that appears above the footer should be added to this view. (Find In Page/SnackBars)
    fileprivate var findInPageBar: FindInPageBar?
    
    // Single data source used for all favorites vcs
    let backgroundDataSource = NTPDataSource()
    let feedDataSource = FeedDataSource()
    
    var loadQueue = Deferred<Void>()

    lazy var mailtoLinkHandler: MailtoLinkHandler = MailtoLinkHandler()

    // Custom Search Engine
    var openSearchEngine: OpenSearchReference?

    lazy var customSearchEngineButton = OpenSearchEngineButton(hidesWhenDisabled: false).then {
        $0.addTarget(self, action: #selector(addCustomSearchEngineForFocusedElement), for: .touchUpInside)
        $0.accessibilityIdentifier = "BrowserViewController.customSearchEngineButton"
    }
    var customSearchBarButton: UIBarButtonItem?

    // popover rotation handling
    var displayedPopoverController: UIViewController?
    var updateDisplayedPopoverProperties: (() -> Void)?

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

    // These views wrap the top and bottom toolbars to provide background effects on them
    var header: UIStackView!
    var footer: UIView!
    fileprivate var topTouchArea: UIButton!
    
    // These constraints allow to show/hide tabs bar
    var webViewContainerTopOffset: Constraint?
    
    // Backdrop used for displaying greyed background for private tabs
    var webViewContainerBackdrop: UIView!

    var scrollController = TabScrollingController()

    var keyboardState: KeyboardState?

    var pendingToast: Toast? // A toast that might be waiting for BVC to appear before displaying
    var downloadToast: DownloadToast? // A toast that is showing the combined download progress

    // Tracking navigation items to record history types.
    // TODO: weak references?
    var ignoredNavigation = Set<WKNavigation>()
    var typedNavigation = [WKNavigation: VisitType]()
    var navigationToolbar: ToolbarProtocol {
        return toolbar ?? topToolbar
    }
    
    /// Keep track of the URL request that was upgraded so that we can add it to the HTTPS page stats
    var pendingHTTPUpgrades = [String: URLRequest]()

    // Keep track of allowed `URLRequest`s from `webView(_:decidePolicyFor:decisionHandler:)` so
    // that we can obtain the originating `URLRequest` when a `URLResponse` is received. This will
    // allow us to re-trigger the `URLRequest` if the user requests a file to be downloaded.
    var pendingRequests = [String: URLRequest]()

    // This is set when the user taps "Download Link" from the context menu. We then force a
    // download of the next request through the `WKNavigationDelegate` that matches this web view.
    weak var pendingDownloadWebView: WKWebView?

    let downloadQueue = DownloadQueue()
    
    fileprivate var contentBlockListDeferred: Deferred<()>?
    
    // Web filters
    
    let safeBrowsing: SafeBrowsing?
    
    let rewards: BraveRewards
    let legacyWallet: BraveLedger?
    let rewardsObserver: LedgerObserver
    private var promotionFetchTimer: Timer?
    private var notificationsHandler: AdsNotificationHandler?
    private(set) var publisher: PublisherInfo?
    
    let vpnProductInfo = VPNProductInfo()
    
    /// Boolean which is tracking If a product notification is presented
    /// in order to not to try to present another one over existing popover
    var benchmarkNotificationPresented = false
    
    /// Number of Ads/Trackers used a limit to show benchmark notification
    let benchmarkNumberOfTrackers = 10
    
    /// Used to determine when to present benchmark pop-overs
    /// Current session ad count is compared with live ad count
    /// So user will not be introduced with a pop-over directly
    let benchmarkCurrentSessionAdCount = BraveGlobalShieldStats.shared.adblock + BraveGlobalShieldStats.shared.trackingProtection

    init(profile: Profile, tabManager: TabManager, crashedLastSession: Bool,
         safeBrowsingManager: SafeBrowsing? = SafeBrowsing()) {
        self.profile = profile
        self.tabManager = tabManager
        self.readerModeCache = ReaderMode.cache(for: tabManager.selectedTab)
        self.crashedLastSession = crashedLastSession
        self.safeBrowsing = safeBrowsingManager

        let configuration: BraveRewardsConfiguration
        if AppConstants.buildChannel.isPublic {
            configuration = .production
        } else {
            if let override = Preferences.Rewards.EnvironmentOverride(rawValue: Preferences.Rewards.environmentOverride.value), override != .none {
                switch override {
                case .dev:
                    configuration = .default
                case .staging:
                    configuration = .staging
                case .prod:
                    configuration = .production
                default:
                    configuration = .staging
                }
            } else {
                configuration = AppConstants.buildChannel == .debug ? .staging : .production
            }
        }

        configuration.buildChannel = BraveAdsBuildChannel().then {
          $0.name = AppConstants.buildChannel.rawValue
          $0.isRelease = AppConstants.buildChannel == .release
        }
        Self.migrateAdsConfirmations(for: configuration)
        legacyWallet = Self.legacyWallet(for: configuration)
        if let wallet = legacyWallet {
            // Legacy ledger is disabled by default
            wallet.isAutoContributeEnabled = false
            // Ensure we remove any pending contributions or recurring tips from the legacy wallet
            wallet.removeAllPendingContributions { _ in }
            wallet.listRecurringTips { publishers in
                publishers.forEach {
                    wallet.removeRecurringTip(publisherId: $0.id)
                }
            }
        }
        rewards = BraveRewards(configuration: configuration)
        if !BraveRewards.isAvailable {
            // Disable rewards services in case previous user already enabled
            // rewards in previous build
            rewards.isAdsEnabled = false
        } else {
            if rewards.isEnabled && !Preferences.Rewards.rewardsToggledOnce.value {
                Preferences.Rewards.rewardsToggledOnce.value = true
            }
            // Update defaults
            rewards.ledger.minimumVisitDuration = 8
            rewards.ledger.minimumNumberOfVisits = 1
            rewards.ledger.allowUnverifiedPublishers = false
            rewards.ledger.allowVideoContributions = true
            rewards.ledger.contributionAmount = Double.greatestFiniteMagnitude
        }
        rewardsObserver = LedgerObserver(ledger: rewards.ledger)
        deviceCheckClient = DeviceCheckClient(environment: configuration.environment)
        
        super.init(nibName: nil, bundle: nil)
        didInit()
        
        rewards.delegate = self
    }
    
    static func legacyWallet(for config: BraveRewardsConfiguration) -> BraveLedger? {
        let fm = FileManager.default
        let stateStorage = URL(fileURLWithPath: config.stateStoragePath)
        let legacyLedger = stateStorage.appendingPathComponent("legacy_ledger")

        // Check if we've already migrated the users wallet to the `legacy_rewards` folder
        if fm.fileExists(atPath: legacyLedger.path) {
            BraveLedger.environment = config.environment
            return BraveLedger(stateStoragePath: legacyLedger.path)
        }
        
        // We've already performed an attempt at migration, if there wasn't a legacy folder, then
        // we have no legacy wallet.
        if Preferences.Rewards.migratedLegacyWallet.value {
            return nil
        }
        
        // Ledger exists in the state storage under `ledger` folder, if that folder doesn't exist
        // then the user hasn't actually launched the app before and doesn't need to migrate
        let ledgerFolder = stateStorage.appendingPathComponent("ledger")
        if !fm.fileExists(atPath: ledgerFolder.path) {
            // No wallet, therefore no legacy folder needed
            Preferences.Rewards.migratedLegacyWallet.value = true
            return nil
        }
        
        do {
            // Copy the current `ledger` directory into the new legacy state storage path
            try fm.copyItem(at: ledgerFolder, to: legacyLedger)
            // Remove the old Rewards DB so that it starts fresh
            try fm.removeItem(atPath: ledgerFolder.appendingPathComponent("Rewards.db").path)
            // And remove the sqlite journal file if it exists
            let journalPath = ledgerFolder.appendingPathComponent("Rewards.db-journal").path
            if fm.fileExists(atPath: journalPath) {
                try fm.removeItem(atPath: journalPath)
            }
            
            Preferences.Rewards.migratedLegacyWallet.value = true
            BraveLedger.environment = config.environment
            return BraveLedger(stateStoragePath: legacyLedger.path)
        } catch {
            log.error("Failed to migrate legacy wallet into a new folder: \(error)")
            return nil
        }
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

            if let tab = self.tabManager.selectedTab {
                WindowRenderHelperScript.executeScript(for: tab)
            }
        })
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        
        for tab in tabManager.tabsForCurrentMode where tab.id != tabManager.selectedTab?.id {
            tab.newTabPageViewController = nil
        }
    }
    
    private var rewardsEnabledObserveration: NSKeyValueObservation?

    fileprivate func didInit() {
        screenshotHelper = ScreenshotHelper(controller: self)
        tabManager.addDelegate(self)
        tabManager.addNavigationDelegate(self)
        downloadQueue.delegate = self
        
        // Observe some user preferences
        Preferences.Privacy.privateBrowsingOnly.observe(from: self)
        Preferences.General.tabBarVisibility.observe(from: self)
        Preferences.General.themeNormalMode.observe(from: self)
        Preferences.General.alwaysRequestDesktopSite.observe(from: self)
        Preferences.Shields.allShields.forEach { $0.observe(from: self) }
        Preferences.Privacy.blockAllCookies.observe(from: self)
        Preferences.Rewards.hideRewardsIcon.observe(from: self)
        Preferences.Rewards.rewardsToggledOnce.observe(from: self)
        rewardsEnabledObserveration = rewards.observe(\.isEnabled, options: [.new]) { [weak self] _, _ in
            guard let self = self else { return }
            self.updateRewardsButtonState()
            self.setupAdsNotificationHandler()
        }
        Preferences.NewTabPage.selectedCustomTheme.observe(from: self)
        // Lists need to be compiled before attempting tab restoration
        contentBlockListDeferred = ContentBlockerHelper.compileBundledLists()
        
        setupRewardsObservers()
        promotionFetchTimer = Timer.scheduledTimer(
            withTimeInterval: 1.hours,
            repeats: true,
            block: { [weak self] _ in
                guard let self = self else { return }
                if self.rewards.isEnabled {
                    self.rewards.ledger.fetchPromotions(nil)
                }
            }
        )
        
        Preferences.NewTabPage.attemptToShowClaimRewardsNotification.value = true
        
        backgroundDataSource.initializeFavorites = { sites in
            DispatchQueue.main.async {
                defer { Preferences.NewTabPage.preloadedFavoritiesInitialized.value = true }
                
                if Preferences.NewTabPage.preloadedFavoritiesInitialized.value
                    || Favorite.hasFavorites { return }
                
                guard let sites = sites, !sites.isEmpty else {
                    FavoritesHelper.addDefaultFavorites()
                    return
                }
                
                let customFavorites = sites.compactMap { $0.asFavoriteSite }
                Favorite.add(from: customFavorites)
            }
        }
        
        setupAdsNotificationHandler()
        backgroundDataSource.replaceFavoritesIfNeeded = { sites in
            if Preferences.NewTabPage.initialFavoritesHaveBeenReplaced.value { return }
            
            guard let sites = sites, !sites.isEmpty else { return }
            
            DispatchQueue.main.async {
                let defaultFavorites = PreloadedFavorites.getList()
                let currentFavorites = Favorite.allFavorites
                
                if defaultFavorites.count != currentFavorites.count {
                    return
                }
                
                let exactSameFavorites = Favorite.allFavorites
                    .filter {
                        guard let urlString = $0.url,
                            let url = URL(string: urlString),
                            let title = $0.displayTitle else {
                                return false
                        }
                        
                        return defaultFavorites.contains(where: { defaultFavorite in
                            defaultFavorite.url == url && defaultFavorite.title == title
                        })
                }
                
                if currentFavorites.count == exactSameFavorites.count {
                    let customFavorites = sites.compactMap { $0.asFavoriteSite }
                    Preferences.NewTabPage.initialFavoritesHaveBeenReplaced.value = true
                    Favorite.forceOverwriteFavorites(with: customFavorites)
                }
            }
        }
    }
    
    let deviceCheckClient: DeviceCheckClient?
    
    private func setupAdsNotificationHandler() {
        notificationsHandler = AdsNotificationHandler(ads: rewards.ads, presentingController: self)
        notificationsHandler?.canShowNotifications = { [weak self] in
            guard let self = self else { return false }
            return !PrivateBrowsingManager.shared.isPrivateBrowsing &&
                !self.topToolbar.inOverlayMode
        }
        notificationsHandler?.actionOccured = { [weak self] notification, action in
            guard let self = self else { return }
            if action == .opened {
                var url = URL(string: notification.targetURL)
                if url == nil, let percentEncodedURLString =
                    notification.targetURL.addingPercentEncoding(withAllowedCharacters: .urlQueryAllowed) {
                    // Try to percent-encode the string and try that
                    url = URL(string: percentEncodedURLString)
                }
                guard let targetURL = url else {
                    assertionFailure("Invalid target URL for creative instance id: \(notification.creativeInstanceID)")
                    return
                }
                let request = URLRequest(url: targetURL)
                self.tabManager.addTabAndSelect(request, isPrivate: PrivateBrowsingManager.shared.isPrivateBrowsing)
            }
        }
    }
    
    private func setupRewardsObservers() {
        rewards.ledger.add(rewardsObserver)
        rewardsObserver.walletInitalized = { [weak self] result in
            guard let self = self, let client = self.deviceCheckClient else { return }
            if result == .walletCreated {
                self.rewards.ledger.setupDeviceCheckEnrollment(client) { }                
                self.updateRewardsButtonState()
            }
        }
        rewardsObserver.promotionsAdded = { [weak self] promotions in
            self?.claimPendingPromotions()
        }
        rewardsObserver.fetchedPanelPublisher = { [weak self] publisher, tabId in
            guard let self = self, self.isViewLoaded, let tab = self.tabManager.selectedTab, tab.rewardsId == tabId else { return }
            self.publisher = publisher
        }
    }
    
    override var preferredStatusBarStyle: UIStatusBarStyle {
        let isDark = Theme.of(tabManager.selectedTab).isDark
        if isDark {
            return .lightContent
        }
        
        // Light content, so using other status bar options
        return .darkContent
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

        topToolbar.setShowToolbar(!showToolbar)
        toolbar?.removeFromSuperview()
        toolbar?.tabToolbarDelegate = nil
        toolbar = nil

        if showToolbar {
            toolbar = BottomToolbarView()
            toolbar?.setSearchButtonState(url: tabManager.selectedTab?.url)
            footer.addSubview(toolbar!)
            toolbar?.tabToolbarDelegate = self

            let theme = Theme.of(tabManager.selectedTab)
            toolbar?.applyTheme(theme)

            updateTabCountUsingTabManager(self.tabManager)
        }

        view.setNeedsUpdateConstraints()
        
        if let tab = tabManager.selectedTab,
               let webView = tab.webView {
            updateURLBar()
            navigationToolbar.updateBackStatus(webView.canGoBack)
            navigationToolbar.updateForwardStatus(webView.canGoForward)
            topToolbar.locationView.loading = tab.loading
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
                self.statusBarOverlay.backgroundColor = self.topToolbar.backgroundColor
                self.setNeedsStatusBarAppearanceUpdate()
            }
        }, completion: { _ in
            if let tab = self.tabManager.selectedTab {
                WindowRenderHelperScript.executeScript(for: tab)
            }
        })
    }
    
    override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
        super.traitCollectionDidChange(previousTraitCollection)
        
        if UITraitCollection.current.userInterfaceStyle != previousTraitCollection?.userInterfaceStyle {
            // Reload UI
            applyTheme(Theme.of(tabManager.selectedTab))
        }
    }

    func dismissVisibleMenus() {
        displayedPopoverController?.dismiss(animated: true)
    }

    @objc func appDidEnterBackgroundNotification() {
        displayedPopoverController?.dismiss(animated: false) {
            self.updateDisplayedPopoverProperties = nil
            self.displayedPopoverController = nil
        }
    }

    @objc func tappedTopArea() {
        scrollController.showToolbars(animated: true)
    }

    @objc func appWillResignActiveNotification() {
        // Dismiss any popovers that might be visible
        displayedPopoverController?.dismiss(animated: false) {
            self.updateDisplayedPopoverProperties = nil
            self.displayedPopoverController = nil
        }

        // If we are displaying a private tab, hide any elements in the tab that we wouldn't want shown
        // when the app is in the home switcher
        if let tab = tabManager.selectedTab, tab.isPrivate {
            webViewContainerBackdrop.alpha = 1
            webViewContainer.alpha = 0
            topToolbar.locationContainer.alpha = 0
            presentedViewController?.popoverPresentationController?.containerView?.alpha = 0
            presentedViewController?.view.alpha = 0
        }
    }
    
    @objc func vpnConfigChanged() {
        // Load latest changes to the vpn.
        NEVPNManager.shared().loadFromPreferences { _ in }
    }

    @objc func appDidBecomeActiveNotification() {
        // Re-show any components that might have been hidden because they were being displayed
        // as part of a private mode tab
        UIView.animate(withDuration: 0.2, delay: 0, options: UIView.AnimationOptions(), animations: {
            self.webViewContainer.alpha = 1
            self.topToolbar.locationContainer.alpha = 1
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
        NotificationCenter.default.do {
            $0.addObserver(self, selector: #selector(appWillResignActiveNotification),
                           name: UIApplication.willResignActiveNotification, object: nil)
            $0.addObserver(self, selector: #selector(appDidBecomeActiveNotification),
                           name: UIApplication.didBecomeActiveNotification, object: nil)
            $0.addObserver(self, selector: #selector(appDidEnterBackgroundNotification),
                           name: UIApplication.didEnterBackgroundNotification, object: nil)
            $0.addObserver(self, selector: #selector(resetNTPNotification),
                           name: .adsOrRewardsToggledInSettings, object: nil)
            $0.addObserver(self, selector: #selector(vpnConfigChanged),
                           name: .NEVPNConfigurationChange, object: nil)
            $0.addObserver(self, selector: #selector(updateShieldNotifications),
                           name: NSNotification.Name(rawValue: BraveGlobalShieldStats.didUpdateNotification), object: nil)
        }
        
        KeyboardHelper.defaultHelper.addDelegate(self)
        UNUserNotificationCenter.current().delegate = self
        
        view.addLayoutGuide(pageOverlayLayoutGuide)

        webViewContainerBackdrop = UIView()
        webViewContainerBackdrop.backgroundColor = UIColor.Photon.grey50
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
        topToolbar = TopToolbarView()
        topToolbar.translatesAutoresizingMaskIntoConstraints = false
        topToolbar.delegate = self
        topToolbar.tabToolbarDelegate = self
        header = UIStackView().then {
            $0.axis = .vertical
            $0.clipsToBounds = true
            $0.translatesAutoresizingMaskIntoConstraints = false
        }
        header.addArrangedSubview(topToolbar)
        
        tabsBar = TabsBarViewController(tabManager: tabManager)
        tabsBar.delegate = self
        header.addArrangedSubview(tabsBar.view)
        
        view.addSubview(header)
        
        addChild(tabsBar)
        tabsBar.didMove(toParent: self)

        // UIAccessibilityCustomAction subclass holding an AccessibleAction instance does not work, thus unable to generate AccessibleActions and UIAccessibilityCustomActions "on-demand" and need to make them "persistent" e.g. by being stored in BVC
        pasteGoAction = AccessibleAction(name: Strings.pasteAndGoTitle, handler: { () -> Bool in
            if let pasteboardContents = UIPasteboard.general.string {
                self.topToolbar(self.topToolbar, didSubmitText: pasteboardContents)
                return true
            }
            return false
        })
        pasteAction = AccessibleAction(name: Strings.pasteTitle, handler: { () -> Bool in
            if let pasteboardContents = UIPasteboard.general.string {
                // Enter overlay mode and make the search controller appear.
                self.topToolbar.enterOverlayMode(pasteboardContents, pasted: true, search: true)

                return true
            }
            return false
        })
        copyAddressAction = AccessibleAction(name: Strings.copyAddressTitle, handler: { () -> Bool in
            if let url = self.topToolbar.currentURL {
                UIPasteboard.general.url = url as URL
            }
            return true
        })

        view.addSubview(alertStackView)
        footer = UIView()
        footer.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(footer)
        alertStackView.axis = .vertical
        alertStackView.alignment = .center

        clipboardBarDisplayHandler = ClipboardBarDisplayHandler(tabManager: tabManager)
        clipboardBarDisplayHandler?.delegate = self
        
        scrollController.topToolbar = topToolbar
        scrollController.header = header
        scrollController.tabsBar = tabsBar
        scrollController.footer = footer
        scrollController.snackBars = alertStackView

        self.updateToolbarStateForTraitCollection(self.traitCollection)

        setupConstraints()
        
        updateRewardsButtonState()
        
        applyTheme(Theme.of(tabManager.selectedTab))

        // Setup UIDropInteraction to handle dragging and dropping
        // links into the view from other apps.
        let dropInteraction = UIDropInteraction(delegate: self)
        view.addInteraction(dropInteraction)
        
        if AppConstants.buildChannel.isPublic && AppReview.shouldRequestReview() {
            // Request Review when the main-queue is free or on the next cycle.
            DispatchQueue.main.async {
                SKStoreReviewController.requestReview()
            }
        }
        
        LegacyBookmarksHelper.restore_1_12_Bookmarks() {
            log.info("Bookmarks from old database were successfully restored")
        }
        
        vpnProductInfo.load()
        BraveVPN.initialize()
        
        showWalletTransferExpiryPanelIfNeeded()
        
        // We stop ever attempting migration after 3 times.
        if Preferences.Chromium.syncV2BookmarksMigrationCount.value < 3 {
            self.migrateToChromiumBookmarks { success in
                if !success {
                    DispatchQueue.main.async {
                        let alert = UIAlertController(title: Strings.Sync.v2MigrationErrorTitle,
                                                      message: Strings.Sync.v2MigrationErrorMessage,
                                                      preferredStyle: .alert)
                        alert.addAction(UIAlertAction(title: Strings.OKString, style: .default, handler: nil))
                        self.present(alert, animated: true)
                    }
                }
            }
        } else {
            // After 3 tries, we mark Migration as successful.
            // There is nothing more we can do for the user other than to let them export/import bookmarks.
            Preferences.Chromium.syncV2BookmarksMigrationCompleted.value = true
        }
        
        if #available(iOS 14, *), !Preferences.DefaultBrowserIntro.defaultBrowserNotificationScheduled.value {
            scheduleDefaultBrowserNotification()
        }
    }
    
    private func migrateToChromiumBookmarks(_ completion: @escaping (_ success: Bool) -> Void) {
        let showInterstitialPage = { (url: URL?) -> Bool in
            guard let url = url else {
                log.error("Cannot open bookmarks page in new tab")
                return false
            }
            
            return BookmarksInterstitialPageHandler.showBookmarksPage(tabManager: self.tabManager, url: url)
        }
        
        Migration.braveCoreBookmarksMigrator?.migrate({ success in
            Preferences.Chromium.syncV2BookmarksMigrationCount.value += 1
            
            if !success {
                guard let url = BraveCoreMigrator.datedBookmarksURL else {
                    completion(showInterstitialPage(BraveCoreMigrator.bookmarksURL))
                    return
                }
                
                Migration.braveCoreBookmarksMigrator?.exportBookmarks(to: url) { success in
                    if success {
                        completion(showInterstitialPage(url))
                    } else {
                        completion(showInterstitialPage(BraveCoreMigrator.bookmarksURL))
                    }
                }
            } else {
                completion(true)
            }
        })
    }
    
    fileprivate let defaultBrowserNotificationId = "defaultBrowserNotification"
    
    private func scheduleDefaultBrowserNotification() {
        let center = UNUserNotificationCenter.current()
        
        center.requestAuthorization(options: [.provisional, .alert, .sound, .badge]) { granted, error in
            if let error = error {
                log.error("Failed to request notifications permissions: \(error)")
                return
            }
            
            if !granted {
                log.info("Not authorized to schedule a notification")
                return
            }
            
            center.getPendingNotificationRequests { [weak self] requests in
                guard let self = self else { return }
                if requests.contains(where: { $0.identifier == self.defaultBrowserNotificationId }) {
                    // Already has one scheduled no need to schedule again.
                    return
                }
                
                let content = UNMutableNotificationContent().then {
                    $0.title = Strings.DefaultBrowserCallout.notificationTitle
                    $0.body = Strings.DefaultBrowserCallout.notificationBody
                }
                
                let timeToShow = AppConstants.buildChannel.isPublic ? 2.hours : 2.minutes
                let timeTrigger = UNTimeIntervalNotificationTrigger(timeInterval: timeToShow, repeats: false)
                
                let request = UNNotificationRequest(identifier: self.defaultBrowserNotificationId,
                                                    content: content,
                                                    trigger: timeTrigger)
                
                center.add(request) { error in
                    if let error = error {
                        log.error("Failed to add notification: \(error)")
                        return
                    }
                    
                    Preferences.DefaultBrowserIntro.defaultBrowserNotificationScheduled.value = true
                }
            }
        }
    }
    
    fileprivate func setupTabs() {
        contentBlockListDeferred?.uponQueue(.main) { _ in
            let isPrivate = Preferences.Privacy.privateBrowsingOnly.value
            let noTabsAdded = self.tabManager.tabsForCurrentMode.isEmpty
            
            var tabToSelect: Tab?
            
            if noTabsAdded {
                // Two scenarios if there are no tabs in tabmanager:
                // 1. We have not restored tabs yet, attempt to restore or make a new tab if there is nothing.
                // 2. We are in private browsing mode and need to add a new private tab.
                tabToSelect = isPrivate ? self.tabManager.addTab(isPrivate: true) : self.tabManager.restoreAllTabs()
            } else {
                tabToSelect = self.tabManager.tabsForCurrentMode.last
            }
            self.tabManager.selectTab(tabToSelect)
            self.loadQueue.fillIfUnfilled(())
        }
    }

    fileprivate func setupConstraints() {
        header.snp.makeConstraints { make in
            scrollController.headerTopConstraint = make.top.equalTo(view.safeArea.top).constraint
            make.left.right.equalTo(self.view)
        }
        
        topToolbar.snp.makeConstraints { make in
            make.height.equalTo(UIConstants.topToolbarHeight)
        }
        
        tabsBar.view.snp.makeConstraints { make in
            make.height.equalTo(UX.TabsBar.height)
        }

        webViewContainerBackdrop.snp.makeConstraints { make in
            make.edges.equalTo(webViewContainer)
        }
        
        topTouchArea.snp.makeConstraints { make in
            make.top.left.right.equalTo(self.view)
            make.height.equalTo(BrowserViewControllerUX.showHeaderTapAreaHeight)
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
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)

        checkCrashRestoration()
        
        updateTabCountUsingTabManager(tabManager)
        clipboardBarDisplayHandler?.checkIfShouldDisplayBar()
        
        if let tabId = tabManager.selectedTab?.rewardsId, rewards.ledger.selectedTabId == 0 {
            rewards.ledger.selectedTabId = tabId
        }
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
            self.tabManager.addTabAndSelect(isPrivate: PrivateBrowsingManager.shared.isPrivateBrowsing)
            return
        }
        let alert = UIAlertController.restoreTabsAlert(
            okayCallback: { _ in
                self.setupTabs()
            },
            noCallback: { _ in
                TabMO.deleteAll()
                self.tabManager.addTabAndSelect(isPrivate: PrivateBrowsingManager.shared.isPrivateBrowsing)
            }
        )
        self.present(alert, animated: true, completion: nil)
    }

    fileprivate func canRestoreTabs() -> Bool {
        // Make sure there's at least one real tab open
        return !TabMO.getAll().compactMap({ $0.url }).isEmpty
    }

    override func viewDidAppear(_ animated: Bool) {
        presentOnboardingIntro()
        DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
            self.presentVPNCallout()
        }
        
        if #available(*, iOS 14) {
            presentDefaultBrowserIntroScreen()
        }
        
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
            show(toast: toast, afterWaiting: ButtonToastUX.toastDelay)
        }
        showQueuedAlertIfAvailable()
        
        if PrivateBrowsingManager.shared.isPrivateBrowsing {
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.2) {
                self.presentDuckDuckGoCallout()
            }
        }
    }
    
    func presentOnboardingIntro() {
        if Preferences.DebugFlag.skipOnboardingIntro == true { return }
        
        // 1. Existing user.
        // 2. User already completed onboarding.
        if Preferences.General.basicOnboardingCompleted.value == OnboardingState.completed.rawValue {
            // The user doesn't have ads in their region and they've completed rewards.
            if !BraveAds.isCurrentLocaleSupported()
                &&
                Preferences.General.basicOnboardingProgress.value == OnboardingProgress.rewards.rawValue {
                return
            }
        }
        
        // The user either skipped or didn't complete onboarding.
        let isRewardsEnabled = rewards.isEnabled
        let currentProgress = OnboardingProgress(rawValue: Preferences.General.basicOnboardingProgress.value) ?? .none
        
        // 1. Existing user.
        // 2. The user skipped onboarding before.
        // 3. 60 days have passed since they last saw onboarding.
        if Preferences.General.basicOnboardingCompleted.value == OnboardingState.skipped.rawValue {

            guard let daysUntilNextPrompt = Preferences.General.basicOnboardingNextOnboardingPrompt.value else {
                return
            }
            
            // 60 days has passed since the user last saw the onboarding.. it's time to show the onboarding again..
            if daysUntilNextPrompt <= Date() && !isRewardsEnabled {
                guard let onboarding = OnboardingNavigationController(
                    profile: profile,
                    onboardingType: .existingUserRewardsOff(currentProgress),
                    rewards: rewards,
                    theme: Theme.of(tabManager.selectedTab)
                    ) else { return }
                
                onboarding.onboardingDelegate = self
                present(onboarding, animated: true)
                
                Preferences.General.basicOnboardingNextOnboardingPrompt.value = Date(timeIntervalSinceNow: BrowserViewController.onboardingDaysInterval)
            }
            
            return
        }
        
        // 1. Rewards are on/off (existing user)
        // 2. User hasn't seen the rewards part of the onboarding yet.
        if (Preferences.General.basicOnboardingCompleted.value == OnboardingState.completed.rawValue)
            &&
            (Preferences.General.basicOnboardingProgress.value == OnboardingProgress.searchEngine.rawValue) {
            
            guard !isRewardsEnabled, let onboarding = OnboardingNavigationController(
                profile: profile,
                onboardingType: .existingUserRewardsOff(currentProgress),
                rewards: rewards,
                theme: Theme.of(tabManager.selectedTab)
                ) else { return }
            
            onboarding.onboardingDelegate = self
            present(onboarding, animated: true)
            return
        }
        
        // 1. Rewards are on/off (existing user)
        // 2. User hasn't seen the rewards part of the onboarding yet because their version of the app is insanely OLD and somehow the progress value doesn't exist.
        if (Preferences.General.basicOnboardingCompleted.value == OnboardingState.completed.rawValue)
            &&
            (Preferences.General.basicOnboardingProgress.value == OnboardingProgress.none.rawValue) {
            
            guard !isRewardsEnabled, let onboarding = OnboardingNavigationController(
                profile: profile,
                onboardingType: .existingUserRewardsOff(currentProgress),
                rewards: rewards,
                theme: Theme.of(tabManager.selectedTab)
                ) else { return }
            
            onboarding.onboardingDelegate = self
            present(onboarding, animated: true)
            return
        }
        
        // 1. User is brand new
        // 2. User hasn't completed onboarding
        // 3. We don't care how much progress they made. Onboarding is only complete when ALL of it is complete.
        if Preferences.General.basicOnboardingCompleted.value != OnboardingState.completed.rawValue {
            // The user has never completed the onboarding..
            
            guard let onboarding = OnboardingNavigationController(
                profile: profile,
                onboardingType: .newUser(currentProgress),
                rewards: rewards,
                theme: Theme.of(tabManager.selectedTab)
                ) else { return }
            
            onboarding.onboardingDelegate = self
            present(onboarding, animated: true)
            return
        }
    }
    
    private func presentVPNCallout() {
        if Preferences.DebugFlag.skipNTPCallouts == true { return }
        
        let onboardingNotCompleted =
            Preferences.General.basicOnboardingCompleted.value == OnboardingState.completed.rawValue
        let notEnoughAppLaunches = Preferences.VPN.appLaunchCountForVPNPopup.value < BraveVPN.appLaunchesToShowVPNPopup
        let showedPopup = Preferences.VPN.popupShowed

        if onboardingNotCompleted
            || notEnoughAppLaunches
            || showedPopup.value
            || !VPNProductInfo.isComplete {
            return
        }
        
        let popup = EnableVPNPopupViewController().then {
            $0.isModalInPresentation = true
            $0.modalPresentationStyle = .overFullScreen
        }
        
        popup.enableVPNTapped = { [weak self] in
            self?.presentCorrespondingVPNViewController()
        }
        
        present(popup, animated: false)
        
        showedPopup.value = true
    }
    
    /// Shows a vpn screen based on vpn state.
    func presentCorrespondingVPNViewController() {
        guard let vc = BraveVPN.vpnState.enableVPNDestinationVC else { return }
        let nav = SettingsNavigationController(rootViewController: vc)
        nav.navigationBar.topItem?.leftBarButtonItem =
            .init(barButtonSystemItem: .cancel, target: nav, action: #selector(nav.done))
        let idiom = UIDevice.current.userInterfaceIdiom
        
        UIDevice.current.forcePortraitIfIphone(for: UIApplication.shared)
        
        nav.modalPresentationStyle = idiom == .phone ? .pageSheet : .formSheet
        present(nav, animated: true)
    }
    
    /// Whether or not to show the Default Browser intro callout. It's set at app launch in AppDelegate
    var shouldShowIntroScreen = false

    private func presentDefaultBrowserIntroScreen() {
        if Preferences.DebugFlag.skipNTPCallouts == true { return }
        
        if !shouldShowIntroScreen {
            return
        }
        
        shouldShowIntroScreen = false
        
        let vc = DefaultBrowserIntroCalloutViewController(theme: Theme.of(tabManager.selectedTab)) 
        let idiom = UIDevice.current.userInterfaceIdiom
        vc.modalPresentationStyle = idiom == .phone ? .pageSheet : .formSheet
        present(vc, animated: true)
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
        
        rewards.ledger.selectedTabId = 0
    }

    override func viewDidDisappear(_ animated: Bool) {
        super.viewDidDisappear(animated)
    }

    func resetBrowserChrome() {
        // animate and reset transform for tab chrome
        topToolbar.updateAlphaForSubviews(1)
        footer.alpha = 1

        [header, footer, readerModeBar].forEach { view in
                view?.transform = .identity
        }
        statusBarOverlay.isHidden = false
    }
    
    /// A layout guide defining where the favorites and NTP overlay are placed
    private let pageOverlayLayoutGuide = UILayoutGuide()

    override func updateViewConstraints() {
        webViewContainer.snp.remakeConstraints { make in
            make.left.right.equalTo(self.view)
            
            webViewContainerTopOffset = make.top.equalTo(readerModeBar?.snp.bottom ?? self.header.snp.bottom).constraint

            let findInPageHeight = (findInPageBar == nil) ? 0 : UIConstants.toolbarHeight
            make.bottom.equalTo(self.footer.snp.top).offset(-findInPageHeight)
        }

        footer.snp.remakeConstraints { make in
            scrollController.footerBottomConstraint = make.bottom.equalTo(self.view.snp.bottom).constraint
            make.leading.trailing.equalTo(self.view)
            let height = self.toolbar == nil ? 0 : UIConstants.bottomToolbarHeight
            make.height.equalTo(height)
        }
        
        // Remake constraints even if we're already showing the home controller.
        // The home controller may change sizes if we tap the URL bar while on about:home.
        pageOverlayLayoutGuide.snp.remakeConstraints { make in
            webViewContainerTopOffset = make.top.equalTo(readerModeBar?.snp.bottom ?? self.header.snp.bottom).constraint
            
            make.left.right.equalTo(self.view)
            make.bottom.equalTo(self.footer.snp.top)
        }

        alertStackView.snp.remakeConstraints { make in
            make.centerX.equalTo(self.view)
            make.width.equalTo(self.view.safeArea.width)
            if let keyboardHeight = keyboardState?.intersectionHeightForView(self.view), keyboardHeight > 0 {
                make.bottom.equalTo(self.view).offset(-keyboardHeight)
            } else if let toolbar = self.toolbar {
                make.bottom.lessThanOrEqualTo(toolbar.snp.top)
                make.bottom.lessThanOrEqualTo(self.view.safeArea.bottom)
            } else {
                make.bottom.equalTo(self.view.safeArea.bottom)
            }
        }
        
        // Setup the bottom toolbar
        toolbar?.snp.remakeConstraints { make in
            make.edges.equalTo(self.footer)
        }
        
        super.updateViewConstraints()
    }
    
    private(set) var favoritesController: FavoritesViewController?
    
    private func displayFavoritesController() {
        if favoritesController == nil {
            let favoritesController = FavoritesViewController { [weak self] bookmark, action in
                self?.handleFavoriteAction(favorite: bookmark, action: action)
            }
            favoritesController.applyTheme(Theme.of(tabManager.selectedTab))
            self.favoritesController = favoritesController
            
            addChild(favoritesController)
            view.addSubview(favoritesController.view)
            favoritesController.didMove(toParent: self)
            
            favoritesController.view.snp.makeConstraints {
                $0.top.leading.trailing.equalTo(pageOverlayLayoutGuide)
                $0.bottom.equalTo(view)
            }
        }
        guard let favoritesController = favoritesController else { return }
        favoritesController.view.alpha = 0.0
        let animator = UIViewPropertyAnimator(duration: 0.2, dampingRatio: 1.0) {
            favoritesController.view.alpha = 1
        }
        animator.addCompletion { _ in
            self.webViewContainer.accessibilityElementsHidden = true
            UIAccessibility.post(notification: .screenChanged, argument: nil)
        }
        animator.startAnimation()
    }
    
    private func hideFavoritesController() {
        guard let controller = favoritesController else { return }
        self.favoritesController = nil
        UIView.animate(withDuration: 0.1, delay: 0, options: [.beginFromCurrentState], animations: {
            controller.view.alpha = 0.0
        }, completion: { _ in
            controller.willMove(toParent: nil)
            controller.view.removeFromSuperview()
            controller.removeFromParent()
            self.webViewContainer.accessibilityElementsHidden = false
            UIAccessibility.post(notification: .screenChanged, argument: nil)
        })
    }
    
    fileprivate func showNewTabPageController() {
        guard let selectedTab = tabManager.selectedTab else { return }
        if selectedTab.newTabPageViewController == nil {
            let ntpController = NewTabPageViewController(tab: selectedTab,
                                                         profile: profile,
                                                         dataSource: backgroundDataSource,
                                                         feedDataSource: feedDataSource,
                                                         rewards: rewards)
            ntpController.delegate = self
            selectedTab.newTabPageViewController = ntpController
        }
        guard let ntpController = selectedTab.newTabPageViewController else {
            assertionFailure("homePanelController is still nil after assignment.")
            return
        }
        
        if let activeController = activeNewTabPageViewController, ntpController != activeController {
            // Remove active controller first
            activeController.willMove(toParent: nil)
            activeController.removeFromParent()
            activeController.view.removeFromSuperview()
        }
        
        if ntpController.parent == nil {
            activeNewTabPageViewController = ntpController
            
            addChild(ntpController)
            view.addSubview(ntpController.view)
            ntpController.didMove(toParent: self)
            
            ntpController.view.snp.makeConstraints {
                $0.edges.equalTo(pageOverlayLayoutGuide)
            }
            ntpController.view.layoutIfNeeded()
            
            // We have to run this animation, even if the view is already showing because there may be a hide animation running
            // and we want to be sure to override its results.
            UIView.animate(withDuration: 0.2, animations: {
                ntpController.view.alpha = 1
            }, completion: { finished in
                if finished {
                    self.webViewContainer.accessibilityElementsHidden = true
                    UIAccessibility.post(notification: .screenChanged, argument: nil)
                }
            })
        }
    }
    
    private(set) weak var activeNewTabPageViewController: NewTabPageViewController?
    
    fileprivate func hideActiveNewTabPageController(_ isReaderModeURL: Bool = false) {
        guard let controller = activeNewTabPageViewController else { return }
        
        UIView.animate(withDuration: 0.2, animations: {
            controller.view.alpha = 0.0
        }, completion: { finished in
            controller.willMove(toParent: nil)
            controller.view.removeFromSuperview()
            controller.removeFromParent()
            self.webViewContainer.accessibilityElementsHidden = false
            UIAccessibility.post(notification: .screenChanged, argument: nil)
            
            // Refresh the reading view toolbar since the article record may have changed
            if let readerMode = self.tabManager.selectedTab?.getContentScript(name: ReaderMode.name()) as? ReaderMode,
               readerMode.state == .active,
               isReaderModeURL {
                self.showReaderModeBar(animated: false)
            }
        })
    }

    fileprivate func updateInContentHomePanel(_ url: URL?) {
        if !topToolbar.inOverlayMode {
            guard let url = url else {
                hideActiveNewTabPageController()
                return
            }
            if url.isAboutHomeURL && !url.isErrorPageURL {
                showNewTabPageController()
            } else if !url.isLocalUtility || url.isReaderModeURL || url.isErrorPageURL {
                hideActiveNewTabPageController(url.isReaderModeURL)
            }
        }
    }

    fileprivate func showSearchController() {
        if searchController != nil { return }

        let tabType = TabType.of(tabManager.selectedTab)
        searchController = SearchViewController(forTabType: tabType)
        
        guard let searchController = searchController else { return }

        searchController.searchEngines = profile.searchEngines
        searchController.searchDelegate = self
        searchController.profile = self.profile

        searchLoader = SearchLoader()
        searchLoader?.addListener(searchController)
        searchLoader?.autocompleteSuggestionHandler = { [weak self] completion in
            self?.topToolbar.setAutocompleteSuggestion(completion)
        }

        addChild(searchController)
        view.addSubview(searchController.view)
        searchController.view.snp.makeConstraints { make in
            make.top.equalTo(self.topToolbar.snp.bottom)
            make.left.right.bottom.equalTo(self.view)
            return
        }
        
        searchController.didMove(toParent: self)
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
    
    private func updateApplicationShortcuts() {
        if let delegate = UIApplication.shared.delegate as? AppDelegate {
            delegate.updateShortcutItems(UIApplication.shared)
        }
    }

    fileprivate func hideSearchController() {
        if let searchController = searchController {
            searchController.willMove(toParent: nil)
            searchController.view.removeFromSuperview()
            searchController.removeFromParent()
            self.searchController = nil
            searchLoader = nil
        }
    }

    func finishEditingAndSubmit(_ url: URL, visitType: VisitType) {
        if url.isBookmarklet {
            topToolbar.leaveOverlayMode()
            
            guard let tab = tabManager.selectedTab else {
                return
            }
            
            // Another Fix for: https://github.com/brave/brave-ios/pull/2296
            // Disable any sort of privileged execution contexts
            // IE: The user must explicitly tap a bookmark they have saved.
            // Block all other contexts such as redirects, downloads, embed, linked, etc..
            if visitType == .bookmark {
                if let webView = tab.webView, let code = url.bookmarkletCodeComponent {
                    webView.evaluateSafeJavaScript(functionName: code, sandboxed: false, asFunction: false) { _, error in
                        if let error = error {
                            log.error(error)
                        }
                    }
                }
            }
        } else {
            topToolbar.currentURL = url
            topToolbar.leaveOverlayMode()

            guard let tab = tabManager.selectedTab else {
                return
            }

            tab.loadRequest(URLRequest(url: url))
        }
    }

    override func accessibilityPerformEscape() -> Bool {
        if topToolbar.inOverlayMode {
            topToolbar.didClickCancel()
            return true
        } else if let selectedTab = tabManager.selectedTab, selectedTab.canGoBack {
            selectedTab.goBack()
            return true
        }
        return false
    }

    override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey: Any]?, context: UnsafeMutableRawPointer?) {
        
        guard let webView = object as? WKWebView else {
            log.error("An object of type: \(String(describing: object)) is being observed instead of a WKWebView")
            return  // False alarm.. the source MUST be a web view.
        }
        
        // WebView is a zombie and somehow still has an observer attached to it
        guard let tab = tabManager[webView] else {
            log.error("WebView: \(webView) has been removed from TabManager but still has attached observers")
            return
        }
        
        // Must handle ALL keypaths
        guard let kp = keyPath, let path = KVOConstants(rawValue: kp) else {
            assertionFailure("Unhandled KVO key: \(keyPath ?? "nil")")
            return
        }
        
        switch path {
        case .estimatedProgress:
            guard tab === tabManager.selectedTab,
                let progress = change?[.newKey] as? Float else { break }
            if webView.url?.isLocalUtility == false {
                topToolbar.updateProgressBar(progress)
            } else {
                topToolbar.hideProgressBar()
            }
        case .loading:
            guard let loading = change?[.newKey] as? Bool else { break }
            
            if tab === tabManager.selectedTab {
                topToolbar.locationView.loading = tab.loading
            }
            
            if !loading {
                runScriptsOnWebView(webView)
            }
        case .URL:
            guard let tab = tabManager[webView] else { break }
            
            // Special case for "about:blank" popups, if the webView.url is nil, keep the tab url as "about:blank"
            if tab.url?.absoluteString == "about:blank" && webView.url == nil {
                break
            }
            
            // To prevent spoofing, only change the URL immediately if the new URL is on
            // the same origin as the current URL. Otherwise, do nothing and wait for
            // didCommitNavigation to confirm the page load.
            if tab.url?.origin == webView.url?.origin {
                tab.url = webView.url
                
                if tab === tabManager.selectedTab && !tab.restoring {
                    updateUIForReaderHomeStateForTab(tab)
                }
                // Catch history pushState navigation, but ONLY for same origin navigation,
                // for reasons above about URL spoofing risk.
                navigateInTab(tab: tab)
            }
        case .title:
            // Ensure that the tab title *actually* changed to prevent repeated calls
            // to navigateInTab(tab:).
            guard let title = (webView.title?.isEmpty == true ? webView.url?.absoluteString : webView.title) else { break }
            if !title.isEmpty && title != tab.lastTitle {
                navigateInTab(tab: tab)
                tabsBar.updateSelectedTabTitle()
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
            tab.userScriptManager?.isU2FEnabled = webView.hasOnlySecureContent
            tab.userScriptManager?.isPaymentRequestEnabled = webView.hasOnlySecureContent
            if tab.secureContentState == .secure && !webView.hasOnlySecureContent {
                tab.secureContentState = .insecure
            }
            
            if tabManager.selectedTab === tab {
                topToolbar.secureContentState = tab.secureContentState
            }
        case .serverTrust:
            guard let tab = tabManager[webView] else {
                break
            }

            tab.secureContentState = .unknown
            
            guard let serverTrust = tab.webView?.serverTrust else {
                if let url = tab.webView?.url ?? tab.url {
                    if url.isAboutHomeURL || url.isAboutURL || url.scheme == "about" {
                        tab.secureContentState = .localHost
                        if tabManager.selectedTab === tab {
                            topToolbar.secureContentState = .localHost
                        }
                        break
                    }
                    
                    if url.isErrorPageURL {
                        if ErrorPageHelper.certificateError(for: url) != 0 {
                            tab.secureContentState = .insecure
                            if tabManager.selectedTab === tab {
                                topToolbar.secureContentState = .insecure
                            }
                            break
                        }
                    }
                    
                    if url.isReaderModeURL || url.isLocal {
                        tab.secureContentState = .unknown
                        if tabManager.selectedTab === tab {
                            topToolbar.secureContentState = .unknown
                        }
                        break
                    }
                    
                    // All our checks failed, we show the page as insecure
                    tab.secureContentState = .insecure
                } else {
                    // When there is no URL, it's likely a new tab.
                    tab.secureContentState = .localHost
                }
                
                if tabManager.selectedTab === tab {
                    topToolbar.secureContentState = tab.secureContentState
                }
                break
            }
            
            let policies = [
                SecPolicyCreateBasicX509(),
                SecPolicyCreateSSL(true, tab.webView?.url?.host as CFString?)
            ]
            
            SecTrustSetPolicies(serverTrust, policies as CFTypeRef)
            SecTrustEvaluateAsync(serverTrust, DispatchQueue.global()) { _, secTrustResult in
                switch secTrustResult {
                case .proceed, .unspecified:
                    tab.secureContentState = .secure
                default:
                    tab.secureContentState = .insecure
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
            webView.evaluateSafeJavaScript(functionName: "__firefox__.NoImageMode.setEnabled", args: ["true"])
        }
    }

    func updateUIForReaderHomeStateForTab(_ tab: Tab) {
        updateURLBar()
        scrollController.showToolbars(animated: false)

        if let url = tab.url {
            if url.isReaderModeURL {
                showReaderModeBar(animated: false)
                NotificationCenter.default.addObserver(self, selector: #selector(dynamicFontChanged), name: .dynamicFontChanged, object: nil)
            } else {
                hideReaderModeBar(animated: false)
                NotificationCenter.default.removeObserver(self, name: .dynamicFontChanged, object: nil)
            }

            updateInContentHomePanel(url as URL)
        }
    }
    
    // This variable is used to keep track of current page. It is used to detect internal site navigation
    // to report internal page load to Rewards lib
    var rewardsXHRLoadURL: URL?
    
    /// Updates the URL bar security, text and button states.
    fileprivate func updateURLBar() {
        guard let tab = tabManager.selectedTab else { return }
        if let url = tab.url, !url.isLocal {
            // Notify Rewards of new page load.
            if let rewardsURL = rewardsXHRLoadURL,
                url.host == rewardsURL.host,
                url.isMediaSiteURL {
                tabManager.selectedTab?.reportPageNaviagtion(to: rewards)
                tabManager.selectedTab?.reportPageLoad(to: rewards)
            }
        }
        
        updateRewardsButtonState()
        
        topToolbar.currentURL = tab.url?.displayURL
        if tabManager.selectedTab === tab {
            topToolbar.secureContentState = tab.secureContentState
        }
        
        let isPage = tab.url?.displayURL?.isWebPage() ?? false
        navigationToolbar.updatePageStatus(isPage)
    }

    // MARK: Opening New Tabs

    func switchToPrivacyMode(isPrivate: Bool ) {
        let tabTrayController = self.tabTrayController ?? TabTrayController(tabManager: tabManager, profile: profile, tabTrayDelegate: self)
        if tabTrayController.privateMode != isPrivate {
            tabTrayController.changePrivacyMode(isPrivate)
        }
        self.tabTrayController = tabTrayController
    }

    func switchToTabForURLOrOpen(_ url: URL, isPrivate: Bool = false, isPrivileged: Bool, isExternal: Bool = false) {
        if !isExternal {
            popToBVC()
        }
        
        if let tab = tabManager.getTabForURL(url) {
            tabManager.selectTab(tab)
        } else {
            openURLInNewTab(url, isPrivate: isPrivate, isPrivileged: isPrivileged)
        }
    }

    func openURLInNewTab(_ url: URL?, isPrivate: Bool = false, isPrivileged: Bool) {
        topToolbar.leaveOverlayMode(didCancel: true)

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
    }

    func openBlankNewTab(attemptLocationFieldFocus: Bool, isPrivate: Bool = false, searchFor searchText: String? = nil) {
        popToBVC()
        openURLInNewTab(nil, isPrivate: isPrivate, isPrivileged: true)
        let freshTab = tabManager.selectedTab
        
        // Focus field only if requested and background images are not supported
        if attemptLocationFieldFocus {
            DispatchQueue.main.asyncAfter(deadline: .now() + .milliseconds(300)) {
                // Without a delay, the text field fails to become first responder
                // Check that the newly created tab is still selected.
                // This let's the user spam the Cmd+T button without lots of responder changes.
                guard freshTab == self.tabManager.selectedTab else { return }
                if let text = searchText {
                    self.topToolbar.setLocation(text, search: true)
                }
            }
        }
    }

    func popToBVC() {
        guard let currentViewController = navigationController?.topViewController else {
            return
        }
        currentViewController.dismiss(animated: true, completion: nil)
        
        if currentViewController != self {
            _ = self.navigationController?.popViewController(animated: true)
        } else if topToolbar.inOverlayMode {
            topToolbar.didClickCancel()
        }
    }

    fileprivate func presentActivityViewController(_ url: URL, tab: Tab? = nil, sourceView: UIView?, sourceRect: CGRect, arrowDirection: UIPopoverArrowDirection) {
        let helper = ShareExtensionHelper(url: url, tab: tab)
        
        let findInPageActivity = FindInPageActivity() { [unowned self] in
            self.updateFindInPageVisibility(visible: true)
        }
        
        let requestDesktopSiteActivity = RequestDesktopSiteActivity(tab: tab) { [weak tab] in
            tab?.switchUserAgent()
        }
        
        var activities: [UIActivity] = [findInPageActivity]
        
        // These actions don't apply if we're sharing a temporary document
        if !url.isFileURL {
            // We don't allow to have 2 same favorites.
            if !FavoritesHelper.isAlreadyAdded(url) {
                let addToFavoritesActivity = AddToFavoritesActivity() { [weak tab] in
                    FavoritesHelper.add(url: url, title: tab?.displayTitle)
                }
                activities.append(addToFavoritesActivity)
            }
            activities.append(requestDesktopSiteActivity)
            
            if Preferences.BraveToday.isEnabled.value, let metadata = tab?.pageMetadata,
               !metadata.feeds.isEmpty {
                let feeds: [RSSFeedLocation] = metadata.feeds.compactMap { feed in
                    if let url = URL(string: feed.href) {
                        return RSSFeedLocation(title: feed.title, url: url)
                    }
                    return nil
                }
                if !feeds.isEmpty {
                    let addToBraveToday = AddFeedToBraveTodayActivity() { [weak self] in
                        guard let self = self else { return }
                        let controller = BraveTodayAddSourceResultsViewController(
                            dataSource: self.feedDataSource,
                            searchedURL: url,
                            rssFeedLocations: feeds,
                            sourcesAdded: nil
                        )
                        let container = UINavigationController(rootViewController: controller)
                        let idiom = UIDevice.current.userInterfaceIdiom
                        if #available(iOS 13.0, *) {
                            container.modalPresentationStyle = idiom == .phone ? .pageSheet : .formSheet
                        } else {
                            container.modalPresentationStyle = idiom == .phone ? .fullScreen : .formSheet
                        }
                        self.present(container, animated: true)
                    }
                    activities.append(addToBraveToday)
                }
            }
            
            #if compiler(>=5.3)
            if #available(iOS 14.0, *), let webView = tab?.webView, tab?.temporaryDocument == nil {
                let createPDFActivity = CreatePDFActivity(webView: webView) { [weak self] pdfData in
                    guard let self = self else { return }
                    // Create a valid filename
                    let validFilenameSet = CharacterSet(charactersIn: ":/")
                        .union(.newlines)
                        .union(.controlCharacters)
                        .union(.illegalCharacters)
                    let filename = webView.title?.components(separatedBy: validFilenameSet).joined()
                    let url = URL(fileURLWithPath: NSTemporaryDirectory())
                        .appendingPathComponent("\(filename ?? "Untitled").pdf")
                    do {
                        try pdfData.write(to: url)
                        let pdfActivityController = UIActivityViewController(activityItems: [url], applicationActivities: nil)
                        if let popoverPresentationController = pdfActivityController.popoverPresentationController {
                            popoverPresentationController.sourceView = sourceView
                            popoverPresentationController.sourceRect = sourceRect
                            popoverPresentationController.permittedArrowDirections = arrowDirection
                            popoverPresentationController.delegate = self
                        }
                        self.present(pdfActivityController, animated: true)
                    } catch {
                        log.error("Failed to write PDF to disk: \(error)")
                    }
                }
                activities.append(createPDFActivity)
            }
            #endif
        } else {
            // Check if its a feed, url is a temp document file URL
            if let selectedTab = tabManager.selectedTab,
               (selectedTab.mimeType == "application/xml" || selectedTab.mimeType == "application/json"),
               let tabURL = selectedTab.url {
                let parser = FeedParser(URL: url)
                if case .success(let feed) = parser.parse() {
                    let addToBraveToday = AddFeedToBraveTodayActivity() { [weak self] in
                        guard let self = self else { return }
                        let controller = BraveTodayAddSourceResultsViewController(
                            dataSource: self.feedDataSource,
                            searchedURL: tabURL,
                            rssFeedLocations: [.init(title: feed.title, url: tabURL)],
                            sourcesAdded: nil
                        )
                        let container = UINavigationController(rootViewController: controller)
                        let idiom = UIDevice.current.userInterfaceIdiom
                        if #available(iOS 13.0, *) {
                            container.modalPresentationStyle = idiom == .phone ? .pageSheet : .formSheet
                        } else {
                            container.modalPresentationStyle = idiom == .phone ? .fullScreen : .formSheet
                        }
                        self.present(container, animated: true)
                    }
                    activities.append(addToBraveToday)
                }
            }
        }
        
        if let webView = tabManager.selectedTab?.webView,
           evaluateWebsiteSupportOpenSearchEngine(webView) {
            let addSearchEngineActivity = AddSearchEngineActivity() { [weak self] in
                self?.addCustomSearchEngineForFocusedElement()
            }
                
            activities.append(addSearchEngineActivity)
        }

        let controller = helper.createActivityViewController(items: activities) { [weak self] completed, _, documentUrl  in
            guard let self = self else { return }
            
            if let url = documentUrl {
                self.openPDFInIBooks(url)
            }
            
            self.cleanUpCreateActivity()
        }

        if let popoverPresentationController = controller.popoverPresentationController {
            popoverPresentationController.sourceView = sourceView
            popoverPresentationController.sourceRect = sourceRect
            popoverPresentationController.permittedArrowDirections = arrowDirection
            popoverPresentationController.delegate = self
        }

        present(controller, animated: true, completion: nil)
    }
    
    private func cleanUpCreateActivity() {
        // After dismissing, check to see if there were any prompts we queued up
        showQueuedAlertIfAvailable()

        // Usually the popover delegate would handle nil'ing out the references we have to it
        // on the BVC when displaying as a popover but the delegate method doesn't seem to be
        // invoked on iOS 10. See Bug 1297768 for additional details.
        displayedPopoverController = nil
        updateDisplayedPopoverProperties = nil
    }
    
    private func openPDFInIBooks(_ url: URL) {
        let iBooksURL = "itms-books://\(url.absoluteString)"

        guard let url = URL(string: iBooksURL) else { return }
        
        UIApplication.shared.open(url, options: [:])
    }

    func updateFindInPageVisibility(visible: Bool, tab: Tab? = nil) {
        if visible {
            if findInPageBar == nil {
                let findInPageBar = FindInPageBar()
                self.findInPageBar = findInPageBar
                findInPageBar.delegate = self
                alertStackView.addArrangedSubview(findInPageBar)
                findInPageBar.applyTheme(Theme.of(tabManager.selectedTab))

                findInPageBar.snp.makeConstraints { make in
                    make.height.equalTo(UIConstants.toolbarHeight)
                    make.edges.equalTo(alertStackView)
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
            webView.evaluateSafeJavaScript(functionName: "__firefox__.findDone")
            findInPageBar.removeFromSuperview()
            self.findInPageBar = nil
            updateViewConstraints()
        }
    }

    func navigateInTab(tab: Tab, to navigation: WKNavigation? = nil) {
        tabManager.expireSnackbars()

        guard let webView = tab.webView else {
            print("Cannot navigate in tab without a webView")
            return
        }

        if let url = webView.url {
            // Whether to show search icon or + icon
            toolbar?.setSearchButtonState(url: url)
            
            if !url.isErrorPageURL, !url.isAboutHomeURL, !url.isFileURL {
                // Fire the readability check. This is here and not in the pageShow event handler in ReaderMode.js anymore
                // because that event wil not always fire due to unreliable page caching. This will either let us know that
                // the currently loaded page can be turned into reading mode or if the page already is in reading mode. We
                // ignore the result because we are being called back asynchronous when the readermode status changes.
                webView.evaluateSafeJavaScript(functionName: "\(ReaderModeNamespace).checkReadability", sandboxed: false)

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
            // Ref #2016: Keyboard auto hides while typing
            // For some reason the web view will steal first responder as soon
            // as its added to the view heirarchy below. This line fixes that...
            // somehow...
            webView.resignFirstResponder()
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
    }
    
    private func focusLocationField() {
        if browserLockPopup != nil || duckDuckGoPopup != nil { return }
        topToolbar.tabLocationViewDidTapLocation(topToolbar.locationView)
    }
    
    // MARK: - Browser PIN Callout
    
    private var isBrowserLockEnabled: Bool {
        return KeychainWrapper.sharedAppContainerKeychain.authenticationInfo() != nil
    }
    
    private var browserLockPopup: AlertPopupView?
    
    // MARK: - DuckDuckGo Callout
    
    private var duckDuckGoPopup: AlertPopupView?
    func presentDuckDuckGoCallout(force: Bool = false) {
        if Preferences.DebugFlag.skipNTPCallouts == true { return }
        
        // Don't show when onboarding is showing
        if let presentedViewController = self.presentedViewController, presentedViewController.isKind(of: OnboardingNavigationController.self) {
            return
        }
        
        // Don't show duplicate popups
        if duckDuckGoPopup != nil { return }
        
        // Check to see if its been presented already
        if !SearchEngines.shouldShowDuckDuckGoPromo || (Preferences.Popups.duckDuckGoPrivateSearch.value && !force) {
            return
        }

        // Do not show ddg popup if user already chose it for private browsing.
        if profile.searchEngines.defaultEngine(forType: .privateMode).shortName == OpenSearchEngine.EngineNames.duckDuckGo {
            return
        }
        
        topToolbar.leaveOverlayMode()
        
        let popup = AlertPopupView(imageView: UIImageView(image: #imageLiteral(resourceName: "duckduckgo")), title: Strings.DDGCalloutTitle, message: Strings.DDGCalloutMessage)
        
        popup.addButton(title: Strings.DDGCalloutNo) {
            Preferences.Popups.duckDuckGoPrivateSearch.value = true
            self.duckDuckGoPopup = nil
            return .flyDown
        }
        popup.addButton(title: Strings.DDGCalloutEnable, type: .primary) { [weak self] in
            guard let self = self else { return .flyUp }
            self.duckDuckGoPopup = nil
            
            Preferences.Popups.duckDuckGoPrivateSearch.value = true
            self.profile.searchEngines.updateDefaultEngine(OpenSearchEngine.EngineNames.duckDuckGo, forType: .privateMode)
            
            self.tabManager.selectedTab?.newTabPageViewController?.updateDuckDuckGoVisibility()
            
            return .flyUp
        }
        duckDuckGoPopup = popup
        popup.showWithType(showType: .flyUp)
    }
    
    private func showBookmarkController() {
        let bookmarkViewController = BookmarksViewController(
            folder: Bookmarkv2.lastVisitedFolder(),
            isPrivateBrowsing: PrivateBrowsingManager.shared.isPrivateBrowsing)
        
        bookmarkViewController.toolbarUrlActionsDelegate = self
        
        presentSettingsNavigation(with: bookmarkViewController)
    }
    
    func openAddBookmark() {
        guard let selectedTab = tabManager.selectedTab,
              let selectedUrl = selectedTab.url,
              !(selectedUrl.isLocal || selectedUrl.isReaderModeURL) else {
            return
        }
        
        let bookmarkUrl = selectedUrl.decodeReaderModeURL ?? selectedUrl
        
        let mode = BookmarkEditMode.addBookmark(title: selectedTab.displayTitle, url: bookmarkUrl.absoluteString)
        
        let addBookMarkController = AddEditBookmarkTableViewController(mode: mode)
        
        presentSettingsNavigation(with: addBookMarkController, cancelEnabled: true)
    }
    
    private func presentSettingsNavigation(with controller: UIViewController, cancelEnabled: Bool = false) {
        let navigationController = SettingsNavigationController(rootViewController: controller)
        navigationController.modalPresentationStyle = .formSheet
        
        let cancelBarbutton = UIBarButtonItem(
            barButtonSystemItem: .cancel,
            target: navigationController,
            action: #selector(SettingsNavigationController.done))
        
        let doneBarbutton = UIBarButtonItem(
            barButtonSystemItem: .done,
            target: navigationController,
            action: #selector(SettingsNavigationController.done))
        
        navigationController.navigationBar.topItem?.leftBarButtonItem = cancelEnabled ? cancelBarbutton : nil
        
        navigationController.navigationBar.topItem?.rightBarButtonItem = doneBarbutton
        
        present(navigationController, animated: true)
    }
}

extension BrowserViewController: ClipboardBarDisplayHandlerDelegate {
    func shouldDisplay(clipboardBar bar: ButtonToast) {
        show(toast: bar, duration: ClipboardBarToastUX.toastDelay)
    }
}

extension BrowserViewController: QRCodeViewControllerDelegate {
    func didScanQRCodeWithURL(_ url: URL) {
        openBlankNewTab(attemptLocationFieldFocus: false)
        finishEditingAndSubmit(url, visitType: VisitType.typed)
    }

    func didScanQRCodeWithText(_ text: String) {
        openBlankNewTab(attemptLocationFieldFocus: false)
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

extension BrowserViewController: TopToolbarDelegate {
    func showTabTray() {
        if tabManager.tabsForCurrentMode.isEmpty {
            return
        }
        updateFindInPageVisibility(visible: false)
        
        let tabTrayController = TabTrayController(tabManager: tabManager, profile: profile, tabTrayDelegate: self)
        
        if tabManager.selectedTab == nil {
            tabManager.selectTab(tabManager.tabsForCurrentMode.first)
        }
        if let tab = tabManager.selectedTab {
            screenshotHelper.takeScreenshot(tab)
        }
        
        navigationController?.pushViewController(tabTrayController, animated: true)
        self.tabTrayController = tabTrayController
    }
    
    func topToolbarDidPressReload(_ topToolbar: TopToolbarView) {
        tabManager.selectedTab?.reload()
    }
    
    func topToolbarDidPressStop(_ topToolbar: TopToolbarView) {
        tabManager.selectedTab?.stop()
    }
    
    func topToolbarDidLongPressReloadButton(_ topToolbar: TopToolbarView, from button: UIButton) {
        guard let tab = tabManager.selectedTab else { return }
        let alert = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
        alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))
        
        let toggleActionTitle = tab.isDesktopSite == true ?
            Strings.appMenuViewMobileSiteTitleString : Strings.appMenuViewDesktopSiteTitleString
        alert.addAction(UIAlertAction(title: toggleActionTitle, style: .default, handler: { _ in
            tab.switchUserAgent()
        }))
        
        UIImpactFeedbackGenerator(style: .heavy).bzzt()
        if UIDevice.current.userInterfaceIdiom == .pad {
            alert.popoverPresentationController?.sourceView = self.view
            alert.popoverPresentationController?.sourceRect = self.view.convert(button.frame, from: button.superview)
            alert.popoverPresentationController?.permittedArrowDirections = [.up]
        }
        present(alert, animated: true)
    }

    func topToolbarDidPressTabs(_ topToolbar: TopToolbarView) {
        showTabTray()
    }

    func topToolbarDidPressReaderMode(_ topToolbar: TopToolbarView) {
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

    func topToolbarDidLongPressReaderMode(_ topToolbar: TopToolbarView) -> Bool {
        // Maybe we want to add something here down the road
        return false
    }

    func locationActions(for topToolbar: TopToolbarView) -> [AccessibleAction] {
        if UIPasteboard.general.hasStrings || UIPasteboard.general.hasURLs {
            return [pasteGoAction, pasteAction, copyAddressAction]
        } else {
            return [copyAddressAction]
        }
    }

    func topToolbarDisplayTextForURL(_ topToolbar: URL?) -> (String?, Bool) {
        // use the initial value for the URL so we can do proper pattern matching with search URLs
        var searchURL = self.tabManager.selectedTab?.currentInitialURL
        if searchURL?.isErrorPageURL ?? true {
            searchURL = topToolbar
        }
        if let query = profile.searchEngines.queryForSearchURL(searchURL as URL?) {
            return (query, true)
        } else {
            return (topToolbar?.absoluteString, false)
        }
    }

    func topToolbarDidLongPressLocation(_ topToolbar: TopToolbarView) {
        let alert = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
        
        for action in locationActions(for: topToolbar) {
            alert.addAction(action.alertAction(style: .default))
        }
        
        alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))
        
        let setupPopover = { [unowned self] in
            if let popoverPresentationController = alert.popoverPresentationController {
                popoverPresentationController.sourceView = topToolbar
                popoverPresentationController.sourceRect = topToolbar.frame
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

    func topToolbarDidPressScrollToTop(_ topToolbar: TopToolbarView) {
        if let selectedTab = tabManager.selectedTab, favoritesController == nil {
            // Only scroll to top if we are not showing the home view controller
            selectedTab.webView?.scrollView.setContentOffset(CGPoint.zero, animated: true)
        }
    }

    func topToolbarLocationAccessibilityActions(_ topToolbar: TopToolbarView) -> [UIAccessibilityCustomAction]? {
        return locationActions(for: topToolbar).map { $0.accessibilityCustomAction }
    }

    func topToolbar(_ topToolbar: TopToolbarView, didEnterText text: String) {
        if text.isEmpty {
            hideSearchController()
        } else {
            showSearchController()
            searchController?.searchQuery = text
            searchLoader?.query = text
        }
    }

    func topToolbar(_ topToolbar: TopToolbarView, didSubmitText text: String) {
        processAddressBar(text: text, visitType: nil)
    }

    func processAddressBar(text: String, visitType: VisitType?) {
        if let fixupURL = URIFixup.getURL(text) {
            // The user entered a URL, so use it.
            finishEditingAndSubmit(fixupURL, visitType: visitType ?? .typed)
            return
        }

        // We couldn't build a URL, so pass it on to the search engine.
        submitSearchText(text)
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

    func topToolbarDidEnterOverlayMode(_ topToolbar: TopToolbarView) {
        if .blankPage == NewTabAccessors.getNewTabPage() {
            UIAccessibility.post(notification: .screenChanged, argument: nil)
        } else {
            if let toast = clipboardBarDisplayHandler?.clipboardToast {
                toast.removeFromSuperview()
            }
            displayFavoritesController()
        }
    }

    func topToolbarDidLeaveOverlayMode(_ topToolbar: TopToolbarView) {
        hideSearchController()
        hideFavoritesController()
        updateInContentHomePanel(tabManager.selectedTab?.url as URL?)
    }

    func topToolbarDidBeginDragInteraction(_ topToolbar: TopToolbarView) {
        dismissVisibleMenus()
    }
    
    func topToolbarDidTapBraveShieldsButton(_ topToolbar: TopToolbarView) {
        presentBraveShieldsViewController()
    }
    
    func presentBraveShieldsViewController() {
        guard let selectedTab = tabManager.selectedTab, var url = selectedTab.url else { return }
        if url.isErrorPageURL, let originalURL = url.originalURLFromErrorURL {
            url = originalURL
        }
        if url.isLocalUtility {
            return
        }
        let shields = ShieldsViewController(tab: selectedTab)
        shields.shieldsSettingsChanged = { [unowned self] _ in
            // Update the shields status immediately
            self.topToolbar.refreshShieldsStatus()
            
            // Reload this tab. This will also trigger an update of the brave icon in `TabLocationView` if
            // the setting changed is the global `.AllOff` shield
            self.tabManager.selectedTab?.reload()
            
            // In 1.6 we "reload" the whole web view state, dumping caches, etc. (reload():BraveWebView.swift:495)
            // BRAVE TODO: Port over proper tab reloading with Shields
        }
        shields.showGlobalShieldsSettings = { [unowned self] vc in
            vc.dismiss(animated: true) {
                let shieldsAndPrivacy = BraveShieldsAndPrivacySettingsController(
                    profile: self.profile,
                    tabManager: self.tabManager,
                    feedDataSource: self.feedDataSource
                )
                let container = SettingsNavigationController(rootViewController: shieldsAndPrivacy)
                container.isModalInPresentation = true
                container.modalPresentationStyle =
                    UIDevice.current.userInterfaceIdiom == .phone ? .pageSheet : .formSheet
                shieldsAndPrivacy.navigationItem.rightBarButtonItem = .init(
                    barButtonSystemItem: .done,
                    target: container,
                    action: #selector(SettingsNavigationController.done)
                )
                self.present(container, animated: true)
            }
        }
        let container = PopoverNavigationController(rootViewController: shields)
        let popover = PopoverController(contentController: container, contentSizeBehavior: .preferredContentSize)
        popover.present(from: topToolbar.locationView.shieldsButton, on: self)
    }
    
    // TODO: This logic should be fully abstracted away and share logic from current MenuViewController
    // See: https://github.com/brave/brave-ios/issues/1452
    func topToolbarDidTapBookmarkButton(_ topToolbar: TopToolbarView) {
        showBookmarkController()
    }
    
    func topToolbarDidTapBraveRewardsButton(_ topToolbar: TopToolbarView) {
        showBraveRewardsPanel()
    }
    
    func topToolbarDidLongPressBraveRewardsButton(_ topToolbar: TopToolbarView) {
        showRewardsDebugSettings()
    }
    
    func topToolbarDidTapMenuButton(_ topToolbar: TopToolbarView) {
        tabToolbarDidPressMenu(topToolbar)
    }
}

extension BrowserViewController: ToolbarDelegate {
    func tabToolbarDidPressSearch(_ tabToolbar: ToolbarProtocol, button: UIButton) {
        topToolbar.tabLocationViewDidTapLocation(topToolbar.locationView)
    }
    
    func tabToolbarDidPressBack(_ tabToolbar: ToolbarProtocol, button: UIButton) {
        tabManager.selectedTab?.goBack()
    }

    func tabToolbarDidLongPressBack(_ tabToolbar: ToolbarProtocol, button: UIButton) {
        UIImpactFeedbackGenerator(style: .heavy).bzzt()
        showBackForwardList()
    }
    
    func tabToolbarDidPressForward(_ tabToolbar: ToolbarProtocol, button: UIButton) {
        tabManager.selectedTab?.goForward()
    }
    
    func tabToolbarDidPressShare() {
        func share(url: URL) {
            presentActivityViewController(
                url,
                tab: url.isFileURL ? nil : tabManager.selectedTab,
                sourceView: view,
                sourceRect: view.convert(topToolbar.menuButton.frame, from: topToolbar.menuButton.superview),
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
    
    func tabToolbarDidPressMenu(_ tabToolbar: ToolbarProtocol) {
        let homePanel = MenuViewController(bvc: self, tab: tabManager.selectedTab)
        let popover = PopoverController(contentController: homePanel, contentSizeBehavior: .preferredContentSize)
        // Not dynamic, but trivial at this point, given how UI is currently setup
        popover.color = Theme.of(tabManager.selectedTab).colors.home
        popover.present(from: tabToolbar.menuButton, on: self)
    }
    
    func tabToolbarDidPressAddTab(_ tabToolbar: ToolbarProtocol, button: UIButton) {
        self.openBlankNewTab(attemptLocationFieldFocus: true, isPrivate: PrivateBrowsingManager.shared.isPrivateBrowsing)
    }

    func tabToolbarDidLongPressAddTab(_ tabToolbar: ToolbarProtocol, button: UIButton) {
        showAddTabContextMenu(sourceView: toolbar ?? topToolbar, button: button)
    }
    
    private func addTabAlertActions() -> [UIAlertAction] {
        var actions: [UIAlertAction] = []
        if !PrivateBrowsingManager.shared.isPrivateBrowsing {
            let newPrivateTabAction = UIAlertAction(title: Strings.newPrivateTabTitle, style: .default, handler: { [unowned self] _ in
                // BRAVE TODO: Add check for DuckDuckGo popup (and based on 1.6, whether the browser lock is enabled?)
                // before focusing on the url bar
                self.openBlankNewTab(attemptLocationFieldFocus: true, isPrivate: true)
            })
            actions.append(newPrivateTabAction)
        }
        let bottomActionTitle = PrivateBrowsingManager.shared.isPrivateBrowsing ? Strings.newPrivateTabTitle : Strings.newTabTitle
        actions.append(UIAlertAction(title: bottomActionTitle, style: .default, handler: { [unowned self] _ in
            self.openBlankNewTab(attemptLocationFieldFocus: true, isPrivate: PrivateBrowsingManager.shared.isPrivateBrowsing)
        }))
        return actions
    }
    
    func showAddTabContextMenu(sourceView: UIView, button: UIButton) {
        let alertController = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
        alertController.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))
        addTabAlertActions().forEach(alertController.addAction)
        alertController.popoverPresentationController?.sourceView = sourceView
        alertController.popoverPresentationController?.sourceRect = button.frame
        UIImpactFeedbackGenerator(style: .heavy).bzzt()
        present(alertController, animated: true)
    }
    
    func tabToolbarDidLongPressForward(_ tabToolbar: ToolbarProtocol, button: UIButton) {
        UIImpactFeedbackGenerator(style: .heavy).bzzt()
        showBackForwardList()
    }

    func tabToolbarDidPressTabs(_ tabToolbar: ToolbarProtocol, button: UIButton) {
        showTabTray()
    }
    
    func tabToolbarDidLongPressTabs(_ tabToolbar: ToolbarProtocol, button: UIButton) {
        guard self.presentedViewController == nil else {
            return
        }
        let controller = AlertController(title: nil, message: nil, preferredStyle: .actionSheet)
        
        if (UIDevice.current.userInterfaceIdiom == .pad && tabsBar.view.isHidden) ||
            (UIDevice.current.userInterfaceIdiom == .phone && toolbar == nil) {
            addTabAlertActions().forEach(controller.addAction)
        }
        
        if tabManager.tabsForCurrentMode.count > 1 {
            controller.addAction(UIAlertAction(title: String(format: Strings.closeAllTabsTitle, tabManager.tabsForCurrentMode.count), style: .destructive, handler: { _ in
                self.tabManager.removeAll()
            }), accessibilityIdentifier: "toolbarTabButtonLongPress.closeTab")
        }
        controller.addAction(UIAlertAction(title: Strings.closeTabTitle, style: .destructive, handler: { _ in
            if let tab = self.tabManager.selectedTab {
                self.tabManager.removeTab(tab)
            }
        }), accessibilityIdentifier: "toolbarTabButtonLongPress.closeTab")
        controller.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil), accessibilityIdentifier: "toolbarTabButtonLongPress.cancel")
        controller.popoverPresentationController?.sourceView = toolbar ?? topToolbar
        controller.popoverPresentationController?.sourceRect = button.frame
        UIImpactFeedbackGenerator(style: .heavy).bzzt()
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
    
    func tabToolbarDidSwipeToChangeTabs(_ tabToolbar: ToolbarProtocol, direction: UISwipeGestureRecognizer.Direction) {
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
        topToolbar.leaveOverlayMode(didCancel: true)
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
        tab.addContentScript(readerMode, name: ReaderMode.name(), sandboxed: false)

        // only add the logins helper if the tab is not a private browsing tab
        if !tab.isPrivate {
            let logins = LoginsHelper(tab: tab, profile: profile)
            tab.addContentScript(logins, name: LoginsHelper.name(), sandboxed: false)
        }

        let errorHelper = ErrorPageHelper()
        tab.addContentScript(errorHelper, name: ErrorPageHelper.name(), sandboxed: false)

        let sessionRestoreHelper = SessionRestoreHelper(tab: tab)
        sessionRestoreHelper.delegate = self
        tab.addContentScript(sessionRestoreHelper, name: SessionRestoreHelper.name())

        let findInPageHelper = FindInPageHelper(tab: tab)
        findInPageHelper.delegate = self
        tab.addContentScript(findInPageHelper, name: FindInPageHelper.name(), sandboxed: false)

        let noImageModeHelper = NoImageModeHelper(tab: tab)
        tab.addContentScript(noImageModeHelper, name: NoImageModeHelper.name())
        
        let printHelper = PrintHelper(tab: tab)
        tab.addContentScript(printHelper, name: PrintHelper.name(), sandboxed: false)

        let customSearchHelper = CustomSearchHelper(tab: tab)
        tab.addContentScript(customSearchHelper, name: CustomSearchHelper.name())

        // XXX: Bug 1390200 - Disable NSUserActivity/CoreSpotlight temporarily
        // let spotlightHelper = SpotlightHelper(tab: tab)
        // tab.addHelper(spotlightHelper, name: SpotlightHelper.name())

        tab.addContentScript(LocalRequestHelper(), name: LocalRequestHelper.name(), sandboxed: false)

        tab.contentBlocker.setupTabTrackingProtection()
        tab.addContentScript(tab.contentBlocker, name: ContentBlockerHelper.name(), sandboxed: false)

        tab.addContentScript(FocusHelper(tab: tab), name: FocusHelper.name())
        
        tab.addContentScript(FingerprintingProtection(tab: tab), name: FingerprintingProtection.name(), sandboxed: false)
        
        tab.addContentScript(BraveGetUA(tab: tab), name: BraveGetUA.name(), sandboxed: false)

        if YubiKitDeviceCapabilities.supportsMFIAccessoryKey {
            tab.addContentScript(U2FExtensions(tab: tab), name: U2FExtensions.name(), sandboxed: false)
        }
        
        tab.addContentScript(ResourceDownloadManager(tab: tab), name: ResourceDownloadManager.name(), sandboxed: false)
        
        tab.addContentScript(WindowRenderHelperScript(tab: tab), name: WindowRenderHelperScript.name(), sandboxed: false)
        
        tab.addContentScript(RewardsReporting(rewards: rewards, tab: tab), name: RewardsReporting.name())
        tab.addContentScript(AdsMediaReporting(rewards: rewards, tab: tab), name: AdsMediaReporting.name())
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
        self.view.bringSubviewToFront(self.alertStackView)
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
        self.topToolbar.setLocation(suggestion, search: true)
    }

    func presentSearchSettingsController() {
        let settingsNavigationController = SearchSettingsTableViewController(profile: profile)
        let navController = ModalSettingsNavigationController(rootViewController: settingsNavigationController)

        self.present(navController, animated: true, completion: nil)
    }

    func searchViewController(_ searchViewController: SearchViewController, didHighlightText text: String, search: Bool) {
        self.topToolbar.setLocation(text, search: search)
    }
    
    func searchViewController(_ searchViewController: SearchViewController, shouldFindInPage query: String) {
        topToolbar.leaveOverlayMode()
        updateFindInPageVisibility(visible: true)
        findInPageBar?.text = query
        
    }
    
    func searchViewControllerAllowFindInPage() -> Bool {
        return tabManager.selectedTab?.webView?.url?.isAboutHomeURL == false
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
        
        toolbar?.setSearchButtonState(url: selected?.url)
        if let tab = selected, let webView = tab.webView {
            updateURLBar()
            
            if let url = tab.url, !url.isLocalUtility {
                let previousEstimatedProgress = previous?.webView?.estimatedProgress ?? 1.0
                let selectedEstimatedProgress = webView.estimatedProgress
                
                // Progress should be updated only if there's a difference between tabs.
                // Otherwise we do nothing, so switching between fully loaded tabs won't show the animation.
                if previousEstimatedProgress != selectedEstimatedProgress {
                    topToolbar.updateProgressBar(Float(selectedEstimatedProgress))
                }
            }

            let newTheme = Theme.of(tab)
            if previous == nil || newTheme != Theme.of(previous) {
                applyTheme(newTheme)
            }

            readerModeCache = ReaderMode.cache(for: tab)
            ReaderModeHandlers.readerModeCache = readerModeCache

            scrollController.tab = selected
            webViewContainer.addSubview(webView)
            webView.snp.makeConstraints { make in
                make.left.right.top.bottom.equalTo(self.webViewContainer)
            }
            webView.accessibilityLabel = Strings.webContentAccessibilityLabel
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
        
        self.presentDuckDuckGoCalloutIfNeeded()

        removeAllBars()
        if let bars = selected?.bars {
            for bar in bars {
                showBar(bar, animated: true)
            }
        }

        updateFindInPageVisibility(visible: false, tab: previous)
        updateTabsBarVisibility()

        topToolbar.locationView.loading = selected?.loading ?? false
        navigationToolbar.updateBackStatus(selected?.canGoBack ?? false)
        navigationToolbar.updateForwardStatus(selected?.canGoForward ?? false)
        
        if let readerMode = selected?.getContentScript(name: ReaderMode.name()) as? ReaderMode {
            topToolbar.updateReaderModeState(readerMode.state)
            if readerMode.state == .active {
                showReaderModeBar(animated: false)
            } else {
                hideReaderModeBar(animated: false)
            }
        } else {
            topToolbar.updateReaderModeState(ReaderModeState.unavailable)
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
        topToolbar.leaveOverlayMode(didCancel: true)
        if let url = tab.url, !url.isAboutURL && !tab.isPrivate {
            profile.recentlyClosedTabs.addTab(url as URL, title: tab.title, faviconURL: tab.displayFavicon?.url)
        }
        updateTabsBarVisibility()
        
        rewards.reportTabClosed(tabId: tab.rewardsId)
    }

    func tabManagerDidAddTabs(_ tabManager: TabManager) {
        updateTabCountUsingTabManager(tabManager)
    }

    func tabManagerDidRestoreTabs(_ tabManager: TabManager) {
        updateTabCountUsingTabManager(tabManager)
    }

    func show(toast: Toast, afterWaiting delay: DispatchTimeInterval = SimpleToastUX.toastDelayBefore, duration: DispatchTimeInterval? = SimpleToastUX.toastDismissAfter) {
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
        show(toast: toast, afterWaiting: ButtonToastUX.toastDelay)
    }

    fileprivate func updateTabCountUsingTabManager(_ tabManager: TabManager) {
        let count = tabManager.tabsForCurrentMode.count
        toolbar?.updateTabCount(count)
        topToolbar.updateTabCount(count)
    }
}

/// List of schemes that are allowed to be opened in new tabs.
private let schemesAllowedToBeOpenedAsPopups = ["http", "https", "javascript", "about", "whatsapp"]

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
        
        newTab.url = URL(string: "about:blank")

        return newTab.webView
    }

    func shouldRequestBeOpenedAsPopup(_ request: URLRequest) -> Bool {
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
        var messageAlert = MessageAlert(message: message, frame: frame, completionHandler: completionHandler, suppressHandler: nil)
        handleAlert(webView: webView, alert: &messageAlert) {
            completionHandler()
        }
    }

    func webView(_ webView: WKWebView, runJavaScriptConfirmPanelWithMessage message: String, initiatedByFrame frame: WKFrameInfo, completionHandler: @escaping (Bool) -> Void) {
        var confirmAlert = ConfirmPanelAlert(message: message, frame: frame, completionHandler: completionHandler, suppressHandler: nil)
        handleAlert(webView: webView, alert: &confirmAlert) {
            completionHandler(false)
        }
    }

    func webView(_ webView: WKWebView, runJavaScriptTextInputPanelWithPrompt prompt: String, defaultText: String?, initiatedByFrame frame: WKFrameInfo, completionHandler: @escaping (String?) -> Void) {
        var textInputAlert = TextInputAlert(message: prompt, frame: frame, completionHandler: completionHandler, defaultText: defaultText, suppressHandler: nil)
        handleAlert(webView: webView, alert: &textInputAlert) {
            completionHandler(nil)
        }
    }
    
    func suppressJSAlerts(webView: WKWebView) {
        let script = """
                    window.alert=window.confirm=window.prompt=function(n){},
                    [].slice.apply(document.querySelectorAll('iframe')).forEach(function(n){if(n.contentWindow != window){n.contentWindow.alert=n.contentWindow.confirm=n.contentWindow.prompt=function(n){}}})
                    """
        webView.evaluateSafeJavaScript(functionName: script, sandboxed: false, asFunction: false)
    }
    
    func handleAlert<T: JSAlertInfo>(webView: WKWebView, alert: inout T, completionHandler: @escaping () -> Void) {
        guard let promptingTab = tabManager[webView], !promptingTab.blockAllAlerts else {
            suppressJSAlerts(webView: webView)
            tabManager[webView]?.cancelQueuedAlerts()
            completionHandler()
            return
        }
        promptingTab.alertShownCount += 1
        var suppressBlock: JSAlertInfo.SuppressHandler = {[unowned self] suppress in
            if suppress {
                func suppressDialogues(_: UIAlertAction) {
                    self.suppressJSAlerts(webView: webView)
                    promptingTab.blockAllAlerts = true
                    self.tabManager[webView]?.cancelQueuedAlerts()
                    completionHandler()
                }
                // Show confirm alert here.
                let suppressSheet = UIAlertController(title: nil, message: Strings.suppressAlertsActionMessage, preferredStyle: .actionSheet)
                suppressSheet.addAction(UIAlertAction(title: Strings.suppressAlertsActionTitle, style: .destructive, handler: suppressDialogues))
                suppressSheet.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: { _ in
                    completionHandler()
                }))
                if UIDevice.current.userInterfaceIdiom == .pad, let popoverController = suppressSheet.popoverPresentationController {
                    popoverController.sourceView = self.view
                    popoverController.sourceRect = CGRect(x: self.view.bounds.midX, y: self.view.bounds.midY, width: 0, height: 0)
                    popoverController.permittedArrowDirections = []
                }
                self.present(suppressSheet, animated: true)
            } else {
                completionHandler()
            }
        }
        alert.suppressHandler = promptingTab.alertShownCount > 1 ? suppressBlock : nil
        if shouldDisplayJSAlertForWebView(webView) {
            let controller = alert.alertController()
            controller.delegate = self
            present(controller, animated: true)
        } else {
            promptingTab.queueJavascriptAlertPrompt(alert)
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
                topToolbar.currentURL = tab.url?.displayURL
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
    
    func webView(_ webView: WKWebView, contextMenuConfigurationForElement elementInfo: WKContextMenuElementInfo, completionHandler: @escaping (UIContextMenuConfiguration?) -> Void) {
        
        guard let url = elementInfo.linkURL else { return completionHandler(UIContextMenuConfiguration(identifier: nil, previewProvider: nil, actionProvider: nil)) }
        
        let actionProvider: UIContextMenuActionProvider = { _ -> UIMenu? in
            var actions = [UIAction]()
            
            if let currentTab = self.tabManager.selectedTab {
                let tabType = currentTab.type
                
                if !tabType.isPrivate {
                    let openNewTabAction = UIAction(title: Strings.openNewTabButtonTitle,
                                                    image: UIImage(systemName: "plus")) { _ in
                        self.addTab(url: url, inPrivateMode: false, currentTab: currentTab)
                    }
                    
                    openNewTabAction.accessibilityLabel = "linkContextMenu.openInNewTab"
                    
                    actions.append(openNewTabAction)
                }
                
                let openNewPrivateTabAction = UIAction(title: Strings.openNewPrivateTabButtonTitle,
                                                       image: #imageLiteral(resourceName: "private_glasses").template) { _ in
                    self.addTab(url: url, inPrivateMode: true, currentTab: currentTab)
                }
                openNewPrivateTabAction.accessibilityLabel = "linkContextMenu.openInNewPrivateTab"
                
                actions.append(openNewPrivateTabAction)
                
                let copyAction = UIAction(title: Strings.copyLinkActionTitle,
                                          image: UIImage(systemName: "doc.on.doc")) { _ in
                    UIPasteboard.general.url = url
                }
                copyAction.accessibilityLabel = "linkContextMenu.copyLink"
                
                actions.append(copyAction)
                
                if let braveWebView = webView as? BraveWebView {
                    let shareAction = UIAction(title: Strings.shareLinkActionTitle,
                                               image: UIImage(systemName: "square.and.arrow.up")) { _ in
                        let touchPoint = braveWebView.lastHitPoint
                        let touchSize = CGSize(width: 0, height: 16)
                        let touchRect = CGRect(origin: touchPoint, size: touchSize)
                        
                        self.presentActivityViewController(url, sourceView: self.view,
                                                           sourceRect: touchRect,
                                                           arrowDirection: .any)
                    }
                    
                    shareAction.accessibilityLabel = "linkContextMenu.share"
                    
                    actions.append(shareAction)
                }
                
                let linkPreview = Preferences.General.enableLinkPreview.value
                
                let linkPreviewTitle = linkPreview ?
                    Strings.hideLinkPreviewsActionTitle : Strings.showLinkPreviewsActionTitle
                let linkPreviewAction = UIAction(title: linkPreviewTitle, image: UIImage(systemName: "eye.fill")) { _ in
                    Preferences.General.enableLinkPreview.value.toggle()
                }
            
                actions.append(linkPreviewAction)
            }
            
            return UIMenu(title: url.absoluteString.truncate(length: 100), children: actions)
        }
        
        let linkPreview: UIContextMenuContentPreviewProvider = {
            return LinkPreviewViewController(url: url)
        }
        
        let linkPreviewProvider = Preferences.General.enableLinkPreview.value ? linkPreview : nil
        let config = UIContextMenuConfiguration(identifier: nil, previewProvider: linkPreviewProvider,
                                                actionProvider: actionProvider)
        
        completionHandler(config)
    }
    
    func webView(_ webView: WKWebView, contextMenuForElement elementInfo: WKContextMenuElementInfo, willCommitWithAnimator animator: UIContextMenuInteractionCommitAnimating) {
        guard let url = elementInfo.linkURL else { return }
        webView.load(URLRequest(url: url))
    }
    
    fileprivate func addTab(url: URL, inPrivateMode: Bool, currentTab: Tab) {
        let tab = self.tabManager.addTab(URLRequest(url: url), afterTab: currentTab, isPrivate: inPrivateMode)
        if inPrivateMode && !PrivateBrowsingManager.shared.isPrivateBrowsing {
            self.tabManager.selectTab(tab)
        } else {
            // We're not showing the top tabs; show a toast to quick switch to the fresh new tab.
            let toast = ButtonToast(labelText: Strings.contextMenuButtonToastNewTabOpenedLabelText, buttonText: Strings.contextMenuButtonToastNewTabOpenedButtonText, completion: { buttonPressed in
                if buttonPressed {
                    self.tabManager.selectTab(tab)
                }
            })
            self.show(toast: toast)
        }
        self.scrollController.showToolbars(animated: true)
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
        // BRAVE TODO: Not Sure..
    }

    func tabTrayRequestsPresentationOf(_ viewController: UIViewController) {
        self.present(viewController, animated: false, completion: nil)
    }
}

// MARK: Browser Chrome Theming
extension BrowserViewController: Themeable {
    
    var themeableChildren: [Themeable?]? {
        return [topToolbar,
                toolbar,
                readerModeBar,
                tabsBar,
                tabManager.selectedTab?.newTabPageViewController,
                favoritesController]
    }
    
    func applyTheme(_ theme: Theme) {
        theme.applyAppearanceProperties()
        
        styleChildren(theme: theme)

        statusBarOverlay.backgroundColor = topToolbar.backgroundColor
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
        webView.evaluateSafeJavaScript(functionName: "__firefox__.\(function)", args: [text], sandboxed: false)
    }

    func findInPageHelper(_ findInPageHelper: FindInPageHelper, didUpdateCurrentResult currentResult: Int) {
        findInPageBar?.currentResult = currentResult
    }

    func findInPageHelper(_ findInPageHelper: FindInPageHelper, didUpdateTotalResults totalResults: Int) {
        findInPageBar?.totalResults = totalResults
    }
    
    func findTextInPage(_ direction: TextSearchDirection) {
        guard let seachText = findInPageBar?.text else { return }
                
        find(seachText, function: direction.rawValue)
    }
}

extension BrowserViewController: JSPromptAlertControllerDelegate {
    func promptAlertControllerDidDismiss(_ alertController: JSPromptAlertController) {
        showQueuedAlertIfAvailable()
    }
}

extension BrowserViewController: ToolbarUrlActionsDelegate {
    /// The types of actions a user can do in the menu given a URL
    private enum ToolbarURLAction {
        case openInCurrentTab
        case openInNewTab(isPrivate: Bool)
        case copy
        case share
    }
    
    func openInNewTab(_ url: URL, isPrivate: Bool) {
        topToolbar.leaveOverlayMode()
        select(url, visitType: .unknown, action: .openInNewTab(isPrivate: isPrivate))
    }
    
    func copy(_ url: URL) {
        select(url, visitType: .unknown, action: .copy)
    }
    
    func share(_ url: URL) {
        select(url, visitType: .unknown, action: .share)
    }
    
    func batchOpen(_ urls: [URL]) {
        let tabIsPrivate = TabType.of(tabManager.selectedTab).isPrivate
        self.tabManager.addTabsForURLs(urls, zombie: false, isPrivate: tabIsPrivate)
    }
    
    func select(url: URL, visitType: VisitType) {
        select(url, visitType: visitType, action: .openInCurrentTab)
    }
    
    private func select(_ url: URL, visitType: VisitType, action: ToolbarURLAction) {
        switch action {
        case .openInCurrentTab:
            finishEditingAndSubmit(url, visitType: visitType)
        case .openInNewTab(let isPrivate):
            let tab = tabManager.addTab(PrivilegedRequest(url: url) as URLRequest, afterTab: tabManager.selectedTab, isPrivate: isPrivate)
            if isPrivate && !PrivateBrowsingManager.shared.isPrivateBrowsing {
                tabManager.selectTab(tab)
            } else {
                // If we are showing toptabs a user can just use the top tab bar
                // If in overlay mode switching doesnt correctly dismiss the homepanels
                guard !topToolbar.inOverlayMode else {
                    return
                }
                // We're not showing the top tabs; show a toast to quick switch to the fresh new tab.
                let toast = ButtonToast(labelText: Strings.contextMenuButtonToastNewTabOpenedLabelText, buttonText: Strings.contextMenuButtonToastNewTabOpenedButtonText, completion: { buttonPressed in
                    if buttonPressed {
                        self.tabManager.selectTab(tab)
                    }
                })
                show(toast: toast)
            }
        case .copy:
            UIPasteboard.general.url = url
        case .share:
            presentActivityViewController(
                url,
                sourceView: view,
                sourceRect: view.convert(topToolbar.shareButton.frame, from: topToolbar.shareButton.superview),
                arrowDirection: [.up]
            )
        }
    }
}

extension BrowserViewController: NewTabPageDelegate {
    func navigateToInput(_ input: String, inNewTab: Bool, switchingToPrivateMode: Bool) {
        let isPrivate = PrivateBrowsingManager.shared.isPrivateBrowsing || switchingToPrivateMode
        if inNewTab {
            tabManager.addTabAndSelect(isPrivate: isPrivate)
        }
        processAddressBar(text: input, visitType: .bookmark)
    }
    
    func handleFavoriteAction(favorite: Favorite, action: BookmarksAction) {
        guard let url = favorite.url else { return }
        switch action {
        case .opened(let inNewTab, let switchingToPrivateMode):
            navigateToInput(
                url,
                inNewTab: inNewTab,
                switchingToPrivateMode: switchingToPrivateMode
            )
        case .edited:
            guard let title = favorite.displayTitle, let urlString = favorite.url else { return }
            let editPopup = UIAlertController
                .userTextInputAlert(title: Strings.editBookmark,
                                    message: urlString,
                                    startingText: title, startingText2: favorite.url,
                                    placeholder2: urlString,
                                    keyboardType2: .URL) { callbackTitle, callbackUrl in
                    if let cTitle = callbackTitle, !cTitle.isEmpty, let cUrl = callbackUrl, !cUrl.isEmpty {
                        if URL(string: cUrl) != nil {
                            favorite.update(customTitle: cTitle, url: cUrl)
                        }
                    }
                }
            self.present(editPopup, animated: true)
        }
    }
    
    func focusURLBar() {
        focusLocationField()
    }
    
    func tappedDuckDuckGoCallout() {
        presentDuckDuckGoCallout(force: true)
    }
    
    func brandedImageCalloutActioned(_ state: BrandedImageCalloutState) {
        guard state.hasDetailViewController else { return }
        
        let vc = NTPLearnMoreViewController(state: state, rewards: rewards)
        
        vc.linkHandler = { [weak self] url in
            self?.tabManager.selectedTab?.loadRequest(PrivilegedRequest(url: url) as URLRequest)
        }
        
        addChild(vc)
        view.addSubview(vc.view)
        vc.view.snp.remakeConstraints {
            $0.right.top.bottom.leading.equalToSuperview()
        }
    }
    
    func tappedQRCodeButton(url: URL) {
        let qrPopup = QRCodePopupView(url: url)
        qrPopup.showWithType(showType: .flyUp)
        qrPopup.qrCodeShareHandler = { [weak self] url in
            guard let self = self else { return }
            
            let viewRect = CGRect(origin: self.view.center, size: .zero)
            
            self.presentActivityViewController(url, sourceView: self.view, sourceRect: viewRect,
                                               arrowDirection: .any)
        }
    }
}

extension BrowserViewController: PreferencesObserver {
    func preferencesDidChange(for key: String) {
        switch key {
        case Preferences.General.tabBarVisibility.key:
            updateTabsBarVisibility()
        case Preferences.General.themeNormalMode.key:
            applyTheme(Theme.of(tabManager.selectedTab))
        case Preferences.Privacy.privateBrowsingOnly.key:
            let isPrivate = Preferences.Privacy.privateBrowsingOnly.value
            switchToPrivacyMode(isPrivate: isPrivate)
            PrivateBrowsingManager.shared.isPrivateBrowsing = isPrivate
            setupTabs()
            updateTabsBarVisibility()
            updateApplicationShortcuts()
        case Preferences.General.alwaysRequestDesktopSite.key:
            tabManager.reset()
            self.tabManager.reloadSelectedTab()
        case Preferences.Shields.blockAdsAndTracking.key,
             Preferences.Shields.httpsEverywhere.key,
             Preferences.Shields.blockScripts.key,
             Preferences.Shields.blockPhishingAndMalware.key,
             Preferences.Shields.blockImages.key,
             Preferences.Shields.fingerprintingProtection.key,
             Preferences.Shields.useRegionAdBlock.key:
            tabManager.allTabs.forEach { $0.webView?.reload() }
        case Preferences.Privacy.blockAllCookies.key,
             Preferences.Shields.googleSafeBrowsing.key:
            // All `block all cookies` toggle requires a hard reset of Webkit configuration.
            tabManager.reset()
            if !Preferences.Privacy.blockAllCookies.value {
                HTTPCookie.loadFromDisk { _ in
                    self.tabManager.reloadSelectedTab()
                    for tab in self.tabManager.allTabs where tab != self.tabManager.selectedTab {
                        tab.createWebview()
                        if let url = tab.webView?.url {
                            tab.loadRequest(PrivilegedRequest(url: url) as URLRequest)
                        }
                    }
                }
            } else {
                tabManager.reloadSelectedTab()
            }
        case Preferences.Rewards.hideRewardsIcon.key,
             Preferences.Rewards.rewardsToggledOnce.key:
            updateRewardsButtonState()
        case Preferences.NewTabPage.selectedCustomTheme.key:
            Preferences.NTP.ntpCheckDate.value = nil
            backgroundDataSource.startFetching()
        default:
            log.debug("Received a preference change for an unknown key: \(key) on \(type(of: self))")
            break
        }
    }
}

extension BrowserViewController {
    func openReferralLink(url: URL) {
        self.loadQueue.uponQueue(.main) {
            self.openURLInNewTab(url, isPrivileged: false)
        }
    }
    
    func handleNavigationPath(path: NavigationPath) {
        self.loadQueue.uponQueue(.main) {
            NavigationPath.handle(nav: path, with: self)
        }
    }
}

extension BrowserViewController: OnboardingControllerDelegate {
    func onboardingCompleted(_ onboardingController: OnboardingNavigationController) {
        Preferences.General.basicOnboardingCompleted.value = OnboardingState.completed.rawValue
        Preferences.General.basicOnboardingNextOnboardingPrompt.value = nil
        
        if BraveRewards.isAvailable {
            switch onboardingController.onboardingType {
            case .newUser:
                Preferences.General.basicOnboardingProgress.value = OnboardingProgress.rewards.rawValue                
            default:
                break
            }
        } else {
            switch onboardingController.onboardingType {
            case .newUser:
                Preferences.General.basicOnboardingProgress.value = OnboardingProgress.searchEngine.rawValue
            case .existingUserRewardsOff:
                break
            default:
                break
            }
        }
        
        // Present private browsing prompt if necessary when onboarding has been completed
        onboardingController.dismiss(animated: true) {
            self.presentDuckDuckGoCalloutIfNeeded()
        }
    }
    
    func onboardingSkipped(_ onboardingController: OnboardingNavigationController) {
        Preferences.General.basicOnboardingCompleted.value = OnboardingState.skipped.rawValue
        Preferences.General.basicOnboardingNextOnboardingPrompt.value = Date(timeIntervalSinceNow: BrowserViewController.onboardingDaysInterval)
        
        // Present private browsing prompt if necessary when onboarding has been skipped
        onboardingController.dismiss(animated: true) {
            self.presentDuckDuckGoCalloutIfNeeded()
        }
    }
    
    private func presentDuckDuckGoCalloutIfNeeded() {
        if PrivateBrowsingManager.shared.isPrivateBrowsing && self.presentedViewController == nil {
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.4) {
                self.presentDuckDuckGoCallout()
            }
        }
    }
    
    // 60 days until the next time the user sees the onboarding..
    static let onboardingDaysInterval = TimeInterval(60.days)
}

extension BrowserViewController: UNUserNotificationCenterDelegate {
    func userNotificationCenter(_ center: UNUserNotificationCenter, didReceive response: UNNotificationResponse, withCompletionHandler completionHandler: @escaping () -> Void) {
        if response.notification.request.identifier == defaultBrowserNotificationId {
            guard let settingsUrl = URL(string: UIApplication.openSettingsURLString) else {
                log.error("Failed to unwrap iOS settings URL")
                return
            }
            UIApplication.shared.open(settingsUrl)
        }
        completionHandler()
    }
}
