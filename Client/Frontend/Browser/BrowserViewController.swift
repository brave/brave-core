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
import BraveCore
import CoreData
import StoreKit
import SafariServices
import BraveUI
import NetworkExtension
import FeedKit
import SwiftUI
import class Combine.AnyCancellable
import BraveWallet
import BraveVPN

private let log = Logger.browserLogger

private let KVOs: [KVOConstants] = [
  .estimatedProgress,
  .loading,
  .canGoBack,
  .canGoForward,
  .URL,
  .title,
  .hasOnlySecureContent,
  .serverTrust,
]

private struct BrowserViewControllerUX {
  fileprivate static let showHeaderTapAreaHeight: CGFloat = 32
  fileprivate static let showFooterTapAreaHeight: CGFloat = 44
  fileprivate static let bookmarkStarAnimationDuration: Double = 0.5
  fileprivate static let bookmarkStarAnimationOffset: CGFloat = 80
}

protocol BrowserViewControllerDelegate: AnyObject {
  func openInNewTab(_ url: URL, isPrivate: Bool)
}

public class BrowserViewController: UIViewController, BrowserViewControllerDelegate {
  var webViewContainer: UIView!
  var topToolbar: TopToolbarView!
  var tabsBar: TabsBarViewController!
  var clipboardBarDisplayHandler: ClipboardBarDisplayHandler?
  var readerModeBar: ReaderModeBarView?
  var readerModeCache: ReaderModeCache
  var statusBarOverlay: UIView!
  fileprivate(set) var toolbar: BottomToolbarView?
  var searchController: SearchViewController?
  var favoritesController: FavoritesViewController?
  var screenshotHelper: ScreenshotHelper!
  fileprivate var homePanelIsInline = false
  var searchLoader: SearchLoader?
  let alertStackView = UIStackView()  // All content that appears above the footer should be added to this view. (Find In Page/SnackBars)
  var findInPageBar: FindInPageBar?
  var pageZoomBar: UIHostingController<PageZoomView>?
  private var pageZoomListener: NSObjectProtocol?
  private let collapsedURLBarView = CollapsedURLBarView()

  // Single data source used for all favorites vcs
  public let backgroundDataSource = NTPDataSource()
  let feedDataSource = FeedDataSource()

  private var postSetupTasks: [() -> Void] = []
  private var setupTasksCompleted: Bool = false

  private var privateModeCancellable: AnyCancellable?
  var onPendingRequestUpdatedCancellable: AnyCancellable?

  /// Custom Search Engine
  var openSearchEngine: OpenSearchReference?

  lazy var customSearchEngineButton = OpenSearchEngineButton(hidesWhenDisabled: false).then {
    $0.addTarget(self, action: #selector(addCustomSearchEngineForFocusedElement), for: .touchUpInside)
    $0.accessibilityIdentifier = "BrowserViewController.customSearchEngineButton"
    $0.setContentCompressionResistancePriority(.defaultHigh, for: .horizontal)
    $0.setContentHuggingPriority(.defaultHigh, for: .horizontal)
  }

  var customSearchBarButtonItemGroup: UIBarButtonItemGroup?

  // popover rotation handling
  var displayedPopoverController: UIViewController?
  var updateDisplayedPopoverProperties: (() -> Void)?

  let profile: Profile
  let braveCore: BraveCoreMain
  let tabManager: TabManager
  let migration: Migration?
  let bookmarkManager: BookmarkManager

  /// Whether last session was a crash or not
  fileprivate let crashedLastSession: Bool

  // These views wrap the top and bottom toolbars to provide background effects on them
  let header = HeaderContainerView()
  var footer: UIView!
  fileprivate var topTouchArea: UIButton!
  fileprivate let bottomTouchArea = UIButton()

  // These constraints allow to show/hide tabs bar
  var webViewContainerTopOffset: Constraint?

  // Backdrop used for displaying greyed background for private tabs
  var webViewContainerBackdrop: UIView!

  var toolbarVisibilityViewModel = ToolbarVisibilityViewModel(estimatedTransitionDistance: 44)
  private var toolbarLayoutGuide = UILayoutGuide().then {
    $0.identifier = "toolbar-visibility-layout-guide"
  }
  private var toolbarTopConstraint: Constraint?
  private var toolbarBottomConstraint: Constraint?
  private var toolbarVisibilityCancellable: AnyCancellable?

  var keyboardState: KeyboardState?

  var pendingToast: Toast?  // A toast that might be waiting for BVC to appear before displaying
  var downloadToast: DownloadToast?  // A toast that is showing the combined download progress
  var addToPlayListActivityItem: (enabled: Bool, item: PlaylistInfo?)?  // A boolean to determine If AddToListActivity should be added
  var openInPlaylistActivityItem: (enabled: Bool, item: PlaylistInfo?)?  // A boolean to determine if OpenInPlaylistActivity should be shown

  // Tracking navigation items to record history types.
  // TODO: weak references?
  var ignoredNavigation = Set<WKNavigation>()
  var typedNavigation = [URL: VisitType]()
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

  fileprivate var contentBlockListCompiled: Bool = false
  private var cancellables: Set<AnyCancellable> = []

  // Web filters

  let safeBrowsing: SafeBrowsing?

  let rewards: BraveRewards
  var ledgerObserver: LedgerObserver?
  let legacyWallet: BraveLedger?
  var promotionFetchTimer: Timer?
  private var notificationsHandler: AdsNotificationHandler?
  let notificationsPresenter = BraveNotificationsPresenter()
  var publisher: Ledger.PublisherInfo?

  let vpnProductInfo = VPNProductInfo()

  /// Window Protection instance which will be used for controller requires biometric authentication
  public var windowProtection: WindowProtection?

  // Product Notification Related Properties

  /// Boolean which is tracking If a product notification is presented
  /// in order to not to try to present another one over existing popover
  var benchmarkNotificationPresented = false

  /// Used to determine when to present benchmark pop-overs
  /// Current session ad count is compared with live ad count
  /// So user will not be introduced with a pop-over directly
  let benchmarkCurrentSessionAdCount = BraveGlobalShieldStats.shared.adblock + BraveGlobalShieldStats.shared.trackingProtection

  /// Navigation Helper used for Brave Widgets
  private(set) lazy var navigationHelper = BrowserNavigationHelper(self)

  /// Boolean tracking  if Tab Tray is active on the screen
  /// Used to determine If pop-over should be presented
  var isTabTrayActive = false

  /// Data Source object used to determine blocking stats
  var benchmarkBlockingDataSource: BlockingSummaryDataSource?

  /// Boolean which is tracking If a full screen callout or onboarding is presented
  /// in order to not to try to present another callout  over existing one
  var isOnboardingOrFullScreenCalloutPresented = false

  private(set) var widgetBookmarksFRC: NSFetchedResultsController<Favorite>?
  var widgetFaviconFetchers: [FaviconFetcher] = []
  let deviceCheckClient: DeviceCheckClient?

  /// The currently open WalletStore
  weak var walletStore: WalletStore?

  public init(
    profile: Profile,
    diskImageStore: DiskImageStore?,
    braveCore: BraveCoreMain,
    migration: Migration?,
    crashedLastSession: Bool,
    safeBrowsingManager: SafeBrowsing? = SafeBrowsing()
  ) {
    self.profile = profile
    self.braveCore = braveCore
    self.bookmarkManager = BookmarkManager(bookmarksAPI: braveCore.bookmarksAPI)
    self.migration = migration
    self.crashedLastSession = crashedLastSession
    self.safeBrowsing = safeBrowsingManager

    let configuration: BraveRewards.Configuration
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

    let buildChannel = Ads.BuildChannel().then {
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

    // Initialize Rewards
    self.rewards = BraveRewards(configuration: configuration, buildChannel: buildChannel)

    // Initialize TabManager
    self.tabManager = TabManager(
      prefs: profile.prefs,
      imageStore: diskImageStore,
      rewards: rewards)

    // Setup ReaderMode Cache
    self.readerModeCache = ReaderMode.cache(for: tabManager.selectedTab)

    if !BraveRewards.isAvailable {
      // Disable rewards services in case previous user already enabled
      // rewards in previous build
      rewards.isEnabled = false
    } else {
      if rewards.isEnabled && !Preferences.Rewards.rewardsToggledOnce.value {
        Preferences.Rewards.rewardsToggledOnce.value = true
      }
    }

    self.deviceCheckClient = DeviceCheckClient(environment: configuration.ledgerEnvironment)

    if Locale.current.regionCode == "JP" {
      benchmarkBlockingDataSource = BlockingSummaryDataSource()
    }

    super.init(nibName: nil, bundle: nil)
    didInit()

    rewards.ledgerServiceDidStart = { [weak self] _ in
      self?.setupLedger()
    }

    rewards.ads.captchaHandler = self
    let shouldStartAds = rewards.ads.isEnabled || Preferences.BraveNews.isEnabled.value
    if shouldStartAds {
      // Only start ledger service automatically if ads is enabled
      if rewards.isEnabled {
        rewards.startLedgerService {
          self.legacyWallet?.initializeLedgerService(nil)
        }
      } else {
        rewards.ads.initialize { _ in }
      }
    }

    feedDataSource.rewards = rewards
  }

  static func legacyWallet(for config: BraveRewards.Configuration) -> BraveLedger? {
    let fm = FileManager.default
    let stateStorage = config.storageURL
    let legacyLedger = stateStorage.appendingPathComponent("legacy_ledger")

    // Check if we've already migrated the users wallet to the `legacy_rewards` folder
    if fm.fileExists(atPath: legacyLedger.path) {
      BraveLedger.environment = config.ledgerEnvironment
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
      BraveLedger.environment = config.ledgerEnvironment
      return BraveLedger(stateStoragePath: legacyLedger.path)
    } catch {
      log.error("Failed to migrate legacy wallet into a new folder: \(error)")
      return nil
    }
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override public var supportedInterfaceOrientations: UIInterfaceOrientationMask {
    if UIDevice.current.userInterfaceIdiom == .phone {
      return .allButUpsideDown
    } else {
      return .all
    }
  }

  override public func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
    super.viewWillTransition(to: size, with: coordinator)

    dismissVisibleMenus()

    coordinator.animate(
      alongsideTransition: { context in
        if let popover = self.displayedPopoverController {
          self.updateDisplayedPopoverProperties?()
          self.present(popover, animated: true, completion: nil)
        }
      },
      completion: { _ in
        if let tab = self.tabManager.selectedTab {
          WindowRenderHelperScript.executeScript(for: tab)
        }
      })
  }

  override public func didReceiveMemoryWarning() {
    super.didReceiveMemoryWarning()
    ScriptFactory.shared.clearCaches()

    for tab in tabManager.tabsForCurrentMode where tab.id != tabManager.selectedTab?.id {
      tab.newTabPageViewController = nil
    }
  }

  private var rewardsEnabledObserveration: NSKeyValueObservation?

  fileprivate func didInit() {
    updateApplicationShortcuts()
    screenshotHelper = ScreenshotHelper(tabManager: tabManager)
    tabManager.addDelegate(self)
    tabManager.addNavigationDelegate(self)
    tabManager.makeWalletProvider = { [weak self] tab in
      guard let self = self,
            let provider = self.braveCore.ethereumProvider(with: tab, isPrivateBrowsing: tab.isPrivate) else {
        return nil
      }
      return (provider, js: self.braveCore.providerScript(for: .eth))
    }
    downloadQueue.delegate = self

    // Observe some user preferences
    Preferences.Privacy.privateBrowsingOnly.observe(from: self)
    Preferences.General.tabBarVisibility.observe(from: self)
    Preferences.General.alwaysRequestDesktopSite.observe(from: self)
    Preferences.General.enablePullToRefresh.observe(from: self)
    Preferences.General.mediaAutoBackgrounding.observe(from: self)
    Preferences.General.defaultPageZoomLevel.observe(from: self)
    Preferences.Shields.allShields.forEach { $0.observe(from: self) }
    Preferences.Privacy.blockAllCookies.observe(from: self)
    Preferences.Rewards.hideRewardsIcon.observe(from: self)
    Preferences.Rewards.rewardsToggledOnce.observe(from: self)
    Preferences.Playlist.enablePlaylistMenuBadge.observe(from: self)
    Preferences.Playlist.enablePlaylistURLBarButton.observe(from: self)
    
    pageZoomListener = NotificationCenter.default.addObserver(forName: PageZoomView.notificationName, object: nil, queue: .main) { [weak self] _ in
      self?.tabManager.allTabs.forEach({
        guard let url = $0.webView?.url else { return }
        let zoomLevel = PrivateBrowsingManager.shared.isPrivateBrowsing ? 1.0 : Domain.getPersistedDomain(for: url)?.zoom_level?.doubleValue ?? Preferences.General.defaultPageZoomLevel.value
        $0.webView?.setValue(zoomLevel, forKey: PageZoomView.propertyName)
      })
    }
    
    rewardsEnabledObserveration = rewards.observe(\.isEnabled, options: [.new]) { [weak self] _, _ in
      guard let self = self else { return }
      self.updateRewardsButtonState()
      self.setupAdsNotificationHandler()
    }
    Preferences.NewTabPage.selectedCustomTheme.observe(from: self)
    Preferences.Playlist.webMediaSourceCompatibility.observe(from: self)
    Preferences.PrivacyReports.captureShieldsData.observe(from: self)
    Preferences.PrivacyReports.captureVPNAlerts.observe(from: self)
    Preferences.Wallet.defaultWallet.observe(from: self)
    
    // Lists need to be compiled before attempting tab restoration
    
    var contentBlockListTask: AnyCancellable?
    contentBlockListTask = ContentBlockerHelper.compileBundledLists()
      .receive(on: DispatchQueue.main)
      .sink { [weak self] res in
        if case .failure(let error) = res {
          log.error("Content Blocker failed to compile bundled lists: \(error)")
        }
          
        contentBlockListTask = nil
        
        self?.contentBlockListCompiled = true
        self?.setupTabs()
    } receiveValue: { _ in
      log.debug("Content Blocker successfully compiled bundled lists")
    }

    if rewards.ledger != nil {
      // Ledger was started immediately due to user having ads enabled
      setupLedger()
    }

    Preferences.NewTabPage.attemptToShowClaimRewardsNotification.value = true

    backgroundDataSource.initializeFavorites = { sites in
      DispatchQueue.main.async {
        defer { Preferences.NewTabPage.preloadedFavoritiesInitialized.value = true }

        if Preferences.NewTabPage.preloadedFavoritiesInitialized.value
          || Favorite.hasFavorites {
          return
        }

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
              let title = $0.displayTitle
            else {
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

    // Setup Widgets FRC
    widgetBookmarksFRC = Favorite.frc()
    widgetBookmarksFRC?.fetchRequest.fetchLimit = 16
    widgetBookmarksFRC?.delegate = self
    try? widgetBookmarksFRC?.performFetch()

    updateWidgetFavoritesData()
  }

  private func setupAdsNotificationHandler() {
    notificationsHandler = AdsNotificationHandler(ads: rewards.ads,
                                                  presentingController: self,
                                                  notificationsPresenter: notificationsPresenter)
    notificationsHandler?.canShowNotifications = { [weak self] in
      guard let self = self else { return false }
      return !PrivateBrowsingManager.shared.isPrivateBrowsing && !self.topToolbar.inOverlayMode
    }
    notificationsHandler?.actionOccured = { [weak self] ad, action in
      guard let self = self, let ad = ad else { return }
      if action == .opened {
        var url = URL(string: ad.targetURL)
        if url == nil,
           let percentEncodedURLString =
            ad.targetURL.addingPercentEncoding(withAllowedCharacters: .urlQueryAllowed) {
          // Try to percent-encode the string and try that
          url = URL(string: percentEncodedURLString)
        }
        guard let targetURL = url else {
          assertionFailure("Invalid target URL for creative instance id: \(ad.creativeInstanceID)")
          return
        }
        let request = URLRequest(url: targetURL)
        self.tabManager.addTabAndSelect(request, isPrivate: PrivateBrowsingManager.shared.isPrivateBrowsing)
      }
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

    topToolbar.setShowToolbar(!showToolbar)
    toolbar?.removeFromSuperview()
    toolbar?.tabToolbarDelegate = nil
    toolbar = nil

    if showToolbar {
      toolbar = BottomToolbarView()
      toolbar?.setSearchButtonState(url: tabManager.selectedTab?.url)
      footer.addSubview(toolbar!)
      toolbar?.tabToolbarDelegate = self
      toolbar?.menuButton.setBadges(Array(topToolbar.menuButton.badges.keys))
    }
    updateToolbarUsingTabManager(tabManager)

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
  
  private func updateToolbarSecureContentState(_ secureContentState: TabSecureContentState) {
    topToolbar.secureContentState = secureContentState
    collapsedURLBarView.secureContentState = secureContentState
  }
  
  private func updateToolbarCurrentURL(_ currentURL: URL?) {
    topToolbar.currentURL = currentURL
    collapsedURLBarView.currentURL = currentURL
  }

  override public func willTransition(to newCollection: UITraitCollection, with coordinator: UIViewControllerTransitionCoordinator) {
    super.willTransition(to: newCollection, with: coordinator)

    // During split screen launching on iPad, this callback gets fired before viewDidLoad gets a chance to
    // set things up. Make sure to only update the toolbar state if the view is ready for it.
    if isViewLoaded {
      updateToolbarStateForTraitCollection(newCollection, withTransitionCoordinator: coordinator)
    }

    displayedPopoverController?.dismiss(animated: true, completion: nil)
    coordinator.animate(
      alongsideTransition: { context in
        if self.isViewLoaded {
          self.statusBarOverlay.backgroundColor = self.topToolbar.backgroundColor
          self.setNeedsStatusBarAppearanceUpdate()
        }
      },
      completion: { _ in
        if let tab = self.tabManager.selectedTab {
          WindowRenderHelperScript.executeScript(for: tab)
        }
      })
  }

  override public func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)

    if UITraitCollection.current.userInterfaceStyle != previousTraitCollection?.userInterfaceStyle {
      // Reload UI
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
    toolbarVisibilityViewModel.toolbarState = .expanded
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
    UIView.animate(
      withDuration: 0.2, delay: 0, options: UIView.AnimationOptions(),
      animations: {
        self.webViewContainer.alpha = 1
        self.topToolbar.locationContainer.alpha = 1
        self.presentedViewController?.popoverPresentationController?.containerView?.alpha = 1
        self.presentedViewController?.view.alpha = 1
        self.view.backgroundColor = .clear
      },
      completion: { _ in
        self.webViewContainerBackdrop.alpha = 0
      })

    // Re-show toolbar which might have been hidden during scrolling (prior to app moving into the background)
    toolbarVisibilityViewModel.toolbarState = .expanded
  }

  override public func viewDidLoad() {
    super.viewDidLoad()
    NotificationCenter.default.do {
      $0.addObserver(
        self, selector: #selector(appWillResignActiveNotification),
        name: UIApplication.willResignActiveNotification, object: nil)
      $0.addObserver(
        self, selector: #selector(appDidBecomeActiveNotification),
        name: UIApplication.didBecomeActiveNotification, object: nil)
      $0.addObserver(
        self, selector: #selector(appDidEnterBackgroundNotification),
        name: UIApplication.didEnterBackgroundNotification, object: nil)
      $0.addObserver(
        self, selector: #selector(resetNTPNotification),
        name: .adsOrRewardsToggledInSettings, object: nil)
      $0.addObserver(
        self, selector: #selector(vpnConfigChanged),
        name: .NEVPNConfigurationChange, object: nil)
      $0.addObserver(
        self, selector: #selector(updateShieldNotifications),
        name: NSNotification.Name(rawValue: BraveGlobalShieldStats.didUpdateNotification), object: nil)
    }

    view.backgroundColor = .braveBackground
    KeyboardHelper.defaultHelper.addDelegate(self)
    UNUserNotificationCenter.current().delegate = self

    view.addLayoutGuide(pageOverlayLayoutGuide)

    webViewContainerBackdrop = UIView()
    webViewContainerBackdrop.backgroundColor = .braveBackground
    webViewContainerBackdrop.alpha = 0
    view.addSubview(webViewContainerBackdrop)

    webViewContainer = UIView()
    view.addSubview(webViewContainer)

    // Temporary work around for covering the non-clipped web view content
    statusBarOverlay = UIView()
    statusBarOverlay.backgroundColor = Preferences.General.nightModeEnabled.value ? .nightModeBackground : .urlBarBackground
    view.addSubview(statusBarOverlay)

    topTouchArea = UIButton()
    topTouchArea.isAccessibilityElement = false
    topTouchArea.addTarget(self, action: #selector(tappedTopArea), for: .touchUpInside)
    view.addSubview(topTouchArea)

    bottomTouchArea.isAccessibilityElement = false
    bottomTouchArea.addTarget(self, action: #selector(tappedTopArea), for: .touchUpInside)
    view.addSubview(bottomTouchArea)
    
    // Setup the URL bar, wrapped in a view to get transparency effect
    topToolbar = TopToolbarView()
    topToolbar.translatesAutoresizingMaskIntoConstraints = false
    topToolbar.delegate = self
    topToolbar.tabToolbarDelegate = self

    let toolBarInteraction = UIContextMenuInteraction(delegate: self)
    topToolbar.locationView.addInteraction(toolBarInteraction)

    header.expandedBarStackView.addArrangedSubview(topToolbar)

    tabsBar = TabsBarViewController(tabManager: tabManager)
    tabsBar.delegate = self
    header.expandedBarStackView.addArrangedSubview(tabsBar.view)

    header.collapsedBarContainerView.addSubview(collapsedURLBarView)
    header.collapsedBarContainerView.addTarget(self, action: #selector(tappedTopArea), for: .touchUpInside)
    
    view.addSubview(header)

    addChild(tabsBar)
    tabsBar.didMove(toParent: self)

    view.addSubview(alertStackView)
    footer = UIView()
    footer.translatesAutoresizingMaskIntoConstraints = false
    view.addSubview(footer)
    alertStackView.axis = .vertical
    alertStackView.alignment = .center

    clipboardBarDisplayHandler = ClipboardBarDisplayHandler(tabManager: tabManager)
    clipboardBarDisplayHandler?.delegate = self
    
    view.addLayoutGuide(toolbarLayoutGuide)
    toolbarLayoutGuide.snp.makeConstraints {
      self.toolbarTopConstraint = $0.top.equalTo(view.safeArea.top).constraint
      self.toolbarBottomConstraint = $0.bottom.equalTo(view).constraint
      $0.leading.trailing.equalTo(view)
    }

    self.updateToolbarStateForTraitCollection(self.traitCollection)

    setupConstraints()

    updateRewardsButtonState()

    // Setup UIDropInteraction to handle dragging and dropping
    // links into the view from other apps.
    let dropInteraction = UIDropInteraction(delegate: self)
    view.addInteraction(dropInteraction)

    if AppConstants.buildChannel.isPublic && AppReview.shouldRequestReview() {
      // Request Review when the main-queue is free or on the next cycle.
      DispatchQueue.main.async {
        guard let windowScene = self.currentScene else { return }
        SKStoreReviewController.requestReview(in: windowScene)
      }
    }

    LegacyBookmarksHelper.restore_1_12_Bookmarks() {
      log.info("Bookmarks from old database were successfully restored")
    }

    // Adding a small delay before fetching gives more reliability to it,
    // epsecially when you are connected to a VPN.
    DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
      self.vpnProductInfo.load()
      BraveVPN.initialize()
    }

    showWalletTransferExpiryPanelIfNeeded()

    /// Perform migration to brave-core sync objects
    if !Migration.isChromiumMigrationCompleted,
      !Preferences.Chromium.syncV2PasswordMigrationStarted.value {
      Preferences.Chromium.syncV2ObjectMigrationCount.value = 0
    }

    doSyncMigration()

    if #available(iOS 14, *), !Preferences.DefaultBrowserIntro.defaultBrowserNotificationScheduled.value {
      scheduleDefaultBrowserNotification()
    }

    privateModeCancellable = PrivateBrowsingManager.shared
      .$isPrivateBrowsing
      .removeDuplicates()
      .sink(receiveValue: { [weak self] isPrivateBrowsing in
        if isPrivateBrowsing {
          self?.statusBarOverlay.backgroundColor = .privateModeBackground
        } else {
          self?.statusBarOverlay.backgroundColor =
          Preferences.General.nightModeEnabled.value ? .nightModeBackground : .urlBarBackground
        }
      })
    
    Preferences.General.nightModeEnabled.objectWillChange
      .receive(on: RunLoop.main)
      .sink { [weak self] _ in
        if PrivateBrowsingManager.shared.isPrivateBrowsing {
          self?.statusBarOverlay.backgroundColor = .privateModeBackground
        } else {
          self?.statusBarOverlay.backgroundColor =
          Preferences.General.nightModeEnabled.value ? .nightModeBackground : .urlBarBackground
        }
      }
      .store(in: &cancellables)
  }

  public static let defaultBrowserNotificationId = "defaultBrowserNotification"

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

      center.getPendingNotificationRequests { requests in
        if requests.contains(where: { $0.identifier == Self.defaultBrowserNotificationId }) {
          // Already has one scheduled no need to schedule again.
          return
        }

        let content = UNMutableNotificationContent().then {
          $0.title = Strings.DefaultBrowserCallout.notificationTitle
          $0.body = String(format: Strings.DefaultBrowserCallout.notificationBody, String(ProcessInfo().operatingSystemVersion.majorVersion))
        }

        let timeToShow = AppConstants.buildChannel.isPublic ? 2.hours : 2.minutes
        let timeTrigger = UNTimeIntervalNotificationTrigger(timeInterval: timeToShow, repeats: false)

        let request = UNNotificationRequest(
          identifier: Self.defaultBrowserNotificationId,
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
  
  private func executeAfterSetup(_ block: @escaping () -> Void) {
    if setupTasksCompleted {
      block()
    } else {
      postSetupTasks.append(block)
    }
  }

  fileprivate func setupTabs() {
    assert(contentBlockListCompiled, "Tabs should not be set up until after blocker lists are compiled")
    let isPrivate = Preferences.Privacy.privateBrowsingOnly.value
    let noTabsAdded = self.tabManager.tabsForCurrentMode.isEmpty
    
    var tabToSelect: Tab?
    
    if noTabsAdded {
      // Two scenarios if there are no tabs in tabmanager:
      // 1. We have not restored tabs yet, attempt to restore or make a new tab if there is nothing.
      // 2. We are in private browsing mode and need to add a new private tab.
      tabToSelect = isPrivate ? self.tabManager.addTab(isPrivate: true) : self.tabManager.restoreAllTabs
    } else {
      tabToSelect = self.tabManager.tabsForCurrentMode.last
    }
    self.tabManager.selectTab(tabToSelect)
    
    if !setupTasksCompleted {
      for task in postSetupTasks {
        DispatchQueue.main.async {
          task()
        }
      }
      setupTasksCompleted = true
    }
  }

  fileprivate func setupConstraints() {
    header.snp.makeConstraints { make in
      make.top.equalTo(toolbarLayoutGuide)
      make.left.right.equalTo(self.view)
    }

    collapsedURLBarView.snp.makeConstraints {
      $0.edges.equalToSuperview()
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

    bottomTouchArea.snp.makeConstraints { make in
      make.bottom.left.right.equalTo(self.view)
      make.height.equalTo(BrowserViewControllerUX.showFooterTapAreaHeight)
    }
  }

  override public func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    statusBarOverlay.snp.remakeConstraints { make in
      make.top.left.right.equalTo(self.view)
      make.bottom.equalTo(view.safeArea.top)
    }
    toolbarVisibilityViewModel.transitionDistance = header.expandedBarStackView.bounds.height - header.collapsedBarContainerView.bounds.height
    // Since the height of the WKWebView changes while collapsing we need to use a stable value to determine
    // if the toolbars can collapse
    toolbarVisibilityViewModel.minimumCollapsableContentHeight = webViewContainer.bounds.height + header.bounds.height + footer.bounds.height + view.safeAreaInsets.top + view.safeAreaInsets.bottom
  }

  override public var canBecomeFirstResponder: Bool {
    return true
  }

  override public func becomeFirstResponder() -> Bool {
    // Make the web view the first responder so that it can show the selection menu.
    return tabManager.selectedTab?.webView?.becomeFirstResponder() ?? false
  }

  override public func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)

    checkCrashRestoration()

    updateToolbarUsingTabManager(tabManager)
    clipboardBarDisplayHandler?.checkIfShouldDisplayBar()

    if let tabId = tabManager.selectedTab?.rewardsId, rewards.ledger?.selectedTabId == 0 {
      rewards.ledger?.selectedTabId = tabId
    }
  }

  fileprivate lazy var checkCrashRestoration: () -> Void = {
    if crashedLastSession {
      showRestoreTabsAlert()
    } else {
      if self.contentBlockListCompiled {
        setupTabs()
      }
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
        if self.contentBlockListCompiled {
          self.setupTabs()
        }
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

  override public func viewDidAppear(_ animated: Bool) {
    // Passcode Migration has highest priority, it should be presented over everything else
    presentPassCodeMigration()

    // Present Onboarding to new users, existing users will not see the onboarding
    presentOnboardingIntro()

    // Full Screen Callout Presentation
    // Priority: VPN - Default Browser - Rewards - Sync
    // TODO: Remove the dispatch after with a proper fix and fix calling present functions before super.viewDidAppear
    DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
      self.presentVPNAlertCallout()
      self.presentDefaultBrowserScreenCallout()
      self.presentBraveRewardsScreenCallout()
      self.presentSyncAlertCallout()
    }

    screenshotHelper.viewIsVisible = true
    screenshotHelper.takePendingScreenshots(tabManager.allTabs)

    super.viewDidAppear(animated)

    if let toast = self.pendingToast {
      self.pendingToast = nil
      show(toast: toast, afterWaiting: ButtonToastUX.toastDelay)
    }
    showQueuedAlertIfAvailable()
  }

  /// Whether or not to show the playlist onboarding callout this session
  var shouldShowPlaylistOnboardingThisSession = true

  fileprivate func showQueuedAlertIfAvailable() {
    if let queuedAlertInfo = tabManager.selectedTab?.dequeueJavascriptAlertPrompt() {
      let alertController = queuedAlertInfo.alertController()
      alertController.delegate = self
      present(alertController, animated: true, completion: nil)
    }
  }

  override public func viewWillDisappear(_ animated: Bool) {
    screenshotHelper.viewIsVisible = false
    super.viewWillDisappear(animated)

    rewards.ledger?.selectedTabId = 0
  }

  override public func viewDidDisappear(_ animated: Bool) {
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
  let pageOverlayLayoutGuide = UILayoutGuide()

  override public func updateViewConstraints() {
    webViewContainer.snp.remakeConstraints { make in
      make.left.right.equalTo(self.view)

      webViewContainerTopOffset = make.top.equalTo(self.readerModeBar?.snp.bottom ?? self.header.snp.bottom).constraint

      let findInPageHeight = (findInPageBar == nil) ? 0 : UIConstants.toolbarHeight
      make.bottom.equalTo(self.footer.snp.top).offset(-findInPageHeight)
    }

    footer.snp.remakeConstraints { make in
      make.bottom.equalTo(toolbarLayoutGuide)
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

  fileprivate func showNewTabPageController() {
    guard let selectedTab = tabManager.selectedTab else { return }
    if selectedTab.newTabPageViewController == nil {
      let ntpController = NewTabPageViewController(
        tab: selectedTab,
        profile: profile,
        dataSource: backgroundDataSource,
        feedDataSource: feedDataSource,
        rewards: rewards)
      /// Donate NewTabPage Activity For Custom Suggestions
      let newTabPageActivity =
        ActivityShortcutManager.shared.createShortcutActivity(type: selectedTab.isPrivate ? .newPrivateTab : .newTab)

      ntpController.delegate = self
      ntpController.userActivity = newTabPageActivity

      newTabPageActivity.becomeCurrent()
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
      UIView.animate(
        withDuration: 0.2,
        animations: {
          ntpController.view.alpha = 1
        },
        completion: { finished in
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

    UIView.animate(
      withDuration: 0.2,
      animations: {
        controller.view.alpha = 0.0
      },
      completion: { finished in
        controller.willMove(toParent: nil)
        controller.view.removeFromSuperview()
        controller.removeFromParent()
        self.webViewContainer.accessibilityElementsHidden = false
        UIAccessibility.post(notification: .screenChanged, argument: nil)

        // Refresh the reading view toolbar since the article record may have changed
        if let tab = self.tabManager.selectedTab,
          let readerMode = tab.getContentScript(name: ReaderMode.name()) as? ReaderMode,
          readerMode.state == .active,
          isReaderModeURL {
          self.showReaderModeBar(animated: false)
          self.updatePlaylistURLBar(tab: tab, state: tab.playlistItemState, item: tab.playlistItem)
        }
      })
  }

  /// Shows a vpn screen based on vpn state.
  public func presentCorrespondingVPNViewController() {
    guard let vc = BraveVPN.vpnState.enableVPNDestinationVC else { return }
    let nav = SettingsNavigationController(rootViewController: vc)
    nav.navigationBar.topItem?.leftBarButtonItem =
      .init(barButtonSystemItem: .cancel, target: nav, action: #selector(nav.done))
    let idiom = UIDevice.current.userInterfaceIdiom

    UIDevice.current.forcePortraitIfIphone(for: UIApplication.shared)

    nav.modalPresentationStyle = idiom == .phone ? .pageSheet : .formSheet
    present(nav, animated: true)
  }

  func updateInContentHomePanel(_ url: URL?) {
    let isAboutHomeURL = { () -> Bool in
      if let url = url {
        return InternalURL(url)?.isAboutHomeURL == true
      }
      return false
    }()

    if !topToolbar.inOverlayMode {
      guard let url = url else {
        hideActiveNewTabPageController()
        return
      }

      if isAboutHomeURL {
        showNewTabPageController()
      } else if !url.absoluteString.hasPrefix("\(InternalURL.baseUrl)/\(SessionRestoreHandler.path)") {
        hideActiveNewTabPageController(url.isReaderModeURL)
      }
    } else if isAboutHomeURL {
      showNewTabPageController()
    }
  }

  func updateTabsBarVisibility() {
    defer {
      topToolbar?.line.isHidden = tabsBar?.view.isHidden == false
    }

    if tabManager.selectedTab == nil {
      tabsBar?.view.isHidden = true
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

    let isShowing = tabsBar?.view.isHidden == false
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
    let newTabItem = UIMutableApplicationShortcutItem(
      type: "\(Bundle.main.bundleIdentifier ?? "").NewTab",
      localizedTitle: Strings.quickActionNewTab,
      localizedSubtitle: nil,
      icon: UIApplicationShortcutIcon(templateImageName: "quick_action_new_tab"),
      userInfo: [:])
    
    let privateTabItem = UIMutableApplicationShortcutItem(
      type: "\(Bundle.main.bundleIdentifier ?? "").NewPrivateTab",
      localizedTitle: Strings.quickActionNewPrivateTab,
      localizedSubtitle: nil,
      icon: UIApplicationShortcutIcon(templateImageName: "quick_action_new_private_tab"),
      userInfo: [:])
    
    let scanQRCodeItem = UIMutableApplicationShortcutItem(
      type: "\(Bundle.main.bundleIdentifier ?? "").ScanQRCode",
      localizedTitle: Strings.scanQRCodeViewTitle,
      localizedSubtitle: nil,
      icon: UIApplicationShortcutIcon(templateImageName: "recent-search-qrcode"),
      userInfo: [:])
    
    UIApplication.shared.shortcutItems = Preferences.Privacy.privateBrowsingOnly.value ? [privateTabItem, scanQRCodeItem] : [newTabItem, privateTabItem, scanQRCodeItem]
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
      if visitType == .bookmark, let webView = tab.webView, let code = url.bookmarkletCodeComponent {
        webView.evaluateSafeJavaScript(
          functionName: code,
          contentWorld: .bookmarkletSandbox,
          asFunction: false
        ) { _, error in
          if let error = error {
            log.error(error)
          }
        }
      }
    } else {
      updateToolbarCurrentURL(url)
      topToolbar.leaveOverlayMode()

      guard let tab = tabManager.selectedTab else {
        return
      }

      tab.loadRequest(URLRequest(url: url))

      recordNavigationInTab(url, visitType: visitType)
      updateWebViewPageZoom(tab: tab)
    }
  }

  override public func accessibilityPerformEscape() -> Bool {
    if topToolbar.inOverlayMode {
      topToolbar.didClickCancel()
      return true
    } else if let selectedTab = tabManager.selectedTab, selectedTab.canGoBack {
      selectedTab.goBack()
      return true
    }
    return false
  }

  // This variable is used to keep track of current page. It is used to detect internal site navigation
  // to report internal page load to Rewards lib
  var rewardsXHRLoadURL: URL?

  override public func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey: Any]?, context: UnsafeMutableRawPointer?) {

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
        // `WKWebView.estimatedProgress` is a `Double` type so it must be casted as such
        let progress = change?[.newKey] as? Double
      else { break }
      if let url = webView.url, !InternalURL.isValid(url: url) {
        topToolbar.updateProgressBar(Float(progress))
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

      // Rewards reporting
      if let url = change?[.newKey] as? URL, !url.isLocal {
        // Notify Rewards of new page load.
        if let rewardsURL = rewardsXHRLoadURL,
          url.host == rewardsURL.host {
          tabManager.selectedTab?.reportPageNavigation(to: rewards)
          // Not passing redirection chain here, in page navigation should not use them.
          tabManager.selectedTab?.reportPageLoad(to: rewards, redirectionURLs: [])
        }
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

      tab.userScriptManager?.isPaymentRequestEnabled = webView.hasOnlySecureContent

      if tab.secureContentState == .secure && !webView.hasOnlySecureContent {
        tab.secureContentState = .insecure
      }

      if tabManager.selectedTab === tab {
        updateToolbarSecureContentState(tab.secureContentState)
      }
    case .serverTrust:
      guard let tab = tabManager[webView] else {
        break
      }

      tab.secureContentState = .unknown

      guard let serverTrust = tab.webView?.serverTrust else {
        if let url = tab.webView?.url ?? tab.url {
          if InternalURL.isValid(url: url),
            let internalUrl = InternalURL(url),
            (internalUrl.isAboutURL || internalUrl.isAboutHomeURL) {

            tab.secureContentState = .localHost
            if tabManager.selectedTab === tab {
              updateToolbarSecureContentState(.localHost)
            }
            break
          }

          if InternalURL.isValid(url: url),
            let internalUrl = InternalURL(url),
            internalUrl.isErrorPage {

            if ErrorPageHelper.certificateError(for: url) != 0 {
              tab.secureContentState = .insecure
              if tabManager.selectedTab === tab {
                updateToolbarSecureContentState(.insecure)
              }
              break
            }
          }

          if url.isReaderModeURL || InternalURL.isValid(url: url) {
            tab.secureContentState = .unknown
            if tabManager.selectedTab === tab {
              updateToolbarSecureContentState(.unknown)
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
          updateToolbarSecureContentState(tab.secureContentState)
        }
        break
      }

      let policies = [
        SecPolicyCreateBasicX509(),
        SecPolicyCreateSSL(true, tab.webView?.url?.host as CFString?),
      ]

      SecTrustSetPolicies(serverTrust, policies as CFTypeRef)
      let queue = DispatchQueue.global()
      queue.async {
        SecTrustEvaluateAsyncWithError(serverTrust, queue) { _, secTrustResult, _ in
          DispatchQueue.main.async {
            if secTrustResult {
              tab.secureContentState = .secure
            } else {
              tab.secureContentState = .insecure
            }
            self.updateURLBar()
          }
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
      webView.evaluateSafeJavaScript(functionName: "__firefox__.NoImageMode.setEnabled", args: ["true"], contentWorld: .defaultClient)
    }
  }

  func updateUIForReaderHomeStateForTab(_ tab: Tab) {
    updateURLBar()
    toolbarVisibilityViewModel.toolbarState = .expanded

    if let url = tab.url {
      if url.isReaderModeURL {
        showReaderModeBar(animated: false)
        NotificationCenter.default.addObserver(self, selector: #selector(dynamicFontChanged), name: .dynamicFontChanged, object: nil)
      } else {
        hideReaderModeBar(animated: false)
        NotificationCenter.default.removeObserver(self, name: .dynamicFontChanged, object: nil)
      }

      updateInContentHomePanel(url as URL)
      updatePlaylistURLBar(tab: tab, state: tab.playlistItemState, item: tab.playlistItem)
    }
  }

  /// Updates the URL bar security, text and button states.
  fileprivate func updateURLBar() {
    guard let tab = tabManager.selectedTab else { return }

    updateRewardsButtonState()

    DispatchQueue.main.async {
      if let item = tab.playlistItem {
        if PlaylistItem.itemExists(item) {
          self.updatePlaylistURLBar(tab: tab, state: .existingItem, item: item)
        } else {
          self.updatePlaylistURLBar(tab: tab, state: .newItem, item: item)
        }
      } else {
        self.updatePlaylistURLBar(tab: tab, state: .none, item: nil)
      }
    }

    updateToolbarCurrentURL(tab.url?.displayURL)
    if tabManager.selectedTab === tab {
      updateToolbarSecureContentState(tab.secureContentState)
    }

    let isPage = tab.url?.displayURL?.isWebPage() ?? false
    navigationToolbar.updatePageStatus(isPage)
    updateWebViewPageZoom(tab: tab)
  }

  public func switchToTabForURLOrOpen(_ url: URL, isPrivate: Bool = false, isPrivileged: Bool, isExternal: Bool = false) {
    if !isExternal {
      popToBVC()
    }

    if let tab = tabManager.getTabForURL(url) {
      tabManager.selectTab(tab)
    } else {
      openURLInNewTab(url, isPrivate: isPrivate, isPrivileged: isPrivileged)
    }
  }
  
  func switchToTabOrOpen(id: String?, url: URL) {
    popToBVC()
    
    if let tabID = id, let tab = tabManager.getTabForID(tabID) {
      tabManager.selectTab(tab)
    } else if let tab = tabManager.getTabForURL(url) {
      tabManager.selectTab(tab)
    } else {
      openURLInNewTab(url, isPrivate: PrivateBrowsingManager.shared.isPrivateBrowsing, isPrivileged: false)
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

    _ = tabManager.addTabAndSelect(request, isPrivate: isPrivate)
  }

  public func openBlankNewTab(attemptLocationFieldFocus: Bool, isPrivate: Bool, searchFor searchText: String? = nil, isExternal: Bool = false) {
    if !isExternal {
      popToBVC()
    }

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
          self.topToolbar.submitLocation(text)
        }
      }
    }
  }

  func clearHistoryAndOpenNewTab() {
    // When PB Only mode is enabled
    // All private tabs closed and a new private tab is created
    if Preferences.Privacy.privateBrowsingOnly.value {
      tabManager.removeAll()
      openBlankNewTab(attemptLocationFieldFocus: true, isPrivate: true, isExternal: true)
      popToBVC()
    } else {
      braveCore.historyAPI.deleteAll { [weak self] in
        guard let self = self else { return }

        self.tabManager.clearTabHistory() {
          self.openBlankNewTab(attemptLocationFieldFocus: true, isPrivate: false, isExternal: true)
          self.popToBVC()
        }
      }
    }
  }

  func openInsideSettingsNavigation(with viewController: UIViewController) {
    let settingsNavigationController = SettingsNavigationController(rootViewController: viewController)
    settingsNavigationController.isModalInPresentation = false
    settingsNavigationController.modalPresentationStyle =
      UIDevice.current.userInterfaceIdiom == .phone ? .pageSheet : .formSheet
    settingsNavigationController.navigationBar.topItem?.leftBarButtonItem =
      UIBarButtonItem(barButtonSystemItem: .done, target: settingsNavigationController, action: #selector(settingsNavigationController.done))

    // All menu views should be opened in portrait on iPhones.
    UIDevice.current.forcePortraitIfIphone(for: UIApplication.shared)

    present(settingsNavigationController, animated: true)
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

  func makeShareActivities(for url: URL, tab: Tab?, sourceView: UIView?, sourceRect: CGRect, arrowDirection: UIPopoverArrowDirection) -> [UIActivity] {
    let findInPageActivity = FindInPageActivity() { [unowned self] in
      self.updateFindInPageVisibility(visible: true)
    }
    
    let pageZoomActivity = PageZoomActivity() { [unowned self] in
      self.displayPageZoom(visible: true)
    }

    var activities: [UIActivity] = [findInPageActivity, pageZoomActivity]

    // These actions don't apply if we're sharing a temporary document
    if !url.isFileURL {
      // We don't allow to have 2 same favorites.
      if !FavoritesHelper.isAlreadyAdded(url) {
        activities.append(
          AddToFavoritesActivity() { [weak tab] in
            FavoritesHelper.add(url: url, title: tab?.displayTitle)
          })
      }

      activities.append(
        RequestDesktopSiteActivity(tab: tab) { [weak tab] in
          tab?.switchUserAgent()
        })

      if Preferences.BraveNews.isEnabled.value, let metadata = tab?.pageMetadata,
        !metadata.feeds.isEmpty {
        let feeds: [RSSFeedLocation] = metadata.feeds.compactMap { feed in
          guard let url = URL(string: feed.href) else { return nil }
          return RSSFeedLocation(title: feed.title, url: url)
        }
        if !feeds.isEmpty {
          let addToBraveNews = AddFeedToBraveNewsActivity() { [weak self] in
            guard let self = self else { return }
            let controller = BraveNewsAddSourceResultsViewController(
              dataSource: self.feedDataSource,
              searchedURL: url,
              rssFeedLocations: feeds,
              sourcesAdded: nil
            )
            let container = UINavigationController(rootViewController: controller)
            let idiom = UIDevice.current.userInterfaceIdiom
            container.modalPresentationStyle = idiom == .phone ? .pageSheet : .formSheet
            self.present(container, animated: true)
          }
          activities.append(addToBraveNews)
        }
      }

      if let webView = tab?.webView, tab?.temporaryDocument == nil {
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

    } else {
      // Check if it's a feed, url is a temp document file URL
      if let selectedTab = tabManager.selectedTab,
        (selectedTab.mimeType == "application/xml" || selectedTab.mimeType == "application/json"),
        let tabURL = selectedTab.url {

        let parser = FeedParser(URL: url)
        if case .success(let feed) = parser.parse() {
          let addToBraveNews = AddFeedToBraveNewsActivity() { [weak self] in
            guard let self = self else { return }
            let controller = BraveNewsAddSourceResultsViewController(
              dataSource: self.feedDataSource,
              searchedURL: tabURL,
              rssFeedLocations: [.init(title: feed.title, url: tabURL)],
              sourcesAdded: nil
            )
            let container = UINavigationController(rootViewController: controller)
            let idiom = UIDevice.current.userInterfaceIdiom
            container.modalPresentationStyle = idiom == .phone ? .pageSheet : .formSheet
            self.present(container, animated: true)
          }
          activities.append(addToBraveNews)
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

    return activities
  }

  func presentActivityViewController(_ url: URL, tab: Tab? = nil, sourceView: UIView?, sourceRect: CGRect, arrowDirection: UIPopoverArrowDirection) {
    let activities: [UIActivity] = makeShareActivities(
      for: url,
      tab: tab,
      sourceView: sourceView,
      sourceRect: sourceRect,
      arrowDirection: arrowDirection
    )

    let controller = ShareExtensionHelper.makeActivityViewController(
      selectedURL: url,
      selectedTab: tab,
      applicationActivities: activities
    )

    controller.completionWithItemsHandler = { [weak self] _, _, _, _ in
      self?.cleanUpCreateActivity()
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

  func updateFindInPageVisibility(visible: Bool, tab: Tab? = nil) {
    if visible {
      if findInPageBar == nil {
        let findInPageBar = FindInPageBar()
        self.findInPageBar = findInPageBar
        findInPageBar.delegate = self
        
        displayPageZoom(visible: false)
        alertStackView.addArrangedSubview(findInPageBar)

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
      webView.evaluateSafeJavaScript(functionName: "__firefox__.findDone", contentWorld: .defaultClient)
      findInPageBar.removeFromSuperview()
      self.findInPageBar = nil
      updateViewConstraints()
    }
  }
  
  func displayPageZoom(visible: Bool) {
    if !visible || pageZoomBar != nil {     
      pageZoomBar?.view.removeFromSuperview()

      if let zoomBarView = pageZoomBar?.view {
        alertStackView.removeArrangedSubview(zoomBarView)
      }
        
      updateViewConstraints()
      pageZoomBar = nil
      
      return
    }
    
    guard let webView = tabManager.selectedTab?.webView else { return }
    let pageZoomBar = UIHostingController(rootView: PageZoomView(webView: webView))
    
    pageZoomBar.rootView.dismiss = { [weak self] in
      guard let self = self else { return }
      pageZoomBar.view.removeFromSuperview()
      self.updateViewConstraints()
      self.pageZoomBar = nil
    }
    
    if let findInPageBar = findInPageBar {
      findInPageBar.endEditing(true)
      findInPageBar.removeFromSuperview()
      self.findInPageBar = nil
      updateViewConstraints()
    }
    
    alertStackView.arrangedSubviews.forEach({
      $0.removeFromSuperview()
    })
    alertStackView.addArrangedSubview(pageZoomBar.view)

    pageZoomBar.view.snp.makeConstraints { make in
      make.height.greaterThanOrEqualTo(UIConstants.toolbarHeight)
      make.edges.equalTo(alertStackView)
    }
    
    updateViewConstraints()
    self.pageZoomBar = pageZoomBar
  }
  
  private func updateWebViewPageZoom(tab: Tab) {
    if let currentURL = tab.url {
      let domain = Domain.getPersistedDomain(for: currentURL)
      
      let zoomLevel = PrivateBrowsingManager.shared.isPrivateBrowsing ? 1.0 : domain?.zoom_level?.doubleValue ?? Preferences.General.defaultPageZoomLevel.value
      tab.webView?.setValue(zoomLevel, forKey: PageZoomView.propertyName)
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

      if (!InternalURL.isValid(url: url) || url.isReaderModeURL), !url.isFileURL {
        // Fire the readability check. This is here and not in the pageShow event handler in ReaderMode.js anymore
        // because that event will not always fire due to unreliable page caching. This will either let us know that
        // the currently loaded page can be turned into reading mode or if the page already is in reading mode. We
        // ignore the result because we are being called back asynchronous when the readermode status changes.
        webView.evaluateSafeJavaScript(functionName: "\(ReaderModeNamespace).checkReadability", contentWorld: .defaultClient)

        // Re-run additional scripts in webView to extract updated favicons and metadata.
        runScriptsOnWebView(webView)

        // Only add history of a url which is not a localhost url
        if !tab.isPrivate, !url.isReaderModeURL {
          // The visitType is checked If it is "typed" or not to determine the History object we are adding
          // should be synced or not. This limitation exists on browser side so we are aligning with this
          if let visitType = typedNavigation.first(where: { $0.key.typedDisplayString == url.typedDisplayString })?.value,
            visitType == .typed {
            braveCore.historyAPI.add(url: url, title: tab.title ?? "", dateAdded: Date())
          } else {
            braveCore.historyAPI.add(url: url, title: tab.title ?? "", dateAdded: Date(), isURLTyped: false)
          }
          
          // Saving Tab. Private Mode - not supported yet.
          if !tab.isPrivate {
            tabManager.saveTab(tab)
          }
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

      DispatchQueue.main.asyncAfter(deadline: .now() + .milliseconds(500)) {
        self.screenshotHelper.takeScreenshot(tab)
      }
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

  func scanQRCode() {
    if RecentSearchQRCodeScannerController.hasCameraPermissions {
      let qrCodeController = RecentSearchQRCodeScannerController { [weak self] string in
        guard let self = self else { return }

        if let url = URIFixup.getURL(string) {
          self.didScanQRCodeWithURL(url)
        } else {
          self.didScanQRCodeWithText(string)
        }
      }

      let navigationController = UINavigationController(rootViewController: qrCodeController)
      navigationController.modalPresentationStyle =
        UIDevice.current.userInterfaceIdiom == .phone ? .pageSheet : .formSheet

      self.present(navigationController, animated: true, completion: nil)
    } else {
      let alert = UIAlertController(title: Strings.scanQRCodeViewTitle, message: Strings.scanQRCodePermissionErrorMessage, preferredStyle: .alert)
      alert.addAction(UIAlertAction(title: Strings.scanQRCodeErrorOKButton, style: .default, handler: nil))
      self.present(alert, animated: true, completion: nil)
    }
  }

  private func focusLocationField() {
    topToolbar.tabLocationViewDidTapLocation(topToolbar.locationView)
  }
  
  private func handleToolbarVisibilityStateChange(
    _ state: ToolbarVisibilityViewModel.ToolbarState,
    progress: CGFloat?
  ) {
    guard
      let tab = tabManager.selectedTab,
      tab.mimeType != MIMEType.PDF, // Constraint-based animation is causing PDF docs to flicker.
      let webView = tab.webView,
      !webView.isLoading else {
      
      toolbarTopConstraint?.update(offset: 0)
      toolbarBottomConstraint?.update(offset: 0)
      
      // Check if UI side is collapsed already
      if topToolbar.locationContainer.alpha < 1 {
        let animator = toolbarVisibilityViewModel.toolbarChangePropertyAnimator
        animator.addAnimations { [self] in
          view.layoutIfNeeded()
          topToolbar.locationContainer.alpha = 1
          topToolbar.actionButtons.forEach { $0.alpha = topToolbar.locationContainer.alpha }
          header.collapsedBarContainerView.alpha = 1 - topToolbar.locationContainer.alpha
          tabsBar.view.subviews.forEach { $0.alpha = topToolbar.locationContainer.alpha }
        }
        animator.startAnimation()
      }
      return
    }
    let headerHeight = toolbarVisibilityViewModel.transitionDistance
    let footerHeight = footer.bounds.height
    if let progress = progress {
      switch state {
      case .expanded:
        toolbarTopConstraint?.update(offset: -min(headerHeight, max(0, headerHeight * progress)))
        topToolbar.locationContainer.alpha = max(0, min(1, 1 - (progress * 1.5))) // Have it disappear a bit faster
        toolbarBottomConstraint?.update(offset: min(footerHeight, max(0, footerHeight * progress)))
      case .collapsed:
        toolbarTopConstraint?.update(offset: -min(headerHeight, max(0, headerHeight * (1 - progress))))
        topToolbar.locationContainer.alpha = progress
        toolbarBottomConstraint?.update(offset: min(footerHeight, max(0, footerHeight * (1 - progress))))
      }
      topToolbar.actionButtons.forEach { $0.alpha = topToolbar.locationContainer.alpha }
      tabsBar.view.subviews.forEach { $0.alpha = topToolbar.locationContainer.alpha }
      header.collapsedBarContainerView.alpha = 1 - topToolbar.locationContainer.alpha
      return
    }
    switch state {
    case .expanded:
      toolbarTopConstraint?.update(offset: 0)
      toolbarBottomConstraint?.update(offset: 0)
      topToolbar.locationContainer.alpha = 1
    case .collapsed:
      toolbarTopConstraint?.update(offset: -headerHeight)
      topToolbar.locationContainer.alpha = 0
      toolbarBottomConstraint?.update(offset: footerHeight)
    }
    tabsBar.view.subviews.forEach { $0.alpha = topToolbar.locationContainer.alpha }
    topToolbar.actionButtons.forEach { $0.alpha = topToolbar.locationContainer.alpha }
    header.collapsedBarContainerView.alpha = 1 - topToolbar.locationContainer.alpha
    let animator = toolbarVisibilityViewModel.toolbarChangePropertyAnimator
    animator.addAnimations {
      self.view.layoutIfNeeded()
    }
    animator.startAnimation()
  }
}

extension BrowserViewController: ClipboardBarDisplayHandlerDelegate {
  func shouldDisplay(clipboardBar bar: ButtonToast) {
    show(toast: bar, duration: ClipboardBarToastUX.toastDelay)
  }
}

extension BrowserViewController: QRCodeViewControllerDelegate {
  func didScanQRCodeWithURL(_ url: URL) {
    popToBVC()
    finishEditingAndSubmit(url, visitType: .typed)

    if !url.isBookmarklet && !PrivateBrowsingManager.shared.isPrivateBrowsing {
      RecentSearch.addItem(type: .qrCode, text: nil, websiteUrl: url.absoluteString)
    }
  }

  func didScanQRCodeWithText(_ text: String) {
    popToBVC()
    submitSearchText(text)

    if !PrivateBrowsingManager.shared.isPrivateBrowsing {
      RecentSearch.addItem(type: .qrCode, text: text, websiteUrl: nil)
    }
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
    settingsViewController.dismiss(animated: true)
  }
}

extension BrowserViewController: PresentingModalViewControllerDelegate {
  func dismissPresentedModalViewController(_ modalViewController: UIViewController, animated: Bool) {
    self.dismiss(animated: animated, completion: nil)
  }
}

extension BrowserViewController: TabsBarViewControllerDelegate {
  func tabsBarDidSelectAddNewTab(_ isPrivate: Bool) {
    openBlankNewTab(attemptLocationFieldFocus: true, isPrivate: isPrivate)
  }

  func tabsBarDidSelectTab(_ tabsBarController: TabsBarViewController, _ tab: Tab) {
    if tab == tabManager.selectedTab { return }
    topToolbar.leaveOverlayMode(didCancel: true)
    tabManager.selectTab(tab)
  }

  func tabsBarDidLongPressAddTab(_ tabsBarController: TabsBarViewController, button: UIButton) {
    // The actions are carried to menu actions for Tab-Tray Button
  }
  
  func tabsBarDidChangeReaderModeVisibility(_ isHidden: Bool = true) {
    switch topToolbar.locationView.readerModeState {
    case .active:
      if isHidden {
        hideReaderModeBar(animated: false)
      } else {
        showReaderModeBar(animated: false)
      }
    case .unavailable:
      hideReaderModeBar(animated: false)
    default:
      break
    }
  }
}

extension BrowserViewController: TabDelegate {
  func tab(_ tab: Tab, didCreateWebView webView: WKWebView) {
    webView.scrollView.contentInsetAdjustmentBehavior = .never
    webView.frame = webViewContainer.frame
    
    // Observers that live as long as the tab. Make sure these are all cleared in willDeleteWebView below!
    KVOs.forEach { webView.addObserver(self, forKeyPath: $0.rawValue, options: .new, context: nil) }
    webView.uiDelegate = self

    let formPostHelper = FormPostHelper(tab: tab)
    tab.addContentScript(formPostHelper, name: FormPostHelper.name(), contentWorld: .page)

    let readerMode = ReaderMode(tab: tab)
    readerMode.delegate = self
    tab.addContentScript(readerMode, name: ReaderMode.name(), contentWorld: .defaultClient)

        // only add the logins helper and wallet provider if the tab is not a private browsing tab
    if !tab.isPrivate {
      let logins = LoginsHelper(tab: tab, profile: profile, passwordAPI: braveCore.passwordAPI)
      tab.addContentScript(logins, name: LoginsHelper.name(), contentWorld: .defaultClient)
      tab.addContentScript(EthereumProviderHelper(tab: tab), name: EthereumProviderHelper.name(), contentWorld: .page)
    }

    let errorHelper = ErrorPageHelper(certStore: profile.certStore)
    tab.addContentScript(errorHelper, name: ErrorPageHelper.name(), contentWorld: .page)

    let sessionRestoreHelper = SessionRestoreHelper(tab: tab)
    sessionRestoreHelper.delegate = self
    tab.addContentScript(sessionRestoreHelper, name: SessionRestoreHelper.name(), contentWorld: .defaultClient)

    let findInPageHelper = FindInPageHelper(tab: tab)
    findInPageHelper.delegate = self
    tab.addContentScript(findInPageHelper, name: FindInPageHelper.name(), contentWorld: .defaultClient)

    let noImageModeHelper = NoImageModeHelper(tab: tab)
    tab.addContentScript(noImageModeHelper, name: NoImageModeHelper.name(), contentWorld: .defaultClient)

    let printHelper = PrintHelper(browserController: self, tab: tab)
    tab.addContentScript(printHelper, name: PrintHelper.name(), contentWorld: .page)

    let customSearchHelper = CustomSearchHelper(tab: tab)
    tab.addContentScript(customSearchHelper, name: CustomSearchHelper.name(), contentWorld: .page)

    let nightModeHelper = NightModeHelper(tab: tab)
    tab.addContentScript(nightModeHelper, name: NightModeHelper.name(), contentWorld: .page)

    // XXX: Bug 1390200 - Disable NSUserActivity/CoreSpotlight temporarily
    // let spotlightHelper = SpotlightHelper(tab: tab)
    // tab.addHelper(spotlightHelper, name: SpotlightHelper.name())

    tab.addContentScript(LocalRequestHelper(), name: LocalRequestHelper.name(), contentWorld: .page)

    tab.contentBlocker.setupTabTrackingProtection()
    tab.addContentScript(tab.contentBlocker, name: ContentBlockerHelper.name(), contentWorld: .page)

    tab.addContentScript(FocusHelper(tab: tab), name: FocusHelper.name(), contentWorld: .defaultClient)

    tab.addContentScript(BraveGetUA(tab: tab), name: BraveGetUA.name(), contentWorld: .page)
    tab.addContentScript(
      BraveSearchScriptHandler(
        tab: tab,
        profile: profile,
        rewards: rewards),
      name: BraveSearchScriptHandler.name(), contentWorld: .page)

    tab.addContentScript(
      BraveTalkScriptHandler(
        tab: tab,
        rewards: rewards),
      name: BraveTalkScriptHandler.name(), contentWorld: .page)

    tab.addContentScript(ResourceDownloadManager(tab: tab), name: ResourceDownloadManager.name(), contentWorld: .defaultClient)

    tab.addContentScript(WindowRenderHelperScript(tab: tab), name: WindowRenderHelperScript.name(), contentWorld: .defaultClient)

    let playlistHelper = PlaylistHelper(tab: tab)
    playlistHelper.delegate = self
    tab.addContentScript(playlistHelper, name: PlaylistHelper.name(), contentWorld: .page)

    tab.addContentScript(RewardsReporting(rewards: rewards, tab: tab), name: RewardsReporting.name(), contentWorld: .page)
    tab.addContentScript(AdsMediaReporting(rewards: rewards, tab: tab), name: AdsMediaReporting.name(), contentWorld: .defaultClient)
    tab.addContentScript(ReadyStateScriptHelper(tab: tab), name: ReadyStateScriptHelper.name(), contentWorld: .page)
    tab.addContentScript(DeAmpHelper(tab: tab), name: DeAmpHelper.name(), contentWorld: .defaultClient)
    tab.addContentScript(tab.requestBlockingContentHelper, name: RequestBlockingContentHelper.name(), contentWorld: .page)
  }

  func tab(_ tab: Tab, willDeleteWebView webView: WKWebView) {
    tab.cancelQueuedAlerts()
    KVOs.forEach { webView.removeObserver(self, forKeyPath: $0.rawValue) }
    toolbarVisibilityViewModel.endScrollViewObservation(webView.scrollView)
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
    UIView.animate(
      withDuration: animated ? 0.25 : 0,
      animations: {
        self.alertStackView.insertArrangedSubview(bar, at: 0)
        self.view.layoutIfNeeded()
      })
  }

  func removeBar(_ bar: SnackBar, animated: Bool) {
    UIView.animate(
      withDuration: animated ? 0.25 : 0,
      animations: {
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

  /// Triggered when "Find in Page" is selected on selected text
  func tab(_ tab: Tab, didSelectFindInPageFor selectedText: String) {
    updateFindInPageVisibility(visible: true)
    findInPageBar?.text = selectedText
  }

  /// Triggered when "Search with Brave" is selected on selected web text
  func tab(_ tab: Tab, didSelectSearchWithBraveFor selectedText: String) {
    let engine = profile.searchEngines.defaultEngine()

    guard let url = engine.searchURLForQuery(selectedText) else {
      assertionFailure("If this returns nil, investigate why and add proper handling or commenting")
      return
    }

    tabManager.addTabAndSelect(
      URLRequest(url: url),
      afterTab: tab,
      isPrivate: tab.isPrivate
    )
    
    if !PrivateBrowsingManager.shared.isPrivateBrowsing {
      RecentSearch.addItem(type: .text, text: selectedText, websiteUrl: url.absoluteString)
    }
  }

  func showRequestRewardsPanel(_ tab: Tab) {
    let vc = BraveTalkRewardsOptInViewController()

    // Edge case: user disabled Rewards button and wants to access free Brave Talk
    // We re-enable the button again. It can be disabled in settings later.
    Preferences.Rewards.hideRewardsIcon.value = false

    let popover = PopoverController(
      contentController: vc,
      contentSizeBehavior: .preferredContentSize)
    popover.addsConvenientDismissalMargins = false
    popover.present(from: topToolbar.locationView.rewardsButton, on: self)
    popover.popoverDidDismiss = { _ in
      // This gets called if popover is dismissed by user gesture
      // This does not conflict with 'Enable Rewards' button.
      tab.rewardsEnabledCallback?(false)
    }

    vc.rewardsEnabledHandler = { [weak self] in
      guard let self = self else { return }

      self.rewards.isEnabled = true
      tab.rewardsEnabledCallback?(true)

      let vc2 = BraveTalkOptInSuccessViewController()
      let popover2 = PopoverController(
        contentController: vc2,
        contentSizeBehavior: .preferredContentSize)
      popover2.present(from: self.topToolbar.locationView.rewardsButton, on: self)
    }

    vc.linkTapped = { [unowned self] request in
      tab.rewardsEnabledCallback?(false)
      self.tabManager
        .addTabAndSelect(request, isPrivate: PrivateBrowsingManager.shared.isPrivateBrowsing)
    }
  }

  func stopMediaPlayback(_ tab: Tab) {
    tabManager.allTabs.forEach({
      PlaylistHelper.stopPlayback(tab: $0)
    })
  }
  
  func showWalletNotification(_ tab: Tab, origin: URLOrigin) {
    // only display notification when BVC is front and center
    guard presentedViewController == nil,
          Preferences.Wallet.displayWeb3Notifications.value else {
      return
    }
    let walletNotificaton = WalletNotification(priority: .low, origin: origin) { [weak self] action in
      if action == .connectWallet {
        self?.presentWalletPanel(tab: tab)
      }
    }
    notificationsPresenter.display(notification: walletNotificaton, from: self)
  }
  
  func isTabVisible(_ tab: Tab) -> Bool {
    tabManager.selectedTab === tab
  }

  func updateURLBarWalletButton() {
    let shouldShowWalletButton = tabManager.selectedTab?.isWalletIconVisible == true
    if shouldShowWalletButton {
      Task { @MainActor in
        let isPendingRequestAvailable = await isPendingRequestAvailable()
        topToolbar.updateWalletButtonState(isPendingRequestAvailable ? .activeWithPendingRequest : .active)
      }
    } else {
      topToolbar.updateWalletButtonState(.inactive)
    }
  }

  @MainActor
  private func isPendingRequestAvailable() async -> Bool {
    let privateMode = PrivateBrowsingManager.shared.isPrivateBrowsing
    /// If we have an open `WalletStore`, use that so we can assign the pending request if the wallet is open,
    /// which allows us to store the new `PendingRequest` triggering a modal presentation for that request.
    guard let cryptoStore = self.walletStore?.cryptoStore ?? CryptoStore.from(privateMode: privateMode) else {
      return false
    }
    if await cryptoStore.isPendingRequestAvailable() {
      return true
    } else if let selectedTabOrigin = tabManager.selectedTab?.url?.origin {
      return WalletProviderPermissionRequestsManager.shared.hasPendingRequest(for: selectedTabOrigin, coinType: .eth)
    }
    return false
  }
}

extension BrowserViewController: SearchViewControllerDelegate {
  func searchViewController(_ searchViewController: SearchViewController, didSubmit query: String, braveSearchPromotion: Bool) {
    topToolbar.leaveOverlayMode()
    processAddressBar(text: query, visitType: .typed, isBraveSearchPromotion: braveSearchPromotion)
  }

  func searchViewController(_ searchViewController: SearchViewController, didSelectURL url: URL) {
    finishEditingAndSubmit(url, visitType: .typed)
  }

  func searchViewController(_ searchViewController: SearchViewController, didSelectOpenTab tabInfo: (id: String?, url: URL)) {
    switchToTabOrOpen(id: tabInfo.id, url: tabInfo.url)
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
    if let url = tabManager.selectedTab?.webView?.url,
      let internalURL = InternalURL(url),
      internalURL.isAboutHomeURL {
      return false
    }
    return true
  }
}

extension BrowserViewController: TabManagerDelegate {
  func tabManager(_ tabManager: TabManager, didSelectedTabChange selected: Tab?, previous: Tab?) {
    // Remove the old accessibilityLabel. Since this webview shouldn't be visible, it doesn't need it
    // and having multiple views with the same label confuses tests.
    if let wv = previous?.webView {
      toolbarVisibilityViewModel.endScrollViewObservation(wv.scrollView)
      
      wv.endEditing(true)
      wv.accessibilityLabel = nil
      wv.accessibilityElementsHidden = true
      wv.accessibilityIdentifier = nil
      wv.removeFromSuperview()
    }

    toolbar?.setSearchButtonState(url: selected?.url)
    if let tab = selected, let webView = tab.webView {
      toolbarVisibilityViewModel.beginObservingScrollView(webView.scrollView)
      toolbarVisibilityCancellable = toolbarVisibilityViewModel.objectWillChange
        .receive(on: DispatchQueue.main)
        .sink(receiveValue: { [weak self] in
          guard let self = self else { return }
          let (state, progress) = (self.toolbarVisibilityViewModel.toolbarState,
                                   self.toolbarVisibilityViewModel.interactiveTransitionProgress)
          self.handleToolbarVisibilityStateChange(state, progress: progress)
        })
      
      updateURLBar()

      if let url = tab.url, !InternalURL.isValid(url: url) {
        let previousEstimatedProgress = previous?.webView?.estimatedProgress ?? 1.0
        let selectedEstimatedProgress = webView.estimatedProgress

        // Progress should be updated only if there's a difference between tabs.
        // Otherwise we do nothing, so switching between fully loaded tabs won't show the animation.
        if previousEstimatedProgress != selectedEstimatedProgress {
          topToolbar.updateProgressBar(Float(selectedEstimatedProgress))
        }
      }

      readerModeCache = ReaderMode.cache(for: tab)
      ReaderModeHandlers.readerModeCache = readerModeCache

      webViewContainer.addSubview(webView)
      webView.snp.remakeConstraints { make in
        make.left.right.top.bottom.equalTo(self.webViewContainer)
      }

      // This is a terrible workaround for a bad iOS 12 bug where PDF
      // content disappears any time the view controller changes (i.e.
      // the user taps on the tabs tray). It seems the only way to get
      // the PDF to redraw is to either reload it or revisit it from
      // back/forward list. To try and avoid hitting the network again
      // for the same PDF, we revisit the current back/forward item and
      // restore the previous scrollview zoom scale and content offset
      // after a short 100ms delay. *facepalm*
      //
      // https://bugzilla.mozilla.org/show_bug.cgi?id=1516524
      if tab.mimeType == MIMEType.PDF {
        let previousZoomScale = webView.scrollView.zoomScale
        let previousContentOffset = webView.scrollView.contentOffset

        if let currentItem = webView.backForwardList.currentItem {
          webView.go(to: currentItem)
        }

        DispatchQueue.main.asyncAfter(deadline: .now() + .milliseconds(100)) {
          webView.scrollView.setZoomScale(previousZoomScale, animated: false)
          webView.scrollView.setContentOffset(previousContentOffset, animated: false)
        }
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

    updateToolbarUsingTabManager(tabManager)

    removeAllBars()
    if let bars = selected?.bars {
      for bar in bars {
        showBar(bar, animated: true)
      }
    }

    updateFindInPageVisibility(visible: false, tab: previous)
    displayPageZoom(visible: false)
    updateTabsBarVisibility()
    selected?.updatePullToRefreshVisibility()

    topToolbar.locationView.loading = selected?.loading ?? false
    navigationToolbar.updateBackStatus(selected?.canGoBack ?? false)
    navigationToolbar.updateForwardStatus(selected?.canGoForward ?? false)

    let shouldShowPlaylistURLBarButton = selected?.url?.isPlaylistSupportedSiteURL == true

    if let readerMode = selected?.getContentScript(name: ReaderMode.name()) as? ReaderMode,
      !shouldShowPlaylistURLBarButton {
      topToolbar.updateReaderModeState(readerMode.state)
      if readerMode.state == .active {
        showReaderModeBar(animated: false)
      } else {
        hideReaderModeBar(animated: false)
      }

      updatePlaylistURLBar(tab: selected, state: selected?.playlistItemState ?? .none, item: selected?.playlistItem)
    } else {
      topToolbar.updateReaderModeState(ReaderModeState.unavailable)
    }

    updateInContentHomePanel(selected?.url as URL?)

    notificationsPresenter.removeNotification(with: WalletNotification.Constant.id)
    WalletProviderPermissionRequestsManager.shared.cancelAllPendingRequests()
    updateURLBarWalletButton()
  }

  func tabManager(_ tabManager: TabManager, willAddTab tab: Tab) {
  }

  func tabManager(_ tabManager: TabManager, didAddTab tab: Tab) {
    // If we are restoring tabs then we update the count once at the end
    if !tabManager.isRestoring {
      updateToolbarUsingTabManager(tabManager)
    }
    tab.tabDelegate = self
    tab.walletKeyringService = BraveWallet.KeyringServiceFactory.get(privateMode: tab.isPrivate)
    updateTabsBarVisibility()
  }

  func tabManager(_ tabManager: TabManager, willRemoveTab tab: Tab) {
    tab.webView?.removeFromSuperview()
  }

  func tabManager(_ tabManager: TabManager, didRemoveTab tab: Tab) {
    updateToolbarUsingTabManager(tabManager)
    // tabDelegate is a weak ref (and the tab's webView may not be destroyed yet)
    // so we don't expcitly unset it.
    topToolbar.leaveOverlayMode(didCancel: true)
    updateTabsBarVisibility()

    if !PrivateBrowsingManager.shared.isPrivateBrowsing {
      rewards.reportTabClosed(tabId: Int(tab.rewardsId))
    }
  }

  func tabManagerDidAddTabs(_ tabManager: TabManager) {
    updateToolbarUsingTabManager(tabManager)
  }

  func tabManagerDidRestoreTabs(_ tabManager: TabManager) {
    updateToolbarUsingTabManager(tabManager)
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

    toast.showToast(
      viewController: self, delay: delay, duration: duration,
      makeConstraints: { make in
        make.left.right.equalTo(self.view)
        make.bottom.equalTo(self.webViewContainer)
      })
  }

  func tabManagerDidRemoveAllTabs(_ tabManager: TabManager, toast: ButtonToast?) {
    guard let toast = toast, !PrivateBrowsingManager.shared.isPrivateBrowsing else {
      return
    }
    show(toast: toast, afterWaiting: ButtonToastUX.toastDelay)
  }

  func updateToolbarUsingTabManager(_ tabManager: TabManager) {
    // Update Tab Count on Tab-Tray Button
    let count = tabManager.tabsForCurrentMode.count
    toolbar?.updateTabCount(count)
    topToolbar?.updateTabCount(count)

    // Update Actions for Tab-Tray Button
    var newTabMenuChildren: [UIAction] = []
    var addTabMenuChildren: [UIAction] = []

    if !PrivateBrowsingManager.shared.isPrivateBrowsing {
      let openNewPrivateTab = UIAction(
        title: Strings.newPrivateTabTitle,
        image: UIImage(systemName: "plus.square.fill.on.square.fill"),
        handler: UIAction.deferredActionHandler { [unowned self] _ in
          self.openBlankNewTab(attemptLocationFieldFocus: true, isPrivate: true)
        })

      if (UIDevice.current.userInterfaceIdiom == .pad && tabsBar?.view.isHidden == true) || (UIDevice.current.userInterfaceIdiom == .phone && toolbar == nil) {
        newTabMenuChildren.append(openNewPrivateTab)
      }

      addTabMenuChildren.append(openNewPrivateTab)
    }

    let openNewTab = UIAction(
      title: PrivateBrowsingManager.shared.isPrivateBrowsing ? Strings.newPrivateTabTitle : Strings.newTabTitle,
      image: PrivateBrowsingManager.shared.isPrivateBrowsing ? UIImage(systemName: "plus.square.fill.on.square.fill") : UIImage(systemName: "plus.square.on.square"),
      handler: UIAction.deferredActionHandler { [unowned self] _ in
        self.openBlankNewTab(attemptLocationFieldFocus: true, isPrivate: PrivateBrowsingManager.shared.isPrivateBrowsing)
      })

    if (UIDevice.current.userInterfaceIdiom == .pad && tabsBar.view.isHidden) || (UIDevice.current.userInterfaceIdiom == .phone && toolbar == nil) {
      newTabMenuChildren.append(openNewTab)
    }
    addTabMenuChildren.append(openNewTab)

    var bookmarkMenuChildren: [UIAction] = []
    
    let containsWebPage = tabManager.selectedTab?.containsWebPage == true

    if tabManager.openedWebsitesCount > 0, containsWebPage {
      let bookmarkActiveTab = UIAction(
        title: Strings.addToMenuItem,
        image: UIImage(systemName: "book"),
        handler: UIAction.deferredActionHandler { [unowned self] _ in
          self.openAddBookmark()
        })

      bookmarkMenuChildren.append(bookmarkActiveTab)
    }
    
    if tabManager.tabsForCurrentMode.count > 1 {
      let bookmarkAllTabs = UIAction(
        title: String.localizedStringWithFormat(Strings.bookmarkAllTabsTitle, tabManager.tabsForCurrentMode.count),
        image: UIImage(systemName: "book"),
        handler: UIAction.deferredActionHandler { [unowned self] _ in
          let mode = BookmarkEditMode.addFolderUsingTabs(title: Strings.savedTabsFolderTitle, tabList: tabManager.tabsForCurrentMode)
          let addBookMarkController = AddEditBookmarkTableViewController(bookmarkManager: bookmarkManager, mode: mode)

          presentSettingsNavigation(with: addBookMarkController, cancelEnabled: true)
        })

      bookmarkMenuChildren.append(bookmarkAllTabs)
    }
    
    var duplicateTabMenuChildren: [UIAction] = []

    if containsWebPage, let selectedTab = tabManager.selectedTab, let url = selectedTab.fetchedURL {
      let duplicateActiveTab = UIAction(
        title: Strings.duplicateActiveTab,
        image: UIImage(systemName: "plus.square.on.square"),
        handler: UIAction.deferredActionHandler { [weak selectedTab] _ in
          guard let selectedTab = selectedTab else { return }
          
          tabManager.addTabAndSelect(
               URLRequest(url: url),
               afterTab: selectedTab,
               isPrivate: selectedTab.isPrivate
           )
        })

      duplicateTabMenuChildren.append(duplicateActiveTab)
    }

    var closeTabMenuChildren: [UIAction] = []

    let closeActiveTab = UIAction(
      title: String(format: Strings.closeTabTitle),
      image: UIImage(systemName: "xmark"),
      attributes: .destructive,
      handler: UIAction.deferredActionHandler { [unowned self] _ in
        if let tab = tabManager.selectedTab {
          if topToolbar.locationView.readerModeState == .active {
            hideReaderModeBar(animated: false)
          }
          
          tabManager.removeTab(tab)
        }
      })

    closeTabMenuChildren.append(closeActiveTab)

    if tabManager.tabsForCurrentMode.count > 1 {
      let closeAllTabs = UIAction(
        title: String(format: Strings.closeAllTabsTitle, tabManager.tabsForCurrentMode.count),
        image: UIImage(systemName: "xmark"),
        attributes: .destructive,
        handler: UIAction.deferredActionHandler { [unowned self] _ in

          let alert = UIAlertController(title: nil, message: Strings.closeAllTabsPrompt, preferredStyle: .actionSheet)
          let cancelAction = UIAlertAction(title: Strings.CancelString, style: .cancel)
          let closedTabsTitle = String(format: Strings.closeAllTabsTitle, tabManager.tabsForCurrentMode.count)
          let closeAllAction = UIAlertAction(title: closedTabsTitle, style: .destructive) { _ in
            tabManager.removeAll()
          }
          alert.addAction(closeAllAction)
          alert.addAction(cancelAction)

          if let popoverPresentation = alert.popoverPresentationController,
              let tabsButton = toolbar?.tabsButton ?? topToolbar?.tabsButton {
            popoverPresentation.sourceView = tabsButton
            popoverPresentation.sourceRect =
              .init(x: tabsButton.frame.width / 2, y: tabsButton.frame.height, width: 1, height: 1)
          }

          self.present(alert, animated: true)
        })

      closeTabMenuChildren.append(closeAllTabs)
    }

    let newTabMenu = UIMenu(title: "", options: .displayInline, children: newTabMenuChildren)
    let addTabMenu = UIMenu(title: "", options: .displayInline, children: addTabMenuChildren)
    let bookmarkMenu = UIMenu(title: "", options: .displayInline, children: bookmarkMenuChildren)
    let duplicateTabMenu = UIMenu(title: "", options: .displayInline, children: duplicateTabMenuChildren)
    let closeTabMenu = UIMenu(title: "", options: .displayInline, children: closeTabMenuChildren)

    toolbar?.tabsButton.menu = UIMenu(title: "", identifier: nil, children: [closeTabMenu, duplicateTabMenu, bookmarkMenu, newTabMenu])
    topToolbar?.tabsButton.menu = UIMenu(title: "", identifier: nil, children: [closeTabMenu, duplicateTabMenu, bookmarkMenu, newTabMenu])

    // Update Actions for Add-Tab Button
    toolbar?.addTabButton.menu = UIMenu(title: "", identifier: nil, children: [addTabMenu])
  }
}

/// List of schemes that are allowed to be opened in new tabs.
private let schemesAllowedToBeOpenedAsPopups = ["http", "https", "javascript", "about", "whatsapp"]

extension BrowserViewController: WKUIDelegate {
  public func webView(_ webView: WKWebView, createWebViewWith configuration: WKWebViewConfiguration, for navigationAction: WKNavigationAction, windowFeatures: WKWindowFeatures) -> WKWebView? {
    guard let parentTab = tabManager[webView] else { return nil }

    guard !navigationAction.isInternalUnprivileged,
      shouldRequestBeOpenedAsPopup(navigationAction.request)
    else {
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
    
    toolbarVisibilityViewModel.toolbarState = .expanded

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

  public func webView(_ webView: WKWebView, runJavaScriptAlertPanelWithMessage message: String, initiatedByFrame frame: WKFrameInfo, completionHandler: @escaping () -> Void) {
    var messageAlert = MessageAlert(message: message, frame: frame, completionHandler: completionHandler, suppressHandler: nil)
    handleAlert(webView: webView, alert: &messageAlert) {
      completionHandler()
    }
  }

  public func webView(_ webView: WKWebView, runJavaScriptConfirmPanelWithMessage message: String, initiatedByFrame frame: WKFrameInfo, completionHandler: @escaping (Bool) -> Void) {
    var confirmAlert = ConfirmPanelAlert(message: message, frame: frame, completionHandler: completionHandler, suppressHandler: nil)
    handleAlert(webView: webView, alert: &confirmAlert) {
      completionHandler(false)
    }
  }

  public func webView(_ webView: WKWebView, runJavaScriptTextInputPanelWithPrompt prompt: String, defaultText: String?, initiatedByFrame frame: WKFrameInfo, completionHandler: @escaping (String?) -> Void) {
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
    webView.evaluateSafeJavaScript(functionName: script, contentWorld: .defaultClient, asFunction: false)
  }

  func handleAlert<T: JSAlertInfo>(webView: WKWebView, alert: inout T, completionHandler: @escaping () -> Void) {
    guard let promptingTab = tabManager[webView], !promptingTab.blockAllAlerts else {
      suppressJSAlerts(webView: webView)
      tabManager[webView]?.cancelQueuedAlerts()
      completionHandler()
      return
    }
    promptingTab.alertShownCount += 1
    let suppressBlock: JSAlertInfo.SuppressHandler = { [unowned self] suppress in
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
        suppressSheet.addAction(
          UIAlertAction(
            title: Strings.cancelButtonTitle, style: .cancel,
            handler: { _ in
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
  public func webView(_ webView: WKWebView, didFailProvisionalNavigation navigation: WKNavigation!, withError error: Error) {
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
        updateToolbarCurrentURL(tab.url?.displayURL)
        updateWebViewPageZoom(tab: tab)
      }
      return
    }

    if let url = error.userInfo[NSURLErrorFailingURLErrorKey] as? URL {
      ErrorPageHelper(certStore: profile.certStore).loadPage(error, forUrl: url, inWebView: webView)

      // If the local web server isn't working for some reason (Firefox cellular data is
      // disabled in settings, for example), we'll fail to load the session restore URL.
      // We rely on loading that page to get the restore callback to reset the restoring
      // flag, so if we fail to load that page, reset it here.
      if InternalURL(url)?.aboutComponent == "sessionrestore" {
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

  public func webView(_ webView: WKWebView, contextMenuConfigurationForElement elementInfo: WKContextMenuElementInfo, completionHandler: @escaping (UIContextMenuConfiguration?) -> Void) {

    guard let url = elementInfo.linkURL else { return completionHandler(UIContextMenuConfiguration(identifier: nil, previewProvider: nil, actionProvider: nil)) }

    let actionProvider: UIContextMenuActionProvider = { _ -> UIMenu? in
      var actions = [UIAction]()

      if let currentTab = self.tabManager.selectedTab {
        let tabType = currentTab.type

        if !tabType.isPrivate {
          let openNewTabAction = UIAction(
            title: Strings.openNewTabButtonTitle,
            image: UIImage(systemName: "plus")
          ) { _ in
            self.addTab(url: url, inPrivateMode: false, currentTab: currentTab)
          }

          openNewTabAction.accessibilityLabel = "linkContextMenu.openInNewTab"

          actions.append(openNewTabAction)
        }

        let openNewPrivateTabAction = UIAction(
          title: Strings.openNewPrivateTabButtonTitle,
          image: UIImage(named: "private_glasses", in: .current, compatibleWith: nil)!.template
        ) { _ in
          self.addTab(url: url, inPrivateMode: true, currentTab: currentTab)
        }
        openNewPrivateTabAction.accessibilityLabel = "linkContextMenu.openInNewPrivateTab"

        actions.append(openNewPrivateTabAction)

        let copyAction = UIAction(
          title: Strings.copyLinkActionTitle,
          image: UIImage(systemName: "doc.on.doc")
        ) { _ in
          UIPasteboard.general.url = url
        }
        copyAction.accessibilityLabel = "linkContextMenu.copyLink"

        actions.append(copyAction)

        if let braveWebView = webView as? BraveWebView {
          let shareAction = UIAction(
            title: Strings.shareLinkActionTitle,
            image: UIImage(systemName: "square.and.arrow.up")
          ) { _ in
            let touchPoint = braveWebView.lastHitPoint
            let touchRect = CGRect(origin: touchPoint, size: .zero)

            // TODO: Find a way to add fixes #3323 and #2961 here:
            // Normally we use `tab.temporaryDocument` for the downloaded file on the tab.
            // `temporaryDocument` returns the downloaded file to disk on the current tab.
            // Using a downloaded file url results in having functions like "Save to files" available.
            // It also attaches the file (image, pdf, etc) and not the url to emails, slack, etc.
            // Since this is **not** a tab but a standalone web view, the downloaded temporary file is **not** available.
            // This results in the fixes for #3323 and #2961 not being included in this share scenario.
            // This is not a regression, we simply never handled this scenario in both fixes.
            // Some possibile fixes include:
            // - Detect the file type and download it if necessary and don't rely on the `tab.temporaryDocument`.
            // - Add custom "Save to file" functionality (needs investigation).
            self.presentActivityViewController(
              url, sourceView: braveWebView,
              sourceRect: touchRect,
              arrowDirection: .any)
          }

          shareAction.accessibilityLabel = "linkContextMenu.share"

          actions.append(shareAction)
        }

        let linkPreview = Preferences.General.enableLinkPreview.value

        let linkPreviewTitle = linkPreview ? Strings.hideLinkPreviewsActionTitle : Strings.showLinkPreviewsActionTitle
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
    let config = UIContextMenuConfiguration(
      identifier: nil, previewProvider: linkPreviewProvider,
      actionProvider: actionProvider)

    completionHandler(config)
  }

  public func webView(_ webView: WKWebView, contextMenuForElement elementInfo: WKContextMenuElementInfo, willCommitWithAnimator animator: UIContextMenuInteractionCommitAnimating) {
    guard let url = elementInfo.linkURL else { return }
    webView.load(URLRequest(url: url))
  }

  fileprivate func addTab(url: URL, inPrivateMode: Bool, currentTab: Tab) {
    let tab = self.tabManager.addTab(URLRequest(url: url), afterTab: currentTab, isPrivate: inPrivateMode)
    if inPrivateMode && !PrivateBrowsingManager.shared.isPrivateBrowsing {
      self.tabManager.selectTab(tab)
    } else {
      // We're not showing the top tabs; show a toast to quick switch to the fresh new tab.
      let toast = ButtonToast(
        labelText: Strings.contextMenuButtonToastNewTabOpenedLabelText, buttonText: Strings.contextMenuButtonToastNewTabOpenedButtonText,
        completion: { buttonPressed in
          if buttonPressed {
            self.tabManager.selectTab(tab)
          }
        })
      self.show(toast: toast)
    }
    self.toolbarVisibilityViewModel.toolbarState = .expanded
  }
}

// MARK: - UIPopoverPresentationControllerDelegate

extension BrowserViewController: UIPopoverPresentationControllerDelegate {
  public func popoverPresentationControllerDidDismissPopover(_ popoverPresentationController: UIPopoverPresentationController) {
    displayedPopoverController = nil
    updateDisplayedPopoverProperties = nil
  }
}

extension BrowserViewController: UIAdaptivePresentationControllerDelegate {
  // Returning None here makes sure that the Popover is actually presented as a Popover and
  // not as a full-screen modal, which is the default on compact device classes.
  public func adaptivePresentationStyle(for controller: UIPresentationController, traitCollection: UITraitCollection) -> UIModalPresentationStyle {
    return .none
  }
}

extension BrowserViewController: SessionRestoreHelperDelegate {
  func sessionRestoreHelper(_ helper: SessionRestoreHelper, didRestoreSessionForTab tab: Tab) {
    tab.restoring = false

    if let tab = tabManager.selectedTab {
      updateUIForReaderHomeStateForTab(tab)
    }
  }
}

extension BrowserViewController: TabTrayDelegate {
  func tabOrderChanged() {
    tabsBar.updateData()
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

    if let delegate = webView.findInPageDelegate {
      let backwards = function == TextSearchDirection.previous.rawValue

      delegate.find(string: text, backwards: backwards) { [weak self] index, total in
        guard let self = self else { return }
        self.findInPageBar?.totalResults = Int(total)
        self.findInPageBar?.currentResult = index
      }
    } else {
      webView.evaluateSafeJavaScript(functionName: "__firefox__.\(function)", args: [text], contentWorld: .defaultClient)
    }
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
      updateURLBarWalletButton()
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
        let toast = ButtonToast(
          labelText: Strings.contextMenuButtonToastNewTabOpenedLabelText, buttonText: Strings.contextMenuButtonToastNewTabOpenedButtonText,
          completion: { buttonPressed in
            if buttonPressed {
              self.tabManager.selectTab(tab)
            }
          })
        show(toast: toast)
      }
      updateURLBarWalletButton()
    case .copy:
      UIPasteboard.general.url = url
    case .share:
      if presentedViewController != nil {
        dismiss(animated: true) {
          self.presentActivityViewController(
            url,
            sourceView: self.view,
            sourceRect: self.view.convert(self.topToolbar.shareButton.frame, from: self.topToolbar.shareButton.superview),
            arrowDirection: [.up]
          )
        }
      } else {
        presentActivityViewController(
          url,
          sourceView: view,
          sourceRect: view.convert(topToolbar.shareButton.frame, from: topToolbar.shareButton.superview),
          arrowDirection: [.up]
        )
      }
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
      let editPopup =
        UIAlertController
        .userTextInputAlert(
          title: Strings.editBookmark,
          message: urlString,
          startingText: title, startingText2: favorite.url,
          placeholder2: urlString,
          keyboardType2: .URL
        ) { callbackTitle, callbackUrl in
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

      self.presentActivityViewController(
        url, sourceView: self.view, sourceRect: viewRect,
        arrowDirection: .any)
    }
  }
}

extension BrowserViewController: PreferencesObserver {
  public func preferencesDidChange(for key: String) {
    switch key {
    case Preferences.General.tabBarVisibility.key:
      updateTabsBarVisibility()
    case Preferences.Privacy.privateBrowsingOnly.key:
      PrivateBrowsingManager.shared.isPrivateBrowsing = Preferences.Privacy.privateBrowsingOnly.value
      setupTabs()
      updateTabsBarVisibility()
      updateApplicationShortcuts()
    case Preferences.General.alwaysRequestDesktopSite.key:
      tabManager.reset()
      tabManager.reloadSelectedTab()
    case Preferences.General.enablePullToRefresh.key:
      tabManager.selectedTab?.updatePullToRefreshVisibility()
    case Preferences.Shields.blockAdsAndTracking.key,
      Preferences.Shields.blockScripts.key,
      Preferences.Shields.blockPhishingAndMalware.key,
      Preferences.Shields.blockImages.key,
      Preferences.Shields.fingerprintingProtection.key,
      Preferences.Shields.useRegionAdBlock.key:
      tabManager.allTabs.forEach { $0.webView?.reload() }
    case Preferences.General.defaultPageZoomLevel.key:
      tabManager.allTabs.forEach({
        guard let url = $0.webView?.url else { return }
        let zoomLevel = PrivateBrowsingManager.shared.isPrivateBrowsing ? 1.0 : Domain.getPersistedDomain(for: url)?.zoom_level?.doubleValue ?? Preferences.General.defaultPageZoomLevel.value
        $0.webView?.setValue(zoomLevel, forKey: PageZoomView.propertyName)
      })
    case Preferences.Shields.httpsEverywhere.key:
      tabManager.reset()
      tabManager.reloadSelectedTab()
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
    case Preferences.Playlist.webMediaSourceCompatibility.key:
      if UIDevice.isIpad {
        tabManager.allTabs.forEach {
          $0.userScriptManager?.isWebCompatibilityMediaSourceAPIEnabled = Preferences.Playlist.webMediaSourceCompatibility.value
          $0.webView?.reload()
        }
      }
    case Preferences.General.mediaAutoBackgrounding.key:
      tabManager.allTabs.forEach {
        $0.userScriptManager?.isMediaBackgroundPlaybackEnabled = Preferences.General.mediaAutoBackgrounding.value
        $0.webView?.reload()
      }
    case Preferences.Playlist.enablePlaylistMenuBadge.key,
      Preferences.Playlist.enablePlaylistURLBarButton.key:
      let selectedTab = tabManager.selectedTab
      updatePlaylistURLBar(
        tab: selectedTab,
        state: selectedTab?.playlistItemState ?? .none,
        item: selectedTab?.playlistItem)
    case Preferences.PrivacyReports.captureShieldsData.key:
      PrivacyReportsManager.scheduleProcessingBlockedRequests()
      PrivacyReportsManager.scheduleNotification(debugMode: !AppConstants.buildChannel.isPublic)
    case Preferences.PrivacyReports.captureVPNAlerts.key:
      PrivacyReportsManager.scheduleVPNAlertsTask()
    case Preferences.Wallet.defaultWallet.key:
      tabManager.reset()
      tabManager.reloadSelectedTab()
      notificationsPresenter.removeNotification(with: WalletNotification.Constant.id)
      WalletProviderPermissionRequestsManager.shared.cancelAllPendingRequests()
      let privateMode = PrivateBrowsingManager.shared.isPrivateBrowsing
      if let cryptoStore = CryptoStore.from(privateMode: privateMode) {
        cryptoStore.rejectAllPendingWebpageRequests()
      }
      updateURLBarWalletButton()
    default:
      log.debug("Received a preference change for an unknown key: \(key) on \(type(of: self))")
      break
    }
  }
}

extension BrowserViewController {
  public func openReferralLink(url: URL) {
    executeAfterSetup { [self] in
      openURLInNewTab(url, isPrivileged: false)
    }
  }

  public func handleNavigationPath(path: NavigationPath) {
    executeAfterSetup {
      NavigationPath.handle(nav: path, with: self)
    }
  }
}

extension BrowserViewController: UNUserNotificationCenterDelegate {
  public func userNotificationCenter(_ center: UNUserNotificationCenter, didReceive response: UNNotificationResponse, withCompletionHandler completionHandler: @escaping () -> Void) {
    if response.notification.request.identifier == Self.defaultBrowserNotificationId {
      guard let settingsUrl = URL(string: UIApplication.openSettingsURLString) else {
        log.error("Failed to unwrap iOS settings URL")
        return
      }
      UIApplication.shared.open(settingsUrl)
    } else if response.notification.request.identifier == PrivacyReportsManager.notificationID {
      openPrivacyReport()
    }
    completionHandler()
  }
}

// Privacy reports
extension BrowserViewController {
  public func openPrivacyReport() {
    if PrivateBrowsingManager.shared.isPrivateBrowsing {
      return
    }
    
    let host = UIHostingController(rootView: PrivacyReportsManager.prepareView())
    host.rootView.onDismiss = { [weak host] in
      host?.dismiss(animated: true)
    }
    
    host.rootView.openPrivacyReportsUrl = { [weak self] in
      guard let self = self else { return }
      let tab = self.tabManager.addTab(
        PrivilegedRequest(url: BraveUX.privacyReportsURL) as URLRequest,
        afterTab: self.tabManager.selectedTab,
        // Privacy Reports view is unavailable in private mode.
        isPrivate: false)
      self.tabManager.selectTab(tab)
    }
    
    self.present(host, animated: true)
  }
}
