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

class SceneDelegate: UIResponder, UIWindowSceneDelegate {

  // This property must be non-null because even though it's optional,
  // Chromium force unwraps it and uses it. For this reason, we always set this window property to the scene's main window.
  internal var window: UIWindow?
  private var windowProtection: WindowProtection?
  static var shouldHandleUrpLookup = false

  private var cancellables: Set<AnyCancellable> = []
  private let log = Logger(subsystem: Bundle.main.bundleIdentifier!, category: "scene-delegate")

  func scene(_ scene: UIScene, willConnectTo session: UISceneSession, options connectionOptions: UIScene.ConnectionOptions) {
    guard let windowScene = (scene as? UIWindowScene) else { return }

    let browserViewController = createBrowserWindow(
      scene: windowScene,
      braveCore: AppState.shared.braveCore,
      profile: AppState.shared.profile,
      diskImageStore: AppState.shared.diskImageStore,
      migration: AppState.shared.migration,
      rewards: AppState.shared.rewards,
      newsFeedDataSource: AppState.shared.newsFeedDataSource,
      userActivity: connectionOptions.userActivities.first ?? session.stateRestorationActivity
    )
    
    let conditions = scene.activationConditions
    conditions.canActivateForTargetContentIdentifierPredicate = NSPredicate(value: true)
    if let windowId = session.userInfo?["WindowID"] as? UUID {
      let preferPredicate = NSPredicate(format: "self == %@", windowId.uuidString)
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

    if SceneDelegate.shouldHandleUrpLookup {
      // TODO: Find a better way to do this when multiple windows are involved.
      SceneDelegate.shouldHandleUrpLookup = false

      if let urp = UserReferralProgram.shared {
        browserViewController.handleReferralLookup(urp)
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
          log.error("Failed to unwrap iOS settings URL")
          return
        }
        UIApplication.shared.open(settingsUrl)
      } else if response.notification.request.identifier == PrivacyReportsManager.notificationID {
        browserViewController.openPrivacyReport()
      }
    }
  }

  func sceneDidDisconnect(_ scene: UIScene) {
    log.debug("SCENE DISCONNECTED")
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
    // Also send the ping only after the URP lookup has processed.
    if Preferences.URP.referralLookupOutstanding.value == false {
      AppState.shared.dau.sendPingToServer()
    }
    
    BraveSkusManager.refreshSKUCredential(isPrivate: scene.browserViewController?.privateBrowsingManager.isPrivateBrowsing == true)
  }

  func sceneWillResignActive(_ scene: UIScene) {
    Preferences.AppState.backgroundedCleanly.value = true
    scene.userActivity?.resignCurrent()
  }

  func sceneWillEnterForeground(_ scene: UIScene) {
    if let scene = scene as? UIWindowScene {
      scene.browserViewController?.windowProtection = windowProtection
    }
  }

  func sceneDidEnterBackground(_ scene: UIScene) {
    AppState.shared.profile.shutdown()
    BraveVPN.sendVPNWorksInBackgroundNotification()
  }

  func scene(_ scene: UIScene, openURLContexts URLContexts: Set<UIOpenURLContext>) {
    guard let scene = scene as? UIWindowScene else {
      log.debug("Invalid Scene - Scene is not a UIWindowScene")
      return
    }

    URLContexts.forEach({
      guard let routerpath = NavigationPath(url: $0.url, isPrivateBrowsing: scene.browserViewController?.privateBrowsingManager.isPrivateBrowsing == true) else {
        log.debug("Invalid Navigation Path: \($0.url)")
        return
      }

      scene.browserViewController?.handleNavigationPath(path: routerpath)
    })
  }
  
  func scene(_ scene: UIScene, didUpdate userActivity: NSUserActivity) {
    log.debug("Updated User Activity for Scene")
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
    
    if let userActivity = userActivity {
      // Restore the scene with the WindowID from the User-Activity
      
      let windowIdString = userActivity.userInfo?["WindowID"] as? String ?? ""
      windowId = UUID(uuidString: windowIdString) ?? UUID()
      isPrivate = userActivity.userInfo?["isPrivate"] as? Bool == true
      privateBrowsingManager.isPrivateBrowsing = isPrivate
      urlToOpen = userActivity.userInfo?["OpenURL"] as? URL
      
      // Create a new session window
      SessionWindow.createWindow(isPrivate: isPrivate, isSelected: true, uuid: windowId)
      
      scene.userActivity = userActivity
      scene.session.userInfo?["WindowID"] = windowId
      scene.session.userInfo?["isPrivate"] = isPrivate
    } else if let sceneWindowId = scene.session.userInfo?["WindowID"] as? String,
              let sceneIsPrivate = scene.session.userInfo?["isPrivate"] as? Bool,
              let windowUUID = UUID(uuidString: sceneWindowId) {
      
      // Restore the scene from the Session's User-Info WindowID
      
      windowId = windowUUID
      isPrivate = sceneIsPrivate
      privateBrowsingManager.isPrivateBrowsing = sceneIsPrivate
      urlToOpen = scene.session.userInfo?["OpenURL"] as? URL
      
      scene.userActivity = BrowserState.userActivity(for: windowId, isPrivate: isPrivate)
    } else {
      // Should NOT be possible to get here.
      // However, if a controller is NOT active, and tapping the app-icon opens a New-Window
      // Then we need to make sure not to restore that "New" Window
      // So we iterate all the windows and if there is no active window, then we need to "Restore" one.
      // If a window is already active, we need to create a new blank window.
      
      if let activeWindowId = SessionWindow.getActiveWindow(context: DataController.swiftUIContext)?.windowId {
        let activeSession = UIApplication.shared.openSessions
          .compactMap({ $0.userInfo?["WindowID"] as? String })
          .first(where: { $0 == activeWindowId.uuidString })
        
        if activeSession != nil {
          // An existing window is already active on screen
          // So create a new window
          let newWindowId = UUID()
          SessionWindow.createWindow(isPrivate: false, isSelected: true, uuid: newWindowId)
          windowId = newWindowId
        } else {
          // Restore the active window since none is active on screen
          windowId = activeWindowId
        }
      } else {
        // Should be impossible to get here. There must always be an active window.
        // However, if for some reason there is none, then we should create one.
        let newWindowId = UUID()
        SessionWindow.createWindow(isPrivate: false, isSelected: true, uuid: newWindowId)
        windowId = newWindowId
      }
      
      isPrivate = false
      privateBrowsingManager.isPrivateBrowsing = false
      urlToOpen = nil
      
      scene.userActivity = BrowserState.userActivity(for: windowId, isPrivate: false)
      scene.session.userInfo = ["WindowID": windowId.uuidString,
                                "isPrivate": false]
    }

    // Create a browser instance
    let browserViewController = BrowserViewController(
      windowId: windowId,
      profile: profile,
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
        guard let sceneWindowId = $0.session.userInfo?["WindowID"] as? String else {
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
      browserViewController.loadViewIfNeeded()
      browserViewController.switchToTabForURLOrOpen(urlToOpen, isPrivileged: false)
    }

    return browserViewController
  }
}

extension SceneDelegate: UIViewControllerRestoration {
  public static func viewController(withRestorationIdentifierPath identifierComponents: [String], coder: NSCoder) -> UIViewController? {
    return nil
  }
}

extension BrowserViewController {
  func handleReferralLookup(_ urp: UserReferralProgram) {

    if Preferences.URP.referralLookupOutstanding.value == true {
      urp.referralLookup() { referralCode, offerUrl in
        // Attempting to send ping after first urp lookup.
        // This way we can grab the referral code if it exists, see issue #2586.
        AppState.shared.dau.sendPingToServer()
        if let code = referralCode {
          let retryTime = AppConstants.buildChannel.isPublic ? 1.days : 10.minutes
          let retryDeadline = Date() + retryTime

          Preferences.NewTabPage.superReferrerThemeRetryDeadline.value = retryDeadline
          
          // TODO: Set the code in core somehow if we want to support Super Referrals again
          //       then call updateSponsoredImageComponentIfNeeded
        }

        guard let url = offerUrl?.asURL else { return }
        self.openReferralLink(url: url)
      }
    } else {
      urp.pingIfEnoughTimePassed()
    }
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
