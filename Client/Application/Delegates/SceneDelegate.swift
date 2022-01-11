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

private let log = Logger.browserLogger

class SceneDelegate: UIResponder, UIWindowSceneDelegate {

    private var windowProtection: WindowProtection?
    private var sceneInfo: AppDelegate.SceneInfoModel?
    static var shouldHandleUrpLookup = false
    
    private var cancellables: Set<AnyCancellable> = []

    func scene(_ scene: UIScene, willConnectTo session: UISceneSession, options connectionOptions: UIScene.ConnectionOptions) {
        guard let windowScene = (scene as? UIWindowScene) else { return }
        
        sceneInfo = (UIApplication.shared.delegate as? AppDelegate)?.sceneInfo(for: session)
        guard let sceneInfo = sceneInfo else {
            return
        }
        
        // We have to wait until pre1.12 migration is done until we proceed with database
        // initialization. This is because Database container may change. See bugs #3416, #3377.
        DataController.shared.initializeOnce()
        Migration.postCoreDataInitMigrations()
        
        Preferences.General.themeNormalMode.objectWillChange
            .merge(with: PrivateBrowsingManager.shared.objectWillChange)
            .receive(on: RunLoop.main)
            .sink { [weak self] _ in
                self?.updateTheme()
            }
            .store(in: &cancellables)

        // Create a browser instance
        guard let browserViewController = createBrowserWindow(scene: windowScene,
                                                              profile: sceneInfo.profile,
                                                              diskImageStore: sceneInfo.diskImageStore,
                                                              migration: sceneInfo.migration) else {
            fatalError("Failed to create browser instance")
        }
        
        if SceneDelegate.shouldHandleUrpLookup {
            // TODO: Find a better way to do this when multiple windows are involved.
            SceneDelegate.shouldHandleUrpLookup = false
            
            if let urp = UserReferralProgram.shared {
                browserViewController.handleReferralLookup(urp, checkClipboard: false)
            }
        }
        
        // Setup Playlist
        // This restores the playlist incomplete downloads. So if a download was started
        // and interrupted on application death, we restart it on next launch.
        PlaylistManager.shared.restoreSession()
        
        // Setup Playlist Car-Play
        // TODO: Decide what to do if we have multiple windows
        // as it is only possible to have a single car-play instance.
        // Once we move to iOS 14+, this is easy to fix as we just pass car-play a `MediaStreamer`
        // instance instead of a `BrowserViewController`.
        PlaylistCarplayManager.shared.do {
            $0.browserController = browserViewController
        }
        
        // Assign each browser a navigation controller
        let navigationController = UINavigationController(rootViewController: browserViewController).then {
            $0.isNavigationBarHidden = true
            $0.edgesForExtendedLayout = UIRectEdge(rawValue: 0)
        }
        
        // Assign each browser a window of its own
        let window = UIWindow(windowScene: windowScene).then {
            $0.backgroundColor = .black
            $0.overrideUserInterfaceStyle = expectedThemeOverride
            $0.tintColor = UIColor {
                if $0.userInterfaceStyle == .dark {
                    return .braveLighterBlurple
                }
                return .braveBlurple
            }
            
            $0.rootViewController = navigationController
        }
        
        // TODO: Refactor to accept a UIWindowScene
        // Then store the `windowProtection` in the `BrowserViewController` directly.
        // As each instance should have its own protection?
        self.windowProtection = WindowProtection(window: window)
        window.makeKeyAndVisible()
        
        // Open shared URLs on launch if there are any
        if !connectionOptions.urlContexts.isEmpty {
            self.scene(windowScene, openURLContexts: connectionOptions.urlContexts)
        }
    }

    func sceneDidDisconnect(_ scene: UIScene) {
        
    }

    func sceneDidBecomeActive(_ scene: UIScene) {
        guard let appDelegate = UIApplication.shared.delegate as? AppDelegate,
              let scene = scene as? UIWindowScene,
              let profile = sceneInfo?.profile else {
            return
        }
        
        appDelegate.shutdownWebServer?.invalidate()
        appDelegate.shutdownWebServer = nil
        
        Preferences.AppState.backgroundedCleanly.value = false

        profile.reopen()
        appDelegate.setUpWebServer(profile)
        
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
            appDelegate.dau.sendPingToServer()
        }
    }

    func sceneWillResignActive(_ scene: UIScene) {
        Preferences.AppState.backgroundedCleanly.value = true
    }

    func sceneWillEnterForeground(_ scene: UIScene) {
        // The reason we need to call this method here instead of `applicationDidBecomeActive`
        // is that this method is only invoked whenever the application is entering the foreground where as
        // `applicationDidBecomeActive` will get called whenever the Touch ID authentication overlay disappears.
        AdblockResourceDownloader.shared.startLoading()
        
        if let scene = scene as? UIWindowScene {
            scene.browserViewController?.showWalletTransferExpiryPanelIfNeeded()
            scene.browserViewController?.windowProtection = windowProtection
        }
    }

    func sceneDidEnterBackground(_ scene: UIScene) {
        guard let appDelegate = UIApplication.shared.delegate as? AppDelegate else {
            return
        }
        
        appDelegate.syncOnDidEnterBackground(application: UIApplication.shared)
        BraveVPN.sendVPNWorksInBackgroundNotification()
    }
    
    func scene(_ scene: UIScene, openURLContexts URLContexts: Set<UIOpenURLContext>) {
        guard let scene = scene as? UIWindowScene else {
            log.debug("Invalid Scene - Scene is not a UIWindowScene")
            return
        }
        
        URLContexts.forEach({
            guard let routerpath = NavigationPath(url: $0.url) else {
                log.debug("Invalid Navigation Path: \($0.url)")
                return
            }
            
            scene.browserViewController?.handleNavigationPath(path: routerpath)
        })
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
            guard let siteURL = intentURL, let url = URL(string: siteURL) else {
                return
            }

            if let browserViewController = scene.browserViewController {
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
        return nil
    }
}

extension SceneDelegate {
    private var expectedThemeOverride: UIUserInterfaceStyle {
        let themeOverride = DefaultTheme(
            rawValue: Preferences.General.themeNormalMode.value
        )?.userInterfaceStyleOverride ?? .unspecified
        let isPrivateBrowsing = PrivateBrowsingManager.shared.isPrivateBrowsing
        return isPrivateBrowsing ? .dark : themeOverride
    }
    
    private func updateTheme() {
        guard let window = UIApplication.shared.windows.first(where: { (window) -> Bool in window.isKeyWindow}) else { return }
        UIView.transition(with: window, duration: 0.15, options: [.transitionCrossDissolve], animations: {
            window.overrideUserInterfaceStyle = self.expectedThemeOverride
        }, completion: nil)
    }
}

extension SceneDelegate {
    private func createBrowserWindow(scene: UIWindowScene, profile: Profile, diskImageStore: DiskImageStore?, migration: Migration?) -> BrowserViewController? {
        // There has to be an application delegate
        guard let appDelegate = UIApplication.shared.delegate as? AppDelegate else {
            return nil
        }
        
        // Make sure current private browsing flag respects the private browsing only user preference
        PrivateBrowsingManager.shared.isPrivateBrowsing = Preferences.Privacy.privateBrowsingOnly.value
        
        // Don't track crashes if we're building the development environment due to the fact that terminating/stopping
        // the simulator via Xcode will count as a "crash" and lead to restore popups in the subsequent launch
        let crashedLastSession = !Preferences.AppState.backgroundedCleanly.value && AppConstants.buildChannel != .debug
        
        // Create a browser instance
        let browserViewController = BrowserViewController(
            profile: profile,
            diskImageStore: diskImageStore,
            braveCore: appDelegate.braveCore,
            migration: migration,
            crashedLastSession: crashedLastSession)
        
        browserViewController.do {
            $0.edgesForExtendedLayout = []
            
            // Add restoration class, the factory that will return the ViewController we will restore with.
            $0.restorationIdentifier = NSStringFromClass(BrowserViewController.self)
            $0.restorationClass = SceneDelegate.self
            
            // Remove Ad-Grant Reminders
            $0.removeScheduledAdGrantReminders()
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
    func handleReferralLookup(_ urp: UserReferralProgram, checkClipboard: Bool) {
        let initialOnboarding =
            Preferences.General.basicOnboardingProgress.value == OnboardingProgress.none.rawValue
        
        // FIXME: Update to iOS14 clipboard api once ready (#2838)
        if initialOnboarding && UIPasteboard.general.hasStrings {
            log.debug("Skipping URP call at app launch, this is handled in privacy consent onboarding screen")
            return
        }
        
        if Preferences.URP.referralLookupOutstanding.value == true {
            var refCode: String?
            
            if Preferences.URP.referralCode.value == nil {
                UrpLog.log("No ref code exists on launch, attempting clipboard retrieval")
                let savedRefCode = checkClipboard ? UIPasteboard.general.string : nil
                refCode = UserReferralProgram.sanitize(input: savedRefCode)
                
                if refCode != nil {
                    UrpLog.log("Clipboard ref code found: " + (savedRefCode ?? "!Clipboard Empty!"))
                    UrpLog.log("Clearing clipboard.")
                    UIPasteboard.general.clearPasteboard()
                }
            }
            
            urp.referralLookup(refCode: refCode) { referralCode, offerUrl in
                // Attempting to send ping after first urp lookup.
                // This way we can grab the referral code if it exists, see issue #2586.
                (UIApplication.shared.delegate as? AppDelegate)?.dau.sendPingToServer()
                if let code = referralCode {
                    let retryTime = AppConstants.buildChannel.isPublic ? 1.days : 10.minutes
                    let retryDeadline = Date() + retryTime
                    
                    Preferences.NewTabPage.superReferrerThemeRetryDeadline.value = retryDeadline
                    
                    self.backgroundDataSource
                        .fetchSpecificResource(.superReferral(code: code))
                } else {
                    self.backgroundDataSource.startFetching()
                }
                
                guard let url = offerUrl?.asURL else { return }
                self.openReferralLink(url: url)
            }
        } else {
            urp.pingIfEnoughTimePassed()
            self.backgroundDataSource.startFetching()
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
