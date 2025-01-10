// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import AIChat
import BraveCore
import BraveNews
import BraveShared
import BraveShields
import BraveUI
import BraveVPN
import BraveWallet
import CertificateUtilities
import CoreData
import Data
import Favicon
import Growth
import NetworkExtension
import Onboarding
import Preferences
import ScreenTime
import Shared
import SnapKit
import SpeechRecognition
import Storage
import StoreKit
import SwiftUI
import Translation
import UIKit
import WebKit
import os.log

import class Combine.AnyCancellable

#if canImport(BraveTalk)
import BraveTalk
#endif

#if canImport(BraveTalk)
import BraveTalk
#endif

// swift-format-ignore
private let KVOs: [KVOConstants] = [
  .estimatedProgress,
  .loading,
  .canGoBack,
  .canGoForward,
  .url,
  .title,
  .hasOnlySecureContent,
  .serverTrust,
  ._sampledPageTopColor,
]

public class BrowserViewController: UIViewController {
  let webViewContainer = UIView()
  private(set) lazy var screenshotHelper = ScreenshotHelper(tabManager: tabManager)

  private(set) lazy var topToolbar: TopToolbarView = {
    // Setup the URL bar, wrapped in a view to get transparency effect
    let topToolbar = TopToolbarView(
      voiceSearchSupported: speechRecognizer.isVoiceSearchAvailable,
      privateBrowsingManager: privateBrowsingManager
    )
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
  private(set) lazy var header = HeaderContainerView(privateBrowsingManager: privateBrowsingManager)
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

  private(set) lazy var statusBarOverlay: UIView = {
    // Temporary work around for covering the non-clipped web view content
    let statusBarOverlay = UIView()
    statusBarOverlay.backgroundColor = privateBrowsingManager.browserColors.chromeBackground
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

  var pageZoomBar: UIHostingController<PageZoomView>?
  private var pageZoomListener: NSObjectProtocol?
  private var openTabsModelStateListener: SendTabToSelfModelStateListener?
  private var syncServiceStateListener: AnyObject?
  let collapsedURLBarView = CollapsedURLBarView()

  // Single data source used for all favorites vcs
  public let backgroundDataSource: NTPDataSource
  let feedDataSource: FeedDataSource

  private var postSetupTasks: [() -> Void] = []
  private var setupTasksCompleted: Bool = false

  private var privateModeCancellable: AnyCancellable?
  private var appReviewCancelable: AnyCancellable?
  private var adFeatureLinkageCancelable: AnyCancellable?
  var onPendingRequestUpdatedCancellable: AnyCancellable?

  // Translation
  let translationHostingController: UIHostingController<AnyView> = .init(
    rootView: AnyView(EmptyView())
  )

  /// Voice Search
  var voiceSearchViewController: PopupViewController<SpeechToTextInputView>?
  var voiceSearchCancelable: AnyCancellable?
  let speechRecognizer = SpeechRecognizer()

  /// Custom Search Engine
  var openSearchEngine: OpenSearchReference?

  lazy var customSearchEngineButton = OpenSearchEngineButton(hidesWhenDisabled: false).then {
    $0.addTarget(
      self,
      action: #selector(addCustomSearchEngineForFocusedElement),
      for: .touchUpInside
    )
    $0.accessibilityIdentifier = "BrowserViewController.customSearchEngineButton"
    $0.setContentCompressionResistancePriority(.defaultHigh, for: .horizontal)
    $0.setContentHuggingPriority(.defaultHigh, for: .horizontal)
  }

  var customSearchBarButtonItemGroup: UIBarButtonItemGroup?

  // popover rotation handling
  var displayedPopoverController: UIViewController?
  var updateDisplayedPopoverProperties: (() -> Void)?

  public let windowId: UUID
  let profile: Profile
  let attributionManager: AttributionManager
  let braveCore: BraveCoreMain
  let tabManager: TabManager
  let migration: Migration?
  let bookmarkManager: BookmarkManager
  public let privateBrowsingManager: PrivateBrowsingManager

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

  /// A toast that might be waiting for BVC to appear before displaying
  var pendingToast: Toast?
  /// A toast that is showing the combined download progress
  var downloadToast: DownloadToast?
  /// A toast which is active and not yet dismissed
  var activeButtonToast: Toast?
  /// An infobar displaying a privacy notice when a search result ad is clicked
  var searchResultAdClickedInfoBar: SearchResultAdClickedInfoBar?
  /// A boolean to determine If AddToListActivity should be added
  var addToPlayListActivityItem: (enabled: Bool, item: PlaylistInfo?)?
  /// A boolean to determine if OpenInPlaylistActivity should be shown
  var openInPlaylistActivityItem: (enabled: Bool, item: PlaylistInfo?)?
  var shouldDownloadNavigationResponse: Bool = false

  var navigationToolbar: ToolbarProtocol {
    return toolbar ?? topToolbar
  }

  // Keep track of allowed `URLRequest`s from `webView(_:decidePolicyFor:decisionHandler:)` so
  // that we can obtain the originating `URLRequest` when a `URLResponse` is received. This will
  // allow us to re-trigger the `URLRequest` if the user requests a file to be downloaded.
  var pendingRequests = [String: URLRequest]()

  let downloadQueue = DownloadQueue()

  private var cancellables: Set<AnyCancellable> = []

  let rewards: BraveRewards
  var rewardsObserver: RewardsObserver?
  var promotionFetchTimer: Timer?
  private var notificationsHandler: AdsNotificationHandler?
  let notificationsPresenter = BraveNotificationsPresenter()
  var publisher: BraveCore.BraveRewards.PublisherInfo?

  let vpnProductInfo = BraveVPNProductInfo()

  /// Window Protection instance which will be used for controller requires biometric authentication
  public var windowProtection: WindowProtection?

  // Product Notification Related Properties

  /// Used to determine when to present benchmark pop-overs
  /// Current session ad count is compared with live ad count
  /// So user will not be introduced with a pop-over directly
  let benchmarkCurrentSessionAdCount =
    BraveGlobalShieldStats.shared.adblock + BraveGlobalShieldStats.shared.trackingProtection

  /// Navigation Helper used for Brave Widgets
  private(set) lazy var navigationHelper = BrowserNavigationHelper(self)

  /// Boolean tracking  if Tab Tray is active on the screen
  /// Used to determine If pop-over should be presented
  var isTabTrayActive = false

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

  var processAddressBarTask: Task<(), Never>?
  var topToolbarDidPressReloadTask: Task<(), Never>?

  /// In app purchase obsever for VPN Subscription action
  let iapObserver: BraveVPNInAppPurchaseObserver

  private let ntpP3AHelper: NewTabPageP3AHelper

  public init(
    windowId: UUID,
    profile: Profile,
    attributionManager: AttributionManager,
    braveCore: BraveCoreMain,
    rewards: BraveRewards,
    migration: Migration?,
    crashedLastSession: Bool,
    newsFeedDataSource: FeedDataSource,
    privateBrowsingManager: PrivateBrowsingManager
  ) {
    self.windowId = windowId
    self.profile = profile
    self.attributionManager = attributionManager
    self.braveCore = braveCore
    self.bookmarkManager = BookmarkManager(bookmarksAPI: braveCore.bookmarksAPI)
    self.rewards = rewards
    self.migration = migration
    self.crashedLastSession = crashedLastSession
    self.privateBrowsingManager = privateBrowsingManager
    self.feedDataSource = newsFeedDataSource
    feedDataSource.historyAPI = braveCore.historyAPI
    backgroundDataSource = .init(
      service: braveCore.backgroundImagesService,
      privateBrowsingManager: privateBrowsingManager
    )

    // Initialize TabManager
    self.tabManager = TabManager(
      windowId: windowId,
      prefs: profile.prefs,
      rewards: rewards,
      tabGeneratorAPI: braveCore.tabGeneratorAPI,
      historyAPI: braveCore.historyAPI,
      privateBrowsingManager: privateBrowsingManager
    )

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

    self.deviceCheckClient = DeviceCheckClient(
      environment: BraveRewards.Configuration.current().environment
    )

    iapObserver = BraveVPN.iapObserver
    ntpP3AHelper = .init(p3aUtils: braveCore.p3aUtils, rewards: rewards)

    super.init(nibName: nil, bundle: nil)
    didInit()

    iapObserver.delegate = self

    rewards.rewardsServiceDidStart = { [weak self] _ in
      self?.setupLedger()
    }

    rewards.ads.captchaHandler = self
    if rewards.shouldStartAds {
      // Only start rewards service automatically if ads is enabled
      if rewards.isEnabled {
        rewards.startRewardsService(nil)
      } else {
        rewards.ads.initialize { _ in }
      }
    }

    self.feedDataSource.getAdsAPI = {
      // The ads object gets re-recreated when shutdown, so we need to make sure News fetches it out of
      // the BraveRewards container
      return rewards.ads
    }

    // Observer watching tab information is sent by another device
    openTabsModelStateListener = braveCore.sendTabAPI.add(
      SendTabToSelfStateObserver { [weak self] stateChange in
        if case .sendTabToSelfEntriesAddedRemotely(let newEntries) = stateChange {
          // Fetching the last URL that has been sent from synced sessions
          if let requestedURL = newEntries.last?.url {
            self?.presentTabReceivedToast(url: requestedURL)
          }
        }
      }
    )

    // Observer watching state change in sync chain
    syncServiceStateListener = braveCore.syncAPI.addServiceStateObserver { [weak self] in
      guard let self = self else { return }
      // Observe Sync State in order to determine if the sync chain is deleted
      // from another device - Clean local sync chain
      if self.braveCore.syncAPI.shouldLeaveSyncGroup {
        self.braveCore.syncAPI.leaveSyncGroup()
      }
    }

    if Preferences.Privacy.screenTimeEnabled.value, !ProcessInfo.processInfo.isiOSAppOnVisionOS {
      // Accessing `STWebpageController` on Vision OS results in a crash
      screenTimeViewController = STWebpageController()
    }
  }

  deinit {
    // Remove the open tabs model state observer
    if let observer = openTabsModelStateListener {
      braveCore.sendTabAPI.removeObserver(observer)
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

  override public func viewWillTransition(
    to size: CGSize,
    with coordinator: UIViewControllerTransitionCoordinator
  ) {
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
      }
    )
  }

  override public func didReceiveMemoryWarning() {
    super.didReceiveMemoryWarning()
    ScriptFactory.shared.clearCaches()

    Task {
      await AdBlockGroupsManager.shared.didReceiveMemoryWarning()
    }

    for tab in tabManager.tabsForCurrentMode where tab.id != tabManager.selectedTab?.id {
      tab.newTabPageViewController = nil
    }
  }

  private var rewardsEnabledObserveration: NSKeyValueObservation?

  fileprivate func didInit() {
    updateApplicationShortcuts()
    tabManager.addDelegate(self)
    tabManager.addNavigationDelegate(self)
    UserScriptManager.shared.fetchWalletScripts(from: braveCore.braveWalletAPI)
    downloadQueue.delegate = self

    // Observe some user preferences
    Preferences.Privacy.privateBrowsingOnly.observe(from: self)
    Preferences.General.tabBarVisibility.observe(from: self)
    Preferences.UserAgent.alwaysRequestDesktopSite.observe(from: self)
    Preferences.General.enablePullToRefresh.observe(from: self)
    Preferences.General.mediaAutoBackgrounding.observe(from: self)
    Preferences.General.youtubeHighQuality.observe(from: self)
    Preferences.General.defaultPageZoomLevel.observe(from: self)
    Preferences.Shields.allShields.forEach { $0.observe(from: self) }
    Preferences.Privacy.blockAllCookies.observe(from: self)
    Preferences.Rewards.hideRewardsIcon.observe(from: self)
    Preferences.Rewards.rewardsToggledOnce.observe(from: self)
    Preferences.Playlist.enablePlaylistMenuBadge.observe(from: self)
    Preferences.Playlist.enablePlaylistURLBarButton.observe(from: self)
    Preferences.Playlist.syncSharedFoldersAutomatically.observe(from: self)
    Preferences.NewTabPage.backgroundMediaTypeRaw.observe(from: self)
    ShieldPreferences.blockAdsAndTrackingLevelRaw.observe(from: self)
    Preferences.Privacy.screenTimeEnabled.observe(from: self)
    Preferences.Translate.translateEnabled.observe(from: self)

    pageZoomListener = NotificationCenter.default.addObserver(
      forName: PageZoomView.notificationName,
      object: nil,
      queue: .main
    ) { [weak self] _ in
      self?.tabManager.allTabs.forEach({
        guard let url = $0.webView?.url else { return }
        let zoomLevel =
          self?.privateBrowsingManager.isPrivateBrowsing == true
          ? 1.0
          : Domain.getPersistedDomain(for: url)?.zoom_level?.doubleValue
            ?? Preferences.General.defaultPageZoomLevel.value

        $0.webView?.setValue(zoomLevel, forKey: PageZoomHandler.propertyName)
      })
    }

    rewardsEnabledObserveration = rewards.ads.observe(\.isEnabled, options: [.new]) {
      [weak self] _, _ in
      guard let self = self else { return }
      self.updateRewardsButtonState()
      self.setupAdsNotificationHandler()
      self.recordAdsUsageType()
    }
    Preferences.PrivacyReports.captureShieldsData.observe(from: self)
    Preferences.PrivacyReports.captureVPNAlerts.observe(from: self)
    Preferences.Wallet.defaultEthWallet.observe(from: self)

    if rewards.rewardsAPI != nil {
      // Ledger was started immediately due to user having ads enabled
      setupLedger()
    }

    Preferences.NewTabPage.attemptToShowClaimRewardsNotification.value = true

    backgroundDataSource.initializeFavorites = { sites in
      DispatchQueue.main.async {
        defer { Preferences.NewTabPage.preloadedFavoritiesInitialized.value = true }

        if Preferences.NewTabPage.preloadedFavoritiesInitialized.value
          || Favorite.hasFavorites
        {
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
        let defaultFavorites = FavoritesPreloadedData.getList()
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

    // P3A Record
    maybeRecordInitialShieldsP3A()
    recordVPNUsageP3A(vpnEnabled: BraveVPN.isConnected)
    recordAccessibilityDisplayZoomEnabledP3A()
    recordAccessibilityDocumentsDirectorySizeP3A()
    recordTimeBasedNumberReaderModeUsedP3A(activated: false)
    recordGeneralBottomBarLocationP3A()
    PlaylistP3A.recordHistogram()
    recordAdsUsageType()
    recordDefaultBrowserLikelyhoodP3A()
    recordWeeklyUsage()
    recordURLBarSubmitLocationP3A(from: nil)
    recordCreateTabAction(location: nil)

    // Revised Review Handling
    AppReviewManager.shared.handleAppReview(for: .revisedCrossPlatform, using: self)
  }

  private func setupAdsNotificationHandler() {
    notificationsHandler = AdsNotificationHandler(
      ads: rewards.ads,
      presentingController: self,
      notificationsPresenter: notificationsPresenter
    )
    notificationsHandler?.canShowNotifications = { [weak self] in
      guard let self = self else { return false }
      return !self.privateBrowsingManager.isPrivateBrowsing && !self.topToolbar.inOverlayMode
    }
    notificationsHandler?.actionOccured = { [weak self] ad, action in
      guard let self = self, let ad = ad else { return }
      if action == .opened {
        var url = URL(string: ad.targetURL)
        if url == nil,
          let percentEncodedURLString =
            ad.targetURL.addingPercentEncoding(withAllowedCharacters: .urlQueryAllowed)
        {
          // Try to percent-encode the string and try that
          url = URL(string: percentEncodedURLString)
        }
        guard let targetURL = url else {
          assertionFailure("Invalid target URL for creative instance id: \(ad.creativeInstanceID)")
          return
        }
        let request = URLRequest(url: targetURL)
        self.tabManager.addTabAndSelect(
          request,
          isPrivate: self.privateBrowsingManager.isPrivateBrowsing
        )
      }
    }
  }

  func shouldShowFooterForTraitCollection(_ previousTraitCollection: UITraitCollection) -> Bool {
    return previousTraitCollection.verticalSizeClass != .compact
      && previousTraitCollection.horizontalSizeClass != .regular
  }

  private func updateUsingBottomBar(using traitCollection: UITraitCollection) {
    isUsingBottomBar =
      Preferences.General.isUsingBottomBar.value && traitCollection.horizontalSizeClass == .compact
      && traitCollection.verticalSizeClass == .regular
      && traitCollection.userInterfaceIdiom == .phone

    // Reinserts the fav controller whos parent is based on bottom bar
    if let favoritesController {
      insertFavoritesControllerView(favoritesController: favoritesController)
    }
  }

  public override func viewSafeAreaInsetsDidChange() {
    super.viewSafeAreaInsetsDidChange()

    topTouchArea.isEnabled = view.safeAreaInsets.top > 0
    statusBarOverlay.isHidden = view.safeAreaInsets.top.isZero
  }

  fileprivate func updateToolbarStateForTraitCollection(
    _ newCollection: UITraitCollection,
    withTransitionCoordinator coordinator: UIViewControllerTransitionCoordinator? = nil
  ) {
    let showToolbar = shouldShowFooterForTraitCollection(newCollection)
    bottomTouchArea.isEnabled = showToolbar
    topToolbar.setShowToolbar(showToolbar)

    if (showToolbar && toolbar == nil) || (!showToolbar && toolbar != nil) {
      toolbar?.removeFromSuperview()
      toolbar?.tabToolbarDelegate = nil
      toolbar = nil

      if showToolbar {
        toolbar = BottomToolbarView(privateBrowsingManager: privateBrowsingManager)
        toolbar?.setSearchButtonState(url: tabManager.selectedTab?.url)
        footer.addSubview(toolbar!)
        toolbar?.tabToolbarDelegate = self
        toolbar?.menuButton.setBadges(Array(topToolbar.menuButton.badges.keys))
      }
      view.setNeedsUpdateConstraints()
    }

    updateToolbarUsingTabManager(tabManager)
    updateUsingBottomBar(using: newCollection)

    if let tab = tabManager.selectedTab,
      let webView = tab.webView
    {
      updateURLBar()
      updateBackForwardActionStatus(for: webView)
      topToolbar.locationView.loading = tab.loading
    }

    toolbarVisibilityViewModel.toolbarState = .expanded
    updateTabsBarVisibility()
  }

  func updateToolbarSecureContentState(_ secureContentState: TabSecureContentState) {
    topToolbar.secureContentState = secureContentState
    collapsedURLBarView.secureContentState = secureContentState
  }

  func updateToolbarCurrentURL(_ currentURL: URL?) {
    topToolbar.currentURL = currentURL
    collapsedURLBarView.currentURL = currentURL
    updateScreenTimeUrl(currentURL)
  }

  override public func willTransition(
    to newCollection: UITraitCollection,
    with coordinator: UIViewControllerTransitionCoordinator
  ) {
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
          self.updateStatusBarOverlayColor()
          self.bottomBarKeyboardBackground.backgroundColor = self.topToolbar.backgroundColor
          self.setNeedsStatusBarAppearanceUpdate()
        }
      }
    )
  }

  func dismissVisibleMenus() {
    displayedPopoverController?.dismiss(animated: true)
  }

  @objc func sceneDidEnterBackgroundNotification(_ notification: NSNotification) {
    guard let scene = notification.object as? UIScene, scene == currentScene else {
      return
    }

    displayedPopoverController?.dismiss(animated: false) {
      self.updateDisplayedPopoverProperties = nil
      self.displayedPopoverController = nil
    }
  }

  @objc func appWillTerminateNotification() {
    tabManager.saveAllTabs()
    tabManager.removePrivateWindows()
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

  @objc func sceneWillResignActiveNotification(_ notification: NSNotification) {
    guard let scene = notification.object as? UIScene, scene == currentScene else {
      return
    }

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
      activeNewTabPageViewController?.view.alpha = 0
      favoritesController?.view.alpha = 0
      searchController?.view.alpha = 0
      header.contentView.alpha = 0
      presentedViewController?.popoverPresentationController?.containerView?.alpha = 0
      presentedViewController?.view.alpha = 0
    }

    // Stop Voice Search and dismiss controller
    stopVoiceSearch()
  }

  @objc func vpnConfigChanged() {
    // Load latest changes to the vpn.
    NEVPNManager.shared().loadFromPreferences { _ in }

    if case .purchased(let enabled) = BraveVPN.vpnState, enabled {
      recordVPNUsageP3A(vpnEnabled: true)
    }
  }

  @objc func sceneDidBecomeActiveNotification(_ notification: NSNotification) {
    guard let scene = notification.object as? UIScene, scene == currentScene else {
      return
    }

    guard let tab = tabManager.selectedTab, tab.isPrivate else {
      return
    }
    // Re-show any components that might have been hidden because they were being displayed
    // as part of a private mode tab
    UIView.animate(
      withDuration: 0.2,
      delay: 0,
      options: UIView.AnimationOptions(),
      animations: {
        self.webViewContainer.alpha = 1
        self.header.contentView.alpha = 1
        self.activeNewTabPageViewController?.view.alpha = 1
        self.favoritesController?.view.alpha = 1
        self.searchController?.view.alpha = 1
        self.presentedViewController?.popoverPresentationController?.containerView?.alpha = 1
        self.presentedViewController?.view.alpha = 1
        self.view.backgroundColor = .clear
      },
      completion: { _ in
        self.webViewContainerBackdrop.alpha = 0
      }
    )
  }

  private(set) var isUsingBottomBar: Bool = false {
    didSet {
      header.isUsingBottomBar = isUsingBottomBar
      collapsedURLBarView.isUsingBottomBar = isUsingBottomBar
      searchController?.isUsingBottomBar = isUsingBottomBar
      bottomBarKeyboardBackground.isHidden = !isUsingBottomBar
      topToolbar.displayTabTraySwipeGestureRecognizer?.isEnabled = isUsingBottomBar
      updateTabsBarVisibility()
      updateStatusBarOverlayColor()
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

    addChild(translationHostingController)
    view.addSubview(translationHostingController.view)
    translationHostingController.didMove(toParent: self)

    view.addSubview(alertStackView)
    view.addSubview(bottomTouchArea)
    view.addSubview(topTouchArea)
    view.addSubview(bottomBarKeyboardBackground)
    view.addSubview(footer)
    view.addSubview(statusBarOverlay)
    view.addSubview(header)

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
        self,
        selector: #selector(sceneWillResignActiveNotification(_:)),
        name: UIScene.willDeactivateNotification,
        object: nil
      )
      $0.addObserver(
        self,
        selector: #selector(sceneDidBecomeActiveNotification(_:)),
        name: UIScene.didActivateNotification,
        object: nil
      )
      $0.addObserver(
        self,
        selector: #selector(sceneDidEnterBackgroundNotification),
        name: UIScene.didEnterBackgroundNotification,
        object: nil
      )
      $0.addObserver(
        self,
        selector: #selector(appWillTerminateNotification),
        name: UIApplication.willTerminateNotification,
        object: nil
      )
      $0.addObserver(
        self,
        selector: #selector(resetNTPNotification),
        name: .adsOrRewardsToggledInSettings,
        object: nil
      )
      $0.addObserver(
        self,
        selector: #selector(vpnConfigChanged),
        name: .NEVPNConfigurationChange,
        object: nil
      )
    }

    BraveGlobalShieldStats.shared.$adblock
      .scan(
        (BraveGlobalShieldStats.shared.adblock, BraveGlobalShieldStats.shared.adblock),
        { ($0.1, $1) }
      )
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
    header.collapsedBarContainerView.addTarget(
      self,
      action: #selector(tappedCollapsedURLBar),
      for: .touchUpInside
    )
    updateRewardsButtonState()

    // Setup UIDropInteraction to handle dragging and dropping
    // links into the view from other apps.
    let dropInteraction = UIDropInteraction(delegate: self)
    view.addInteraction(dropInteraction)
    topToolbar.addInteraction(dropInteraction)

    // Adding a small delay before fetching gives more reliability to it,
    // epsecially when you are connected to a VPN.
    Task.delayed(bySeconds: 1.0) { @MainActor in
      // Refresh Skus VPN Credentials before loading VPN state
      await BraveSkusManager(isPrivateMode: self.privateBrowsingManager.isPrivateBrowsing)?
        .refreshVPNCredentials()

      self.vpnProductInfo.load()
      if let customCredential = Preferences.VPN.skusCredential.value,
        let customCredentialDomain = Preferences.VPN.skusCredentialDomain.value,
        let vpnCredential = BraveSkusWebHelper.fetchVPNCredential(
          customCredential,
          domain: customCredentialDomain
        )
      {
        BraveVPN.initialize(customCredential: vpnCredential)
      } else {
        BraveVPN.initialize(customCredential: nil)
      }
    }

    // Schedule Default Browser Local Notification
    // If notification is not already scheduled or
    // an external URL opened in Brave (which indicates Brave is set as default)
    if !Preferences.DefaultBrowserIntro.defaultBrowserNotificationScheduled.value {
      scheduleDefaultBrowserNotification()
    }

    privateModeCancellable = privateBrowsingManager
      .$isPrivateBrowsing
      .removeDuplicates()
      .receive(on: RunLoop.main)
      .sink(receiveValue: { [weak self] isPrivateBrowsing in
        guard let self = self else { return }
        self.updateStatusBarOverlayColor()
        self.bottomBarKeyboardBackground.backgroundColor = self.topToolbar.backgroundColor
        self.collapsedURLBarView.browserColors = self.privateBrowsingManager.browserColors
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

    adFeatureLinkageCancelable = attributionManager
      .$adFeatureLinkage
      .removeDuplicates()
      .sink(receiveValue: { [weak self] featureLinkageType in
        guard let self = self else { return }
        switch featureLinkageType {
        case .playlist:
          self.presentPlaylistController()
        case .vpn:
          self.navigationHelper.openVPNBuyScreen(iapObserver: self.iapObserver)
        default:
          return
        }
      })

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
        Logger.module.error(
          "Failed to request notifications permissions: \(error.localizedDescription, privacy: .public)"
        )
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
          $0.body = Strings.DefaultBrowserCallout.notificationBody
        }

        let timeToShow = AppConstants.isOfficialBuild ? 2.hours : 2.minutes
        let timeTrigger = UNTimeIntervalNotificationTrigger(
          timeInterval: timeToShow,
          repeats: false
        )

        let request = UNNotificationRequest(
          identifier: Self.defaultBrowserNotificationId,
          content: content,
          trigger: timeTrigger
        )

        center.add(request) { error in
          if let error = error {
            Logger.module.error(
              "Failed to add notification: \(error.localizedDescription, privacy: .public)"
            )
            return
          }

          Preferences.DefaultBrowserIntro.defaultBrowserNotificationScheduled.value = true
        }
      }
    }
  }

  private func cancelScheduleDefaultBrowserNotification() {
    let center = UNUserNotificationCenter.current()
    center.removePendingNotificationRequests(withIdentifiers: [Self.defaultBrowserNotificationId])

    Preferences.DefaultBrowserIntro.defaultBrowserNotificationIsCanceled.value = true
  }

  private func executeAfterSetup(_ block: @escaping () -> Void) {
    if setupTasksCompleted {
      block()
    } else {
      postSetupTasks.append(block)
    }
  }

  private func setupTabs() {
    let isPrivate =
      privateBrowsingManager.isPrivateBrowsing || Preferences.Privacy.privateBrowsingOnly.value
    let noTabsAdded = self.tabManager.tabsForCurrentMode.isEmpty

    var tabToSelect: Tab?

    if noTabsAdded {
      // Two scenarios if there are no tabs in tabmanager:
      // 1. We have not restored tabs yet, attempt to restore or make a new tab if there is nothing.
      // 2. We are in private browsing mode and need to add a new private tab.
      tabToSelect =
        isPrivate ? self.tabManager.addTab(isPrivate: true) : self.tabManager.restoreAllTabs
    } else {
      if let selectedTab = tabManager.selectedTab, !selectedTab.isPrivate {
        tabToSelect = selectedTab
      } else {
        tabToSelect = tabManager.tabsForCurrentMode.last
      }
    }
    self.tabManager.selectTab(tabToSelect)

    // Shred Domain's with SiteShieldLevel.appExit
    self.tabManager.forgetDataOnAppExitDomains()

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

    toolbarVisibilityViewModel.transitionDistance =
      header.expandedBarStackView.bounds.height - header.collapsedBarContainerView.bounds.height
    // Since the height of the WKWebView changes while collapsing we need to use a stable value to determine
    // if the toolbars can collapse. We don't subtract the bottom safe area inset because the footer includes
    // that safe area
    toolbarVisibilityViewModel.minimumCollapsableContentHeight =
      view.bounds.height - view.safeAreaInsets.top

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

    if let tabId = tabManager.selectedTab?.rewardsId, rewards.rewardsAPI?.selectedTabId == 0 {
      rewards.rewardsAPI?.selectedTabId = tabId
    }
  }

  public override func viewIsAppearing(_ animated: Bool) {
    super.viewIsAppearing(animated)

    if #available(iOS 17, *) {
      // Have to defer this to the next cycle to avoid an iOS bug which lays out the toolbars without any
      // bottom safe area, resulting in a layout bug.
      DispatchQueue.main.async {
        // On iOS 17 rotating the device with a full screen modal presented (e.g. Playlist, Tab Tray)
        // to landscape then back to portrait does not trigger `traitCollectionDidChange`/`willTransition`/etc
        // calls and so the toolbar remains in the wrong state.
        self.updateToolbarStateForTraitCollection(self.traitCollection)
      }
    }

    // Present Onboarding to new users, existing users will not see the onboarding
    presentOnboardingIntro()
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
      self.tabManager.addTabAndSelect(isPrivate: self.privateBrowsingManager.isPrivateBrowsing)
      return
    }
    let alert = UIAlertController.restoreTabsAlert(
      okayCallback: { _ in
        self.setupTabs()
      },
      noCallback: { _ in
        SessionTab.deleteAll()
        self.tabManager.addTabAndSelect(isPrivate: self.privateBrowsingManager.isPrivateBrowsing)
      }
    )
    self.present(alert, animated: true, completion: nil)
  }

  fileprivate func canRestoreTabs() -> Bool {
    // Make sure there's at least one real tab open
    return !SessionTab.all().compactMap({ $0.url }).isEmpty
  }

  override public func viewDidAppear(_ animated: Bool) {
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

    let isPrivateBrowsing = SessionWindow.from(windowId: windowId)?.isPrivate == true
    var userActivity = view.window?.windowScene?.userActivity

    if let userActivity = userActivity {
      BrowserState.setWindowInfo(
        for: userActivity,
        windowId: windowId.uuidString,
        isPrivate: isPrivateBrowsing
      )
    } else {
      userActivity = BrowserState.userActivity(
        for: windowId.uuidString,
        isPrivate: isPrivateBrowsing
      )
    }

    if let scene = view.window?.windowScene {
      scene.userActivity = userActivity
      BrowserState.setWindowInfo(
        for: scene.session,
        windowId: windowId.uuidString,
        isPrivate: isPrivateBrowsing
      )
    }

    for session in UIApplication.shared.openSessions {
      UIApplication.shared.requestSceneSessionRefresh(session)
    }
  }

  /// Whether or not to show the playlist onboarding callout this session
  var shouldShowPlaylistOnboardingThisSession = true

  /// Wheter or not to show the translate onboarding callout this session
  var shouldShowTranslationOnboardingThisSession = true

  public func showQueuedAlertIfAvailable() {
    if let queuedAlertInfo = tabManager.selectedTab?.dequeueJavascriptAlertPrompt() {
      let alertController = queuedAlertInfo.alertController()
      alertController.delegate = self
      present(alertController, animated: true, completion: nil)
    }
  }

  override public func viewWillDisappear(_ animated: Bool) {
    screenshotHelper.viewIsVisible = false
    super.viewWillDisappear(animated)

    rewards.rewardsAPI?.selectedTabId = 0
    view.window?.windowScene?.userActivity = nil
  }

  /// A layout guide defining where the favorites and NTP overlay are placed
  let pageOverlayLayoutGuide = UILayoutGuide()

  /// A single controller per bvc/window. Using one controller per tab or webview causes crashes.
  var screenTimeViewController: STWebpageController?

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

    if let screenTimeViewController = screenTimeViewController,
      screenTimeViewController.parent != nil
    {
      screenTimeViewController.view.snp.remakeConstraints {
        $0.edges.equalTo(webViewContainer)
      }
    }

    webViewContainer.snp.remakeConstraints { make in
      make.left.right.equalTo(self.view)

      if self.isUsingBottomBar {
        webViewContainerTopOffset =
          make.top.equalTo(self.readerModeBar?.snp.bottom ?? self.toolbarLayoutGuide.snp.top)
          .constraint
      } else {
        webViewContainerTopOffset =
          make.top.equalTo(self.readerModeBar?.snp.bottom ?? self.header.snp.bottom).constraint
      }

      if self.isUsingBottomBar {
        make.bottom.equalTo(self.header.snp.top)
      } else {
        make.bottom.equalTo(self.footer.snp.top)
      }
    }

    header.snp.remakeConstraints { make in
      if self.isUsingBottomBar {
        // Need to check Find In Page Bar is enabled in order to aligh it properly when bottom-bar is enabled
        var shouldEvaluateKeyboardConstraints = false
        var activeKeyboardHeight: CGFloat = 0
        var searchEngineSettingsDismissed = false

        if let keyboardHeight = keyboardState?.intersectionHeightForView(self.view) {
          activeKeyboardHeight = keyboardHeight
        }

        if let presentedNavigationController = presentedViewController
          as? ModalSettingsNavigationController,
          let presentedRootController = presentedNavigationController.viewControllers.first,
          presentedRootController is SearchSettingsViewController
        {
          searchEngineSettingsDismissed = true
        }

        shouldEvaluateKeyboardConstraints =
          (activeKeyboardHeight > 0)
          && (presentedViewController == nil || searchEngineSettingsDismissed)

        if shouldEvaluateKeyboardConstraints {
          var offset = -activeKeyboardHeight
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
      if toolbar == nil {
        make.height.equalTo(0)
      }
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
        webViewContainerTopOffset =
          make.top.equalTo(readerModeBar?.snp.bottom ?? self.toolbarLayoutGuide).constraint
      } else {
        webViewContainerTopOffset =
          make.top.equalTo(readerModeBar?.snp.bottom ?? self.header.snp.bottom).constraint
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

      if let keyboardHeight = keyboardState?.intersectionHeightForView(self.view),
        keyboardHeight > 0
      {
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
        rewards: rewards,
        privateBrowsingManager: privateBrowsingManager,
        p3aHelper: ntpP3AHelper
      )
      // Donate NewTabPage Activity For Custom Suggestions
      let newTabPageActivity =
        ActivityShortcutManager.shared.createShortcutActivity(
          type: selectedTab.isPrivate ? .newPrivateTab : .newTab
        )

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
      let subview = isUsingBottomBar ? header : statusBarOverlay
      view.insertSubview(ntpController.view, belowSubview: subview)
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
        }
      )
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
          let readerMode = tab.getContentScript(name: ReaderModeScriptHandler.scriptName)
            as? ReaderModeScriptHandler,
          readerMode.state == .active,
          isReaderModeURL
        {
          self.showReaderModeBar(animated: false)
          self.updatePlaylistURLBar(tab: tab, state: tab.playlistItemState, item: tab.playlistItem)
        }
      }
    )
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
      } else if !url.absoluteString.hasPrefix(
        "\(InternalURL.baseUrl)/\(SessionRestoreHandler.path)"
      ) {
        hideActiveNewTabPageController(url.isInternalURL(for: .readermode))
      }
    } else if isAboutHomeURL {
      showNewTabPageController()
    }
  }

  func updateTabsBarVisibility() {
    defer {
      toolbar?.line.isHidden = isUsingBottomBar
    }

    tabsBar.view.removeFromSuperview()
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
      guard
        let tabBarVisibility = TabBarVisibility(
          rawValue: Preferences.General.tabBarVisibility.value
        )
      else {
        // This should never happen
        assertionFailure(
          "Invalid tab bar visibility preference: \(Preferences.General.tabBarVisibility.value)."
        )
        return tabCount > 1
      }
      switch tabBarVisibility {
      case .always:
        return tabCount > 1 || UIDevice.current.userInterfaceIdiom == .pad
      case .landscapeOnly:
        return (tabCount > 1 && UIDevice.current.orientation.isLandscape)
          || UIDevice.current.userInterfaceIdiom == .pad
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
      userInfo: [:]
    )

    let privateTabItem = UIMutableApplicationShortcutItem(
      type: "\(Bundle.main.bundleIdentifier ?? "").NewPrivateTab",
      localizedTitle: Strings.quickActionNewPrivateTab,
      localizedSubtitle: nil,
      icon: UIApplicationShortcutIcon(templateImageName: "quick_action_new_private_tab"),
      userInfo: [:]
    )

    var scanQRCodeItem: UIMutableApplicationShortcutItem?
    if !ProcessInfo.processInfo.isiOSAppOnVisionOS {
      scanQRCodeItem = UIMutableApplicationShortcutItem(
        type: "\(Bundle.main.bundleIdentifier ?? "").ScanQRCode",
        localizedTitle: Strings.scanQRCodeViewTitle,
        localizedSubtitle: nil,
        icon: UIApplicationShortcutIcon(templateImageName: "recent-search-qrcode"),
        userInfo: [:]
      )
    }

    UIApplication.shared.shortcutItems =
      Preferences.Privacy.privateBrowsingOnly.value
      ? [privateTabItem] : [newTabItem, privateTabItem]

    if let scanQRCodeItem {
      UIApplication.shared.shortcutItems?.append(scanQRCodeItem)
    }
  }

  /// The method that executes the url and make changes in UI to reset the toolbars
  /// for urls coming from various sources
  /// If url is bookmarklet check if it is coming from user defined source to decide whether to execute
  /// using isUserDefinedURLNavigation
  /// - Parameters:
  ///   - url: The url submitted
  ///   - isUserDefinedURLNavigation: Boolean for  determining if url navigation is done from user defined spot
  ///     user defined spot like Favourites or Bookmarks
  func finishEditingAndSubmit(_ url: URL, isUserDefinedURLNavigation: Bool = false) {
    if url.isBookmarklet {
      topToolbar.leaveOverlayMode()

      guard let tab = tabManager.selectedTab else {
        return
      }

      // Another Fix for: https://github.com/brave/brave-ios/pull/2296
      // Disable any sort of privileged execution contexts
      // IE: The user must explicitly tap a bookmark they have saved.
      // Block all other contexts such as redirects, downloads, embed, linked, etc..
      if isUserDefinedURLNavigation, let webView = tab.webView,
        let code = url.bookmarkletCodeComponent
      {
        webView.evaluateSafeJavaScript(
          functionName: code,
          contentWorld: .page,
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

      updateWebViewPageZoom(tab: tab)
    }
  }

  func showWeb3ServiceInterstitialPage(service: Web3Service, originalURL: URL) {
    topToolbar.leaveOverlayMode()

    guard let tab = tabManager.selectedTab,
      let encodedURL = originalURL.absoluteString.addingPercentEncoding(
        withAllowedCharacters: .alphanumerics
      ),
      let internalUrl = URL(
        string:
          "\(InternalURL.baseUrl)/\(Web3DomainHandler.path)?\(Web3NameServiceScriptHandler.ParamKey.serviceId.rawValue)=\(service.rawValue)&url=\(encodedURL)"
      )
    else {
      return
    }
    let scriptHandler =
      tab.getContentScript(name: Web3NameServiceScriptHandler.scriptName)
      as? Web3NameServiceScriptHandler
    scriptHandler?.originalURL = originalURL

    tab.webView?.load(PrivilegedRequest(url: internalUrl) as URLRequest)
  }

  override public func accessibilityPerformEscape() -> Bool {
    if topToolbar.inOverlayMode {
      topToolbar.didClickCancel()
      return true
    } else if let selectedTab = tabManager.selectedTab, selectedTab.canGoBack {
      selectedTab.goBack()
      resetExternalAlertProperties(selectedTab)
      return true
    }
    return false
  }

  override public func observeValue(
    forKeyPath keyPath: String?,
    of object: Any?,
    change: [NSKeyValueChangeKey: Any]?,
    context: UnsafeMutableRawPointer?
  ) {

    guard let webView = object as? WKWebView else {
      Logger.module.error(
        "An object of type: \(String(describing: object), privacy: .public) is being observed instead of a WKWebView"
      )
      return  // False alarm.. the source MUST be a web view.
    }

    // WebView is a zombie and somehow still has an observer attached to it
    guard let tab = tabManager[webView] else {
      Logger.module.error(
        "WebView has been removed from TabManager but still has attached observers"
      )
      return
    }

    // Must handle ALL keypaths
    guard let kp = keyPath else {
      assertionFailure("Unhandled KVO key: \(keyPath ?? "nil")")
      return
    }

    let path = KVOConstants(keyPath: kp)
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
        // There is a bug in WebKit where if you cancel a load on a request the progress can stick to 0.1
        if !tab.loading, webView.estimatedProgress != 1 {
          topToolbar.updateProgressBar(1)
        }
      }
    case .url:
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
      } else {
        updateURLBar()

        // If navigation will start from NTP, tab display url will be nil until
        // didCommit is called and it will cause url bar be empty in that period
        // To fix this when tab display url is empty, webview url is used
        if tab === tabManager.selectedTab, tab.url?.displayURL == nil {
          if let url = webView.url, !url.isLocal, !InternalURL.isValid(url: url) {
            updateToolbarCurrentURL(url.displayURL)
          }
        } else if tab === tabManager.selectedTab, tab.url?.displayURL?.scheme == "about",
          !webView.isLoading
        {
          if let url = webView.url {
            tab.url = url

            if !tab.restoring {
              updateUIForReaderHomeStateForTab(tab)
            }

            navigateInTab(tab: tab)
          }
        } else if tab === tabManager.selectedTab, tab.isDisplayingBasicAuthPrompt {
          updateToolbarCurrentURL(
            URL(string: "\(InternalURL.baseUrl)/\(InternalURL.Path.basicAuth.rawValue)")
          )
        }
      }

      // Rewards reporting
      if let url = change?[.newKey] as? URL, !url.isLocal {
        // Notify Brave Rewards library of the same document navigation.
        if let tab = tabManager.selectedTab,
          let rewardsURL = tab.rewardsXHRLoadURL,
          url.host == rewardsURL.host
        {
          tab.reportPageNavigation(to: rewards)
          if let url = webView.url {
            tab.reportPageLoad(to: rewards, redirectChain: [url])
          }
        }
      }

      // Update the estimated progress when the URL changes. Estimated progress may update to 0.1 when the url
      // is still an internal URL even though a request may be pending for a web page.
      if tab === tabManager.selectedTab, let url = webView.url,
        !InternalURL.isValid(url: url), webView.estimatedProgress > 0
      {
        topToolbar.updateProgressBar(Float(webView.estimatedProgress))
      }

      Task {
        await tab.updateSecureContentState()
        self.logSecureContentState(tab: tab, path: .url, change: change)
        if self.tabManager.selectedTab === tab {
          self.updateToolbarSecureContentState(tab.lastKnownSecureContentState)
        }
      }
    case .title:
      // Ensure that the tab title *actually* changed to prevent repeated calls
      // to navigateInTab(tab:).
      guard
        let title = (webView.title?.isEmpty == true ? webView.url?.absoluteString : webView.title)
      else { break }
      if !title.isEmpty && title != tab.lastTitle {
        navigateInTab(tab: tab)
        tabsBar.updateSelectedTabTitle()

        if let url = webView.url,
          webView.configuration.preferences.isFraudulentWebsiteWarningEnabled,
          webView.responds(to: Selector(("_safeBrowsingWarning"))),
          webView.value(forKey: "_safeBrowsingWarning") != nil
        {
          tab.url = url  // We can update the URL whenever showing an interstitial warning
          updateToolbarCurrentURL(url.displayURL)
          updateInContentHomePanel(url)
        }
      }
    case .canGoBack, .canGoForward:
      guard tab === tabManager.selectedTab else {
        break
      }

      updateBackForwardActionStatus(for: webView)
    case .hasOnlySecureContent:
      Task {
        await tab.updateSecureContentState()
        self.logSecureContentState(tab: tab, path: .hasOnlySecureContent, change: change)
        if tabManager.selectedTab === tab {
          self.updateToolbarSecureContentState(tab.lastKnownSecureContentState)
        }
      }
    case .serverTrust:
      Task {
        await tab.updateSecureContentState()
        self.logSecureContentState(tab: tab, path: .serverTrust, change: change)
        if self.tabManager.selectedTab === tab {
          self.updateToolbarSecureContentState(tab.lastKnownSecureContentState)
        }
      }
    case ._sampledPageTopColor:
      updateStatusBarOverlayColor()
    default:
      assertionFailure("Unhandled KVO key: \(kp)")
    }
  }

  func logSecureContentState(
    tab: Tab,
    path: KVOConstants? = nil,
    change: [NSKeyValueChangeKey: Any]? = nil
  ) {
    var text = """
      Tab URL: \(tab.url?.absoluteString ?? "Empty Tab URL")
       Secure State: \(tab.lastKnownSecureContentState.rawValue)
      """

    if let keyPath = path?.keyPath {
      text.append("\n Value Observed: \(keyPath)\n")
    }

    if let webView = tab.webView {
      text.append(
        """
         WebView url: \(webView.url?.absoluteString ?? "nil")
         WebView hasOnlySecureContent: \(webView.hasOnlySecureContent ? "true" : "false")
         WebView serverTrust: \(webView.serverTrust != nil ? "present" : "nil")
        """
      )
    }

    if let change, path == .serverTrust, let newServerTrust = change[.newKey] {
      text.append("\n Change: \(newServerTrust != nil ? "present" : "nil")")
    } else if let change, let value = change[.newKey] {
      text.append("\n Change: \(String(describing: value))")
    }

    DebugLogger.log(for: .secureState, text: text)
  }

  func updateBackForwardActionStatus(for webView: WKWebView?) {
    guard let webView = webView else { return }

    if let forwardListItem = webView.backForwardList.forwardList.first,
      forwardListItem.url.isInternalURL(for: .readermode)
    {
      navigationToolbar.updateForwardStatus(false)
    } else {
      navigationToolbar.updateForwardStatus(webView.canGoForward)
    }

    navigationToolbar.updateBackStatus(webView.canGoBack)
  }

  func updateUIForReaderHomeStateForTab(_ tab: Tab) {
    updateURLBar()
    toolbarVisibilityViewModel.toolbarState = .expanded

    if let url = tab.url {
      if url.isInternalURL(for: .readermode) {
        showReaderModeBar(animated: false)
        NotificationCenter.default.addObserver(
          self,
          selector: #selector(dynamicFontChanged),
          name: .dynamicFontChanged,
          object: nil
        )
      } else {
        hideReaderModeBar(animated: false)
        NotificationCenter.default.removeObserver(self, name: .dynamicFontChanged, object: nil)
      }

      updateInContentHomePanel(url as URL)
      updateScreenTimeUrl(url)
      updatePlaylistURLBar(tab: tab, state: tab.playlistItemState, item: tab.playlistItem)
    }
  }

  /// Updates the URL bar security, text and button states.
  func updateURLBar() {
    guard let tab = tabManager.selectedTab else { return }

    updateRewardsButtonState()

    DispatchQueue.main.async {
      if let item = tab.playlistItem {
        if PlaylistItem.itemExists(uuid: item.tagId)
          || PlaylistItem.itemExists(pageSrc: item.pageSrc)
        {
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
      self.updateToolbarSecureContentState(tab.lastKnownSecureContentState)
    }

    let isPage = tab.url?.isWebPage() ?? false
    navigationToolbar.updatePageStatus(isPage)
    updateWebViewPageZoom(tab: tab)
  }

  public func moveTab(tabId: UUID, to browser: BrowserViewController) {
    guard let tab = tabManager.allTabs.filter({ $0.id == tabId }).first,
      let url = tab.url
    else {
      return
    }

    let isPrivate = tab.isPrivate
    tabManager.removeTab(tab)
    browser.tabManager.addTabsForURLs([url], zombie: false, isPrivate: isPrivate)
  }

  public func switchToTabForURLOrOpen(_ url: URL, isPrivate: Bool = false, isPrivileged: Bool) {
    popToBVC(isAnimated: false)

    if let tab = tabManager.getTabForURL(url, isPrivate: isPrivate) {
      tabManager.selectTab(tab)
    } else {
      openURLInNewTab(url, isPrivate: isPrivate, isPrivileged: isPrivileged)
    }
  }

  func switchToTabOrOpen(id: UUID?, url: URL) {
    popToBVC()

    if let tabID = id, let tab = tabManager.getTabForID(tabID) {
      tabManager.selectTab(tab)
    } else if let tab = tabManager.getTabForURL(
      url,
      isPrivate: privateBrowsingManager.isPrivateBrowsing
    ) {
      tabManager.selectTab(tab)
    } else {
      openURLInNewTab(url, isPrivate: privateBrowsingManager.isPrivateBrowsing, isPrivileged: false)
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
      // We also need to respect private browsing mode when opening URLs directly.
      // If the only tab open is NTP, AND the private mode matches that of the tab request,
      // only then we can open the tab directly.
      if tabManager.isBrowserEmptyForCurrentMode
        && tabManager.privateBrowsingManager.isPrivateBrowsing == isPrivate
      {
        finishEditingAndSubmit(url)
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

  public func openBlankNewTab(
    attemptLocationFieldFocus: Bool,
    isPrivate: Bool,
    searchFor searchText: String? = nil,
    isExternal: Bool = false
  ) {
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

  func openInNewWindow(url: URL?, isPrivate: Bool) {
    let activity = BrowserState.userActivity(
      for: UUID().uuidString,
      isPrivate: isPrivate,
      openURL: url
    )

    let options = UIScene.ActivationRequestOptions().then {
      $0.requestingScene = view.window?.windowScene
    }

    UIApplication.shared.requestSceneSessionActivation(
      nil,
      userActivity: activity,
      options: options,
      errorHandler: { error in
        Logger.module.error("Error creating new window: \(error)")
      }
    )
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

        self.tabManager.clearTabHistory {
          self.openBlankNewTab(attemptLocationFieldFocus: false, isPrivate: false, isExternal: true)
          self.popToBVC()
        }
      }
    }
  }

  func openInsideSettingsNavigation(with viewController: UIViewController) {
    let settingsNavigationController = SettingsNavigationController(
      rootViewController: viewController
    )
    settingsNavigationController.isModalInPresentation = false
    settingsNavigationController.modalPresentationStyle =
      UIDevice.current.userInterfaceIdiom == .phone ? .pageSheet : .formSheet
    settingsNavigationController.navigationBar.topItem?.leftBarButtonItem =
      UIBarButtonItem(
        barButtonSystemItem: .done,
        target: settingsNavigationController,
        action: #selector(settingsNavigationController.done)
      )

    // All menu views should be opened in portrait on iPhones.
    DeviceOrientation.shared.changeOrientationToPortraitOnPhone()

    present(settingsNavigationController, animated: true)
  }

  func popToBVC(isAnimated: Bool = true, completion: (() -> Void)? = nil) {
    guard let currentViewController = navigationController?.topViewController else {
      return
    }
    currentViewController.dismiss(animated: isAnimated, completion: completion)

    if currentViewController != self {
      _ = self.navigationController?.popViewController(animated: true)
    } else if topToolbar.inOverlayMode {
      topToolbar.didClickCancel()
    }
  }

  func displayPageZoom(visible: Bool) {
    if !visible || pageZoomBar != nil {
      pageZoomBar?.view.removeFromSuperview()
      updateViewConstraints()
      pageZoomBar = nil

      return
    }

    guard let selectTab = tabManager.selectedTab else { return }
    let zoomHandler = PageZoomHandler(
      tab: selectTab,
      isPrivateBrowsing: privateBrowsingManager.isPrivateBrowsing
    )
    let pageZoomBar = UIHostingController(rootView: PageZoomView(zoomHandler: zoomHandler))

    pageZoomBar.rootView.dismiss = { [weak self] in
      guard let self = self else { return }
      pageZoomBar.view.removeFromSuperview()
      self.updateViewConstraints()
      self.pageZoomBar = nil
    }

    alertStackView.arrangedSubviews.forEach({
      $0.removeFromSuperview()
    })
    alertStackView.addArrangedSubview(pageZoomBar.view)

    pageZoomBar.view.snp.makeConstraints { make in
      make.height.greaterThanOrEqualTo(UIConstants.toolbarHeight)
      make.height.equalTo(UIConstants.toolbarHeight).priority(.high)
      make.edges.equalTo(alertStackView)
    }

    updateViewConstraints()
    self.pageZoomBar = pageZoomBar
  }

  func updateWebViewPageZoom(tab: Tab) {
    if let currentURL = tab.url {
      let domain = Domain.getPersistedDomain(for: currentURL)

      let zoomLevel =
        privateBrowsingManager.isPrivateBrowsing
        ? 1.0 : domain?.zoom_level?.doubleValue ?? Preferences.General.defaultPageZoomLevel.value
      tab.webView?.setValue(zoomLevel, forKey: PageZoomHandler.propertyName)
    }
  }

  public override var preferredStatusBarStyle: UIStatusBarStyle {
    if isUsingBottomBar, let tab = tabManager.selectedTab,
      tab.url.map(InternalURL.isValid) == false,
      let color = tab.webView?.sampledPageTopColor
    {
      return color.isLight ? .darkContent : .lightContent
    }
    return super.preferredStatusBarStyle
  }

  func updateStatusBarOverlayColor() {
    defer { setNeedsStatusBarAppearanceUpdate() }
    guard isUsingBottomBar, let tab = tabManager.selectedTab,
      tab.url.map(InternalURL.isValid) == false,
      let color = tab.webView?.sampledPageTopColor
    else {
      statusBarOverlay.backgroundColor = privateBrowsingManager.browserColors.chromeBackground
      return
    }
    statusBarOverlay.backgroundColor = color
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

      if !InternalURL.isValid(url: url) || url.isInternalURL(for: .readermode), !url.isFileURL {
        // Fire the readability check. This is here and not in the pageShow event handler in ReaderMode.js anymore
        // because that event will not always fire due to unreliable page caching. This will either let us know that
        // the currently loaded page can be turned into reading mode or if the page already is in reading mode. We
        // ignore the result because we are being called back asynchronous when the readermode status changes.
        webView.evaluateSafeJavaScript(
          functionName: "\(readerModeNamespace).checkReadability",
          contentWorld: ReaderModeScriptHandler.scriptSandbox
        )

        // Only add history of a url which is not a localhost url
        if !url.isInternalURL(for: .readermode) {
          if !tab.isPrivate {
            braveCore.historyAPI.add(url: url, title: tab.title, dateAdded: Date())
          }

          // Saving Tab.
          tabManager.saveTab(tab)
        }
      }

      TabEvent.post(.didChangeURL(url), for: tab)
    }

    tabsBar.reloadDataAndRestoreSelectedTab(isAnimated: false)

    if tab === tabManager.selectedTab {
      updateStatusBarOverlayColor()

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
      let alert = UIAlertController(
        title: Strings.scanQRCodeViewTitle,
        message: Strings.scanQRCodePermissionErrorMessage,
        preferredStyle: .alert
      )
      alert.addAction(
        UIAlertAction(title: Strings.scanQRCodeErrorOKButton, style: .default, handler: nil)
      )
      self.present(alert, animated: true, completion: nil)
    }
  }

  func toggleReaderMode() {
    guard let tab = tabManager.selectedTab else { return }
    if let readerMode = tab.getContentScript(name: ReaderModeScriptHandler.scriptName)
      as? ReaderModeScriptHandler
    {
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

  func handleToolbarVisibilityStateChange(
    _ state: ToolbarVisibilityViewModel.ToolbarState,
    progress: CGFloat?
  ) {
    guard
      let tab = tabManager.selectedTab,
      let webView = tab.webView,
      !webView.isLoading
    else {

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
          tabsBar.view.alpha = topToolbar.locationContainer.alpha
          toolbar?.actionButtons.forEach { $0.alpha = topToolbar.locationContainer.alpha }
        }
        animator.startAnimation()
      }
      return
    }
    let headerHeight = isUsingBottomBar ? 0 : toolbarVisibilityViewModel.transitionDistance
    let footerHeight =
      footer.bounds.height
      + (isUsingBottomBar
        ? toolbarVisibilityViewModel.transitionDistance - view.safeAreaInsets.bottom : 0)
    // Changing the web view size while scrolling and a PDF is visible causes strange flickering, so only show
    // final expanded/collapsed states while a PDF is visible
    if let progress = progress, tab.mimeType != MIMEType.pdf {
      switch state {
      case .expanded:
        toolbarTopConstraint?.update(offset: -min(headerHeight, max(0, headerHeight * progress)))
        // Have it disappear a bit (1.5x) faster
        topToolbar.locationContainer.alpha = max(0, min(1, 1 - (progress * 1.5)))
        toolbarBottomConstraint?.update(offset: min(footerHeight, max(0, footerHeight * progress)))
      case .collapsed:
        toolbarTopConstraint?.update(
          offset: -min(headerHeight, max(0, headerHeight * (1 - progress)))
        )
        topToolbar.locationContainer.alpha = progress
        toolbarBottomConstraint?.update(
          offset: min(footerHeight, max(0, footerHeight * (1 - progress)))
        )
      }
      topToolbar.actionButtons.forEach { $0.alpha = topToolbar.locationContainer.alpha }
      tabsBar.view.alpha = topToolbar.locationContainer.alpha
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
    tabsBar.view.alpha = topToolbar.locationContainer.alpha
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
    let overlayText = URLFormatter.formatURL(
      url.absoluteString,
      formatTypes: [],
      unescapeOptions: []
    )

    popToBVC {
      self.topToolbar.enterOverlayMode(overlayText, pasted: false, search: false)
    }

    if !url.isBookmarklet && !privateBrowsingManager.isPrivateBrowsing {
      RecentSearch.addItem(type: .qrCode, text: nil, websiteUrl: url.absoluteString)
    }
  }

  func didScanQRCodeWithText(_ text: String) {
    popToBVC()
    submitSearchText(text)

    if !privateBrowsingManager.isPrivateBrowsing {
      RecentSearch.addItem(type: .qrCode, text: text, websiteUrl: nil)
    }
  }
}

extension BrowserViewController: SettingsDelegate {
  func settingsOpenURLInNewTab(_ url: URL) {
    popToBVC()

    self.openURLInNewTab(
      url,
      isPrivate: privateBrowsingManager.isPrivateBrowsing,
      isPrivileged: false
    )
  }

  func settingsOpenURLs(_ urls: [URL], loadImmediately: Bool) {
    let tabIsPrivate = TabType.of(tabManager.selectedTab).isPrivate
    self.tabManager.addTabsForURLs(urls, zombie: !loadImmediately, isPrivate: tabIsPrivate)
  }
}

extension BrowserViewController: PresentingModalViewControllerDelegate {
  func dismissPresentedModalViewController(_ modalViewController: UIViewController, animated: Bool)
  {
    self.dismiss(animated: animated, completion: nil)
  }
}

extension BrowserViewController: TabsBarViewControllerDelegate {
  func tabsBarDidSelectAddNewTab(_ isPrivate: Bool) {
    recordCreateTabAction(location: .toolbar)
    openBlankNewTab(attemptLocationFieldFocus: false, isPrivate: isPrivate)
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

  func tabsBarDidSelectAddNewWindow(_ isPrivate: Bool) {
    self.openInNewWindow(url: nil, isPrivate: isPrivate)
  }
}

extension BrowserViewController: TabDelegate {
  func tab(_ tab: Tab, didCreateWebView webView: WKWebView) {
    webView.frame = webViewContainer.frame

    // Observers that live as long as the tab. Make sure these are all cleared in willDeleteWebView below!
    KVOs.forEach { webView.addObserver(self, forKeyPath: $0.keyPath, options: .new, context: nil) }
    webView.uiDelegate = self

    var injectedScripts: [TabContentScript] = [
      ReaderModeScriptHandler(),
      ErrorPageHelper(certStore: profile.certStore),
      SessionRestoreScriptHandler(),
      BlockedDomainScriptHandler(),
      HTTPBlockedScriptHandler(tabManager: tabManager),
      PrintScriptHandler(browserController: self),
      CustomSearchScriptHandler(),
      DarkReaderScriptHandler(),
      FocusScriptHandler(),
      BraveGetUA(),
      BraveSearchScriptHandler(profile: profile, rewards: rewards),
      ResourceDownloadScriptHandler(),
      DownloadContentScriptHandler(browserController: self),
      PlaylistScriptHandler(tab: tab),
      PlaylistFolderSharingScriptHandler(),
      RewardsReportingScriptHandler(rewards: rewards),
      AdsMediaReportingScriptHandler(rewards: rewards),
      ReadyStateScriptHandler(),
      DeAmpScriptHandler(),
      SiteStateListenerScriptHandler(),
      CosmeticFiltersScriptHandler(),
      URLPartinessScriptHandler(),
      FaviconScriptHandler(),
      Web3NameServiceScriptHandler(),
      YoutubeQualityScriptHandler(tab: tab),
      BraveLeoScriptHandler(),
      BraveSkusScriptHandler(),

      tab.contentBlocker,
      tab.requestBlockingContentHelper,

      BraveTranslateScriptLanguageDetectionHandler(),
      BraveTranslateScriptHandler(),
    ]

    #if canImport(BraveTalk)
    injectedScripts.append(
      BraveTalkScriptHandler(
        rewards: rewards,
        launchNativeBraveTalk: { [weak self] tab, room, token in
          self?.launchNativeBraveTalk(tab: tab, room: room, token: token)
        }
      )
    )
    #endif

    // Only add the logins handler and wallet provider if the tab is NOT a private browsing tab
    if !tab.isPrivate {
      injectedScripts += [
        LoginsScriptHandler(profile: profile, passwordAPI: braveCore.passwordAPI),
        EthereumProviderScriptHandler(),
        SolanaProviderScriptHandler(),
        BraveSearchResultAdScriptHandler(),
      ]
    }

    // XXX: Bug 1390200 - Disable NSUserActivity/CoreSpotlight temporarily
    // let spotlightHelper = SpotlightHelper(tab: tab)
    // tab.addHelper(spotlightHelper, name: SpotlightHelper.name())

    injectedScripts.forEach {
      tab.addContentScript(
        $0,
        name: type(of: $0).scriptName,
        contentWorld: type(of: $0).scriptSandbox
      )
    }

    (tab.getContentScript(name: ReaderModeScriptHandler.scriptName) as? ReaderModeScriptHandler)?
      .delegate = self
    (tab.getContentScript(name: SessionRestoreScriptHandler.scriptName)
      as? SessionRestoreScriptHandler)?.delegate = self
    (tab.getContentScript(name: PlaylistScriptHandler.scriptName) as? PlaylistScriptHandler)?
      .delegate = self
    (tab.getContentScript(name: PlaylistFolderSharingScriptHandler.scriptName)
      as? PlaylistFolderSharingScriptHandler)?.delegate = self
    (tab.getContentScript(name: Web3NameServiceScriptHandler.scriptName)
      as? Web3NameServiceScriptHandler)?.delegate = self

    // Translate Helper
    tab.translateHelper = BraveTranslateTabHelper(tab: tab, delegate: self)
  }

  func tab(_ tab: Tab, willDeleteWebView webView: WKWebView) {
    tab.cancelQueuedAlerts()
    KVOs.forEach { webView.removeObserver(self, forKeyPath: $0.keyPath) }
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
      }
    )
  }

  func removeBar(_ bar: SnackBar, animated: Bool) {
    UIView.animate(
      withDuration: animated ? 0.25 : 0,
      animations: {
        bar.removeFromSuperview()
      }
    )
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
    if let findInteraction = tab.webView?.findInteraction {
      findInteraction.searchText = selectedText
      findInteraction.presentFindNavigator(showingReplace: false)
    }
  }

  /// Triggered when "Search with Brave" is selected on selected web text
  func tab(_ tab: Tab, didSelectSearchWithBraveFor selectedText: String) {
    let engine = profile.searchEngines.defaultEngine(
      forType: tab.isPrivate ? .privateMode : .standard
    )

    guard let url = engine?.searchURLForQuery(selectedText) else {
      assertionFailure("If this returns nil, investigate why and add proper handling or commenting")
      return
    }

    tabManager.addTabAndSelect(
      URLRequest(url: url),
      afterTab: tab,
      isPrivate: tab.isPrivate
    )

    if !privateBrowsingManager.isPrivateBrowsing {
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
      contentSizeBehavior: .preferredContentSize
    )
    popover.addsConvenientDismissalMargins = false
    popover.present(from: topToolbar.rewardsButton, on: self)
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
        contentSizeBehavior: .preferredContentSize
      )
      popover2.present(from: self.topToolbar.rewardsButton, on: self)
    }

    vc.linkTapped = { [unowned self] request in
      tab.rewardsEnabledCallback?(false)
      self.tabManager
        .addTabAndSelect(request, isPrivate: privateBrowsingManager.isPrivateBrowsing)
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
      Preferences.Wallet.displayWeb3Notifications.value
    else {
      return
    }
    let origin = tab.getOrigin()
    let tabDappStore = tab.tabDappStore
    let walletNotificaton = WalletNotification(
      priority: .low,
      origin: origin,
      isUsingBottomBar: isUsingBottomBar
    ) { [weak self] action in
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
        topToolbar.updateWalletButtonState(
          isPendingRequestAvailable ? .activeWithPendingRequest : .active
        )
      }
    } else {
      topToolbar.updateWalletButtonState(.inactive)
    }
  }

  func didReloadTab(_ tab: Tab) {
    // Resetting External Alert Properties
    resetExternalAlertProperties(tab)
  }

  @MainActor
  private func isPendingRequestAvailable() async -> Bool {
    let privateMode = privateBrowsingManager.isPrivateBrowsing
    // If we have an open `WalletStore`, use that so we can assign the pending request if the wallet is open,
    // which allows us to store the new `PendingRequest` triggering a modal presentation for that request.
    guard
      let cryptoStore = self.walletStore?.cryptoStore
        ?? CryptoStore.from(
          ipfsApi: braveCore.ipfsAPI,
          walletP3A: braveCore.braveWalletAPI.walletP3A(),
          privateMode: privateMode
        )
    else {
      return false
    }
    if await cryptoStore.isPendingRequestAvailable() {
      return true
    } else if let selectedTabOrigin = tabManager.selectedTab?.url?.origin {
      if WalletProviderAccountCreationRequestManager.shared.hasPendingRequest(
        for: selectedTabOrigin,
        coinType: .sol
      ) {
        return true
      }
      return WalletProviderPermissionRequestsManager.shared.hasPendingRequest(
        for: selectedTabOrigin,
        coinType: .eth
      )
    }
    return false
  }

  func resetExternalAlertProperties(_ tab: Tab?) {
    if let tab = tab {
      tab.resetExternalAlertProperties()
    }
  }
}

extension BrowserViewController: SearchViewControllerDelegate {
  func searchViewController(
    _ searchViewController: SearchViewController,
    didSubmit query: String,
    braveSearchPromotion: Bool
  ) {
    topToolbar.leaveOverlayMode()
    processAddressBar(text: query, isBraveSearchPromotion: braveSearchPromotion)
  }

  func searchViewController(
    _ searchViewController: SearchViewController,
    didSubmitAIChat query: String
  ) {
    self.popToBVC()
    self.openBraveLeo(with: query)
  }

  func searchViewController(_ searchViewController: SearchViewController, didSelectURL url: URL) {
    finishEditingAndSubmit(url)
  }

  func searchViewController(
    _ searchViewController: SearchViewController,
    didSelectOpenTab tabInfo: (id: UUID?, url: URL)
  ) {
    switchToTabOrOpen(id: tabInfo.id, url: tabInfo.url)
  }

  func searchViewController(
    _ searchViewController: SearchViewController,
    didLongPressSuggestion suggestion: String
  ) {
    self.topToolbar.setLocation(suggestion, search: true)
  }

  func presentSearchSettingsController() {
    let settingsNavigationController = SearchSettingsViewController(
      profile: profile,
      privateBrowsingManager: privateBrowsingManager
    )
    let navController = ModalSettingsNavigationController(
      rootViewController: settingsNavigationController
    )

    self.present(navController, animated: true, completion: nil)
  }

  func searchViewController(
    _ searchViewController: SearchViewController,
    didHighlightText text: String,
    search: Bool
  ) {
    self.topToolbar.setLocation(text, search: search)
  }

  func searchViewController(
    _ searchViewController: SearchViewController,
    shouldFindInPage query: String
  ) {
    topToolbar.leaveOverlayMode()
    if let findInteraction = tabManager.selectedTab?.webView?.findInteraction {
      findInteraction.searchText = query
      findInteraction.presentFindNavigator(showingReplace: false)
    }
  }

  func searchViewControllerAllowFindInPage() -> Bool {
    if let url = tabManager.selectedTab?.webView?.url,
      let internalURL = InternalURL(url),
      internalURL.isAboutHomeURL
    {
      return false
    }
    return true
  }
}

// MARK: - UIPopoverPresentationControllerDelegate

extension BrowserViewController: UIPopoverPresentationControllerDelegate {
  public func popoverPresentationControllerDidDismissPopover(
    _ popoverPresentationController: UIPopoverPresentationController
  ) {
    displayedPopoverController = nil
    updateDisplayedPopoverProperties = nil
  }
}

extension BrowserViewController: UIAdaptivePresentationControllerDelegate {
  // Returning None here makes sure that the Popover is actually presented as a Popover and
  // not as a full-screen modal, which is the default on compact device classes.
  public func adaptivePresentationStyle(
    for controller: UIPresentationController,
    traitCollection: UITraitCollection
  ) -> UIModalPresentationStyle {
    return .none
  }

  public func presentationControllerDidDismiss(_ presentationController: UIPresentationController) {
    // need to update tab bar visibility after user dismiss the `ChromeWebViewController`
    updateTabsBarVisibility()
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

  func didCreateTab() {
    recordCreateTabAction(location: .tabTray)
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

    select(url, action: .openInNewTab(isPrivate: isPrivate), isUserDefinedURLNavigation: false)
  }

  func copy(_ url: URL) {
    select(url, action: .copy, isUserDefinedURLNavigation: false)
  }

  func share(_ url: URL) {
    select(url, action: .share, isUserDefinedURLNavigation: false)
  }

  func batchOpen(_ urls: [URL]) {
    let tabIsPrivate = TabType.of(tabManager.selectedTab).isPrivate
    self.tabManager.addTabsForURLs(urls, zombie: false, isPrivate: tabIsPrivate)
  }

  func select(url: URL, isUserDefinedURLNavigation: Bool) {
    select(url, action: .openInCurrentTab, isUserDefinedURLNavigation: isUserDefinedURLNavigation)
  }

  private func select(_ url: URL, action: ToolbarURLAction, isUserDefinedURLNavigation: Bool) {
    switch action {
    case .openInCurrentTab:
      finishEditingAndSubmit(url, isUserDefinedURLNavigation: isUserDefinedURLNavigation)
      updateURLBarWalletButton()
    case .openInNewTab(let isPrivate):
      let tab = tabManager.addTab(
        PrivilegedRequest(url: url) as URLRequest,
        afterTab: tabManager.selectedTab,
        isPrivate: isPrivate
      )
      if isPrivate && !privateBrowsingManager.isPrivateBrowsing {
        tabManager.selectTab(tab)
      } else {
        // If we are showing toptabs a user can just use the top tab bar
        // If in overlay mode switching doesnt correctly dismiss the homepanels
        guard !topToolbar.inOverlayMode else {
          return
        }
        // We're not showing the top tabs; show a toast to quick switch to the fresh new tab.
        let toast = ButtonToast(
          labelText: Strings.contextMenuButtonToastNewTabOpenedLabelText,
          buttonText: Strings.contextMenuButtonToastNewTabOpenedButtonText,
          completion: { buttonPressed in
            if buttonPressed {
              self.tabManager.selectTab(tab)
            }
          }
        )
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
            sourceRect: self.view.convert(
              self.topToolbar.shareButton.frame,
              from: self.topToolbar.shareButton.superview
            ),
            arrowDirection: [.up]
          )
        }
      } else {
        presentActivityViewController(
          url,
          sourceView: view,
          sourceRect: view.convert(
            topToolbar.shareButton.frame,
            from: topToolbar.shareButton.superview
          ),
          arrowDirection: [.up]
        )
      }
    }
  }
}

extension BrowserViewController: NewTabPageDelegate {
  func navigateToInput(_ input: String, inNewTab: Bool, switchingToPrivateMode: Bool) {
    handleURLInput(
      input,
      inNewTab: inNewTab,
      switchingToPrivateMode: switchingToPrivateMode,
      isFavourite: false
    )
  }

  func handleFavoriteAction(favorite: Favorite, action: BookmarksAction) {
    guard let url = favorite.url else { return }
    switch action {
    case .opened(let inNewTab, let switchingToPrivateMode):
      if switchingToPrivateMode, Preferences.Privacy.privateBrowsingLock.value {
        self.askForLocalAuthentication { [weak self] success, error in
          if success {
            self?.handleURLInput(
              url,
              inNewTab: inNewTab,
              switchingToPrivateMode: switchingToPrivateMode,
              isFavourite: true
            )
          }
        }
      } else {
        handleURLInput(
          url,
          inNewTab: inNewTab,
          switchingToPrivateMode: switchingToPrivateMode,
          isFavourite: true
        )
      }
    case .edited:
      guard let title = favorite.displayTitle, let urlString = favorite.url else { return }
      let editPopup =
        UIAlertController
        .userTextInputAlert(
          title: Strings.editFavorite,
          message: urlString,
          startingText: title,
          startingText2: favorite.url,
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

  /// Handling url input action and passing down if input is launched from favourites
  private func handleURLInput(
    _ input: String,
    inNewTab: Bool,
    switchingToPrivateMode: Bool,
    isFavourite: Bool
  ) {
    let isPrivate = privateBrowsingManager.isPrivateBrowsing || switchingToPrivateMode
    if inNewTab {
      tabManager.addTabAndSelect(isPrivate: isPrivate)
    }

    // Used to determine url navigation coming from a bookmark
    // And handle it differently under finishEditingAndSubmit for bookmarklets
    processAddressBar(text: input, isUserDefinedURLNavigation: isFavourite)
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
        url,
        sourceView: self.view,
        sourceRect: viewRect,
        arrowDirection: .any
      )
    }
  }
}

extension BrowserViewController: PreferencesObserver {
  public func preferencesDidChange(for key: String) {
    switch key {
    case Preferences.General.tabBarVisibility.key:
      updateTabsBarVisibility()
    case Preferences.Privacy.privateBrowsingOnly.key:
      privateBrowsingManager.isPrivateBrowsing = Preferences.Privacy.privateBrowsingOnly.value
      setupTabs()
      updateTabsBarVisibility()
      updateApplicationShortcuts()
    case Preferences.UserAgent.alwaysRequestDesktopSite.key:
      tabManager.reset()
      tabManager.reloadSelectedTab()
    case Preferences.General.enablePullToRefresh.key:
      tabManager.selectedTab?.updatePullToRefreshVisibility()
    case Preferences.Shields.blockScripts.key,
      Preferences.Shields.blockImages.key,
      Preferences.Shields.useRegionAdBlock.key:
      tabManager.reloadSelectedTab()
    case ShieldPreferences.blockAdsAndTrackingLevelRaw.key:
      tabManager.reloadSelectedTab()
      recordGlobalAdBlockShieldsP3A()
    case Preferences.Shields.fingerprintingProtection.key:
      tabManager.reloadSelectedTab()
      recordGlobalFingerprintingShieldsP3A()
    case Preferences.General.defaultPageZoomLevel.key:
      tabManager.allTabs.forEach({
        guard let url = $0.webView?.url else { return }
        let zoomLevel =
          $0.isPrivate
          ? 1.0
          : Domain.getPersistedDomain(for: url)?.zoom_level?.doubleValue
            ?? Preferences.General.defaultPageZoomLevel.value
        $0.webView?.setValue(zoomLevel, forKey: PageZoomHandler.propertyName)
      })
    case Preferences.Privacy.blockAllCookies.key,
      Preferences.Shields.googleSafeBrowsing.key:
      // All `block all cookies` toggle requires a hard reset of Webkit configuration.
      tabManager.reset()
      if !Preferences.Privacy.blockAllCookies.value {
        self.tabManager.reloadSelectedTab()
        for tab in self.tabManager.allTabs where tab != self.tabManager.selectedTab {
          tab.createWebview()
          if let url = tab.webView?.url {
            tab.loadRequest(PrivilegedRequest(url: url) as URLRequest)
          }
        }
      } else {
        tabManager.reloadSelectedTab()
      }
    case Preferences.Rewards.hideRewardsIcon.key,
      Preferences.Rewards.rewardsToggledOnce.key:
      updateRewardsButtonState()
    case Preferences.General.mediaAutoBackgrounding.key:
      tabManager.selectedTab?.setScripts(scripts: [
        .mediaBackgroundPlay: Preferences.General.mediaAutoBackgrounding.value
      ])
      tabManager.reloadSelectedTab()
    case Preferences.General.youtubeHighQuality.key:
      tabManager.allTabs.forEach {
        YoutubeQualityScriptHandler.setEnabled(
          option: Preferences.General.youtubeHighQuality,
          for: $0
        )
      }
    case Preferences.Playlist.enablePlaylistMenuBadge.key,
      Preferences.Playlist.enablePlaylistURLBarButton.key:
      let selectedTab = tabManager.selectedTab
      updatePlaylistURLBar(
        tab: selectedTab,
        state: selectedTab?.playlistItemState ?? .none,
        item: selectedTab?.playlistItem
      )
    case Preferences.PrivacyReports.captureShieldsData.key:
      PrivacyReportsManager.scheduleProcessingBlockedRequests(
        isPrivateBrowsing: privateBrowsingManager.isPrivateBrowsing
      )
      PrivacyReportsManager.scheduleNotification(debugMode: !AppConstants.isOfficialBuild)
    case Preferences.PrivacyReports.captureVPNAlerts.key:
      PrivacyReportsManager.scheduleVPNAlertsTask()
    case Preferences.Wallet.defaultEthWallet.key:
      tabManager.reset()
      tabManager.reloadSelectedTab()
      notificationsPresenter.removeNotification(with: WalletNotification.Constant.id)
      WalletProviderPermissionRequestsManager.shared.cancelAllPendingRequests(for: [.eth])
      WalletProviderAccountCreationRequestManager.shared.cancelAllPendingRequests(coins: [.eth])
      let privateMode = privateBrowsingManager.isPrivateBrowsing
      if let cryptoStore = CryptoStore.from(
        ipfsApi: braveCore.ipfsAPI,
        walletP3A: braveCore.braveWalletAPI.walletP3A(),
        privateMode: privateMode
      ) {
        cryptoStore.rejectAllPendingWebpageRequests()
      }
      updateURLBarWalletButton()
    case Preferences.Wallet.defaultSolWallet.key:
      tabManager.reset()
      tabManager.reloadSelectedTab()
      notificationsPresenter.removeNotification(with: WalletNotification.Constant.id)
      WalletProviderPermissionRequestsManager.shared.cancelAllPendingRequests(for: [.sol])
      WalletProviderAccountCreationRequestManager.shared.cancelAllPendingRequests(coins: [.sol])
      let privateMode = privateBrowsingManager.isPrivateBrowsing
      if let cryptoStore = CryptoStore.from(
        ipfsApi: braveCore.ipfsAPI,
        walletP3A: braveCore.braveWalletAPI.walletP3A(),
        privateMode: privateMode
      ) {
        cryptoStore.rejectAllPendingWebpageRequests()
      }
      updateURLBarWalletButton()
    case Preferences.Playlist.syncSharedFoldersAutomatically.key:
      syncPlaylistFolders()
    case Preferences.NewTabPage.backgroundMediaTypeRaw.key:
      recordAdsUsageType()
    case Preferences.Privacy.screenTimeEnabled.key:
      if Preferences.Privacy.screenTimeEnabled.value, !ProcessInfo.processInfo.isiOSAppOnVisionOS {
        // Accessing `STWebpageController` on Vision OS results in a crash
        screenTimeViewController = STWebpageController()
        if let tab = tabManager.selectedTab {
          recordScreenTimeUsage(for: tab)
        }
      } else {
        screenTimeViewController?.view.removeFromSuperview()
        screenTimeViewController?.willMove(toParent: nil)
        screenTimeViewController?.removeFromParent()
        screenTimeViewController?.suppressUsageRecording = true
        screenTimeViewController = nil
      }
    case Preferences.Translate.translateEnabled.key:
      tabManager.selectedTab?.setScripts(scripts: [
        .braveTranslate: Preferences.Translate.translateEnabled.value
      ])
      tabManager.reloadSelectedTab()
    default:
      Logger.module.debug(
        "Received a preference change for an unknown key: \(key, privacy: .public) on \(type(of: self), privacy: .public)"
      )
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
    // Remove Default Browser Callout - Do not show scheduled notification
    // in case an external url is triggered
    if case .url(let navigatedURL, _) = path {
      if navigatedURL?.isWebPage(includeDataURIs: false) == true {
        Preferences.General.lastHTTPURLOpenedDate.value = .now
        recordDefaultBrowserLikelyhoodP3A(openedHTTPLink: true)

        Preferences.General.defaultBrowserCalloutDismissed.value = true
        Preferences.DefaultBrowserIntro.defaultBrowserNotificationScheduled.value = true

        // Remove pending notification if default browser is set brave
        // Recognized by external link is open
        if !Preferences.DefaultBrowserIntro.defaultBrowserNotificationIsCanceled.value {
          cancelScheduleDefaultBrowserNotification()
        }
      }
    }

    executeAfterSetup {
      Task { @MainActor in
        if self.profile.searchEngines.orderedEngines.isEmpty {
          // Wait until search engines are ready
          await self.profile.searchEngines.waitForSearchEngines()
        }

        NavigationPath.handle(nav: path, with: self)
      }
    }
  }

  public func submitSearchText(_ text: String, isBraveSearchPromotion: Bool = false) {
    var engine = profile.searchEngines.defaultEngine(
      forType: privateBrowsingManager.isPrivateBrowsing ? .privateMode : .standard
    )

    if isBraveSearchPromotion {
      let braveSearchEngine = profile.searchEngines.orderedEngines.first {
        $0.shortName == OpenSearchEngine.EngineNames.brave
      }

      if let searchEngine = braveSearchEngine {
        engine = searchEngine
      }
    }

    if let searchURL = engine?.searchURLForQuery(
      text,
      isBraveSearchPromotion: isBraveSearchPromotion
    ) {
      // We couldn't find a matching search keyword, so do a search query.
      finishEditingAndSubmit(searchURL)
    } else {
      // We still don't have a valid URL, so something is broken. Give up.
      print("Error handling URL entry: \"\(text)\".")
      assertionFailure("Couldn't generate search URL: \(text)")
    }
  }
}

extension BrowserViewController {
  func presentTabReceivedToast(url: URL) {
    // 'Tab Received' indicator will only be shown in normal browsing
    if !privateBrowsingManager.isPrivateBrowsing {
      let toast = ButtonToast(
        labelText: Strings.Callout.tabReceivedCalloutTitle,
        image: UIImage(braveSystemNamed: "leo.smartphone.tablet-portrait"),
        buttonText: Strings.goButtonTittle,
        completion: { [weak self] buttonPressed in
          guard let self = self else { return }

          if buttonPressed {
            self.tabManager.addTabAndSelect(URLRequest(url: url), isPrivate: false)
          }
        }
      )

      show(toast: toast, duration: ButtonToastUX.toastDismissAfter)
    }
  }
}

extension BrowserViewController: UNUserNotificationCenterDelegate {
  public func userNotificationCenter(
    _ center: UNUserNotificationCenter,
    didReceive response: UNNotificationResponse,
    withCompletionHandler completionHandler: @escaping () -> Void
  ) {
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
  public func screenshotServiceGeneratePDFRepresentation(
    _ screenshotService: UIScreenshotService
  ) async -> (Data?, Int, CGRect) {
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
      rect.origin.y =
        webView.scrollView.contentSize.height - rect.height - webView.scrollView.contentOffset.y

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
    if privateBrowsingManager.isPrivateBrowsing {
      return
    }

    let host = UIHostingController(
      rootView: PrivacyReportsManager.prepareView(
        isPrivateBrowsing: privateBrowsingManager.isPrivateBrowsing
      )
    )

    host.rootView.openPrivacyReportsUrl = { [weak self] in
      guard let self = self else { return }
      let tab = self.tabManager.addTab(
        PrivilegedRequest(url: .brave.privacyFeatures) as URLRequest,
        afterTab: self.tabManager.selectedTab,
        // Privacy Reports view is unavailable in private mode.
        isPrivate: false
      )
      self.tabManager.selectTab(tab)
    }

    self.present(host, animated: true)
  }
}

extension BrowserViewController: BraveVPNInAppPurchaseObserverDelegate {
  public func purchasedOrRestoredProduct(validateReceipt: Bool) {
    // No-op
  }

  public func purchaseFailed(error: BraveVPNInAppPurchaseObserver.PurchaseError) {
    // No-op
  }

  public func handlePromotedInAppPurchase() {
    // Open VPN Buy Screen before system triggers buy action
    // Delaying the VPN Screen launch delibrately to syncronize promoted purchase launch
    Task.delayed(bySeconds: 2.0) { @MainActor in
      self.popToBVC()
      self.navigationHelper.openVPNBuyScreen(iapObserver: self.iapObserver)
    }
  }
}

// Certificate info
extension BrowserViewController {

  func displayPageCertificateInfo() {
    guard let tab = tabManager.selectedTab, let webView = tab.webView else {
      Logger.module.error("Invalid WebView")
      return
    }

    let getServerTrustForErrorPage = { () -> SecTrust? in
      do {
        if let url = webView.url {
          return try ErrorPageHelper.serverTrust(from: url)
        }
      } catch {
        Logger.module.error("\(error.localizedDescription)")
      }

      return nil
    }

    guard let trust = webView.serverTrust ?? getServerTrustForErrorPage() else {
      return
    }

    let host = webView.url?.host

    Task.detached {
      let serverCertificates: [SecCertificate] =
        SecTrustCopyCertificateChain(trust) as? [SecCertificate] ?? []

      // TODO: Instead of showing only the first cert in the chain,
      // have a UI that allows users to select any certificate in the chain (similar to Desktop browsers)
      if let serverCertificate = serverCertificates.first,
        let certificate = BraveCertificateModel(certificate: serverCertificate)
      {

        var errorDescription: String?

        do {
          try await BraveCertificateUtils.evaluateTrust(trust, for: host)
        } catch {
          Logger.module.error("\(error.localizedDescription)")

          // Remove the common-name from the first part of the error message
          // This is because the certificate viewer already displays it.
          // If it doesn't match, it won't be removed, so this is fine.
          errorDescription = error.localizedDescription
          if let range = errorDescription?.range(of: "“\(certificate.subjectName.commonName)” ")
            ?? errorDescription?.range(of: "\"\(certificate.subjectName.commonName)\" ")
          {
            errorDescription =
              errorDescription?.replacingCharacters(in: range, with: "").capitalizeFirstLetter
          }
        }

        await MainActor.run { [errorDescription] in
          // System components sit on top so we want to dismiss it
          webView.findInteraction?.dismissFindNavigator()
          let certificateViewController = CertificateViewController(
            certificate: certificate,
            evaluationError: errorDescription
          )
          certificateViewController.modalPresentationStyle = .pageSheet
          certificateViewController.sheetPresentationController?.detents = [.medium(), .large()]
          self.present(certificateViewController, animated: true)
        }
      }
    }
  }
}

extension BrowserViewController {
  private func openAIChatURL(_ url: URL) {
    let forcedPrivate = self.privateBrowsingManager.isPrivateBrowsing
    self.openURLInNewTab(url, isPrivate: forcedPrivate, isPrivileged: false)
  }

  func openBraveLeo(with query: String? = nil) {
    if !FeatureList.kAIChat.enabled {
      let alert = UIAlertController(
        title: Strings.AIChat.leoDisabledMessageTitle,
        message: Strings.AIChat.leoDisabledMessageDescription,
        preferredStyle: .alert
      )
      let action = UIAlertAction(title: Strings.OKString, style: .default)
      alert.addAction(action)
      present(alert, animated: true)
      return
    }

    let webView = (query == nil) ? tabManager.selectedTab?.webView : nil

    let model = AIChatViewModel(
      braveCore: braveCore,
      webView: webView,
      script: BraveLeoScriptHandler.self,
      braveTalkScript: self.braveTalkJitsiCoordinator,
      querySubmited: query
    )

    let chatController = UIHostingController(
      rootView: AIChatView(
        model: model,
        speechRecognizer: speechRecognizer,
        openURL: openAIChatURL
      )
    )
    present(chatController, animated: true)
  }
}

extension BraveTalkJitsiCoordinator: AIChatBraveTalkJavascript {
  @MainActor
  public func getTranscript() async -> String? {
    if self.isCallActive {
      return await jitsiTranscriptProcessor?.getTranscript()
    }
    return nil
  }
}
