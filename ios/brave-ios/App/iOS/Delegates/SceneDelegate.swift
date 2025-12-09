// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Brave
import BraveCore
import BraveNews
import BraveShared
import BraveVPN
import BraveWidgetsModels
import BrowserIntentsModels
import Combine
import CoreSpotlight
import Data
import DesignSystem
import Growth
import Preferences
import Shared
import Storage
import SwiftUI
import UIKit
import os.log

extension Logger {
  fileprivate static var module: Logger {
    .init(
      subsystem: "\(Bundle.main.bundleIdentifier ?? "com.brave.ios")",
      category: "SceneDelegate"
    )
  }
}

/// State that must be associated with a profile-specific data
struct ProfileState {
  var rewards: Brave.BraveRewards
  var migrations: BraveProfileMigrations
  var dau: DAU
  var attributionManager: AttributionManager

  init(profileController: BraveProfileController) {
    // Setup DAU
    dau = DAU(braveCoreStats: profileController.braveStats)

    // Setup Attribution manager
    attributionManager = AttributionManager(
      dau: dau,
      urp: UserReferralProgram(
        braveCoreStats: profileController.braveStats
      )
    )

    // Setup Rewards & Ads
    let configuration = BraveRewards.Configuration.current()
    Migration.migrateAdsConfirmations(for: configuration)
    rewards = BraveRewards(configuration: configuration)

    // Setup BraveCore profile migrations
    migrations = BraveProfileMigrations(profileController: profileController)
    migrations.launchMigrations()
  }
}

struct SceneState {
  var window: UIWindow
  var windowScene: UIWindowScene
  var connectionOptions: UIScene.ConnectionOptions
}

// A SwiftUI version of the LaunchScreen.storyboard used for the app's launch
private struct LaunchScreen: View {
  var body: some View {
    VStack(spacing: 0) {
      Color.clear
        .frame(height: 44)
        .background(Color(uiColor: .systemGray6))
      Color(uiColor: .braveSeparator)
        .frame(height: 1)
      Spacer()
      Color(uiColor: .braveSeparator)
        .frame(height: 1)
      Color.clear
        .frame(height: 44)
        .background(Color(uiColor: .systemGray6))
    }
    .background(Color(uiColor: .systemBackground))
  }
}

class SceneDelegate: UIResponder, UIWindowSceneDelegate {

  // This property must be non-null because even though it's optional,
  // Chromium force unwraps it and uses it. For this reason, we always set this window property to the scene's main window.
  var window: UIWindow?
  private var windowProtection: WindowProtection?
  static var shouldHandleUrpLookup = false
  static var shouldHandleInstallAttributionFetch = false

  // We currently only load a single (default) profile, so no need to create multiple states per
  // window/scene created.
  private static var profileState: ProfileState?

  private var cancellables: Set<AnyCancellable> = []

  func scene(
    _ scene: UIScene,
    willConnectTo session: UISceneSession,
    options connectionOptions: UIScene.ConnectionOptions
  ) {
    guard let windowScene = (scene as? UIWindowScene) else { return }

    let conditions = scene.activationConditions
    conditions.canActivateForTargetContentIdentifierPredicate = NSPredicate(value: true)
    if let windowId = session.userInfo?["WindowID"] as? String {
      let preferPredicate = NSPredicate(format: "self == %@", windowId)
      conditions.prefersToActivateForTargetContentIdentifierPredicate = preferPredicate
    }

    // Assign each browser a window of its own
    let window = UIWindow(windowScene: windowScene).then {
      $0.backgroundColor = UIColor(braveSystemName: .iosBrowserChromeBackgroundIos)
      $0.overrideUserInterfaceStyle = expectedThemeOverride(for: windowScene)
      $0.tintColor = .braveBlurpleTint
    }
    window.rootViewController = UIHostingController(rootView: LaunchScreen())
    window.makeKeyAndVisible()
    self.window = window

    let sceneState = SceneState(
      window: window,
      windowScene: windowScene,
      connectionOptions: connectionOptions
    )

    Task { @MainActor in
      let (profileController, profileState) = await loadDefaultProfile()
      Self.profileState = profileState
      PlaylistCoordinator.shared.isPlaylistAvailable =
        profileController.profile.prefs.isPlaylistAvailable
      let browserViewController = prepareBrowserViewController(
        profileController: profileController,
        profileState: profileState,
        sceneState: sceneState
      )
      let container = UINavigationController(rootViewController: browserViewController)
      container.isNavigationBarHidden = true
      container.edgesForExtendedLayout = []
      window.rootViewController = container
      window.backgroundColor = .black

      self.windowProtection = WindowProtection(windowScene: windowScene)
      browserViewController.windowProtection = windowProtection

      performPostSceneConnectionTasks(
        profileState: profileState,
        sceneState: sceneState,
        browserViewController: browserViewController
      )
    }
  }

  func sceneDidDisconnect(_ scene: UIScene) {
    Logger.module.debug("[SCENE] - Scene Disconnected")
  }

  func sceneDidBecomeActive(_ scene: UIScene) {
    // Warning: The scene may be active without a profile being loaded on first launch
    scene.userActivity?.becomeCurrent()

    guard let appDelegate = UIApplication.shared.delegate as? AppDelegate,
      let scene = scene as? UIWindowScene
    else {
      return
    }

    if let windowId =
      (scene.userActivity?.userInfo?["WindowID"] ?? scene.session.userInfo?["WindowID"]) as? String,
      let windowUUID = UUID(uuidString: windowId)
    {
      SessionWindow.setSelected(windowId: windowUUID)
    }

    AppState.shared.uptimeMonitor.beginMonitoring()

    appDelegate.receivedURLs = nil
    UIApplication.shared.applicationIconBadgeNumber = 0

    if let browserViewController = scene.browserViewController {
      Preferences.AppState.backgroundedCleanly.value = false
      sendDAUPingIfNeeded()
      refreshSKUsCredentials(in: scene)
      handleQuickActionsIfNeeded(browserViewController: browserViewController)
    }
  }

  func sceneWillResignActive(_ scene: UIScene) {
    Preferences.AppState.backgroundedCleanly.value = true
    Preferences.AppState.shouldDeferPromotedPurchase.value = false
    scene.userActivity?.resignCurrent()
    AppState.shared.uptimeMonitor.pauseMonitoring()
  }

  func sceneDidEnterBackground(_ scene: UIScene) {
    BraveVPN.sendVPNWorksInBackgroundNotification()
  }

  func scene(_ scene: UIScene, openURLContexts contexts: Set<UIOpenURLContext>) {
    guard let scene = scene as? UIWindowScene else {
      Logger.module.error("[SCENE] - Scene is not a UIWindowScene")
      return
    }

    for context in contexts {
      guard
        let routerpath = NavigationPath(
          url: context.url,
          isPrivateBrowsing: scene.browserViewController?.privateBrowsingManager.isPrivateBrowsing
            == true
        )
      else {
        Logger.module.error("[SCENE] - Invalid Navigation Path: \(context.url)")
        return
      }

      if case .url(let navigationPathURL, _) = routerpath, let pathURL = navigationPathURL,
        pathURL.isFileURL
      {
        defer {
          pathURL.stopAccessingSecurityScopedResource()
        }

        let canAccessFileURL = pathURL.startAccessingSecurityScopedResource()

        if !canAccessFileURL {
          //File can not be accessed pass the url text to search engine
          scene.browserViewController?.submitSearchText(pathURL.absoluteString)
          continue
        }
      }

      scene.browserViewController?.handleNavigationPath(path: routerpath)
    }
  }

  func scene(_ scene: UIScene, didUpdate userActivity: NSUserActivity) {
    Logger.module.debug("[SCENE] - Updated User Activity for Scene")
  }

  func scene(_ scene: UIScene, continue userActivity: NSUserActivity) {
    handleCustomUserActivityActions(scene, userActivity: userActivity)
  }

  func windowScene(
    _ windowScene: UIWindowScene,
    performActionFor shortcutItem: UIApplicationShortcutItem,
    completionHandler: @escaping (Bool) -> Void
  ) {
    if let browserViewController = windowScene.browserViewController {
      QuickActions.sharedInstance.handleShortCutItem(
        shortcutItem,
        withBrowserViewController: browserViewController
      )
      completionHandler(true)
    } else {
      completionHandler(false)
    }
  }
}

extension SceneDelegate {

  @MainActor private func loadDefaultProfile() async -> (BraveProfileController, ProfileState) {
    let braveCore = AppState.shared.braveCore
    if let profileController = braveCore.profileController, let profileState = Self.profileState {
      return (profileController, profileState)
    }
    // Setup default profile & profile state
    let defaultProfileController = await braveCore.loadDefaultProfile()
    let profileState = ProfileState(profileController: defaultProfileController)
    return (defaultProfileController, profileState)
  }

  @objc private func enableUserSelectedTypesForSync() {
    guard let profileController = AppState.shared.braveCore.profileController,
      profileController.syncAPI.isInSyncGroup
    else {
      Logger.module.info("Sync is not active")
      return
    }

    profileController.syncAPI.enableSyncTypes(
      syncProfileService: profileController.syncProfileService
    )
  }

  private func setupThemeObservation(
    on browserViewController: BrowserViewController,
    windowScene: UIWindowScene
  ) {
    Preferences.General.themeNormalMode.objectWillChange
      .receive(on: RunLoop.main)
      .sink { [weak self, weak windowScene] _ in
        guard let self = self, let windowScene else { return }
        self.updateTheme(for: windowScene)
      }
      .store(in: &cancellables)

    Preferences.General.nightModeEnabled.objectWillChange
      .receive(on: RunLoop.main)
      .sink { [weak self, weak windowScene] _ in
        guard let self = self, let windowScene else { return }
        self.updateTheme(for: windowScene)
      }
      .store(in: &cancellables)

    browserViewController.privateBrowsingManager.$isPrivateBrowsing
      .removeDuplicates()
      .receive(on: RunLoop.main)
      .sink { [weak self, weak windowScene] _ in
        guard let self = self, let windowScene else { return }
        self.updateTheme(for: windowScene)
      }
      .store(in: &cancellables)
  }

  private func prepareBrowserViewController(
    profileController: BraveProfileController,
    profileState: ProfileState,
    sceneState: SceneState
  ) -> BrowserViewController {
    let windowScene = sceneState.windowScene

    let browserViewController = createBrowserWindow(
      scene: windowScene,
      braveCore: AppState.shared.braveCore,
      profileController: profileController,
      profile: AppState.shared.profile,
      attributionManager: profileState.attributionManager,
      rewards: profileState.rewards,
      newsFeedDataSource: AppState.shared.newsFeedDataSource,
      userActivity: sceneState.connectionOptions.userActivities.first
    )

    // Setup Playlist Car-Play
    // TODO: Decide what to do if we have multiple windows
    // as it is only possible to have a single car-play instance.
    // Once we move to iOS 14+, this is easy to fix as we just pass car-play a `MediaStreamer`
    // instance instead of a `BrowserViewController`.
    PlaylistCoordinator.shared.do {
      $0.browserController = browserViewController
    }

    return browserViewController
  }

  private func performPostSceneConnectionTasks(
    profileState: ProfileState,
    sceneState: SceneState,
    browserViewController: BrowserViewController
  ) {
    let connectionOptions = sceneState.connectionOptions
    let windowScene = sceneState.windowScene

    setupThemeObservation(on: browserViewController, windowScene: windowScene)

    // Open shared URLs on launch if there are any
    if !connectionOptions.urlContexts.isEmpty {
      self.scene(windowScene, openURLContexts: connectionOptions.urlContexts)
    }

    if let shortcutItem = connectionOptions.shortcutItem {
      QuickActions.sharedInstance.launchedShortcutItem = shortcutItem
    }

    if let response = connectionOptions.notificationResponse {
      if response.notification.request.identifier
        == BrowserViewController.defaultBrowserNotificationId
      {
        if let settingsUrl = URL(string: UIApplication.openSettingsURLString) {
          UIApplication.shared.open(settingsUrl)
        }
      } else if response.notification.request.identifier == PrivacyReportsManager.notificationID {
        browserViewController.openPrivacyReport()
      }
    }

    // Handle URP Lookup at first launch
    if SceneDelegate.shouldHandleUrpLookup {
      SceneDelegate.shouldHandleUrpLookup = false

      profileState.attributionManager.handleReferralLookup { [weak browserViewController] url in
        browserViewController?.openReferralLink(url: url)
      }
    }

    // Handle Install Attribution Fetch at first launch
    if SceneDelegate.shouldHandleInstallAttributionFetch {
      SceneDelegate.shouldHandleInstallAttributionFetch = false
      let attributionManager = profileState.attributionManager

      // First time user should send dau ping after onboarding last stage _ p3a consent screen
      // The reason p3a user consent is necesserray to call search ad install attribution API methods
      if !Preferences.AppState.dailyUserPingAwaitingUserConsent.value {
        // If P3A is not enabled, send the organic install code at daily pings which is BRV001
        // User has not opted in to share private and anonymous product insights
        if AppState.shared.braveCore.p3aUtils.isP3AEnabled {
          Task { @MainActor in
            do {
              try await attributionManager.handleSearchAdsInstallAttribution()
            } catch {
              Logger.module.debug("Error fetching ads attribution default code is sent \(error)")
              // Sending default organic install code for dau
              attributionManager.setupReferralCodeAndPingServer()
            }
          }
        } else {
          // Sending default organic install code for dau
          attributionManager.setupReferralCodeAndPingServer()
        }
      }
    }

    // Adding Observer to enable sync types
    NotificationCenter.default.addObserver(
      self,
      selector: #selector(enableUserSelectedTypesForSync),
      name: BraveServiceStateObserver.coreServiceLoadedNotification,
      object: nil
    )

    PrivacyReportsManager.scheduleNotification(debugMode: !AppConstants.isOfficialBuild)
    PrivacyReportsManager.consolidateData()
    PrivacyReportsManager.scheduleProcessingBlockedRequests(
      isPrivateBrowsing: browserViewController.privateBrowsingManager.isPrivateBrowsing
    )
    PrivacyReportsManager.scheduleVPNAlertsTask()

    // Handle Custom Activity and Intents
    if let currentActivity = sceneState.connectionOptions.userActivities.first {
      handleCustomUserActivityActions(sceneState.windowScene, userActivity: currentActivity)
    }

    if sceneState.windowScene.activationState == .foregroundActive {
      // Perform any actions that would also execute in sceneDidBecomeActive
      Preferences.AppState.backgroundedCleanly.value = false
      sendDAUPingIfNeeded()
      refreshSKUsCredentials(in: sceneState.windowScene)
      handleQuickActionsIfNeeded(browserViewController: browserViewController)
    }
  }

  private func handleQuickActionsIfNeeded(browserViewController: BrowserViewController) {
    // handle quick actions is available
    let quickActions = QuickActions.sharedInstance
    if let shortcut = quickActions.launchedShortcutItem {
      // dispatch asynchronously so that BVC is all set up for handling new tabs
      // when we try and open them
      quickActions.handleShortCutItem(shortcut, withBrowserViewController: browserViewController)
      quickActions.launchedShortcutItem = nil
    }
  }

  private func sendDAUPingIfNeeded() {
    guard let profileState = Self.profileState else { return }
    // We try to send DAU ping each time the app goes to foreground to work around network edge cases
    // (offline, bad connection etc.).
    // Also send the ping only after install attribution has processed.
    if Preferences.URP.installAttributionLookupOutstanding.value == false {
      profileState.dau.sendPingToServer()
    }
  }

  private func refreshSKUsCredentials(in scene: UIWindowScene) {
    Task { @MainActor in
      let isPrivateBrowsing =
        scene.browserViewController?.privateBrowsingManager.isPrivateBrowsing == true
      await BraveSkusManager(isPrivateMode: isPrivateBrowsing)?.refreshVPNCredentials()
    }
  }

  private func handleCustomUserActivityActions(_ scene: UIScene, userActivity: NSUserActivity) {
    guard let scene = scene as? UIWindowScene else {
      return
    }

    handleCustomUserActivityTypes(scene, userActivity: userActivity)
    handleCustomUserIntents(scene, userActivity: userActivity)
  }

  private func handleCustomUserActivityTypes(_ scene: UIWindowScene, userActivity: NSUserActivity) {
    switch userActivity.activityType {
    case CSSearchableItemActionType:
      // Otherwise, check if the `NSUserActivity` is a CoreSpotlight item and switch to its tab or
      // open a new one.
      if let userInfo = userActivity.userInfo,
        let urlString = userInfo[CSSearchableItemActivityIdentifier] as? String,
        let url = URL(string: urlString)
      {
        let isPrivateBrowsing =
          scene.browserViewController?.privateBrowsingManager.isPrivateBrowsing == true
        scene.browserViewController?.switchToTabForURLOrOpen(
          url,
          isPrivate: isPrivateBrowsing,
          isPrivileged: false
        )
        return
      }
    case ActivityType.newTab.identifier:
      if let browserViewController = scene.browserViewController {
        ActivityShortcutManager.shared.performShortcutActivity(
          type: .newTab,
          using: browserViewController
        )
      }

      return
    case ActivityType.newPrivateTab.identifier:
      if let browserViewController = scene.browserViewController {
        ActivityShortcutManager.shared.performShortcutActivity(
          type: .newPrivateTab,
          using: browserViewController
        )
      }

      return
    case ActivityType.openHistoryList.identifier:
      if let browserViewController = scene.browserViewController {
        ActivityShortcutManager.shared.performShortcutActivity(
          type: .openHistoryList,
          using: browserViewController
        )
      }

      return
    case ActivityType.openBookmarks.identifier:
      if let browserViewController = scene.browserViewController {
        ActivityShortcutManager.shared.performShortcutActivity(
          type: .openBookmarks,
          using: browserViewController
        )
      }

      return
    case ActivityType.clearBrowsingHistory.identifier:
      if let browserViewController = scene.browserViewController {
        ActivityShortcutManager.shared.performShortcutActivity(
          type: .clearBrowsingHistory,
          using: browserViewController
        )
      }

      return
    case ActivityType.enableBraveVPN.identifier:
      if let browserViewController = scene.browserViewController {
        ActivityShortcutManager.shared.performShortcutActivity(
          type: .enableBraveVPN,
          using: browserViewController
        )
      }

      return
    case ActivityType.openBraveNews.identifier:
      let isNewsAvailable =
        AppState.shared.braveCore.profileController?.profile.prefs.isBraveNewsAvailable ?? true
      if isNewsAvailable, let browserViewController = scene.browserViewController {
        ActivityShortcutManager.shared.performShortcutActivity(
          type: .openBraveNews,
          using: browserViewController
        )
      }

      return
    case ActivityType.openPlayList.identifier:
      if let browserViewController = scene.browserViewController {
        ActivityShortcutManager.shared.performShortcutActivity(
          type: .openPlayList,
          using: browserViewController
        )
      }

    case ActivityType.openSyncedTabs.identifier:
      if let browserViewController = scene.browserViewController {
        ActivityShortcutManager.shared.performShortcutActivity(
          type: .openSyncedTabs,
          using: browserViewController
        )
      }
      return
    default:
      break
    }

    if let url = userActivity.webpageURL {
      switch UniversalLinkManager.universalLinkType(for: url, checkPath: false) {
      case .buyVPN:
        scene.browserViewController?.presentCorrespondingVPNViewController()
        return
      case .none:
        break
      }

      let isPrivateBrowsing =
        scene.browserViewController?.privateBrowsingManager.isPrivateBrowsing == true
      scene.browserViewController?.switchToTabForURLOrOpen(
        url,
        isPrivate: isPrivateBrowsing,
        isPrivileged: false
      )
      return
    }
  }

  private func handleCustomUserIntents(_ scene: UIWindowScene, userActivity: NSUserActivity) {
    if let intent = userActivity.interaction?.intent as? OpenWebsiteIntent {
      switchToTabForIntentURL(scene, intentURL: intent.websiteURL)
      return
    }

    if let intent = userActivity.interaction?.intent as? OpenHistoryWebsiteIntent {
      switchToTabForIntentURL(scene, intentURL: intent.websiteURL)
      return
    }

    if let intent = userActivity.interaction?.intent as? OpenBookmarkWebsiteIntent {
      switchToTabForIntentURL(scene, intentURL: intent.websiteURL)
      return
    }
  }

  private func switchToTabForIntentURL(_ scene: UIWindowScene, intentURL: String?) {
    if let browserViewController = scene.browserViewController {
      guard let siteURL = intentURL, let url = URL(string: siteURL), url.isWebPage() else {
        browserViewController.openBlankNewTab(
          attemptLocationFieldFocus: false,
          isPrivate: Preferences.Privacy.privateBrowsingOnly.value
        )
        return
      }

      browserViewController.switchToTabForURLOrOpen(
        url,
        isPrivate: Preferences.Privacy.privateBrowsingOnly.value,
        isPrivileged: false
      )
    }
    return
  }
}

extension SceneDelegate {
  private func expectedThemeOverride(for scene: UIWindowScene?) -> UIUserInterfaceStyle {

    // The expected appearance theme should be dark mode when night mode is enabled for websites
    let themeValue =
      Preferences.General.nightModeEnabled.value
      ? DefaultTheme.dark.rawValue : Preferences.General.themeNormalMode.value

    let themeOverride =
      DefaultTheme(rawValue: themeValue)?.userInterfaceStyleOverride ?? .unspecified
    return themeOverride
  }

  private func updateTheme(for scene: UIWindowScene) {
    scene.windows.forEach { window in
      UIView.transition(
        with: window,
        duration: 0.15,
        options: [.transitionCrossDissolve],
        animations: {
          window.overrideUserInterfaceStyle = self.expectedThemeOverride(for: scene)
        },
        completion: nil
      )
    }
  }
}

extension SceneDelegate {
  private func createBrowserWindow(
    scene: UIWindowScene,
    braveCore: BraveCoreMain,
    profileController: BraveProfileController,
    profile: LegacyBrowserProfile,
    attributionManager: AttributionManager,
    rewards: Brave.BraveRewards,
    newsFeedDataSource: BraveNews.FeedDataSource,
    userActivity: NSUserActivity?
  ) -> BrowserViewController {
    let privateBrowsingManager = PrivateBrowsingManager()

    // Don't track crashes if we're building the development environment due to the fact that terminating/stopping
    // the simulator via Xcode will count as a "crash" and lead to restore popups in the subsequent launch
    let crashedLastSession =
      !Preferences.AppState.backgroundedCleanly.value && AppConstants.isOfficialBuild

    // Store the scene's activities
    let windowId: UUID
    let isPrivate: Bool
    let urlToOpen: URL?

    if UIApplication.shared.supportsMultipleScenes {
      var windowInfo: BrowserState.SessionState
      if let userActivity = userActivity {
        windowInfo = BrowserState.getNewWindowInfo(from: userActivity)
      } else {
        windowInfo = .init(
          windowId: BrowserState.getWindowId(from: scene.session),
          isPrivate: Preferences.Privacy.privateBrowsingOnly.value
        )
      }

      if let existingWindowId = windowInfo.windowId,
        let windowUUID = UUID(uuidString: existingWindowId)
      {
        // Restore the scene from the User-Info WindowID
        windowId = windowUUID
        isPrivate = windowInfo.isPrivate
        privateBrowsingManager.isPrivateBrowsing = windowInfo.isPrivate
        urlToOpen = windowInfo.openURL

        // Create a new session window if it does not already exist
        SessionWindow.createWindow(isSelected: true, uuid: windowId)
        Logger.module.info("[SCENE] - SESSION RESTORED")
      } else {
        // Try to restore active window
        let windowInfo = restoreOrCreateWindow()
        windowId = windowInfo.windowId
        isPrivate = windowInfo.isPrivate
        privateBrowsingManager.isPrivateBrowsing = windowInfo.isPrivate
        urlToOpen = nil
      }
    } else {
      // iPhones don't care about user-activity or session info since it will always have one window anyway
      let windowInfo = restoreOrCreateWindow()
      windowId = windowInfo.windowId
      isPrivate = windowInfo.isPrivate
      privateBrowsingManager.isPrivateBrowsing = windowInfo.isPrivate
      urlToOpen = nil
    }

    scene.userActivity = BrowserState.userActivity(for: windowId.uuidString)
    BrowserState.setWindowId(for: scene.session, windowId: windowId.uuidString)

    // Create a browser instance
    let browserViewController = BrowserViewController(
      windowId: windowId,
      profile: profile,
      attributionManager: attributionManager,
      braveCore: braveCore,
      profileController: profileController,
      rewards: rewards,
      crashedLastSession: crashedLastSession,
      newsFeedDataSource: newsFeedDataSource,
      privateBrowsingManager: privateBrowsingManager
    )

    browserViewController.do {
      $0.edgesForExtendedLayout = []

      // Add restoration class, the factory that will return the ViewController we will restore with.
      $0.restorationIdentifier = BrowserState.sceneId
      $0.restorationClass = SceneDelegate.self

      // Remove Ad-Grant Reminders
      $0.removeScheduledAdGrantReminders()
    }

    if let tabIdString = userActivity?.userInfo?["TabID"] as? String,
      let tabWindowId = userActivity?.userInfo?["TabWindowID"] as? String,
      let tabId = UUID(uuidString: tabIdString)
    {

      let currentTabScene = UIApplication.shared.connectedScenes.compactMap({ $0 as? UIWindowScene }
      ).filter({
        guard let sceneWindowId = BrowserState.getWindowId(from: $0.session) else {
          return false
        }

        return sceneWindowId == tabWindowId
      }).first

      if let currentTabScene = currentTabScene,
        let currentTabSceneBrowser = currentTabScene.browserViewController
      {
        browserViewController.loadViewIfNeeded()
        currentTabSceneBrowser.moveTab(tabId: tabId, to: browserViewController)
      }
    }

    if let urlToOpen = urlToOpen {
      DispatchQueue.main.async {
        browserViewController.loadViewIfNeeded()
        browserViewController.switchToTabForURLOrOpen(
          urlToOpen,
          isPrivate: isPrivate,
          isPrivileged: false
        )
      }
    }

    return browserViewController
  }

  private func restoreOrCreateWindow() -> (windowId: UUID, isPrivate: Bool, urlToOpen: URL?) {
    // Find active windows/sessions
    let activeWindow = SessionWindow.getActiveWindow(context: DataController.swiftUIContext)
    let activeSession = UIApplication.shared.openSessions
      .compactMap({ BrowserState.getWindowId(from: $0) })
      .first(where: { $0 == activeWindow?.windowId.uuidString })
    let isPrivate = Preferences.Privacy.privateBrowsingOnly.value

    if activeSession != nil {
      if !UIApplication.shared.supportsMultipleScenes {
        // iPhones should not create new windows
        if let activeWindow = activeWindow {
          // If there's no active window, fall through and create one
          return (activeWindow.windowId, isPrivate, nil)
        }
      }

      // An existing window is already active on screen
      // So create a new window
      let windowId = UUID()
      SessionWindow.createWindow(isSelected: true, uuid: windowId)
      Logger.module.info("[SCENE] - CREATED NEW WINDOW")
      return (windowId, isPrivate, nil)
    }

    // Restore the active window if possible
    let windowId: UUID
    if !UIApplication.shared.supportsMultipleScenes {
      // iPhones don't have multi-window so we can restore the active window OR first window found
      windowId = activeWindow?.windowId ?? SessionWindow.all().first?.windowId ?? UUID()
    } else {
      windowId = activeWindow?.windowId ?? UUID()
    }

    // Create a new session window if it does not already exist
    SessionWindow.createWindow(isSelected: true, uuid: windowId)
    Logger.module.info("[SCENE] - RESTORING ACTIVE WINDOW OR CREATING A NEW WINDOW")
    return (windowId, isPrivate, nil)
  }
}

extension SceneDelegate: UIViewControllerRestoration {
  public static func viewController(
    withRestorationIdentifierPath identifierComponents: [String],
    coder: NSCoder
  ) -> UIViewController? {
    return nil
  }
}

extension UIWindowScene {
  /// A single scene should only have ONE browserViewController
  /// However, it is possible that someone can create multiple,
  /// Therefore, we support this possibility if needed
  var browserViewControllers: [BrowserViewController] {
    windows.compactMap({
      $0.rootViewController as? UINavigationController
    }).flatMap({
      $0.viewControllers.compactMap({
        $0 as? BrowserViewController
      })
    })
  }

  /// A scene should only ever have one browserViewController
  /// Returns the first instance of `BrowserViewController` that is found in the current scene
  var browserViewController: BrowserViewController? {
    return browserViewControllers.first
  }
}

extension UIView {
  /// Returns the `Scene` that this view belongs to.
  /// If the view does not belong to a scene, it returns the scene of its parent
  /// Otherwise returns nil if no scene is associated with this view.
  var currentScene: UIWindowScene? {
    if let scene = window?.windowScene {
      return scene
    }

    if let scene = superview?.currentScene {
      return scene
    }

    return nil
  }
}

extension UIViewController {
  /// Returns the `Scene` that this controller belongs to.
  /// If the controller does not belong to a scene, it returns the scene of its presenter or parent.
  /// Otherwise returns nil if no scene is associated with this controller.
  var currentScene: UIWindowScene? {
    if let scene = view.window?.windowScene {
      return scene
    }

    if let scene = parent?.currentScene {
      return scene
    }

    if let scene = presentingViewController?.currentScene {
      return scene
    }

    return nil
  }
}
