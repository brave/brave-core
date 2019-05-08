/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Shared
import Storage
import AVFoundation
import XCGLogger
import MessageUI
import SDWebImage
import SwiftKeychainWrapper
import LocalAuthentication
import CoreSpotlight
import UserNotifications
import BraveShared
import Data

private let log = Logger.browserLogger

let LatestAppVersionProfileKey = "latestAppVersion"
private let InitialPingSentKey = "initialPingSent"

class AppDelegate: UIResponder, UIApplicationDelegate, UIViewControllerRestoration {
    public static func viewController(withRestorationIdentifierPath identifierComponents: [String], coder: NSCoder) -> UIViewController? {
        return nil
    }

    var window: UIWindow?
    var browserViewController: BrowserViewController!
    var rootViewController: UIViewController!
    weak var profile: Profile?
    var tabManager: TabManager!

    weak var application: UIApplication?
    var launchOptions: [AnyHashable: Any]?

    let appVersion = Bundle.main.infoDictionaryString(forKey: "CFBundleShortVersionString")

    var receivedURLs: [URL]?
    
    var authenticator: AppAuthenticator?
    
    /// Object used to handle server pings
    let dau = DAU()

    @discardableResult func application(_ application: UIApplication, willFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?) -> Bool {
        //
        // Determine if the application cleanly exited last time it was used. We default to true in
        // case we have never done this before. Then check if the "ApplicationCleanlyBackgrounded" user
        // default exists and whether was properly set to true on app exit.
        //
        // Then we always set the user default to false. It will be set to true when we the application
        // is backgrounded.
        //

        // Hold references to willFinishLaunching parameters for delayed app launch
        self.application = application
        self.launchOptions = launchOptions

        self.window = UIWindow(frame: UIScreen.main.bounds)
        self.window!.backgroundColor = UIColor.Photon.White100

        AdBlockStats.shared.startLoading()
        AdblockResourceDownloader.shared.regionalAdblockResourcesSetup()
        
        HttpsEverywhereStats.shared.startLoading()
        
        // Passcode checking, must happen on immediate launch
        if !DataController.shared.storeExists() {
            // Since passcode is stored in keychain it persists between installations.
            //  If there is no database (either fresh install, or deleted in file system), there is no real reason
            //  to passcode the browser (no data to protect).
            // Main concern is user installs Brave after a long period of time, cannot recall passcode, and can
            //  literally never use Brave. This bypasses this situation, while not using a modifiable pref.
            KeychainWrapper.sharedAppContainerKeychain.setAuthenticationInfo(nil)
        }

        // Short circuit the app if we want to email logs from the debug menu
        if DebugSettingsBundleOptions.launchIntoEmailComposer {
            self.window?.rootViewController = UIViewController()
            presentEmailComposerWithLogs()
            return true
        } else {
            return startApplication(application, withLaunchOptions: launchOptions)
        }
    }

    @discardableResult fileprivate func startApplication(_ application: UIApplication, withLaunchOptions launchOptions: [AnyHashable: Any]?) -> Bool {
        log.info("startApplication begin")
        
        // Set the Firefox UA for browsing.
        setUserAgent()

        // Start the keyboard helper to monitor and cache keyboard state.
        KeyboardHelper.defaultHelper.startObserving()

        DynamicFontHelper.defaultHelper.startObserving()

        MenuHelper.defaultHelper.setItems()

        let logDate = Date()
        // Create a new sync log file on cold app launch. Note that this doesn't roll old logs.
        Logger.syncLogger.newLogWithDate(logDate)

        Logger.browserLogger.newLogWithDate(logDate)

        let profile = getProfile(application)
        let profilePrefix = profile.prefs.getBranchPrefix()
        Migration.launchMigrations(keyPrefix: profilePrefix)

        if !DebugSettingsBundleOptions.disableLocalWebServer {
            // Set up a web server that serves us static content. Do this early so that it is ready when the UI is presented.
            setUpWebServer(profile)
        }
        
        var imageStore: DiskImageStore?
        do {
            imageStore = try DiskImageStore(files: profile.files, namespace: "TabManagerScreenshots", quality: UIConstants.ScreenshotQuality)
        } catch {
            log.error("Failed to create an image store for files: \(profile.files) and namespace: \"TabManagerScreenshots\": \(error.localizedDescription)")
            assertionFailure()
        }
        
        // Temporary fix for Bug 1390871 - NSInvalidArgumentException: -[WKContentView menuHelperFindInPage]: unrecognized selector
        if let clazz = NSClassFromString("WKCont" + "ent" + "View"), let swizzledMethod = class_getInstanceMethod(TabWebViewMenuHelper.self, #selector(TabWebViewMenuHelper.swizzledMenuHelperFindInPage)) {
            class_addMethod(clazz, MenuHelper.SelectorFindInPage, method_getImplementation(swizzledMethod), method_getTypeEncoding(swizzledMethod))
        }

        self.tabManager = TabManager(prefs: profile.prefs, imageStore: imageStore)
        self.tabManager.stateDelegate = self

        // Make sure current private browsing flag respects the private browsing only user preference
        PrivateBrowsingManager.shared.isPrivateBrowsing = Preferences.Privacy.privateBrowsingOnly.value
        
        // Don't track crashes if we're building the development environment due to the fact that terminating/stopping
        // the simulator via Xcode will count as a "crash" and lead to restore popups in the subsequent launch
        let crashedLastSession = !Preferences.AppState.backgroundedCleanly.value && AppConstants.BuildChannel != .developer
        Preferences.AppState.backgroundedCleanly.value = false
        browserViewController = BrowserViewController(profile: self.profile!, tabManager: self.tabManager, crashedLastSession: crashedLastSession)
        browserViewController.edgesForExtendedLayout = []

        // Add restoration class, the factory that will return the ViewController we will restore with.
        browserViewController.restorationIdentifier = NSStringFromClass(BrowserViewController.self)
        browserViewController.restorationClass = AppDelegate.self

        let navigationController = UINavigationController(rootViewController: browserViewController)
        navigationController.delegate = self
        navigationController.isNavigationBarHidden = true
        navigationController.edgesForExtendedLayout = UIRectEdge(rawValue: 0)
        rootViewController = navigationController

        self.window!.rootViewController = rootViewController

        self.updateAuthenticationInfo()
        SystemUtils.onFirstRun()

        log.info("startApplication end")
        return true
    }

    func applicationWillTerminate(_ application: UIApplication) {
        // We have only five seconds here, so let's hope this doesn't take too long.
        self.profile?.shutdown()

        // Allow deinitializers to close our database connections.
        self.profile = nil
        self.tabManager = nil
        self.browserViewController = nil
        self.rootViewController = nil
    }

    /**
     * We maintain a weak reference to the profile so that we can pause timed
     * syncs when we're backgrounded.
     *
     * The long-lasting ref to the profile lives in BrowserViewController,
     * which we set in application:willFinishLaunchingWithOptions:.
     *
     * If that ever disappears, we won't be able to grab the profile to stop
     * syncing... but in that case the profile's deinit will take care of things.
     */
    func getProfile(_ application: UIApplication) -> Profile {
        if let profile = self.profile {
            return profile
        }
        let p = BrowserProfile(localName: "profile")
        self.profile = p
        return p
    }

    func application(_ application: UIApplication, didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?) -> Bool {
        // Override point for customization after application launch.
        var shouldPerformAdditionalDelegateHandling = true

        UIScrollView.doBadSwizzleStuff()

        #if BUDDYBUILD
            print("Setting up BuddyBuild SDK")
            BuddyBuildSDK.setup()
        #endif
        
        window!.makeKeyAndVisible()
        
        authenticator = AppAuthenticator(protectedWindow: window!, promptImmediately: true, isPasscodeEntryCancellable: false)

        // Now roll logs.
        DispatchQueue.global(qos: DispatchQoS.background.qosClass).async {
            Logger.syncLogger.deleteOldLogsDownToSizeLimit()
            Logger.browserLogger.deleteOldLogsDownToSizeLimit()
        }

        // If a shortcut was launched, display its information and take the appropriate action
        if let shortcutItem = launchOptions?[UIApplication.LaunchOptionsKey.shortcutItem] as? UIApplicationShortcutItem {

            QuickActions.sharedInstance.launchedShortcutItem = shortcutItem
            // This will block "performActionForShortcutItem:completionHandler" from being called.
            shouldPerformAdditionalDelegateHandling = false
        }

        // Force the ToolbarTextField in LTR mode - without this change the UITextField's clear
        // button will be in the incorrect position and overlap with the input text. Not clear if
        // that is an iOS bug or not.
        AutocompleteTextField.appearance().semanticContentAttribute = .forceLeftToRight
        
        let isFirstLaunch = Preferences.General.isFirstLaunch.value
        Preferences.General.isFirstLaunch.value = false
        
        if isFirstLaunch {
            FavoritesHelper.addDefaultFavorites()
            profile?.searchEngines.setupDefaultRegionalSearchEngines()
        }
        
        if let urp = UserReferralProgram.shared {
            if isFirstLaunch {
                urp.referralLookup { url in
                    guard let url = url else { return }
                    DispatchQueue.main.asyncAfter(deadline: .now() + 1, execute: {
                        try? self.browserViewController.openURLInNewTab(url.asURL(), isPrivileged: false)
                    })
                }
            } else {
                urp.pingIfEnoughTimePassed()
            }
        } else {
            log.error("Failed to initialize user referral program")
            UrpLog.log("Failed to initialize user referral program")
        }

        UINavigationBar.appearance().tintColor = BraveUX.BraveOrange
      
        (UISwitch.appearance() as UISwitch).do {
            $0.tintColor = BraveUX.SwitchTintColor
            $0.onTintColor = BraveUX.BraveOrange
        }
      
        return shouldPerformAdditionalDelegateHandling
    }

    func application(_ application: UIApplication, open url: URL, options: [UIApplication.OpenURLOptionsKey: Any] = [:]) -> Bool {
        guard let routerpath = NavigationPath(url: url) else {
            return false
        }

        DispatchQueue.main.async {
            NavigationPath.handle(nav: routerpath, with: self.browserViewController)
        }
        return true
    }

    // We sync in the foreground only, to avoid the possibility of runaway resource usage.
    // Eventually we'll sync in response to notifications.
    func applicationDidBecomeActive(_ application: UIApplication) {
        authenticator?.hideBackgroundedBlur()
        
        Preferences.AppState.backgroundedCleanly.value = false
        
        guard !DebugSettingsBundleOptions.launchIntoEmailComposer else {
            return
        }

        if let profile = self.profile {
            profile.reopen()
        }

        // We could load these here, but then we have to futz with the tab counter
        // and making NSURLRequests.
        self.browserViewController.loadQueuedTabs(receivedURLs: self.receivedURLs)
        self.receivedURLs = nil
        application.applicationIconBadgeNumber = 0

        // handle quick actions is available
        let quickActions = QuickActions.sharedInstance
        if let shortcut = quickActions.launchedShortcutItem {
            // dispatch asynchronously so that BVC is all set up for handling new tabs
            // when we try and open them
            quickActions.handleShortCutItem(shortcut, withBrowserViewController: browserViewController)
            quickActions.launchedShortcutItem = nil
        }
        
        // We try to send DAU ping each time the app goes to foreground to work around network edge cases
        // (offline, bad connection etc.)
        dau.sendPingToServer()
    }

    func applicationDidEnterBackground(_ application: UIApplication) {
        syncOnDidEnterBackground(application: application)
    }

    fileprivate func syncOnDidEnterBackground(application: UIApplication) {
        guard let profile = self.profile else {
            return
        }
      
        // BRAVE TODO: Decide whether or not we want to use this for our own sync down the road

        var taskId: UIBackgroundTaskIdentifier = UIBackgroundTaskIdentifier(rawValue: 0)
        taskId = application.beginBackgroundTask (expirationHandler: {
            print("Running out of background time, but we have a profile shutdown pending.")
            self.shutdownProfileWhenNotActive(application)
            application.endBackgroundTask(taskId)
        })

//        if profile.hasSyncableAccount() {
//            profile.syncManager.syncEverything(why: .backgrounded).uponQueue(.main) { _ in
//                self.shutdownProfileWhenNotActive(application)
//                application.endBackgroundTask(taskId)
//            }
//        } else {
            profile.shutdown()
            application.endBackgroundTask(taskId)
//        }
    }

    fileprivate func shutdownProfileWhenNotActive(_ application: UIApplication) {
        // Only shutdown the profile if we are not in the foreground
        guard application.applicationState != .active else {
            return
        }

        profile?.shutdown()
    }

    func applicationWillEnterForeground(_ application: UIApplication) {
        // The reason we need to call this method here instead of `applicationDidBecomeActive`
        // is that this method is only invoked whenever the application is entering the foreground where as
        // `applicationDidBecomeActive` will get called whenever the Touch ID authentication overlay disappears.
        self.updateAuthenticationInfo()
        
        if let authInfo = KeychainWrapper.sharedAppContainerKeychain.authenticationInfo(), authInfo.isPasscodeRequiredImmediately {
            authenticator?.promptUserForAuthentication()
        }
    }
    
    func applicationWillResignActive(_ application: UIApplication) {
        if KeychainWrapper.sharedAppContainerKeychain.authenticationInfo() != nil {
            authenticator?.showBackgroundBlur()
        }
        
        Preferences.AppState.backgroundedCleanly.value = true
    }

    fileprivate func updateAuthenticationInfo() {
        if let authInfo = KeychainWrapper.sharedAppContainerKeychain.authenticationInfo() {
            if !LAContext().canEvaluatePolicy(.deviceOwnerAuthenticationWithBiometrics, error: nil) {
                authInfo.useTouchID = false
                KeychainWrapper.sharedAppContainerKeychain.setAuthenticationInfo(authInfo)
            }
        }
    }

    fileprivate func setUpWebServer(_ profile: Profile) {
        let server = WebServer.sharedInstance
        ReaderModeHandlers.register(server, profile: profile)
        ErrorPageHelper.register(server, certStore: profile.certStore)
        SafeBrowsingHandler.register(server)
        AboutHomeHandler.register(server)
        AboutLicenseHandler.register(server)
        SessionRestoreHandler.register(server)

        if AppConstants.IsRunningTest {
            registerHandlersForTestMethods(server: server.server)
        }

        // Bug 1223009 was an issue whereby CGDWebserver crashed when moving to a background task
        // catching and handling the error seemed to fix things, but we're not sure why.
        // Either way, not implicitly unwrapping a try is not a great way of doing things
        // so this is better anyway.
        do {
            try server.start()
        } catch let err as NSError {
            print("Error: Unable to start WebServer \(err)")
        }
    }

    fileprivate func setUserAgent() {
        let firefoxUA = UserAgent.defaultUserAgent()

        // Set the UA for WKWebView (via defaults), the favicon fetcher, and the image loader.
        // This only needs to be done once per runtime. Note that we use defaults here that are
        // readable from extensions, so they can just use the cached identifier.
        let defaults = UserDefaults(suiteName: AppInfo.sharedContainerIdentifier)!
        defaults.register(defaults: ["UserAgent": firefoxUA])

        SDWebImageDownloader.shared().setValue(firefoxUA, forHTTPHeaderField: "User-Agent")

        // Record the user agent for use by search suggestion clients.
        SearchViewController.userAgent = firefoxUA

        // Some sites will only serve HTML that points to .ico files.
        // The FaviconFetcher is explicitly for getting high-res icons, so use the desktop user agent.
        FaviconFetcher.userAgent = UserAgent.desktopUserAgent()
    }

    fileprivate func presentEmailComposerWithLogs() {
        if let buildNumber = Bundle.main.object(forInfoDictionaryKey: String(kCFBundleVersionKey)) as? NSString {
            let mailComposeViewController = MFMailComposeViewController()
            mailComposeViewController.mailComposeDelegate = self
            mailComposeViewController.setSubject("Debug Info for iOS client version v\(appVersion) (\(buildNumber))")

            if DebugSettingsBundleOptions.attachLogsToDebugEmail {
                do {
                    let logNamesAndData = try Logger.diskLogFilenamesAndData()
                    logNamesAndData.forEach { nameAndData in
                        if let data = nameAndData.1 {
                            mailComposeViewController.addAttachmentData(data, mimeType: "text/plain", fileName: nameAndData.0)
                        }
                    }
                } catch _ {
                    print("Failed to retrieve logs from device")
                }
            }

            self.window?.rootViewController?.present(mailComposeViewController, animated: true, completion: nil)
        }
    }

    func application(_ application: UIApplication, continue userActivity: NSUserActivity, restorationHandler: @escaping ([UIUserActivityRestoring]?) -> Void) -> Bool {

        // If the `NSUserActivity` has a `webpageURL`, it is either a deep link or an old history item
        // reached via a "Spotlight" search before we began indexing visited pages via CoreSpotlight.
        if let url = userActivity.webpageURL {
            let query = url.getQuery()
            
            // Per Adjust documenation, https://docs.adjust.com/en/universal-links/#running-campaigns-through-universal-links,
            // it is recommended that links contain the `deep_link` query parameter. This link will also
            // be url encoded.
            if let deepLink = query["deep_link"]?.removingPercentEncoding, let url = URL(string: deepLink) {
                browserViewController.switchToTabForURLOrOpen(url, isPrivileged: true)
                return true
            }

            browserViewController.switchToTabForURLOrOpen(url, isPrivileged: true)
            return true
        }

        // Otherwise, check if the `NSUserActivity` is a CoreSpotlight item and switch to its tab or
        // open a new one.
        if userActivity.activityType == CSSearchableItemActionType {
            if let userInfo = userActivity.userInfo,
                let urlString = userInfo[CSSearchableItemActivityIdentifier] as? String,
                let url = URL(string: urlString) {
                browserViewController.switchToTabForURLOrOpen(url, isPrivileged: true)
                return true
            }
        }

        return false
    }

    func application(_ application: UIApplication, performActionFor shortcutItem: UIApplicationShortcutItem, completionHandler: @escaping (Bool) -> Void) {
        let handledShortCutItem = QuickActions.sharedInstance.handleShortCutItem(shortcutItem, withBrowserViewController: browserViewController)

        completionHandler(handledShortCutItem)
    }
}

// MARK: - Root View Controller Animations
extension AppDelegate: UINavigationControllerDelegate {
    func navigationController(_ navigationController: UINavigationController, animationControllerFor operation: UINavigationController.Operation, from fromVC: UIViewController, to toVC: UIViewController) -> UIViewControllerAnimatedTransitioning? {
        switch operation {
        case .push:
            return BrowserToTrayAnimator()
        case .pop:
            return TrayToBrowserAnimator()
        default:
            return nil
        }
    }
}

extension AppDelegate: TabManagerStateDelegate {
    func tabManagerWillStoreTabs(_ tabs: [Tab]) {
        // It is possible that not all tabs have loaded yet, so we filter out tabs with a nil URL.
        let storedTabs: [RemoteTab] = tabs.compactMap( Tab.toTab )

        // Don't insert into the DB immediately. We tend to contend with more important
        // work like querying for top sites.
        let queue = DispatchQueue.global(qos: DispatchQoS.background.qosClass)
        queue.asyncAfter(deadline: DispatchTime.now() + Double(Int64(ProfileRemoteTabsSyncDelay * Double(NSEC_PER_MSEC))) / Double(NSEC_PER_SEC)) {
            self.profile?.storeTabs(storedTabs)
        }
    }
}

extension AppDelegate: MFMailComposeViewControllerDelegate {
    func mailComposeController(_ controller: MFMailComposeViewController, didFinishWith result: MFMailComposeResult, error: Error?) {
        // Dismiss the view controller and start the app up
        controller.dismiss(animated: true, completion: nil)
        startApplication(application!, withLaunchOptions: self.launchOptions)
    }
}
