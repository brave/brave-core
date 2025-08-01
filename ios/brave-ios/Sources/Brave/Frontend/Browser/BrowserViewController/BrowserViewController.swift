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
import Web
import WebKit
import os.log

import class Combine.AnyCancellable

#if canImport(BraveTalk)
import BraveTalk
#endif

#if canImport(BraveTalk)
import BraveTalk
#endif

public class BrowserViewController: UIViewController {
  let webViewContainer = UIView()
  private(set) lazy var screenshotHelper = ScreenshotHelper(tabManager: tabManager)

  private(set) lazy var topToolbar: TopToolbarView = {
    // Setup the URL bar, wrapped in a view to get transparency effect
    let topToolbar = TopToolbarView(
      speechRecognizer: speechRecognizer,
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

  public let windowId: UUID
  let profile: LegacyBrowserProfile
  let attributionManager: AttributionManager
  let braveCore: BraveCoreMain
  let profileController: BraveProfileController
  let tabManager: TabManager
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
  /// An infobar displaying a privacy notice when a new tab takeover is viewed
  var newTabTakeoverInfoBar: NewTabTakeoverInfoBar?
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
    profile: LegacyBrowserProfile,
    attributionManager: AttributionManager,
    braveCore: BraveCoreMain,
    profileController: BraveProfileController,
    rewards: BraveRewards,
    crashedLastSession: Bool,
    newsFeedDataSource: FeedDataSource,
    privateBrowsingManager: PrivateBrowsingManager
  ) {
    self.windowId = windowId
    self.profile = profile
    self.attributionManager = attributionManager
    self.braveCore = braveCore
    self.profileController = profileController
    self.bookmarkManager = BookmarkManager(bookmarksAPI: profileController.bookmarksAPI)
    self.rewards = rewards
    self.crashedLastSession = crashedLastSession
    self.privateBrowsingManager = privateBrowsingManager
    self.feedDataSource = newsFeedDataSource
    feedDataSource.historyAPI = profileController.historyAPI
    backgroundDataSource = .init(
      service: profileController.backgroundImagesService,
      rewards: rewards,
      privateBrowsingManager: privateBrowsingManager
    )

    // Add default favorites
    if !Preferences.NewTabPage.preloadedFavoritiesInitialized.value {
      FavoritesHelper.addDefaultFavorites()
      Preferences.NewTabPage.preloadedFavoritiesInitialized.value = true
    }

    // Initialize TabManager
    self.tabManager = TabManager(
      windowId: windowId,
      rewards: rewards,
      braveCore: profileController,
      privateBrowsingManager: privateBrowsingManager
    )

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
    if rewards.isEnabled {
      rewards.startRewardsService(nil)
    } else {
      rewards.ads.initialize { _ in }
    }

    self.feedDataSource.getAdsAPI = {
      // The ads object gets re-recreated when shutdown, so we need to make sure News fetches it out of
      // the BraveRewards container
      return rewards.ads
    }

    // Observer watching tab information is sent by another device
    openTabsModelStateListener = profileController.sendTabAPI.add(
      SendTabToSelfStateObserver { [weak self] stateChange in
        if case .sendTabToSelfEntriesAddedRemotely(let newEntries) = stateChange {
          // Fetching the last URL that has been sent from synced sessions
          if let requestedURL = newEntries.last?.url {
            self?.presentTabReceivedToast(url: requestedURL)
          }
        }
      }
    )

    // Update the preference based on the underlying Chromium prefs
    // Remove this when the pref is deleted: https://github.com/brave/brave-browser/issues/47487
    Preferences.Chromium.syncEnabled.value = profileController.syncAPI.isInSyncGroup

    // Observer watching state change in sync chain
    syncServiceStateListener = profileController.syncAPI.addServiceStateObserver { [weak self] in
      guard let self = self else { return }
      DispatchQueue.main.async {
        // Observe Sync State in order to determine if the sync chain is deleted
        // from another device - Clean local sync chain
        if self.profileController.syncAPI.shouldLeaveSyncGroup {
          self.profileController.syncAPI.leaveSyncGroup()
        }
      }
    }

    if Preferences.Privacy.screenTimeEnabled.value, !ProcessInfo.processInfo.isiOSAppOnVisionOS {
      // Accessing `STWebpageController` on Vision OS results in a crash
      screenTimeViewController = STWebpageController()
    }

    braveCore.adblockService.registerFilterListChanges { [weak self] _ in
      // Filter lists updated, reset selectors cache(s).
      self?.tabManager.allTabs.forEach {
        $0.contentBlocker?.resetSelectorsCache()
      }
    }

    FilterListStorage.shared.$filterLists
      .receive(on: DispatchQueue.main)
      .sink { [weak self] _ in
        // Filter lists selections changed, reset selectors cache(s).
        self?.tabManager.allTabs.forEach {
          $0.contentBlocker?.resetSelectorsCache()
        }
      }
      .store(in: &cancellables)
  }

  deinit {
    // Remove the open tabs model state observer
    if let observer = openTabsModelStateListener {
      profileController.sendTabAPI.removeObserver(observer)
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

    coordinator.animate(
      alongsideTransition: { context in
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
      tab.browserData?.newTabPageViewController = nil
    }
  }

  private var rewardsEnabledObserveration: NSKeyValueObservation?

  fileprivate func didInit() {
    updateApplicationShortcuts()
    tabManager.addDelegate(self)
    UserScriptManager.shared.fetchWalletScripts(from: profileController.braveWalletAPI)
    downloadQueue.delegate = self

    // Observe some user preferences
    Preferences.Privacy.privateBrowsingOnly.observe(from: self)
    Preferences.General.tabBarVisibility.observe(from: self)
    Preferences.General.mediaAutoBackgrounding.observe(from: self)
    Preferences.General.youtubeHighQuality.observe(from: self)
    Preferences.General.defaultPageZoomLevel.observe(from: self)
    Preferences.Shields.allShields.forEach { $0.observe(from: self) }
    Preferences.Privacy.blockAllCookies.observe(from: self)
    Preferences.Rewards.hideRewardsIcon.observe(from: self)
    Preferences.Rewards.rewardsToggledOnce.observe(from: self)
    Preferences.Playlist.enablePlaylistURLBarButton.observe(from: self)
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
        guard let url = $0.visibleURL else { return }
        let zoomLevel =
          self?.privateBrowsingManager.isPrivateBrowsing == true
          ? 1.0
          : Domain.getPersistedDomain(for: url)?.zoom_level?.doubleValue
            ?? Preferences.General.defaultPageZoomLevel.value
        $0.viewScale = zoomLevel
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
    Preferences.Wallet.defaultSolWallet.observe(from: self)

    if rewards.rewardsAPI != nil {
      // Ledger was started immediately due to user having ads enabled
      setupLedger()
    }

    Preferences.NewTabPage.attemptToShowClaimRewardsNotification.value = true

    setupAdsNotificationHandler()

    backgroundDataSource.replaceFavoritesIfNeeded = { sites in
      if Preferences.NewTabPage.initialFavoritesHaveBeenReplaced.value { return }

      guard let sites = sites, !sites.isEmpty else { return }

      Task { @MainActor in
        let defaultFavorites = await FavoritesPreloadedData.getList()
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
        toolbar?.setSearchButtonState(url: tabManager.selectedTab?.visibleURL)
        footer.addSubview(toolbar!)
        toolbar?.tabToolbarDelegate = self
        toolbar?.menuButton.setBadges(Array(topToolbar.menuButton.badges.keys))
      }
      view.setNeedsUpdateConstraints()
    }

    updateToolbarUsingTabManager(tabManager)
    updateUsingBottomBar(using: newCollection)

    if let tab = tabManager.selectedTab {
      updateURLBar()
      updateBackForwardActionStatus(for: tab)
      topToolbar.locationView.loading = tab.isLoading
    }

    toolbarVisibilityViewModel.toolbarState = .expanded
    updateTabsBarVisibility()
  }

  func updateToolbarSecureContentState(_ secureContentState: SecureContentState) {
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

  @objc func appWillTerminateNotification() {
    tabManager.saveAllTabs(synchronously: true)
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

    // TODO: brave/brave-browser/issues/46565
    // Remove when all direct mutations on CoreData types are replaced
    DataController.performOnMainContext { context in
      try? context.save()
    }

    tabManager.saveAllTabs()

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

    // Add Regular tabs to Sync Chain
    executeAfterSetup {
      if Preferences.Chromium.syncOpenTabsEnabled.value {
        self.tabManager.addRegularTabsToSyncChain()
      }
    }

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

    Task { @MainActor in
      // Track sync chain restoration via backup
      let shouldDeleteSyncChain = try await profileController.syncAPI
        .isSyncChainFromCloudRestoration()
      if shouldDeleteSyncChain {
        let alert = UIAlertController(
          title: Strings.Sync.deviceRestoreDetectedTitle,
          message: Strings.Sync.deviceRestoreDetectedMessage,
          preferredStyle: .alert
        )
        alert.addAction(
          .init(title: Strings.Sync.deviceRestoreResetActionTitle, style: .default) {
            [weak self] _ in
            self?.profileController.syncAPI.resetSyncChain()
          }
        )

        alert.addAction(.init(title: Strings.CancelString, style: .destructive))
        self.present(alert, animated: true)
      }
    }

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

    var tabToSelect: (any TabState)?

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
      postSetupTasks.removeAll()
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
    return tabManager.selectedTab?.webViewProxy?.becomeFirstResponder() ?? false
  }

  override public func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    updateToolbarUsingTabManager(tabManager)
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
  }

  /// Whether or not to show the playlist onboarding callout this session
  var shouldShowPlaylistOnboardingThisSession = true

  /// Wheter or not to show the translate onboarding callout this session
  var shouldShowTranslationOnboardingThisSession = true

  public func showQueuedAlertIfAvailable() {
    if let selectedTab = tabManager.selectedTab,
      let queuedAlertInfo = selectedTab.browserData?.dequeueJavascriptAlertPrompt()
    {
      let alertController = queuedAlertInfo.alertController()
      alertController.delegate = self
      selectedTab.shownPromptAlert = alertController

      present(alertController, animated: true, completion: nil)
    }
  }

  override public func viewWillDisappear(_ animated: Bool) {
    screenshotHelper.viewIsVisible = false
    super.viewWillDisappear(animated)
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
        var clearRecentSearchAlertDismissed = false

        if let keyboardHeight = keyboardState?.intersectionHeightForView(self.view) {
          activeKeyboardHeight = keyboardHeight
        }

        if let presentedNavigationController = presentedViewController
          as? ModalSettingsNavigationController,
          let presentedRootController = presentedNavigationController.viewControllers.first,
          presentedRootController is SearchQuickEnginesViewController
        {
          searchEngineSettingsDismissed = true
        }

        if let alertController = presentedViewController
          as? UIAlertController,
          alertController.preferredStyle == .actionSheet,
          let action = alertController.actions.first,
          action.title == Strings.recentSearchClearAlertButton
        {
          clearRecentSearchAlertDismissed = true
        }

        shouldEvaluateKeyboardConstraints =
          (activeKeyboardHeight > 0)
          && (presentedViewController == nil
            || searchEngineSettingsDismissed
            || clearRecentSearchAlertDismissed)

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

      self.webViewContainer.accessibilityElementsHidden = true
      UIAccessibility.post(notification: .screenChanged, argument: nil)
    }
  }

  private(set) weak var activeNewTabPageViewController: NewTabPageViewController?

  fileprivate func hideActiveNewTabPageController(_ isReaderModeURL: Bool = false) {
    guard let controller = activeNewTabPageViewController else { return }

    controller.willMove(toParent: nil)
    controller.view.removeFromSuperview()
    controller.removeFromParent()
    controller.view.alpha = 1
    self.webViewContainer.accessibilityElementsHidden = false
    UIAccessibility.post(notification: .screenChanged, argument: nil)

    // Refresh the reading view toolbar since the article record may have changed
    if let tab = self.tabManager.selectedTab,
      let readerMode = tab.browserData?.getContentScript(
        name: ReaderModeScriptHandler.scriptName
      )
        as? ReaderModeScriptHandler,
      readerMode.state == .active,
      isReaderModeURL,
      let state = tab.playlistItemState
    {
      self.showReaderModeBar(animated: false)
      self.updatePlaylistURLBar(tab: tab, state: state, item: tab.playlistItem)
    }
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
      } else {
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
      if isUserDefinedURLNavigation,
        let code = url.bookmarkletCodeComponent
      {
        tab.evaluateJavaScript(
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

      // Donate Custom Intent Open Website
      if url.isSecureWebPage(), !tab.isPrivate {
        ActivityShortcutManager.shared.donateCustomIntent(
          for: .openWebsite,
          with: url.absoluteString
        )
      }

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
      tab.browserData?.getContentScript(name: Web3NameServiceScriptHandler.scriptName)
      as? Web3NameServiceScriptHandler
    scriptHandler?.originalURL = originalURL

    tab.loadRequest(PrivilegedRequest(url: internalUrl) as URLRequest)
  }

  override public func accessibilityPerformEscape() -> Bool {
    if topToolbar.inOverlayMode {
      topToolbar.didClickCancel()
      return true
    } else if let selectedTab = tabManager.selectedTab, selectedTab.canGoBack {
      selectedTab.goBack()
      selectedTab.browserData?.resetExternalAlertProperties()
      return true
    }
    return false
  }

  func updateBackForwardActionStatus(for tab: some TabState) {
    if let forwardListItem = tab.backForwardList?.forwardList.first,
      forwardListItem.url.isInternalURL(for: .readermode)
    {
      navigationToolbar.updateForwardStatus(false)
    } else {
      navigationToolbar.updateForwardStatus(tab.canGoForward)
    }

    navigationToolbar.updateBackStatus(tab.canGoBack)
  }

  func updateUIForReaderHomeStateForTab(_ tab: some TabState) {
    updateURLBar()
    toolbarVisibilityViewModel.toolbarState = .expanded

    if let url = tab.visibleURL {
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
      updatePlaylistURLBar(tab: tab, state: tab.playlistItemState ?? .none, item: tab.playlistItem)
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

    updateToolbarCurrentURL(tab.visibleURL?.displayURL)
    if tabManager.selectedTab === tab {
      self.updateToolbarSecureContentState(tab.visibleSecureContentState)
    }

    let isPage = tab.visibleURL?.isWebPage() ?? false
    navigationToolbar.updatePageStatus(isPage)
    updateWebViewPageZoom(tab: tab)
  }

  public func moveTab(tabId: UUID, to browser: BrowserViewController) {
    guard let tab = tabManager.allTabs.filter({ $0.id == tabId }).first,
      let url = tab.visibleURL
    else {
      return
    }

    let isPrivate = tab.isPrivate
    tabManager.removeTab(tab)
    browser.tabManager.addTabsForURLs([url], zombie: false, isPrivate: isPrivate)
  }

  public func switchToTabForURLOrOpen(_ url: URL, isPrivate: Bool, isPrivileged: Bool) {
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
        guard freshTab === self.tabManager.selectedTab else { return }
        if let text = searchText {
          self.topToolbar.submitLocation(text)
        } else {
          self.focusURLBar()
        }
      }
    }
  }

  func openInNewWindow(url: URL?, isPrivate: Bool) {
    let activity = BrowserState.newWindowUserActivity(
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
      profileController.historyAPI.deleteAll { [weak self] in
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

  func clearPageZoomDialog() {
    pageZoomBar?.view.removeFromSuperview()
    updateViewConstraints()
    pageZoomBar = nil
  }

  func displayPageZoomDialog() {
    guard let tab = tabManager.selectedTab else { return }
    let zoomHandler = PageZoomHandler(
      tab: tab,
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

  func updateWebViewPageZoom(tab: some TabState) {
    if let currentURL = tab.visibleURL {
      let domain = Domain.getPersistedDomain(for: currentURL)

      let zoomLevel =
        privateBrowsingManager.isPrivateBrowsing
        ? 1.0 : domain?.zoom_level?.doubleValue ?? Preferences.General.defaultPageZoomLevel.value
      tab.viewScale = zoomLevel
    }
  }

  public override var preferredStatusBarStyle: UIStatusBarStyle {
    if isUsingBottomBar, let tab = tabManager.selectedTab,
      tab.visibleURL.map(InternalURL.isValid) == false,
      let color = tab.sampledPageTopColor
    {
      return color.isLight ? .darkContent : .lightContent
    }
    return super.preferredStatusBarStyle
  }

  func updateStatusBarOverlayColor() {
    defer { setNeedsStatusBarAppearanceUpdate() }
    guard isUsingBottomBar, let tab = tabManager.selectedTab,
      tab.visibleURL.map(InternalURL.isValid) == false,
      let color = tab.sampledPageTopColor
    else {
      statusBarOverlay.backgroundColor = privateBrowsingManager.browserColors.chromeBackground
      return
    }
    statusBarOverlay.backgroundColor = color
  }

  func navigateInTab(tab: some TabState) {
    for tab in tabManager.allTabs {
      SnackBarTabHelper.from(tab: tab)?.expireSnackbars()
    }

    if let url = tab.visibleURL {
      // Whether to show search icon or + icon
      toolbar?.setSearchButtonState(url: url)

      if !InternalURL.isValid(url: url) || url.isInternalURL(for: .readermode), !url.isFileURL {
        // Fire the readability check. This is here and not in the pageShow event handler in ReaderMode.js anymore
        // because that event will not always fire due to unreliable page caching. This will either let us know that
        // the currently loaded page can be turned into reading mode or if the page already is in reading mode. We
        // ignore the result because we are being called back asynchronous when the readermode status changes.
        tab.evaluateJavaScript(
          functionName: "\(readerModeNamespace).checkReadability",
          contentWorld: ReaderModeScriptHandler.scriptSandbox
        )

        // Only add history of a url which is not a localhost url
        if !url.isInternalURL(for: .readermode) {
          if !tab.isPrivate {
            profileController.historyAPI.add(url: url, title: tab.title ?? "", dateAdded: Date())
          }

          // Saving Tab.
          tabManager.saveTab(tab)
        }
      }
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
    }

    DispatchQueue.main.asyncAfter(deadline: .now() + .milliseconds(500)) {
      self.screenshotHelper.takeScreenshot(tab)
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
    if let readerMode = tab.browserData?.getContentScript(name: ReaderModeScriptHandler.scriptName)
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
      !tab.isLoading
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
    if let progress = progress, tab.contentsMimeType != MIMEType.pdf {
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
      isPrivileged: url.scheme == InternalURL.scheme
    )
  }

  func settingsOpenURLs(_ urls: [URL], loadImmediately: Bool) {
    let tabIsPrivate = tabManager.selectedTab?.isPrivate ?? false
    self.tabManager.addTabsForURLs(urls, zombie: !loadImmediately, isPrivate: tabIsPrivate)
  }

  // QA Stuff
  func settingsCreateFakeTabs() {
    let urls = (0..<1000).map { URL(string: "https://search.brave.com/search?q=\($0)")! }
    let tabIsPrivate = tabManager.selectedTab?.isPrivate ?? false
    self.tabManager.addTabsForURLs(urls, zombie: true, isPrivate: tabIsPrivate)
  }

  func settingsCreateFakeBookmarks() {
    let urls = (0..<1000).map { URL(string: "https://search.brave.com/search?q=Bookmarks\($0)")! }
    for (index, url) in urls.enumerated() {
      profileController.bookmarksAPI.createBookmark(
        withTitle: "QA-Bookmark - BraveSearch - \(index)",
        url: url
      )
    }
  }

  func settingsCreateFakeHistory() {
    let urls = (0..<1000).map { URL(string: "https://search.brave.com/search?q=History\($0)")! }
    for (index, url) in urls.enumerated() {
      profileController.historyAPI.add(
        url: url,
        title: "QA-History - BraveSearch - \(index)",
        dateAdded: Date()
      )
    }
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

  func tabsBarDidSelectTab(_ tabsBarController: TabsBarViewController, _ tab: some TabState) {
    if tab === tabManager.selectedTab { return }
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

extension BrowserViewController: TabMiscDelegate {
  func showRequestRewardsPanel(_ tab: some TabState) {
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

  func stopMediaPlayback(_ tab: some TabState) {
    tabManager.allTabs.forEach({
      PlaylistScriptHandler.stopPlayback(tab: $0)
    })
  }

  func showWalletNotification(_ tab: some TabState, origin: URLOrigin) {
    // only display notification when BVC is front and center
    guard presentedViewController == nil,
      Preferences.Wallet.displayWeb3Notifications.value,
      let origin = tab.browserData?.getOrigin(),
      let tabDappStore = tab.tabDappStore
    else {
      return
    }
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

  func isTabVisible(_ tab: some TabState) -> Bool {
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

  @MainActor
  private func isPendingRequestAvailable() async -> Bool {
    let privateMode = privateBrowsingManager.isPrivateBrowsing
    // If we have an open `WalletStore`, use that so we can assign the pending request if the wallet is open,
    // which allows us to store the new `PendingRequest` triggering a modal presentation for that request.
    guard
      let cryptoStore = self.walletStore?.cryptoStore
        ?? CryptoStore.from(
          ipfsApi: profileController.ipfsAPI,
          walletP3A: profileController.braveWalletAPI.walletP3A(),
          privateMode: privateMode
        )
    else {
      return false
    }
    if await cryptoStore.isPendingRequestAvailable() {
      return true
    } else if let selectedTabOrigin = tabManager.selectedTab?.visibleURL?.origin {
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
    didSelectPlaylistItem item: PlaylistItem
  ) {
    guard let tab = tabManager.selectedTab else { return }
    popToBVC(isAnimated: true) { [weak self] in
      self?.openPlaylist(tab: tab, item: PlaylistInfo(item: item))
    }
  }

  func searchViewController(
    _ searchViewController: SearchViewController,
    didLongPressSuggestion suggestion: String
  ) {
    self.topToolbar.setLocation(suggestion, search: true)
  }

  func presentQuickSearchEnginesViewController() {
    let quickSearchEnginesViewController = SearchQuickEnginesViewController(
      profile: profile,
      isPrivateBrowsing: privateBrowsingManager.isPrivateBrowsing
    )
    quickSearchEnginesViewController.navigationItem.leftBarButtonItem =
      UIBarButtonItem(
        title: Strings.close,
        style: .done,
        target: self,
        action: #selector(dismissQuickSearchEngines)
      )
    quickSearchEnginesViewController.delegate = searchController
    let navVC = ModalSettingsNavigationController(
      rootViewController: quickSearchEnginesViewController
    )
    self.present(navVC, animated: true, completion: nil)
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
    tabManager.selectedTab?.presentFindInteraction(with: query)
  }

  func searchViewControllerAllowFindInPage() -> Bool {
    if let url = tabManager.selectedTab?.visibleURL,
      let internalURL = InternalURL(url),
      internalURL.isAboutHomeURL
    {
      return false
    }
    return true
  }

  @objc private func dismissQuickSearchEngines() {
    dismiss(animated: true) { [weak self] in
      self?.updateViewConstraints()
      self?.searchController?.layoutSearchEngineScrollView()
    }
  }
}

// MARK: - UIPopoverPresentationControllerDelegate

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
    let tabIsPrivate = tabManager.selectedTab?.isPrivate ?? false
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

  func showNewTabTakeoverInfoBarIfNeeded() {
    if !rewards.ads.shouldDisplayNewTabTakeoverInfobar() {
      return
    }
    rewards.ads.recordNewTabTakeoverInfobarWasDisplayed()

    let newTabTakeoverInfoBar = NewTabTakeoverInfoBar(
      tabManager: self.tabManager,
      onLinkPressed: { [weak self] in
        guard let self else { return }
        self.rewards.ads.suppressNewTabTakeoverInfobar()
      },
      onClosePressed: { [weak self] in
        guard let self else { return }
        self.rewards.ads.suppressNewTabTakeoverInfobar()
      }
    )
    self.show(toast: newTabTakeoverInfoBar, duration: nil)
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
    case Preferences.Shields.blockScripts.key,
      Preferences.Shields.blockImages.key,
      Preferences.Shields.useRegionAdBlock.key:
      tabManager.reloadSelectedTab()
    case ShieldPreferences.blockAdsAndTrackingLevelRaw.key:
      tabManager.reloadSelectedTab()
      recordGlobalAdBlockShieldsP3A()
      // Global shield setting changed, reset selectors cache.
      tabManager.allTabs.forEach({
        $0.contentBlocker?.resetSelectorsCache()
      })
    case Preferences.Shields.fingerprintingProtection.key:
      tabManager.reloadSelectedTab()
      recordGlobalFingerprintingShieldsP3A()
    case Preferences.General.defaultPageZoomLevel.key:
      tabManager.allTabs.forEach({
        guard let url = $0.visibleURL else { return }
        let zoomLevel =
          $0.isPrivate
          ? 1.0
          : Domain.getPersistedDomain(for: url)?.zoom_level?.doubleValue
            ?? Preferences.General.defaultPageZoomLevel.value
        $0.viewScale = zoomLevel
      })
    case Preferences.Privacy.blockAllCookies.key,
      Preferences.Shields.googleSafeBrowsing.key:
      // All `block all cookies` toggle requires a hard reset of Webkit configuration.
      tabManager.reset()
      if !Preferences.Privacy.blockAllCookies.value {
        self.tabManager.reloadSelectedTab()
        for tab in self.tabManager.allTabs where tab !== self.tabManager.selectedTab {
          tab.createWebView()
          if let url = tab.visibleURL {
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
      tabManager.selectedTab?.browserData?.setScripts(scripts: [
        .mediaBackgroundPlay: Preferences.General.mediaAutoBackgrounding.value
      ])
      tabManager.reloadSelectedTab()
    case Preferences.General.youtubeHighQuality.key:
      let status = Reachability.shared.status
      tabManager.allTabs.forEach {
        $0.youtubeQualityTabHelper?.setHighQuality(networkStatus: status)
      }
    case Preferences.Playlist.enablePlaylistURLBarButton.key:
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
        ipfsApi: profileController.ipfsAPI,
        walletP3A: profileController.braveWalletAPI.walletP3A(),
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
        ipfsApi: profileController.ipfsAPI,
        walletP3A: profileController.braveWalletAPI.walletP3A(),
        privateMode: privateMode
      ) {
        cryptoStore.rejectAllPendingWebpageRequests()
      }
      updateURLBarWalletButton()
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
      tabManager.selectedTab?.translationState = .unavailable
      tabManager.selectedTab?.browserData?.setScripts(scripts: [
        .braveTranslate: Preferences.Translate.translateEnabled.value != false
      ])
      // Only reload the tab if the setting was changed from the settings controller
      if presentedViewController is SettingsNavigationController {
        tabManager.reloadSelectedTab()
      }
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
    guard screenshotService.windowScene != nil,
      presentedViewController == nil,
      let tab = tabManager.selectedTab,
      let scrollView = tab.webViewProxy?.scrollView,
      let url = tab.visibleURL,
      url.isWebPage()
    else {
      return (nil, 0, .zero)
    }

    var rect = scrollView.frame
    rect.origin.x = scrollView.contentOffset.x
    rect.origin.y = scrollView.contentSize.height - rect.height - scrollView.contentOffset.y

    do {
      let data = try await tab.createFullPagePDF()
      return (data, 0, data != nil ? rect : .zero)
    } catch {
      return (nil, 0, .zero)
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
    guard let tab = tabManager.selectedTab else {
      Logger.module.error("Invalid WebView")
      return
    }

    let getServerTrustForErrorPage = { () -> SecTrust? in
      do {
        if let url = tab.visibleURL {
          return try ErrorPageHelper.serverTrust(from: url)
        }
      } catch {
        Logger.module.error("\(error.localizedDescription)")
      }

      return nil
    }

    guard let trust = tab.serverTrust ?? getServerTrustForErrorPage() else {
      return
    }

    let host = tab.visibleURL?.host

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
          tab.dismissFindInteraction()
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
      let action = UIAlertAction(title: Strings.OBErrorOkay, style: .default)
      alert.addAction(action)
      present(alert, animated: true)
      return
    }

    if privateBrowsingManager.isPrivateBrowsing {
      let alert = UIAlertController(
        title: Strings.AIChat.leoDisabledPrivateBrowsingMessageTitle,
        message: Strings.AIChat.leoDisabledPrivateBrowsingMessageDescription,
        preferredStyle: .alert
      )
      let action = UIAlertAction(title: Strings.OBErrorOkay, style: .default)
      alert.addAction(action)
      present(alert, animated: true)
      return
    }

    let webDelegate = (query == nil) ? tabManager.selectedTab?.leoTabHelper : nil

    let model = AIChatViewModel(
      braveCore: profileController,
      webDelegate: webDelegate,
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
