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
import StoreKit
import BraveCore
import Combine

private let log = Logger.browserLogger

private let InitialPingSentKey = "initialPingSent"

extension AppDelegate {
    // A model that is passed used in every scene
    struct SceneInfoModel {
        let profile: Profile
        let diskImageStore: DiskImageStore?
        let migration: Migration?
    }
}

class AppDelegate: UIResponder, UIApplicationDelegate {
    var window: UIWindow?
    var braveCore = BraveCoreMain(userAgent: UserAgent.mobile)
    var migration: Migration?

    private weak var application: UIApplication?
    var launchOptions: [AnyHashable: Any]?

    let appVersion = Bundle.main.infoDictionaryString(forKey: "CFBundleShortVersionString")

    var receivedURLs: [URL]?
    var shutdownWebServer: Timer?
    
    /// Object used to handle server pings
    let dau = DAU()
    
    /// Must be added at launch according to Apple's documentation.
    let iapObserver = IAPObserver()
    
    private var cancellables: Set<AnyCancellable> = []
    private var sceneInfo: SceneInfoModel?

    @discardableResult
    func application(_ application: UIApplication, willFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?) -> Bool {
        // Hold references to willFinishLaunching parameters for delayed app launch
        self.application = application
        self.launchOptions = launchOptions
        
        // Brave Core Initialization
        BraveCoreMain.setLogHandler { severity, file, line, messageStartIndex, message in
            if !message.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty {
                let level: XCGLogger.Level = {
                    switch -severity {
                    case 0: return .error
                    case 1: return .info
                    case 2..<7: return .debug
                    default: return .verbose
                    }
                }()
                
                Logger.braveCoreLogger.logln(
                    level,
                    fileName: file,
                    lineNumber: Int(line),
                    // Only print the actual message content, and drop the final character which is
                    // a new line as it will be handled by logln
                    closure: { message.dropFirst(messageStartIndex).dropLast() }
                )
            }
            return true
        }
        
        migration = Migration(braveCore: braveCore)
        // Setup Adblock Stats and HTTPSE Stats.
        AdBlockStats.shared.startLoading()
        
        // TODO: Downgrade to 14.5 once api becomes available.
        if #available(iOS 15, *) {
            // do nothing, use Apple's https solution.
        } else {
            HttpsEverywhereStats.shared.startLoading()
        }
        
        // Setup Application Shortcuts
        updateShortcutItems(application)
        
        // Must happen before passcode check, otherwise may unnecessarily reset keychain
        migration?.moveDatabaseToApplicationDirectory()
        
        // Passcode checking, must happen on immediate launch
        if !DataController.shared.storeExists() {
            // Reset password authentication prior to `WindowProtection`
            // This reset is done if there is no database as the user will be locked out
            // upon reinstall.
            KeychainWrapper.sharedAppContainerKeychain.setAuthenticationInfo(nil)
        }
        
        return startApplication(application, withLaunchOptions: launchOptions)
    }

    @discardableResult
    fileprivate func startApplication(_ application: UIApplication, withLaunchOptions launchOptions: [AnyHashable: Any]?) -> Bool {
        log.info("startApplication begin")
        
        // Set the Safari UA for browsing.
        setUserAgent()

        // Start the keyboard helper to monitor and cache keyboard state.
        KeyboardHelper.defaultHelper.startObserving()
        DynamicFontHelper.defaultHelper.startObserving()

        MenuHelper.defaultHelper.setItems()
        
        SDImageCodersManager.shared.addCoder(PrivateCDNImageCoder())

        let logDate = Date()
        // Create a new sync log file on cold app launch. Note that this doesn't roll old logs.
        Logger.syncLogger.newLogWithDate(logDate)
        Logger.browserLogger.newLogWithDate(logDate)

        // Setup Profile
        let profile = BrowserProfile(localName: "profile")
        
        // Setup DiskImageStore for Screenshots
        let diskImageStore = { () -> DiskImageStore? in
            do {
                return try DiskImageStore(files: profile.files,
                                          namespace: "TabManagerScreenshots",
                                          quality: UIConstants.screenshotQuality)
            } catch {
                log.error("Failed to create an image store for files: \(profile.files) and namespace: \"TabManagerScreenshots\": \(error.localizedDescription)")
                assertionFailure()
            }
            return nil
        }()
        
        // Setup Scene Info
        sceneInfo = SceneInfoModel(profile: profile,
                                   diskImageStore: diskImageStore,
                                   migration: migration)
        
        // Perform migrations
        let profilePrefix = profile.prefs.getBranchPrefix()
        migration?.launchMigrations(keyPrefix: profilePrefix)
        
        // Setup GCD-WebServer
        setUpWebServer(profile)
        
        // Temporary fix for Bug 1390871 - NSInvalidArgumentException: -[WKContentView menuHelperFindInPage]: unrecognized selector
        if let clazz = NSClassFromString("WKCont" + "ent" + "View"), let swizzledMethod = class_getInstanceMethod(TabWebViewMenuHelper.self, #selector(TabWebViewMenuHelper.swizzledMenuHelperFindInPage)) {
            class_addMethod(clazz, MenuHelper.selectorFindInPage, method_getImplementation(swizzledMethod), method_getTypeEncoding(swizzledMethod))
        }
        
        #if !NO_BRAVE_NEWS
        if Preferences.BraveNews.isEnabled.value && !Preferences.BraveNews.userOptedIn.value {
            // Opt-out any user that has not explicitly opted-in
            Preferences.BraveNews.isEnabled.value = false
            // User now has to explicitly opt-in
            Preferences.BraveNews.isShowingOptIn.value = true
        }
        
        if !Preferences.BraveNews.languageChecked.value,
           let languageCode = Locale.preferredLanguages.first?.prefix(2) {
            Preferences.BraveNews.languageChecked.value = true
            // Base opt-in visibility on whether or not the user's language is supported in BT
            Preferences.BraveNews.isShowingOptIn.value = FeedDataSource.supportedLanguages.contains(String(languageCode))
        }
        #endif

        SystemUtils.onFirstRun()
        
        // Schedule Brave Core Priority Tasks
        braveCore.scheduleLowPriorityStartupTasks()

        log.info("startApplication end")
        return true
    }

    func application(_ application: UIApplication, didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?) -> Bool {
        // IAPs can trigger on the app as soon as it launches,
        // for example when a previous transaction was not finished and is in pending state.
        SKPaymentQueue.default().add(iapObserver)
        
        // Override point for customization after application launch.
        var shouldPerformAdditionalDelegateHandling = true
        
        AdblockRustEngine.setDomainResolver { urlCString, start, end in
            guard let urlCString = urlCString else { return }
            let urlString = String(cString: urlCString)
            let parsableURLString: String = {
                // Apple's URL implementation requires a URL be prefixed with a scheme to be
                // parsed properly (otherwise URL(string: X) will resolve to placing the entire
                // contents of X into the `path` property
                if urlString.asURL?.scheme == nil {
                    return "http://\(urlString)"
                }
                return urlString
            }()
            
            guard let url = URL(string: parsableURLString),
                  let baseDomain = url.baseDomain,
                  let range = urlString.range(of: baseDomain) else {
                log.error("Failed to resolve domain ")
                return
            }
            
            let startIndex: Int = urlString.distance(from: urlString.startIndex, to: range.lowerBound)
            let endIndex: Int = urlString.distance(from: urlString.startIndex, to: range.upperBound)
            start?.pointee = UInt32(startIndex)
            end?.pointee = UInt32(endIndex)
        }
        
        UIScrollView.doBadSwizzleStuff()
        applyAppearanceDefaults()

        if Preferences.Rewards.isUsingBAP.value == nil {
            Preferences.Rewards.isUsingBAP.value = Locale.current.regionCode == "JP"
        }
        
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
        
        Preferences.Review.launchCount.value += 1
        
        let isFirstLaunch = Preferences.General.isFirstLaunch.value
        if Preferences.General.basicOnboardingCompleted.value == OnboardingState.undetermined.rawValue {
            Preferences.General.basicOnboardingCompleted.value =
                isFirstLaunch ? OnboardingState.unseen.rawValue : OnboardingState.completed.rawValue
        }
        
        // Check if user has launched the application before and determine if it is a new retention user
        if  Preferences.General.isFirstLaunch.value, Preferences.General.isNewRetentionUser.value == nil {
            Preferences.General.isNewRetentionUser.value = true
        }
        
        if Preferences.DAU.appRetentionLaunchDate.value == nil {
            Preferences.DAU.appRetentionLaunchDate.value = Date()
        }
        
        Preferences.General.isFirstLaunch.value = false
        
        // Search engine setup must be checked outside of 'firstLaunch' loop because of #2770.
        // There was a bug that when you skipped onboarding, default search engine preference
        // was not set.
        if Preferences.Search.defaultEngineName.value == nil {
            sceneInfo?.profile.searchEngines.searchEngineSetup()
        }
        
        // Migration of Yahoo Search Engines
        if !Preferences.Search.yahooEngineMigrationCompleted.value {
            sceneInfo?.profile.searchEngines.migrateDefaultYahooSearchEngines()
        }
        
        if isFirstLaunch {
            Preferences.DAU.installationDate.value = Date()
            
            // VPN credentials are kept in keychain and persist between app reinstalls.
            // To avoid unexpected problems we clear all vpn keychain items.
            // New set of keychain items will be created on purchase or iap restoration.
            BraveVPN.clearCredentials()
        }
        
        if UserReferralProgram.shared != nil {
            if Preferences.URP.referralLookupOutstanding.value == nil {
                // This preference has never been set, and this means it is a new or upgraded user.
                // That distinction must be made to know if a network request for ref-code look up should be made.
                
                // Setting this to an explicit value so it will never get overwritten on subsequent launches.
                // Upgrade users should not have ref code ping happening.
                Preferences.URP.referralLookupOutstanding.value = isFirstLaunch
            }
            
            SceneDelegate.shouldHandleUrpLookup = true
        } else {
            log.error("Failed to initialize user referral program")
            UrpLog.log("Failed to initialize user referral program")
        }
        
        AdblockResourceDownloader.shared.startLoading()
      
        return shouldPerformAdditionalDelegateHandling
    }
    
    func applicationWillTerminate(_ application: UIApplication) {
        // We have only five seconds here, so let's hope this doesn't take too long.
        sceneInfo?.profile.shutdown()
        
        SKPaymentQueue.default().remove(iapObserver)
        
        // Clean up BraveCore
        braveCore.syncAPI.removeAllObservers()
    }
    
    func updateShortcutItems(_ application: UIApplication) {
        let newTabItem = UIMutableApplicationShortcutItem(type: "\(Bundle.main.bundleIdentifier ?? "").NewTab",
            localizedTitle: Strings.quickActionNewTab,
            localizedSubtitle: nil,
            icon: UIApplicationShortcutIcon(templateImageName: "quick_action_new_tab"),
            userInfo: [:])
        
        let privateTabItem = UIMutableApplicationShortcutItem(type: "\(Bundle.main.bundleIdentifier ?? "").NewPrivateTab",
            localizedTitle: Strings.quickActionNewPrivateTab,
            localizedSubtitle: nil,
            icon: UIApplicationShortcutIcon(templateImageName: "quick_action_new_private_tab"),
            userInfo: [:])
        
        let scanQRCodeItem = UIMutableApplicationShortcutItem(type: "\(Bundle.main.bundleIdentifier ?? "").ScanQRCode",
            localizedTitle: Strings.scanQRCodeViewTitle,
            localizedSubtitle: nil,
            icon: UIApplicationShortcutIcon(templateImageName: "recent-search-qrcode"),
            userInfo: [:])

        application.shortcutItems = Preferences.Privacy.privateBrowsingOnly.value ? [privateTabItem, scanQRCodeItem] : [newTabItem, privateTabItem, scanQRCodeItem]
    }
    
    func application(_ application: UIApplication, supportedInterfaceOrientationsFor window: UIWindow?) -> UIInterfaceOrientationMask {
        if let presentedViewController = window?.rootViewController?.presentedViewController {
            return presentedViewController.supportedInterfaceOrientations
        } else {
            return window?.rootViewController?.supportedInterfaceOrientations ?? .portraitUpsideDown
        }
    }

    func syncOnDidEnterBackground(application: UIApplication) {
        // BRAVE TODO: Decide whether or not we want to use this for our own sync down the road

        var taskId: UIBackgroundTaskIdentifier = UIBackgroundTaskIdentifier(rawValue: 0)
        taskId = application.beginBackgroundTask {
            print("Running out of background time, but we have a profile shutdown pending.")
            self.shutdownProfileWhenNotActive(application)
            application.endBackgroundTask(taskId)
        }

        sceneInfo?.profile.shutdown()
        
        application.endBackgroundTask(taskId)
        
        shutdownWebServer?.invalidate()
        shutdownWebServer = Timer.scheduledTimer(withTimeInterval: 2, repeats: false) { [weak self] _ in
            WebServer.sharedInstance.server.stop()
            self?.shutdownWebServer = nil
        }
    }

    fileprivate func shutdownProfileWhenNotActive(_ application: UIApplication) {
        // Only shutdown the profile if we are not in the foreground
        guard application.applicationState != .active else {
            return
        }

        sceneInfo?.profile.shutdown()
    }

    func setUpWebServer(_ profile: Profile) {
        let server = WebServer.sharedInstance
        guard !server.server.isRunning else { return }
        
        let responders: [(String, InternalSchemeResponse)] =
            [(AboutHomeHandler.path, AboutHomeHandler()),
             (AboutLicenseHandler.path, AboutLicenseHandler()),
             (SessionRestoreHandler.path, SessionRestoreHandler()),
             (ErrorPageHandler.path, ErrorPageHandler())]
        responders.forEach { (path, responder) in
            InternalSchemeHandler.responders[path] = responder
        }
        
        ReaderModeHandlers.register(server, profile: profile) //TODO: PORT TO InternalSchemeHandler
        BookmarksInterstitialPageHandler.register(server) //TODO: PORT TO InternalSchemeHandler

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
        let userAgent = UserAgent.userAgentForDesktopMode

        // Set the favicon fetcher, and the image loader.
        // This only needs to be done once per runtime. Note that we use defaults here that are
        // readable from extensions, so they can just use the cached identifier.

        SDWebImageDownloader.shared.setValue(userAgent, forHTTPHeaderField: "User-Agent")

        WebcompatReporter.userAgent = userAgent
        
        // Record the user agent for use by search suggestion clients.
        SearchViewController.userAgent = userAgent

        // Some sites will only serve HTML that points to .ico files.
        // The FaviconFetcher is explicitly for getting high-res icons, so use the desktop user agent.
        FaviconFetcher.htmlParsingUserAgent = UserAgent.desktop
    }

    fileprivate func presentEmailComposerWithLogs() {
        if let buildNumber = Bundle.main.object(forInfoDictionaryKey: String(kCFBundleVersionKey)) as? NSString {
            let mailComposeViewController = MFMailComposeViewController()
            mailComposeViewController.mailComposeDelegate = self
            mailComposeViewController.setSubject("Debug Info for iOS client version v\(appVersion) (\(buildNumber))")

            self.window?.rootViewController?.present(mailComposeViewController, animated: true, completion: nil)
        }
    }
    
    func sceneInfo(for sceneSession: UISceneSession) -> SceneInfoModel? {
        return sceneInfo
    }
}

extension AppDelegate: MFMailComposeViewControllerDelegate {
    func mailComposeController(_ controller: MFMailComposeViewController, didFinishWith result: MFMailComposeResult, error: Error?) {
        // Dismiss the view controller and start the app up
        controller.dismiss(animated: true, completion: nil)
        startApplication(application!, withLaunchOptions: self.launchOptions)
    }
}

extension AppDelegate {
    // MARK: UISceneSession Lifecycle

    func application(_ application: UIApplication, configurationForConnecting connectingSceneSession: UISceneSession, options: UIScene.ConnectionOptions) -> UISceneConfiguration {
        // Called when a new scene session is being created.
        // Use this method to select a configuration to create the new scene with.
        return UISceneConfiguration(name: connectingSceneSession.configuration.name,
                                    sessionRole: connectingSceneSession.role).then {
            $0.sceneClass = connectingSceneSession.configuration.sceneClass
            $0.delegateClass = connectingSceneSession.configuration.delegateClass
        }
    }

    func application(_ application: UIApplication, didDiscardSceneSessions sceneSessions: Set<UISceneSession>) {
        // Called when the user discards a scene session.
        // If any sessions were discarded while the application was not running, this will be called shortly after application:didFinishLaunchingWithOptions.
        // Use this method to release any resources that were specific to the discarded scenes, as they will not return.
    }
}
