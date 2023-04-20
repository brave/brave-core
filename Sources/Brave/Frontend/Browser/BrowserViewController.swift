/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import UIKit
import WebKit
import Shared
import Storage
import SnapKit
import SwiftyJSON
import Data
import BraveShared
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
import BraveNews
import Preferences
import os.log
#if canImport(BraveTalk)
import BraveTalk
#endif
import Favicon
import Onboarding
import Growth
import BraveShields
import CertificateUtilities

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

public class BrowserViewController: UIViewController {
  let webViewContainer = UIView()
  private(set) lazy var screenshotHelper = ScreenshotHelper(tabManager: tabManager)
  
  private(set) lazy var topToolbar: TopToolbarView = {
    // Setup the URL bar, wrapped in a view to get transparency effect
    let topToolbar = TopToolbarView()
    topToolbar.translatesAutoresizingMaskIntoConstraints = false
    topToolbar.delegate = self
    topToolbar.tabToolbarDelegate = self

    let toolBarInteraction = UIContextMenuInteraction(delegate: self)
    topToolbar.locationView.addInteraction(toolBarInteraction)
    return topToolbar
  }()
  
  private(set) lazy var tabsBar: TabsBarViewController = {
    let tabsBar = TabsBarViewController(tabManager: tabManager)
    tabsBar.delegate = self
    return tabsBar
  }()

  // These views wrap the top and bottom toolbars to provide background effects on them
  let header = HeaderContainerView()
  private let headerHeightLayoutGuide = UILayoutGuide()
  
  let footer: UIView = {
    let footer = UIView()
    footer.translatesAutoresizingMaskIntoConstraints = false
    return footer
  }()
  
  private let topTouchArea: UIButton = {
    let topTouchArea = UIButton()
    topTouchArea.isAccessibilityElement = false
    return topTouchArea
  }()
  
  private let bottomTouchArea: UIButton = {
    let bottomTouchArea = UIButton()
    bottomTouchArea.isAccessibilityElement = false
    return bottomTouchArea
  }()
  
  /// These constraints allow to show/hide tabs bar
  private var webViewContainerTopOffset: Constraint?

  /// Backdrop used for displaying greyed background for private tabs
  private let webViewContainerBackdrop: UIView = {
    let webViewContainerBackdrop = UIView()
    webViewContainerBackdrop.backgroundColor = .braveBackground
    webViewContainerBackdrop.alpha = 0
    return webViewContainerBackdrop
  }()
  
  var readerModeBar: ReaderModeBarView?
  var readerModeCache: ReaderModeCache
  
  private let statusBarOverlay: UIView = {
    // Temporary work around for covering the non-clipped web view content
    let statusBarOverlay = UIView()
    statusBarOverlay.backgroundColor = Preferences.General.nightModeEnabled.value ? .nightModeBackground : .urlBarBackground
    return statusBarOverlay
  }()
  
  private(set) var toolbar: BottomToolbarView?
  var searchLoader: SearchLoader?
  var searchController: SearchViewController?
  var favoritesController: FavoritesViewController?
  
  /// All content that appears above the footer should be added to this view. (Find In Page/SnackBars)
  let alertStackView: UIStackView = {
    let alertStackView = UIStackView()
    alertStackView.axis = .vertical
    alertStackView.alignment = .center
    return alertStackView
  }()
  
  var findInPageBar: FindInPageBar?
  var pageZoomBar: UIHostingController<PageZoomView>?
  private var pageZoomListener: NSObjectProtocol?
  private var openTabsModelStateListener: SendTabToSelfModelStateListener?
  private var syncServiceStateListener: AnyObject?
  let collapsedURLBarView = CollapsedURLBarView()

  // Single data source used for all favorites vcs
  public let backgroundDataSource = NTPDataSource()
  let feedDataSource = FeedDataSource()

  private var postSetupTasks: [() -> Void] = []
  private var setupTasksCompleted: Bool = false

  private var privateModeCancellable: AnyCancellable?
  private var appReviewCancelable: AnyCancellable?
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
  private let crashedLastSession: Bool
  
  // A view to place behind the bottom bar down to the toolbar during keyboard animations to avoid
  // the odd look for the URL bar floating
  private let bottomBarKeyboardBackground = UIView().then {
    $0.isUserInteractionEnabled = false
  }

  var toolbarVisibilityViewModel = ToolbarVisibilityViewModel(estimatedTransitionDistance: 44)
  private var toolbarLayoutGuide = UILayoutGuide().then {
    $0.identifier = "toolbar-visibility-layout-guide"
  }
  private var toolbarTopConstraint: Constraint?
  private var toolbarBottomConstraint: Constraint?
  var toolbarVisibilityCancellable: AnyCancellable?

  var keyboardState: KeyboardState?

  var pendingToast: Toast?  // A toast that might be waiting for BVC to appear before displaying
  var downloadToast: DownloadToast?  // A toast that is showing the combined download progress
  var addToPlayListActivityItem: (enabled: Bool, item: PlaylistInfo?)?  // A boolean to determine If AddToListActivity should be added
  var openInPlaylistActivityItem: (enabled: Bool, item: PlaylistInfo?)?  // A boolean to determine if OpenInPlaylistActivity should be shown

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

  private var cancellables: Set<AnyCancellable> = []

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
  /// The string domain will be kept temporarily which is tracking site notification presented
  /// in order to not to process site list again and again
  var currentBenchmarkWebsite = ""

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
  var widgetFaviconFetchers: [Task<Favicon, Error>] = []
  let deviceCheckClient: DeviceCheckClient?
  
  #if canImport(BraveTalk)
  // Brave Talk native implementations
  let braveTalkJitsiCoordinator = BraveTalkJitsiCoordinator()
  #endif

  /// The currently open WalletStore
  weak var walletStore: WalletStore?
  
  var lastEnteredURLVisitType: VisitType = .unknown
  
  var processAddressBarTask: Task<(), Never>?
  var topToolbarDidPressReloadTask: Task<(), Never>?

  public init(
    profile: Profile,
    diskImageStore: DiskImageStore?,
    braveCore: BraveCoreMain,
    migration: Migration?,
    crashedLastSession: Bool
  ) {
    self.profile = profile
    self.braveCore = braveCore
    self.bookmarkManager = BookmarkManager(bookmarksAPI: braveCore.bookmarksAPI)
    self.migration = migration
    self.crashedLastSession = crashedLastSession
    feedDataSource.historyAPI = braveCore.historyAPI
    
    let configuration: BraveRewards.Configuration = .current()

    let buildChannel = BraveAds.BuildChannelInfo().then {
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
      rewards: rewards,
      tabGeneratorAPI: braveCore.tabGeneratorAPI)
    
    // Add Regular tabs to Sync Chain
    if Preferences.Chromium.syncOpenTabsEnabled.value {
      tabManager.addRegularTabsToSyncChain()
    }
    
    // Remove outdated Recently Closed tabs
    tabManager.deleteOutdatedRecentlyClosed()

    // Setup ReaderMode Cache
    self.readerModeCache = ReaderModeScriptHandler.cache(for: tabManager.selectedTab)

    if !BraveRewards.isAvailable {
      // Disable rewards services in case previous user already enabled
      // rewards in previous build
      rewards.isEnabled = false
    } else {
      if rewards.isEnabled && !Preferences.Rewards.rewardsToggledOnce.value {
        Preferences.Rewards.rewardsToggledOnce.value = true
      }
    }

    self.deviceCheckClient = DeviceCheckClient(environment: configuration.environment)

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

    feedDataSource.ads = rewards.ads
    
    // Observer watching tab information is sent by another device
    openTabsModelStateListener = braveCore.sendTabAPI.add(
      SendTabToSelfStateObserver { [weak self] stateChange in
        if case .sendTabToSelfEntriesAddedRemotely(let newEntries) = stateChange {
          // Fetching the last URL that has been sent from synced sessions
          if let requestedURL = newEntries.last?.url {
            self?.presentTabReceivedToast(url: requestedURL)
          }
        }
      })
    
    // Observer watching state change in sync chain
    syncServiceStateListener = braveCore.syncAPI.addServiceStateObserver { [weak self] in
      guard let self = self else { return }
      // Observe Sync State in order to determine if the sync chain is deleted
      // from another device - Clean local sync chain
      if self.braveCore.syncAPI.shouldLeaveSyncGroup {
        self.braveCore.syncAPI.leaveSyncGroup()
      }
    }
  }

  deinit {
    // Remove the open tabs model state observer
    if let observer = openTabsModelStateListener {
      braveCore.sendTabAPI.removeObserver(observer)
    }
  }
  
  static func legacyWallet(for config: BraveRewards.Configuration) -> BraveLedger? {
    let fm = FileManager.default
    let stateStorage = config.storageURL
    let legacyLedger = stateStorage.appendingPathComponent("legacy_ledger")

    // Check if we've already migrated the users wallet to the `legacy_rewards` folder
    if fm.fileExists(atPath: legacyLedger.path) {
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
      return BraveLedger(stateStoragePath: legacyLedger.path)
    } catch {
      adsRewardsLog.error("Failed to migrate legacy wallet into a new folder: \(error.localizedDescription)")
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
#if canImport(BraveTalk)
        self.braveTalkJitsiCoordinator.resetPictureInPictureBounds(.init(size: size))
#endif
      },
      completion: { _ in
        if let tab = self.tabManager.selectedTab {
          WindowRenderScriptHandler.executeScript(for: tab)
        }
      })
  }

  override public func didReceiveMemoryWarning() {
    super.didReceiveMemoryWarning()
    ScriptFactory.shared.clearCaches()
    AdBlockStats.shared.clearCaches()

    for tab in tabManager.tabsForCurrentMode where tab.id != tabManager.selectedTab?.id {
      tab.newTabPageViewController = nil
    }
  }

  private var rewardsEnabledObserveration: NSKeyValueObservation?

  fileprivate func didInit() {
    updateApplicationShortcuts()
    tabManager.addDelegate(self)
    tabManager.addNavigationDelegate(self)
    tabManager.makeWalletEthProvider = { [weak self] tab in
      guard let self = self,
            let provider = self.braveCore.braveWalletAPI.ethereumProvider(with: tab, isPrivateBrowsing: tab.isPrivate),
            let js = self.braveCore.braveWalletAPI.providerScripts(for: .eth)[.ethereum] else {
        return nil
      }
      return (provider, js: js)
    }
    tabManager.makeWalletSolProvider = { [weak self] tab in
      guard let self = self,
            let provider = self.braveCore.braveWalletAPI.solanaProvider(with: tab, isPrivateBrowsing: tab.isPrivate) else {
        return nil
      }
      let scripts = self.braveCore.braveWalletAPI.providerScripts(for: .sol)
      return (provider, jsScripts: scripts)
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
    Preferences.Playlist.syncSharedFoldersAutomatically.observe(from: self)
    
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
    Preferences.Wallet.defaultEthWallet.observe(from: self)

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

    // Eliminate the older usage days
    // Used in App Rating criteria
    AppReviewManager.shared.processMainCriteria(for: .daysInUse)
    
    maybeRecordInitialShieldsP3A()
    recordVPNUsageP3A(vpnEnabled: BraveVPN.isConnected)
    
    // Revised Review Handling
    AppReviewManager.shared.handleAppReview(for: .revisedCrossPlatform, using: self)
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
  
  private func updateUsingBottomBar(using traitCollection: UITraitCollection) {
    isUsingBottomBar = Preferences.General.isUsingBottomBar.value &&
    traitCollection.horizontalSizeClass == .compact &&
    traitCollection.verticalSizeClass == .regular &&
    traitCollection.userInterfaceIdiom == .phone
  }

  public override func viewSafeAreaInsetsDidChange() {
    super.viewSafeAreaInsetsDidChange()
    
    topTouchArea.isEnabled = view.safeAreaInsets.top > 0
    statusBarOverlay.isHidden = view.safeAreaInsets.top.isZero
  }
  
  fileprivate func updateToolbarStateForTraitCollection(_ newCollection: UITraitCollection, withTransitionCoordinator coordinator: UIViewControllerTransitionCoordinator? = nil) {
    let showToolbar = shouldShowFooterForTraitCollection(newCollection)

    topToolbar.setShowToolbar(!showToolbar)
    toolbar?.removeFromSuperview()
    toolbar?.tabToolbarDelegate = nil
    toolbar = nil
    bottomTouchArea.isEnabled = showToolbar

    if showToolbar {
      toolbar = BottomToolbarView()
      toolbar?.setSearchButtonState(url: tabManager.selectedTab?.url)
      footer.addSubview(toolbar!)
      toolbar?.tabToolbarDelegate = self
      toolbar?.menuButton.setBadges(Array(topToolbar.menuButton.badges.keys))
    }
    updateToolbarUsingTabManager(tabManager)
    updateUsingBottomBar(using: newCollection)
    
    view.setNeedsUpdateConstraints()

    if let tab = tabManager.selectedTab,
      let webView = tab.webView {
      updateURLBar()
      navigationToolbar.updateBackStatus(webView.canGoBack)
      navigationToolbar.updateForwardStatus(webView.canGoForward)
      topToolbar.locationView.loading = tab.loading
    }

    toolbarVisibilityViewModel.toolbarState = .expanded
    updateTabsBarVisibility()
  }
  
  private func updateToolbarSecureContentState(_ secureContentState: TabSecureContentState) {
    topToolbar.secureContentState = secureContentState
    collapsedURLBarView.secureContentState = secureContentState
  }
  
  func updateToolbarCurrentURL(_ currentURL: URL?) {
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
          self.bottomBarKeyboardBackground.backgroundColor = self.topToolbar.backgroundColor
          self.setNeedsStatusBarAppearanceUpdate()
        }
      },
      completion: { _ in
        if let tab = self.tabManager.selectedTab {
          WindowRenderScriptHandler.executeScript(for: tab)
        }
      })
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
  
  @objc func appWillTerminateNotification() {
    tabManager.saveAllTabs()
  }
  
  @objc private func tappedCollapsedURLBar() {
    if keyboardState != nil && isUsingBottomBar && !topToolbar.inOverlayMode {
      view.endEditing(true)
    } else {
      tappedTopArea()
    }
  }

  @objc func tappedTopArea() {
    toolbarVisibilityViewModel.toolbarState = .expanded
  }

  @objc func appWillResignActiveNotification() {
    tabManager.saveAllTabs()
    
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
      header.contentView.alpha = 0
      presentedViewController?.popoverPresentationController?.containerView?.alpha = 0
      presentedViewController?.view.alpha = 0
    }
  }

  @objc func vpnConfigChanged() {
    // Load latest changes to the vpn.
    NEVPNManager.shared().loadFromPreferences { _ in }
    
    if case .purchased(let enabled) = BraveVPN.vpnState, enabled {
      recordVPNUsageP3A(vpnEnabled: true)
    }
  }

  @objc func appDidBecomeActiveNotification() {
    guard let tab = tabManager.selectedTab, tab.isPrivate else {
      return
    }
    // Re-show any components that might have been hidden because they were being displayed
    // as part of a private mode tab
    UIView.animate(
      withDuration: 0.2, delay: 0, options: UIView.AnimationOptions(),
      animations: {
        self.webViewContainer.alpha = 1
        self.header.contentView.alpha = 1
        self.presentedViewController?.popoverPresentationController?.containerView?.alpha = 1
        self.presentedViewController?.view.alpha = 1
        self.view.backgroundColor = .clear
      },
      completion: { _ in
        self.webViewContainerBackdrop.alpha = 0
      })
  }
  
  private(set) var isUsingBottomBar: Bool = false {
    didSet {
      header.isUsingBottomBar = isUsingBottomBar
      collapsedURLBarView.isUsingBottomBar = isUsingBottomBar
      searchController?.isUsingBottomBar = isUsingBottomBar
      bottomBarKeyboardBackground.isHidden = !isUsingBottomBar
      topToolbar.displayTabTraySwipeGestureRecognizer?.isEnabled = isUsingBottomBar
      updateTabsBarVisibility()
      updateViewConstraints()
    }
  }

  override public func viewDidLoad() {
    super.viewDidLoad()
    view.backgroundColor = .braveBackground
    
    // Add layout guides
    view.addLayoutGuide(pageOverlayLayoutGuide)
    view.addLayoutGuide(headerHeightLayoutGuide)
    view.addLayoutGuide(toolbarLayoutGuide)
    
    // Add views
    view.addSubview(webViewContainerBackdrop)
    view.addSubview(webViewContainer)
    header.expandedBarStackView.addArrangedSubview(topToolbar)
    header.collapsedBarContainerView.addSubview(collapsedURLBarView)
    
    addChild(tabsBar)
    tabsBar.didMove(toParent: self)

    view.addSubview(alertStackView)
    view.addSubview(bottomTouchArea)
    view.addSubview(topTouchArea)
    view.addSubview(bottomBarKeyboardBackground)
    view.addSubview(footer)
    view.addSubview(header)
    view.addSubview(statusBarOverlay)
    
    // For now we hide some elements so they are not visible
    header.isHidden = true
    footer.isHidden = true
    
    // Setup constraints
    setupConstraints()
    updateToolbarStateForTraitCollection(self.traitCollection)
    
    // Legacy Review Handling
    AppReviewManager.shared.handleAppReview(for: .legacy, using: self)
    
    // Adding Screenshot Service Delegate to browser to fetch full screen webview screenshots
    currentScene?.screenshotService?.delegate = self
    
    self.setupInteractions()
  }
  
  private func setupInteractions() {
    // We now show some elements since we're ready to use the app
    header.isHidden = false
    footer.isHidden = false
    
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
        self, selector: #selector(appWillTerminateNotification),
        name: UIApplication.willTerminateNotification, object: nil)
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
    
    BraveGlobalShieldStats.shared.$adblock
      .scan((BraveGlobalShieldStats.shared.adblock, BraveGlobalShieldStats.shared.adblock), { ($0.1, $1) })
      .sink { [weak self] (oldValue, newValue) in
        let change = newValue - oldValue
        if change > 0 {
          self?.recordDataSavedP3A(change: change)
        }
      }
      .store(in: &cancellables)
    
    KeyboardHelper.defaultHelper.addDelegate(self)
    UNUserNotificationCenter.current().delegate = self
    
    // Add interactions
    topTouchArea.addTarget(self, action: #selector(tappedTopArea), for: .touchUpInside)
    bottomTouchArea.addTarget(self, action: #selector(tappedTopArea), for: .touchUpInside)
    header.collapsedBarContainerView.addTarget(self, action: #selector(tappedCollapsedURLBar), for: .touchUpInside)
    updateRewardsButtonState()

    // Setup UIDropInteraction to handle dragging and dropping
    // links into the view from other apps.
    let dropInteraction = UIDropInteraction(delegate: self)
    view.addInteraction(dropInteraction)
    topToolbar.addInteraction(dropInteraction)

    // Adding a small delay before fetching gives more reliability to it,
    // epsecially when you are connected to a VPN.
    DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
      self.vpnProductInfo.load()
      if let customCredential = Preferences.VPN.skusCredential.value,
         let customCredentialDomain = Preferences.VPN.skusCredentialDomain.value,
          let vpnCredential = BraveSkusWebHelper.fetchVPNCredential(customCredential, domain: customCredentialDomain) {
        BraveVPN.initialize(customCredential: vpnCredential)
      } else {
        BraveVPN.initialize(customCredential: nil)
      }
    }

    if !Preferences.DefaultBrowserIntro.defaultBrowserNotificationScheduled.value {
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
        self?.bottomBarKeyboardBackground.backgroundColor = self?.statusBarOverlay.backgroundColor
      })
    
    appReviewCancelable = AppReviewManager.shared
      .$isRevisedReviewRequired
      .removeDuplicates()
      .sink(receiveValue: { [weak self] isRevisedReviewRequired in
        guard let self = self else { return }
        if isRevisedReviewRequired {
          AppReviewManager.shared.isRevisedReviewRequired = false
          
          // Handle App Rating
          // User made changes to the Brave News sources (tapped close)
          AppReviewManager.shared.handleAppReview(for: .revised, using: self)
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
        self?.bottomBarKeyboardBackground.backgroundColor = self?.statusBarOverlay.backgroundColor
      }
      .store(in: &cancellables)
    
    Preferences.General.isUsingBottomBar.objectWillChange
      .receive(on: RunLoop.main)
      .sink { [weak self] _ in
        guard let self = self else { return }
        self.updateTabsBarVisibility()
        self.updateUsingBottomBar(using: self.traitCollection)
      }
      .store(in: &cancellables)
    
    syncPlaylistFolders()
    checkCrashRestorationOrSetupTabs()
  }

  public static let defaultBrowserNotificationId = "defaultBrowserNotification"

  private func scheduleDefaultBrowserNotification() {
    let center = UNUserNotificationCenter.current()

    center.requestAuthorization(options: [.provisional, .alert, .sound, .badge]) { granted, error in
      if let error = error {
        Logger.module.error("Failed to request notifications permissions: \(error.localizedDescription, privacy: .public)")
        return
      }

      if !granted {
        Logger.module.info("Not authorized to schedule a notification")
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
            Logger.module.error("Failed to add notification: \(error.localizedDescription, privacy: .public)")
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

  private func setupTabs() {
    let isPrivate = Preferences.Privacy.privateBrowsingOnly.value
    let noTabsAdded = self.tabManager.tabsForCurrentMode.isEmpty
    
    var tabToSelect: Tab?
    
    if noTabsAdded {
      // Two scenarios if there are no tabs in tabmanager:
      // 1. We have not restored tabs yet, attempt to restore or make a new tab if there is nothing.
      // 2. We are in private browsing mode and need to add a new private tab.
      tabToSelect = isPrivate ? self.tabManager.addTab(isPrivate: true) : self.tabManager.restoreAllTabs
    } else {
      if let selectedTab = tabManager.selectedTab, !selectedTab.isPrivate {
        tabToSelect = selectedTab
      } else {
        tabToSelect = tabManager.tabsForCurrentMode.last
      }
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

  private func setupConstraints() {
    toolbarLayoutGuide.snp.makeConstraints {
      self.toolbarTopConstraint = $0.top.equalTo(view.safeArea.top).constraint
      self.toolbarBottomConstraint = $0.bottom.equalTo(view).constraint
      $0.leading.trailing.equalTo(view)
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
      make.height.equalTo(32)
    }

    bottomTouchArea.snp.makeConstraints { make in
      make.bottom.left.right.equalTo(self.view)
      make.height.equalTo(44)
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
    // if the toolbars can collapse. We don't subtract the bottom safe area inset because the footer includes
    // that safe area
    toolbarVisibilityViewModel.minimumCollapsableContentHeight = view.bounds.height - view.safeAreaInsets.top
    
    var additionalInsets: UIEdgeInsets = .zero
    if isUsingBottomBar {
      additionalInsets = .init(top: 0, left: 0, bottom: topToolbar.bounds.height, right: 0)
    } else {
      additionalInsets = .init(top: header.bounds.height, left: 0, bottom: 0, right: 0)
    }
    searchController?.additionalSafeAreaInsets = additionalInsets
    favoritesController?.additionalSafeAreaInsets = additionalInsets
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
    updateToolbarUsingTabManager(tabManager)

    if let tabId = tabManager.selectedTab?.rewardsId, rewards.ledger?.selectedTabId == 0 {
      rewards.ledger?.selectedTabId = tabId
    }
  }

  private func checkCrashRestorationOrSetupTabs() {
    if crashedLastSession {
      showRestoreTabsAlert()
    } else {
      setupTabs()
    }
  }

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
        SessionTab.deleteAll()
        self.tabManager.addTabAndSelect(isPrivate: PrivateBrowsingManager.shared.isPrivateBrowsing)
      }
    )
    self.present(alert, animated: true, completion: nil)
  }

  fileprivate func canRestoreTabs() -> Bool {
    // Make sure there's at least one real tab open
    return !SessionTab.all().compactMap({ $0.url }).isEmpty
  }

  override public func viewDidAppear(_ animated: Bool) {
    // Passcode Migration has highest priority, it should be presented over everything else
    presentPassCodeMigration()

    // Present Onboarding to new users, existing users will not see the onboarding
    presentOnboardingIntro()

    // Full Screen Callout Presentation
    DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
      self.presentFullScreenCallouts()
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

  /// A layout guide defining where the favorites and NTP overlay are placed
  let pageOverlayLayoutGuide = UILayoutGuide()

  override public func updateViewConstraints() {
    readerModeBar?.snp.remakeConstraints { make in
      if self.isUsingBottomBar {
        make.top.equalTo(self.view.safeArea.top)
      } else {
        make.top.equalTo(self.header.snp.bottom)
      }
      make.height.equalTo(UIConstants.toolbarHeight)
      make.leading.trailing.equalTo(self.view)
    }
    
    webViewContainer.snp.remakeConstraints { make in
      make.left.right.equalTo(self.view)
      
      if self.isUsingBottomBar {
        webViewContainerTopOffset = make.top.equalTo(self.readerModeBar?.snp.bottom ?? self.toolbarLayoutGuide.snp.top).constraint
      } else {
        webViewContainerTopOffset = make.top.equalTo(self.readerModeBar?.snp.bottom ?? self.header.snp.bottom).constraint
      }

      let findInPageHeight = (findInPageBar == nil) ? 0 : UIConstants.toolbarHeight
      if self.isUsingBottomBar {
        make.bottom.equalTo(self.header.snp.top).offset(-findInPageHeight)
      } else {
        make.bottom.equalTo(self.footer.snp.top).offset(-findInPageHeight)
      }
    }

    header.snp.remakeConstraints { make in
      if self.isUsingBottomBar {
        // Need to check Find In Page Bar is enabled in order to aligh it properly when bottom-bar is enabled
        if let keyboardHeight = keyboardState?.intersectionHeightForView(self.view), keyboardHeight > 0,
           (presentedViewController == nil || findInPageBar != nil) {
          var offset = -keyboardHeight
          if !topToolbar.inOverlayMode {
            // Showing collapsed URL bar while the keyboard is up
            offset += toolbarVisibilityViewModel.transitionDistance
          }
          make.bottom.equalTo(self.view).offset(offset)
        } else {
          if topToolbar.inOverlayMode {
            make.bottom.equalTo(self.view.safeArea.bottom)
          } else {
            make.bottom.equalTo(footer.snp.top)
          }
        }
      } else {
        make.top.equalTo(toolbarLayoutGuide)
      }
      make.left.right.equalTo(self.view)
    }
    
    headerHeightLayoutGuide.snp.remakeConstraints {
      if self.isUsingBottomBar {
        $0.bottom.equalTo(footer.snp.top)
      } else {
        $0.top.equalTo(toolbarLayoutGuide)
      }
      $0.height.equalTo(header)
      $0.leading.trailing.equalTo(self.view)
    }

    footer.snp.remakeConstraints { make in
      make.bottom.equalTo(toolbarLayoutGuide)
      make.leading.trailing.equalTo(self.view)
      let height = self.toolbar == nil ? 0 : UIConstants.bottomToolbarHeight
      make.height.equalTo(height)
    }

    bottomBarKeyboardBackground.snp.remakeConstraints {
      if self.isUsingBottomBar {
        $0.top.equalTo(header)
        $0.bottom.equalTo(footer)
      } else {
        $0.top.bottom.equalTo(footer)
      }
      $0.leading.trailing.equalToSuperview()
    }
    
    // Remake constraints even if we're already showing the home controller.
    // The home controller may change sizes if we tap the URL bar while on about:home.
    pageOverlayLayoutGuide.snp.remakeConstraints { make in
      if self.isUsingBottomBar {
        webViewContainerTopOffset = make.top.equalTo(readerModeBar?.snp.bottom ?? self.toolbarLayoutGuide).constraint
      } else {
        webViewContainerTopOffset = make.top.equalTo(readerModeBar?.snp.bottom ?? self.header.snp.bottom).constraint
      }

      make.left.right.equalTo(self.view)
      if self.isUsingBottomBar {
        make.bottom.equalTo(self.headerHeightLayoutGuide.snp.top)
      } else {
        make.bottom.equalTo(self.footer.snp.top)
      }
    }

    alertStackView.snp.remakeConstraints { make in
      make.centerX.equalTo(self.view)
      make.width.equalTo(self.view.safeArea.width)
      
      if let keyboardHeight = keyboardState?.intersectionHeightForView(self.view), keyboardHeight > 0 {
        if self.isUsingBottomBar {
          var offset = -keyboardHeight
          if !topToolbar.inOverlayMode {
            // Showing collapsed URL bar while the keyboard is up
            offset += toolbarVisibilityViewModel.transitionDistance
          }
          make.bottom.equalTo(header.snp.top)
        } else {
          make.bottom.equalTo(self.view).offset(-keyboardHeight)
        }
      } else if isUsingBottomBar {
        make.bottom.equalTo(header.snp.top)
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
      // Donate NewTabPage Activity For Custom Suggestions
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
      view.insertSubview(ntpController.view, belowSubview: header)
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
          let readerMode = tab.getContentScript(name: ReaderModeScriptHandler.scriptName) as? ReaderModeScriptHandler,
          readerMode.state == .active,
          isReaderModeURL {
          self.showReaderModeBar(animated: false)
          self.updatePlaylistURLBar(tab: tab, state: tab.playlistItemState, item: tab.playlistItem)
        }
      })
  }

  /// Shows a vpn screen based on vpn state.
  public func presentCorrespondingVPNViewController() {
    if BraveSkusManager.keepShowingSessionExpiredState {
      let alert = BraveSkusManager.sessionExpiredStateAlert(loginCallback: { [unowned self] _ in
        self.openURLInNewTab(.brave.account, isPrivate: PrivateBrowsingManager.shared.isPrivateBrowsing,
                             isPrivileged: false)
      })
      
      present(alert, animated: true)
      return
    }
    
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
      toolbar?.line.isHidden = isUsingBottomBar
    }
    
    header.expandedBarStackView.removeArrangedSubview(tabsBar.view)
    if isUsingBottomBar {
      header.expandedBarStackView.insertArrangedSubview(tabsBar.view, at: 0)
    } else {
      header.expandedBarStackView.addArrangedSubview(tabsBar.view)
    }

    if tabManager.selectedTab == nil {
      tabsBar.view.isHidden = true
      return
    }

    func shouldShowTabBar() -> Bool {
      if (topToolbar.inOverlayMode || keyboardState != nil) && isUsingBottomBar {
        return false
      }
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
        return (tabCount > 1 && UIDevice.current.orientation.isLandscape) || UIDevice.current.userInterfaceIdiom == .pad
      case .never:
        return false
      }
    }

    let isShowing = tabsBar.view.isHidden == false
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
            Logger.module.error("\(error.localizedDescription, privacy: .public)")
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

      // Recording the last Visit Type for the url submitted
      lastEnteredURLVisitType = visitType
      updateWebViewPageZoom(tab: tab)
    }
  }
  
  func showIPFSInterstitialPage(originalURL: URL, visitType: VisitType) {
    topToolbar.leaveOverlayMode()

    guard let tab = tabManager.selectedTab, let encodedURL = originalURL.absoluteString.addingPercentEncoding(withAllowedCharacters: .alphanumerics), let internalUrl = URL(string: "\(InternalURL.baseUrl)/\(IPFSSchemeHandler.path)?url=\(encodedURL)") else {
      return
    }
    let scriptHandler = tab.getContentScript(name: Web3IPFSScriptHandler.scriptName) as? Web3IPFSScriptHandler
    scriptHandler?.originalURL = originalURL
    scriptHandler?.visitType = visitType

    tab.webView?.load(PrivilegedRequest(url: internalUrl) as URLRequest)
  }

  func showWeb3ServiceInterstitialPage(service: Web3Service, originalURL: URL, visitType: VisitType = .unknown) {
    topToolbar.leaveOverlayMode()

    guard let tab = tabManager.selectedTab,
          let encodedURL = originalURL.absoluteString.addingPercentEncoding(withAllowedCharacters: .alphanumerics),
          let internalUrl = URL(string: "\(InternalURL.baseUrl)/\(Web3DomainHandler.path)?\(Web3NameServiceScriptHandler.ParamKey.serviceId.rawValue)=\(service.rawValue)&url=\(encodedURL)") else {
      return
    }
    let scriptHandler = tab.getContentScript(name: Web3NameServiceScriptHandler.scriptName) as? Web3NameServiceScriptHandler
    scriptHandler?.originalURL = originalURL
    scriptHandler?.visitType = visitType
    
    tab.webView?.load(PrivilegedRequest(url: internalUrl) as URLRequest)
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
      Logger.module.error("An object of type: \(String(describing: object), privacy: .public) is being observed instead of a WKWebView")
      return  // False alarm.. the source MUST be a web view.
    }

    // WebView is a zombie and somehow still has an observer attached to it
    guard let tab = tabManager[webView] else {
      Logger.module.error("WebView has been removed from TabManager but still has attached observers")
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
      if tab === tabManager.selectedTab {
        topToolbar.locationView.loading = tab.loading
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
      
      // Update the estimated progress when the URL changes. Estimated progress may update to 0.1 when the url
      // is still an internal URL even though a request may be pending for a web page.
      if tab === tabManager.selectedTab, let url = webView.url,
         !InternalURL.isValid(url: url), webView.estimatedProgress > 0 {
        topToolbar.updateProgressBar(Float(webView.estimatedProgress))
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
      
      let host = tab.webView?.url?.host
      
      Task {
        do {
          try await BraveCertificateUtils.evaluateTrust(serverTrust, for: host)
          tab.secureContentState = .secure
        } catch {
          tab.secureContentState = .insecure
        }
        
        self.updateURLBar()
      }
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
  func updateURLBar() {
    guard let tab = tabManager.selectedTab else { return }

    updateRewardsButtonState()

    DispatchQueue.main.async {
      if let item = tab.playlistItem {
        if PlaylistItem.itemExists(uuid: item.tagId) || PlaylistItem.itemExists(pageSrc: item.pageSrc) {
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
  
  func switchToTabOrOpen(id: UUID?, url: URL) {
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
      // If only empty tab present, the url will open in existing tab
      if tabManager.isBrowserEmptyForCurrentMode {
        finishEditingAndSubmit(url, visitType: .link)
        return
      }
      request = isPrivileged ? PrivilegedRequest(url: url) as URLRequest : URLRequest(url: url)
    } else {
      request = nil
    }

    tabManager.addTabAndSelect(request, isPrivate: isPrivate)
    
    // Has to go after since switching tabs will cause the URL bar to update to the selected Tab's url (which
    // is going to be nil still until the web view first commits
    updateToolbarCurrentURL(url)
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
        } else {
          self.focusURLBar()
        }
      }
    }
  }

  func clearHistoryAndOpenNewTab() {
    // When PB Only mode is enabled
    // All private tabs closed and a new private tab is created
    if Preferences.Privacy.privateBrowsingOnly.value {
      tabManager.removeAll()
      openBlankNewTab(attemptLocationFieldFocus: false, isPrivate: true, isExternal: true)
      popToBVC()
    } else {
      braveCore.historyAPI.deleteAll { [weak self] in
        guard let self = self else { return }

        self.tabManager.clearTabHistory() {
          self.openBlankNewTab(attemptLocationFieldFocus: false, isPrivate: false, isExternal: true)
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
    var activities = [UIActivity]()

    // Adding SendTabToSelfActivity conditionally to show device selection screen
    if !PrivateBrowsingManager.shared.isPrivateBrowsing, !url.isLocal, !InternalURL.isValid(url: url), !url.isReaderModeURL,
        braveCore.syncAPI.isSendTabToSelfVisible {
      let sendTabToSelfActivity = SendTabToSelfActivity() { [weak self] in
        guard let self = self else { return }
        
        let deviceList = self.braveCore.sendTabAPI.getListOfSyncedDevices()
        let dataSource = SendableTabInfoDataSource(
          with: deviceList,
          displayTitle: tab?.displayTitle ?? "",
          sendableURL: url)
        
        let controller = SendTabToSelfController(sendTabAPI: self.braveCore.sendTabAPI, dataSource: dataSource)
        
        controller.sendWebSiteHandler = { [weak self] dataSource in
          guard let self = self else { return }
          
          self.present(
            SendTabProcessController(type: .progress, data: dataSource, sendTabAPI: self.braveCore.sendTabAPI),
            animated: true,
            completion: nil)
        }
        self.present(controller, animated: true, completion: nil)
      }

      activities.append(sendTabToSelfActivity)
    }
    
    let findInPageActivity = FindInPageActivity() { [unowned self] in
      if #available(iOS 16.0, *), let findInteraction = self.tabManager.selectedTab?.webView?.findInteraction {
        findInteraction.searchText = ""
        findInteraction.presentFindNavigator(showingReplace: false)
      } else {
        self.updateFindInPageVisibility(visible: true)
      }
    }
    
    let pageZoomActivity = PageZoomActivity() { [unowned self] in
      self.displayPageZoom(visible: true)
    }

    activities.append(contentsOf: [findInPageActivity, pageZoomActivity])

    // These actions don't apply if we're sharing a temporary document
    if !url.isFileURL {
      // We don't allow to have 2 same favorites.
      if !FavoritesHelper.isAlreadyAdded(url) {
        activities.append(
          AddToFavoritesActivity() { [weak self, weak tab] in
            guard let self = self else { return }
            
            FavoritesHelper.add(url: url, title: tab?.displayTitle)
            // Handle App Rating
            // Check for review condition after adding a favorite
            AppReviewManager.shared.handleAppReview(for: .revised, using: self)
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
        let createPDFActivity = CreatePDFActivity() {
          webView.createPDF { [weak self] result in
            dispatchPrecondition(condition: .onQueue(.main))
            guard let self = self else {
              return
            }
            switch result {
            case .success(let pdfData):
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
                Logger.module.error("Failed to write PDF to disk: \(error.localizedDescription, privacy: .public)")
              }
              
            case .failure(let error):
              Logger.module.error("Failed to create PDF with error: \(error.localizedDescription)")
            }
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
    
    if #unavailable(iOS 16.0) {
      if let findInPageBar = findInPageBar {
        updateFindInPageVisibility(visible: false)
        findInPageBar.endEditing(true)
        findInPageBar.removeFromSuperview()
        self.findInPageBar = nil
        updateViewConstraints()
      }
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
  
  func updateWebViewPageZoom(tab: Tab) {
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
        webView.evaluateSafeJavaScript(functionName: "\(ReaderModeNamespace).checkReadability", contentWorld: ReaderModeScriptHandler.scriptSandbox)

        // Only add history of a url which is not a localhost url
        if !tab.isPrivate, !url.isReaderModeURL {
          // The visitType is checked If it is "typed" or not to determine the History object we are adding
          // should be synced or not. This limitation exists on browser side so we are aligning with this
          if let visitType = typedNavigation.first(where: {
            $0.key.typedDisplayString == url.typedDisplayString
          })?.value, visitType == .typed {
            braveCore.historyAPI.add(url: url, title: tab.title, dateAdded: Date())
          } else {
            braveCore.historyAPI.add(url: url, title: tab.title, dateAdded: Date(), isURLTyped: false)
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

        if let url = URIFixup.getURL(string), url.isWebPage(includeDataURIs: false) {
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
  
  func handleToolbarVisibilityStateChange(
    _ state: ToolbarVisibilityViewModel.ToolbarState,
    progress: CGFloat?
  ) {
    guard
      let tab = tabManager.selectedTab,
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
          toolbar?.actionButtons.forEach { $0.alpha = topToolbar.locationContainer.alpha }
        }
        animator.startAnimation()
      }
      return
    }
    let headerHeight = isUsingBottomBar ? 0 : toolbarVisibilityViewModel.transitionDistance
    let footerHeight = footer.bounds.height + (isUsingBottomBar ? toolbarVisibilityViewModel.transitionDistance - view.safeAreaInsets.bottom : 0)
    // Changing the web view size while scrolling and a PDF is visible causes strange flickering, so only show
    // final expanded/collapsed states while a PDF is visible
    if let progress = progress, tab.mimeType != MIMEType.PDF {
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
      toolbar?.actionButtons.forEach { $0.alpha = topToolbar.locationContainer.alpha }
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
    toolbar?.actionButtons.forEach { $0.alpha = topToolbar.locationContainer.alpha }
    let animator = toolbarVisibilityViewModel.toolbarChangePropertyAnimator
    animator.addAnimations {
      self.view.layoutIfNeeded()
    }
    animator.startAnimation()
  }
}

extension BrowserViewController {
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
}

extension BrowserViewController: PresentingModalViewControllerDelegate {
  func dismissPresentedModalViewController(_ modalViewController: UIViewController, animated: Bool) {
    self.dismiss(animated: animated, completion: nil)
  }
}

extension BrowserViewController: TabsBarViewControllerDelegate {
  func tabsBarDidSelectAddNewTab(_ isPrivate: Bool) {
    openBlankNewTab(attemptLocationFieldFocus: false, isPrivate: isPrivate)
  }

  func tabsBarDidSelectTab(_ tabsBarController: TabsBarViewController, _ tab: Tab) {
    if tab == tabManager.selectedTab { return }
    topToolbar.leaveOverlayMode(didCancel: true)
    if #unavailable(iOS 16.0) {
      updateFindInPageVisibility(visible: false)
    }
    
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
    webView.frame = webViewContainer.frame
    
    // Observers that live as long as the tab. Make sure these are all cleared in willDeleteWebView below!
    KVOs.forEach { webView.addObserver(self, forKeyPath: $0.rawValue, options: .new, context: nil) }
    webView.uiDelegate = self
    
    var injectedScripts: [TabContentScript] = [
      ReaderModeScriptHandler(tab: tab),
      ErrorPageHelper(certStore: profile.certStore),
      SessionRestoreScriptHandler(tab: tab),
      PrintScriptHandler(browserController: self, tab: tab),
      CustomSearchScriptHandler(tab: tab),
      NightModeScriptHandler(tab: tab),
      FocusScriptHandler(tab: tab),
      BraveGetUA(tab: tab),
      BraveSearchScriptHandler(tab: tab, profile: profile, rewards: rewards),
      ResourceDownloadScriptHandler(tab: tab),
      DownloadContentScriptHandler(browserController: self, tab: tab),
      WindowRenderScriptHandler(tab: tab),
      PlaylistScriptHandler(tab: tab),
      PlaylistFolderSharingScriptHandler(tab: tab),
      RewardsReportingScriptHandler(rewards: rewards, tab: tab),
      AdsMediaReportingScriptHandler(rewards: rewards, tab: tab),
      ReadyStateScriptHandler(tab: tab),
      DeAmpScriptHandler(tab: tab),
      SiteStateListenerScriptHandler(tab: tab),
      CosmeticFiltersScriptHandler(tab: tab),
      FaviconScriptHandler(tab: tab),
      Web3NameServiceScriptHandler(tab: tab),
      Web3IPFSScriptHandler(tab: tab),
      
      tab.contentBlocker,
      tab.requestBlockingContentHelper,
    ]
    
    if #unavailable(iOS 16.0) {
      injectedScripts.append(FindInPageScriptHandler(tab: tab))
    }
    
#if canImport(BraveTalk)
    injectedScripts.append(BraveTalkScriptHandler(tab: tab, rewards: rewards, launchNativeBraveTalk: { [weak self] tab, room, token in
      self?.launchNativeBraveTalk(tab: tab, room: room, token: token)
    }))
#endif
    
    if let braveSkusHandler = BraveSkusScriptHandler(tab: tab) {
      injectedScripts.append(braveSkusHandler)
    }
    
    // Only add the logins handler and wallet provider if the tab is NOT a private browsing tab
    if !tab.isPrivate {
      injectedScripts += [
        LoginsScriptHandler(tab: tab, profile: profile, passwordAPI: braveCore.passwordAPI),
        EthereumProviderScriptHandler(tab: tab),
        SolanaProviderScriptHandler(tab: tab)
      ]
    }

    // XXX: Bug 1390200 - Disable NSUserActivity/CoreSpotlight temporarily
    // let spotlightHelper = SpotlightHelper(tab: tab)
    // tab.addHelper(spotlightHelper, name: SpotlightHelper.name())
    
    injectedScripts.forEach {
      tab.addContentScript($0, name: type(of: $0).scriptName, contentWorld: type(of: $0).scriptSandbox)
    }
    
    (tab.getContentScript(name: ReaderModeScriptHandler.scriptName) as? ReaderModeScriptHandler)?.delegate = self
    (tab.getContentScript(name: SessionRestoreScriptHandler.scriptName) as? SessionRestoreScriptHandler)?.delegate = self
    if #unavailable(iOS 16.0) {
      (tab.getContentScript(name: FindInPageScriptHandler.scriptName) as? FindInPageScriptHandler)?.delegate = self
    }
    (tab.getContentScript(name: PlaylistScriptHandler.scriptName) as? PlaylistScriptHandler)?.delegate = self
    (tab.getContentScript(name: PlaylistFolderSharingScriptHandler.scriptName) as? PlaylistFolderSharingScriptHandler)?.delegate = self
    (tab.getContentScript(name: Web3NameServiceScriptHandler.scriptName) as? Web3NameServiceScriptHandler)?.delegate = self
    (tab.getContentScript(name: Web3IPFSScriptHandler.scriptName) as? Web3IPFSScriptHandler)?.delegate = self
  }

  func tab(_ tab: Tab, willDeleteWebView webView: WKWebView) {
    tab.cancelQueuedAlerts()
    KVOs.forEach { webView.removeObserver(self, forKeyPath: $0.rawValue) }
    toolbarVisibilityViewModel.endScrollViewObservation(webView.scrollView)
    webView.uiDelegate = nil
    webView.removeFromSuperview()
  }

  func showBar(_ bar: SnackBar, animated: Bool) {
    view.layoutIfNeeded()
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
    if #available(iOS 16.0, *), let findInteraction = tab.webView?.findInteraction {
      findInteraction.searchText = selectedText
      findInteraction.presentFindNavigator(showingReplace: false)
    } else {
      updateFindInPageVisibility(visible: true)
      findInPageBar?.text = selectedText
    }
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
      PlaylistScriptHandler.stopPlayback(tab: $0)
    })
  }
  
  func showWalletNotification(_ tab: Tab, origin: URLOrigin) {
    // only display notification when BVC is front and center
    guard presentedViewController == nil,
          Preferences.Wallet.displayWeb3Notifications.value else {
      return
    }
    let origin = tab.getOrigin()
    let tabDappStore = tab.tabDappStore
    let walletNotificaton = WalletNotification(priority: .low, origin: origin, isUsingBottomBar: isUsingBottomBar) { [weak self] action in
      if action == .connectWallet {
        self?.presentWalletPanel(from: origin, with: tabDappStore)
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
  
  func reloadIPFSSchemeUrl(_ url: URL) {
    handleIPFSSchemeURL(url, visitType: .unknown)
  }

  @MainActor
  private func isPendingRequestAvailable() async -> Bool {
    let privateMode = PrivateBrowsingManager.shared.isPrivateBrowsing
    // If we have an open `WalletStore`, use that so we can assign the pending request if the wallet is open,
    // which allows us to store the new `PendingRequest` triggering a modal presentation for that request.
    guard let cryptoStore = self.walletStore?.cryptoStore ?? CryptoStore.from(ipfsApi: braveCore.ipfsAPI, privateMode: privateMode) else {
      return false
    }
    if await cryptoStore.isPendingRequestAvailable() {
      return true
    } else if let selectedTabOrigin = tabManager.selectedTab?.url?.origin {
      if WalletProviderAccountCreationRequestManager.shared.hasPendingRequest(for: selectedTabOrigin, coinType: .sol) {
        return true
      }
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

  func searchViewController(_ searchViewController: SearchViewController, didSelectOpenTab tabInfo: (id: UUID?, url: URL)) {
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
    if #available(iOS 16.0, *), let findInteraction = tabManager.selectedTab?.webView?.findInteraction {
      findInteraction.searchText = query
      findInteraction.presentFindNavigator(showingReplace: false)
    } else {
      updateFindInPageVisibility(visible: true)
      findInPageBar?.text = query
    }
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

extension BrowserViewController: SessionRestoreScriptHandlerDelegate {
  func sessionRestore(_ handler: SessionRestoreScriptHandler, didRestoreSessionForTab tab: Tab) {
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
    topToolbar.tabLocationViewDidTapLocation(topToolbar.locationView)
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
          $0.setScript(script: .playlistMediaSource, enabled: Preferences.Playlist.webMediaSourceCompatibility.value)
          $0.webView?.reload()
        }
      }
    case Preferences.General.mediaAutoBackgrounding.key:
      tabManager.allTabs.forEach {
        $0.setScript(script: .mediaBackgroundPlay, enabled: Preferences.General.mediaAutoBackgrounding.value)
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
    case Preferences.Wallet.defaultEthWallet.key:
      tabManager.reset()
      tabManager.reloadSelectedTab()
      notificationsPresenter.removeNotification(with: WalletNotification.Constant.id)
      WalletProviderPermissionRequestsManager.shared.cancelAllPendingRequests(for: [.eth])
      WalletProviderAccountCreationRequestManager.shared.cancelAllPendingRequests(coins: [.eth])
      let privateMode = PrivateBrowsingManager.shared.isPrivateBrowsing
      if let cryptoStore = CryptoStore.from(ipfsApi: braveCore.ipfsAPI, privateMode: privateMode) {
        cryptoStore.rejectAllPendingWebpageRequests()
      }
      updateURLBarWalletButton()
    case Preferences.Wallet.defaultSolWallet.key:
      tabManager.reset()
      tabManager.reloadSelectedTab()
      notificationsPresenter.removeNotification(with: WalletNotification.Constant.id)
      WalletProviderPermissionRequestsManager.shared.cancelAllPendingRequests(for: [.sol])
      WalletProviderAccountCreationRequestManager.shared.cancelAllPendingRequests(coins: [.sol])
      let privateMode = PrivateBrowsingManager.shared.isPrivateBrowsing
      if let cryptoStore = CryptoStore.from(ipfsApi: braveCore.ipfsAPI, privateMode: privateMode) {
        cryptoStore.rejectAllPendingWebpageRequests()
      }
      updateURLBarWalletButton()
    case Preferences.Playlist.syncSharedFoldersAutomatically.key:
      syncPlaylistFolders()
    default:
      Logger.module.debug("Received a preference change for an unknown key: \(key, privacy: .public) on \(type(of: self), privacy: .public)")
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

extension BrowserViewController {
  func presentTabReceivedToast(url: URL) {
    // 'Tab Received' indicator will only be shown in normal browsing
    if !PrivateBrowsingManager.shared.isPrivateBrowsing {
      let toast = ButtonToast(
        labelText: Strings.Callout.tabReceivedCalloutTitle,
        image: UIImage(braveSystemNamed: "brave.tablet.and.phone"),
        buttonText: Strings.goButtonTittle,
        completion: { [weak self] buttonPressed in
          guard let self = self else { return }
          
          if buttonPressed {
            self.tabManager.addTabAndSelect(URLRequest(url: url), isPrivate: false)
          }
      })
      
      show(toast: toast, duration: ButtonToastUX.toastDismissAfter)
    }
  }
}

extension BrowserViewController: UNUserNotificationCenterDelegate {
  public func userNotificationCenter(_ center: UNUserNotificationCenter, didReceive response: UNNotificationResponse, withCompletionHandler completionHandler: @escaping () -> Void) {
    if response.notification.request.identifier == Self.defaultBrowserNotificationId {
      guard let settingsUrl = URL(string: UIApplication.openSettingsURLString) else {
        Logger.module.error("Failed to unwrap iOS settings URL")
        return
      }
      UIApplication.shared.open(settingsUrl)
    } else if response.notification.request.identifier == PrivacyReportsManager.notificationID {
      openPrivacyReport()
    }
    completionHandler()
  }
}

// MARK: UIScreenshotServiceDelegate

extension BrowserViewController: UIScreenshotServiceDelegate {
  
  @MainActor
  public func screenshotServiceGeneratePDFRepresentation(_ screenshotService: UIScreenshotService) async -> (Data?, Int, CGRect) {
    await withCheckedContinuation { continuation in
      guard screenshotService.windowScene != nil,
            presentedViewController == nil,
            let webView = tabManager.selectedTab?.webView,
            let url = webView.url,
            url.isWebPage()
      else {
        continuation.resume(returning: (nil, 0, .zero))
        return
      }
      
      var rect = webView.scrollView.frame
      rect.origin.x = webView.scrollView.contentOffset.x
      rect.origin.y = webView.scrollView.contentSize.height - rect.height - webView.scrollView.contentOffset.y
      
      webView.createPDF { result in
  
        switch result {
        case .success(let data):
          continuation.resume(returning: (data, 0, rect))
        case .failure:
          continuation.resume(returning: (nil, 0, .zero))
        }
      }
    }
  }
}

// Privacy reports
extension BrowserViewController {
  public func openPrivacyReport() {
    if PrivateBrowsingManager.shared.isPrivateBrowsing {
      return
    }
    
    let host = UIHostingController(rootView: PrivacyReportsManager.prepareView())
    
    host.rootView.openPrivacyReportsUrl = { [weak self] in
      guard let self = self else { return }
      let tab = self.tabManager.addTab(
        PrivilegedRequest(url: .brave.privacyFeatures) as URLRequest,
        afterTab: self.tabManager.selectedTab,
        // Privacy Reports view is unavailable in private mode.
        isPrivate: false)
      self.tabManager.selectTab(tab)
    }
    
    self.present(host, animated: true)
  }
}
