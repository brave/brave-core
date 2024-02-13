// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import CoreSpotlight
import Combine
import BraveShared
import Shared
import Storage
import Data
import Brave
import BrowserIntentsModels
import BraveWidgetsModels
import BraveVPN
import Growth
import os.log
import BraveCore
import BraveNews
import Preferences

private extension Logger {
  static var module: Logger {
    .init(subsystem: "\(Bundle.main.bundleIdentifier ?? "com.brave.ios")", category: "SceneDelegate")
  }
}

class SceneDelegate: UIResponder, UIWindowSceneDelegate {

  // This property must be non-null because even though it's optional,
  // Chromium force unwraps it and uses it. For this reason, we always set this window property to the scene's main window.
  internal var window: UIWindow?
  private var windowProtection: WindowProtection?
  static var shouldHandleUrpLookup = false
  static var shouldHandleInstallAttributionFetch = false

  private var cancellables: Set<AnyCancellable> = []

  func scene(_ scene: UIScene, willConnectTo session: UISceneSession, options connectionOptions: UIScene.ConnectionOptions) {
    guard let windowScene = (scene as? UIWindowScene) else { return }

    let attributionManager = AttributionManager(dau: AppState.shared.dau, urp: UserReferralProgram.shared)

    let browserViewController = createBrowserWindow(
      scene: windowScene,
      braveCore: AppState.shared.braveCore,
      profile: AppState.shared.profile,
      attributionManager: attributionManager,
      diskImageStore: AppState.shared.diskImageStore,
      migration: AppState.shared.migration,
      rewards: AppState.shared.rewards,
      newsFeedDataSource: AppState.shared.newsFeedDataSource,
      userActivity: connectionOptions.userActivities.first ?? session.stateRestorationActivity
    )
    
    let conditions = scene.activationConditions
    conditions.canActivateForTargetContentIdentifierPredicate = NSPredicate(value: true)
    if let windowId = session.userInfo?["WindowID"] as? String {
      let preferPredicate = NSPredicate(format: "self == %@", windowId)
        conditions.prefersToActivateForTargetContentIdentifierPredicate = preferPredicate
    }
    
    Preferences.General.themeNormalMode.objectWillChange
      .receive(on: RunLoop.main)
      .sink { [weak self, weak scene] _ in
        guard let self = self,
              let scene = scene as? UIWindowScene else { return }
        self.updateTheme(for: scene)
      }
      .store(in: &cancellables)

    Preferences.General.nightModeEnabled.objectWillChange
      .receive(on: RunLoop.main)
      .sink { [weak self, weak scene] _ in
        guard let self = self,
              let scene = scene as? UIWindowScene else { return }
        self.updateTheme(for: scene)
      }
      .store(in: &cancellables)

    browserViewController.privateBrowsingManager.$isPrivateBrowsing
      .removeDuplicates()
      .receive(on: RunLoop.main)
      .sink { [weak self, weak scene] _ in
        guard let self = self,
              let scene = scene as? UIWindowScene else { return }
        self.updateTheme(for: scene)
      }
      .store(in: &cancellables)

    // Handle URP Lookup at first launch
    if SceneDelegate.shouldHandleUrpLookup {
      SceneDelegate.shouldHandleUrpLookup = false
      
      attributionManager.handleReferralLookup { [weak browserViewController] url in
        browserViewController?.openReferralLink(url: url)
      }
    }

    // Setup Playlist Car-Play
    // TODO: Decide what to do if we have multiple windows
    // as it is only possible to have a single car-play instance.
    // Once we move to iOS 14+, this is easy to fix as we just pass car-play a `MediaStreamer`
    // instance instead of a `BrowserViewController`.
    PlaylistCarplayManager.shared.do {
      $0.browserController = browserViewController
    }
    
    self.present(
      browserViewController: browserViewController,
      windowScene: windowScene,
      connectionOptions: connectionOptions
    )
    
    // Handle Install Attribution Fetch at first launch
    if SceneDelegate.shouldHandleInstallAttributionFetch {
      SceneDelegate.shouldHandleInstallAttributionFetch = false

      // First time user should send dau ping after onboarding last stage _ p3a consent screen
      // The reason p3a user consent is necesserray to call search ad install attribution API methods
      if !Preferences.AppState.dailyUserPingAwaitingUserConsent.value {
        // If P3A is not enabled, send the organic install code at daily pings which is BRV001
        // User has not opted in to share completely private and anonymous product insights
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
    
    if Preferences.URP.installAttributionLookupOutstanding.value == nil {
      // Similarly to referral lookup, this prefrence should be set if it is a new user
      // Trigger install attribution fetch only first launch
      Preferences.URP.installAttributionLookupOutstanding.value = Preferences.General.isFirstLaunch.value
    }
        
    PrivacyReportsManager.scheduleNotification(debugMode: !AppConstants.buildChannel.isPublic)
    PrivacyReportsManager.consolidateData()
    PrivacyReportsManager.scheduleProcessingBlockedRequests(isPrivateBrowsing: browserViewController.privateBrowsingManager.isPrivateBrowsing)
    PrivacyReportsManager.scheduleVPNAlertsTask()
  }
  
  private func present(browserViewController: BrowserViewController, windowScene: UIWindowScene, connectionOptions: UIScene.ConnectionOptions) {
    // Assign each browser a navigation controller
    let navigationController = UINavigationController(rootViewController: browserViewController).then {
      $0.isNavigationBarHidden = true
      $0.edgesForExtendedLayout = UIRectEdge(rawValue: 0)
    }
    
    // Assign each browser a window of its own
    let window = UIWindow(windowScene: windowScene).then {
      $0.backgroundColor = .black
      $0.overrideUserInterfaceStyle = expectedThemeOverride(for: windowScene)
      $0.tintColor = .braveBlurpleTint
      
      $0.rootViewController = navigationController
    }
    
    self.window = window

    // TODO: Refactor to accept a UIWindowScene
    // Then store the `windowProtection` in the `BrowserViewController` directly.
    // As each instance should have its own protection?
    self.windowProtection = WindowProtection(window: window)
    window.makeKeyAndVisible()
    
    // Open shared URLs on launch if there are any
    if !connectionOptions.urlContexts.isEmpty {
      self.scene(windowScene, openURLContexts: connectionOptions.urlContexts)
    }

    if let shortcutItem = connectionOptions.shortcutItem {
      QuickActions.sharedInstance.launchedShortcutItem = shortcutItem
    }
    
    if let response = connectionOptions.notificationResponse {
      if response.notification.request.identifier == BrowserViewController.defaultBrowserNotificationId {
        guard let settingsUrl = URL(string: UIApplication.openSettingsURLString) else {
          Logger.module.error("[SCENE] - Failed to unwrap iOS settings URL")
          return
        }
        UIApplication.shared.open(settingsUrl)
      } else if response.notification.request.identifier == PrivacyReportsManager.notificationID {
        browserViewController.openPrivacyReport()
      }
    }
  }

  func sceneDidDisconnect(_ scene: UIScene) {
    Logger.module.debug("[SCENE] - Scene Disconnected")
  }

  func sceneDidBecomeActive(_ scene: UIScene) {
    scene.userActivity?.becomeCurrent()
    
    guard let appDelegate = UIApplication.shared.delegate as? AppDelegate,
      let scene = scene as? UIWindowScene
    else {
      return
    }
    
    if let windowId = (scene.userActivity?.userInfo?["WindowID"] ??
                       scene.session.userInfo?["WindowID"]) as? String,
       let windowUUID = UUID(uuidString: windowId) {
      SessionWindow.setSelected(windowId: windowUUID)
    }
    
    Preferences.AppState.backgroundedCleanly.value = false
    AppState.shared.profile.reopen()
    AppState.shared.uptimeMonitor.beginMonitoring()

    appDelegate.receivedURLs = nil
    UIApplication.shared.applicationIconBadgeNumber = 0

    // handle quick actions is available
    let quickActions = QuickActions.sharedInstance
    if let shortcut = quickActions.launchedShortcutItem {
      // dispatch asynchronously so that BVC is all set up for handling new tabs
      // when we try and open them

      if let browserViewController = scene.browserViewController {
        quickActions.handleShortCutItem(shortcut, withBrowserViewController: browserViewController)
      }

      quickActions.launchedShortcutItem = nil
    }

    // We try to send DAU ping each time the app goes to foreground to work around network edge cases
    // (offline, bad connection etc.).
    // Also send the ping only after the URP lookup and install attribution has processed.
    if Preferences.URP.referralLookupOutstanding.value == true, Preferences.URP.installAttributionLookupOutstanding.value == true {
      AppState.shared.dau.sendPingToServer()
    }
    
    BraveSkusManager.refreshSKUCredential(isPrivate: scene.browserViewController?.privateBrowsingManager.isPrivateBrowsing == true)
  }

  func sceneWillResignActive(_ scene: UIScene) {
    Preferences.AppState.backgroundedCleanly.value = true
    scene.userActivity?.resignCurrent()
    AppState.shared.uptimeMonitor.pauseMonitoring()
  }

  func sceneWillEnterForeground(_ scene: UIScene) {
    if let scene = scene as? UIWindowScene {
      scene.browserViewController?.windowProtection = windowProtection
    }
  }

  func sceneDidEnterBackground(_ scene: UIScene) {
    AppState.shared.profile.shutdown()
    BraveVPN.sendVPNWorksInBackgroundNotification()
    Preferences.AppState.isOnboardingActive.value = false
  }

  func scene(_ scene: UIScene, openURLContexts URLContexts: Set<UIOpenURLContext>) {
    guard let scene = scene as? UIWindowScene else {
      Logger.module.error("[SCENE] - Scene is not a UIWindowScene")
      return
    }

    URLContexts.forEach({
      guard let routerpath = NavigationPath(url: $0.url, isPrivateBrowsing: scene.browserViewController?.privateBrowsingManager.isPrivateBrowsing == true) else {
        Logger.module.error("[SCENE] - Invalid Navigation Path: \($0.url)")
        return
      }

      scene.browserViewController?.handleNavigationPath(path: routerpath)
    })
  }
  
  func scene(_ scene: UIScene, didUpdate userActivity: NSUserActivity) {
    Logger.module.debug("[SCENE] - Updated User Activity for Scene")
  }

  func scene(_ scene: UIScene, continue userActivity: NSUserActivity) {
    guard let scene = scene as? UIWindowScene else {
      return
    }

    if let url = userActivity.webpageURL {
      switch UniversalLinkManager.universalLinkType(for: url, checkPath: false) {
      case .buyVPN:
        scene.browserViewController?.presentCorrespondingVPNViewController()
        return
      case .none:
        break
      }

      scene.browserViewController?.switchToTabForURLOrOpen(url, isPrivileged: true)
      return
    }

    switch userActivity.activityType {
    case CSSearchableItemActionType:
      // Otherwise, check if the `NSUserActivity` is a CoreSpotlight item and switch to its tab or
      // open a new one.
      if let userInfo = userActivity.userInfo,
        let urlString = userInfo[CSSearchableItemActivityIdentifier] as? String,
        let url = URL(string: urlString) {
        scene.browserViewController?.switchToTabForURLOrOpen(url, isPrivileged: false)
        return
      }
    case ActivityType.newTab.identifier:
      if let browserViewController = scene.browserViewController {
        ActivityShortcutManager.shared.performShortcutActivity(
          type: .newTab, using: browserViewController)
      }

      return
    case ActivityType.newPrivateTab.identifier:
      if let browserViewController = scene.browserViewController {
        ActivityShortcutManager.shared.performShortcutActivity(
          type: .newPrivateTab, using: browserViewController)
      }

      return
    case ActivityType.openHistoryList.identifier:
      if let browserViewController = scene.browserViewController {
        ActivityShortcutManager.shared.performShortcutActivity(
          type: .openHistoryList, using: browserViewController)
      }
      
      return
    case ActivityType.openBookmarks.identifier:
      if let browserViewController = scene.browserViewController {
        ActivityShortcutManager.shared.performShortcutActivity(
          type: .openBookmarks, using: browserViewController)
      }

      return
    case ActivityType.clearBrowsingHistory.identifier:
      if let browserViewController = scene.browserViewController {
        ActivityShortcutManager.shared.performShortcutActivity(
          type: .clearBrowsingHistory, using: browserViewController)
      }

      return
    case ActivityType.enableBraveVPN.identifier:
      if let browserViewController = scene.browserViewController {
        ActivityShortcutManager.shared.performShortcutActivity(
          type: .enableBraveVPN, using: browserViewController)
      }

      return
    case ActivityType.openBraveNews.identifier:
      if let browserViewController = scene.browserViewController {
        ActivityShortcutManager.shared.performShortcutActivity(
          type: .openBraveNews, using: browserViewController)
      }

      return
    case ActivityType.openPlayList.identifier:
      if let browserViewController = scene.browserViewController {
        ActivityShortcutManager.shared.performShortcutActivity(
          type: .openPlayList, using: browserViewController)
      }

    case ActivityType.openSyncedTabs.identifier:
      if let browserViewController = scene.browserViewController {
        ActivityShortcutManager.shared.performShortcutActivity(
          type: .openSyncedTabs, using: browserViewController)
      }
      return
    default:
      break
    }

    func switchToTabForIntentURL(intentURL: String?) {
      if let browserViewController = scene.browserViewController {
        guard let siteURL = intentURL, let url = URL(string: siteURL) else {
          browserViewController.openBlankNewTab(
            attemptLocationFieldFocus: false,
            isPrivate: Preferences.Privacy.privateBrowsingOnly.value)
          return
        }
      
        browserViewController.switchToTabForURLOrOpen(
          url,
          isPrivate: Preferences.Privacy.privateBrowsingOnly.value,
          isPrivileged: false)
      }
      return
    }

    if let intent = userActivity.interaction?.intent as? OpenWebsiteIntent {
      switchToTabForIntentURL(intentURL: intent.websiteURL)
      return
    }

    if let intent = userActivity.interaction?.intent as? OpenHistoryWebsiteIntent {
      switchToTabForIntentURL(intentURL: intent.websiteURL)
      return
    }

    if let intent = userActivity.interaction?.intent as? OpenBookmarkWebsiteIntent {
      switchToTabForIntentURL(intentURL: intent.websiteURL)
      return
    }
  }

  func windowScene(_ windowScene: UIWindowScene, performActionFor shortcutItem: UIApplicationShortcutItem, completionHandler: @escaping (Bool) -> Void) {
    if let browserViewController = windowScene.browserViewController {
      QuickActions.sharedInstance.handleShortCutItem(shortcutItem, withBrowserViewController: browserViewController)
      completionHandler(true)
    } else {
      completionHandler(false)
    }
  }

  func stateRestorationActivity(for scene: UIScene) -> NSUserActivity? {
    return scene.userActivity
  }
}

extension SceneDelegate {
  private func expectedThemeOverride(for scene: UIWindowScene?) -> UIUserInterfaceStyle {

    // The expected appearance theme should be dark mode when night mode is enabled for websites
    let themeValue = Preferences.General.nightModeEnabled.value ? DefaultTheme.dark.rawValue : Preferences.General.themeNormalMode.value

    let themeOverride = DefaultTheme(rawValue: themeValue)?.userInterfaceStyleOverride ?? .unspecified
    let isPrivateBrowsing = scene?.browserViewController?.privateBrowsingManager.isPrivateBrowsing == true
    return isPrivateBrowsing ? .dark : themeOverride
  }

  private func updateTheme(for scene: UIWindowScene) {
    scene.windows.forEach { window in
      UIView.transition(
        with: window, duration: 0.15, options: [.transitionCrossDissolve],
        animations: {
          window.overrideUserInterfaceStyle = self.expectedThemeOverride(for: scene)
        }, completion: nil)
    }
  }
}

extension SceneDelegate {
  private func createBrowserWindow(scene: UIWindowScene,
                                   braveCore: BraveCoreMain,
                                   profile: Profile,
                                   attributionManager: AttributionManager,
                                   diskImageStore: DiskImageStore?,
                                   migration: Migration?,
                                   rewards: Brave.BraveRewards,
                                   newsFeedDataSource: BraveNews.FeedDataSource,
                                   userActivity: NSUserActivity?) -> BrowserViewController {

    let privateBrowsingManager = PrivateBrowsingManager()

    // Don't track crashes if we're building the development environment due to the fact that terminating/stopping
    // the simulator via Xcode will count as a "crash" and lead to restore popups in the subsequent launch
    let crashedLastSession = !Preferences.AppState.backgroundedCleanly.value && AppConstants.buildChannel != .debug
    
    // Store the scene's activities
    let windowId: UUID
    let isPrivate: Bool
    let urlToOpen: URL?
    
    if UIApplication.shared.supportsMultipleScenes {
      let windowInfo: BrowserState.SessionState
      if let userActivity = userActivity {
        windowInfo = BrowserState.getWindowInfo(from: userActivity)
      } else {
        windowInfo = BrowserState.getWindowInfo(from: scene.session)
      }
      
      if let existingWindowId = windowInfo.windowId,
         let windowUUID = UUID(uuidString: existingWindowId) {
        // Restore the scene from the User-Info WindowID
        windowId = windowUUID
        isPrivate = windowInfo.isPrivate
        privateBrowsingManager.isPrivateBrowsing = windowInfo.isPrivate
        urlToOpen = windowInfo.openURL
        
        // Create a new session window if it does not already exist
        SessionWindow.createWindow(isPrivate: isPrivate, isSelected: true, uuid: windowId)
        Logger.module.info("[SCENE] - SESSION RESTORED")
      } else {
        // Try to restore active window
        windowId = restoreOrCreateWindow().windowId
        isPrivate = false
        privateBrowsingManager.isPrivateBrowsing = false
        urlToOpen = nil
      }
    } else {
      // iPhones don't care about user-activity or session info since it will always have one window anyway
      windowId = restoreOrCreateWindow().windowId
      isPrivate = false
      privateBrowsingManager.isPrivateBrowsing = false
      urlToOpen = nil
    }
    
    scene.userActivity = BrowserState.userActivity(for: windowId.uuidString, isPrivate: false)
    BrowserState.setWindowInfo(for: scene.session, windowId: windowId.uuidString, isPrivate: false)

    // Create a browser instance
    let browserViewController = BrowserViewController(
      windowId: windowId,
      profile: profile,
      attributionManager: attributionManager,
      diskImageStore: diskImageStore,
      braveCore: braveCore,
      rewards: rewards,
      migration: migration,
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
       let tabId = UUID(uuidString: tabIdString) {
      
      let currentTabScene = UIApplication.shared.connectedScenes.compactMap({ $0 as? UIWindowScene }).filter({
        guard let sceneWindowId = BrowserState.getWindowInfo(from: $0.session).windowId else {
          return false
        }
        
        return sceneWindowId == tabWindowId
      }).first
      
      if let currentTabScene = currentTabScene, let currentTabSceneBrowser = currentTabScene.browserViewController {
        browserViewController.loadViewIfNeeded()
        currentTabSceneBrowser.moveTab(tabId: tabId, to: browserViewController)
      }
    }
    
    if let urlToOpen = urlToOpen {
      DispatchQueue.main.async {
        browserViewController.loadViewIfNeeded()
        browserViewController.switchToTabForURLOrOpen(urlToOpen, isPrivileged: false)
      }
    }

    return browserViewController
  }
  
  private func restoreOrCreateWindow() -> (windowId: UUID, isPrivate: Bool, urlToOpen: URL?) {
    // Find active windows/sessions
    let activeWindow = SessionWindow.getActiveWindow(context: DataController.swiftUIContext)
    let activeSession = UIApplication.shared.openSessions
      .compactMap({ BrowserState.getWindowInfo(from: $0) })
      .first(where: { $0.windowId != nil && $0.windowId == activeWindow?.windowId.uuidString })
      
    if activeSession != nil {
      if !UIApplication.shared.supportsMultipleScenes {
        // iPhones should not create new windows
        if let activeWindow = activeWindow {
          // If there's no active window, fall through and create one
          return (activeWindow.windowId, false, nil)
        }
      }
      
      // An existing window is already active on screen
      // So create a new window
      let windowId = UUID()
      SessionWindow.createWindow(isPrivate: false, isSelected: true, uuid: windowId)
      Logger.module.info("[SCENE] - CREATED NEW WINDOW")
      return (windowId, false, nil)
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
    SessionWindow.createWindow(isPrivate: false, isSelected: true, uuid: windowId)
    Logger.module.info("[SCENE] - RESTORING ACTIVE WINDOW OR CREATING A NEW WINDOW")
    return (windowId, false, nil)
  }
}

extension SceneDelegate: UIViewControllerRestoration {
  public static func viewController(withRestorationIdentifierPath identifierComponents: [String], coder: NSCoder) -> UIViewController? {
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
