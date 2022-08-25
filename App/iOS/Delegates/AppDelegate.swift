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
import Brave
import BraveVPN
import RuntimeWarnings

#if DEBUG
import os
#endif

private let log = Logger.browserLogger

extension AppDelegate {
  // A model that is passed used in every scene
  struct SceneInfoModel {
    let profile: Profile
    let diskImageStore: DiskImageStore?
    let migration: Migration?
  }
}

@main
class AppDelegate: UIResponder, UIApplicationDelegate {
  var window: UIWindow?
  lazy var braveCore: BraveCoreMain = {
    var switches: [BraveCoreSwitch: String] = [:]
    if !AppConstants.buildChannel.isPublic {
      // Check prefs for additional switches
      let activeSwitches = Preferences.BraveCore.activeSwitches.value
      let switchValues = Preferences.BraveCore.switchValues.value
      for activeSwitch in activeSwitches {
        if let value = switchValues[activeSwitch], !value.isEmpty {
          switches[BraveCoreSwitch(rawValue: activeSwitch)] = value
        }
      }
    }
    return BraveCoreMain(userAgent: UserAgent.mobile, additionalSwitches: switches)
  }()
  
  var migration: Migration?

  private weak var application: UIApplication?
  var launchOptions: [AnyHashable: Any]?

  let appVersion = Bundle.main.infoDictionaryString(forKey: "CFBundleShortVersionString")

  var receivedURLs: [URL]?
  var shutdownWebServer: Timer?

  /// Object used to handle server pings
  private(set) lazy var dau = DAU(braveCoreStats: braveCore.braveStats)

  private var cancellables: Set<AnyCancellable> = []
  private var sceneInfo: SceneInfoModel?
  
  override init() {
    #if MOZ_CHANNEL_RELEASE
    AppConstants.buildChannel = .release
    #elseif MOZ_CHANNEL_BETA
    AppConstants.buildChannel = .beta
    #elseif MOZ_CHANNEL_DEV
    AppConstants.buildChannel = .dev
    #elseif MOZ_CHANNEL_ENTERPRISE
    AppConstants.buildChannel = .enterprise
    #elseif MOZ_CHANNEL_DEBUG
    AppConstants.buildChannel = .debug
    #endif
    super.init()
  }

  @discardableResult
  func application(_ application: UIApplication, willFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?) -> Bool {
    // Hold references to willFinishLaunching parameters for delayed app launch
    self.application = application
    self.launchOptions = launchOptions

    // Brave Core Initialization
    BraveCoreMain.setLogHandler { severity, file, line, messageStartIndex, message in
      let message = String(message.dropFirst(messageStartIndex).dropLast())
        .trimmingCharacters(in: .whitespacesAndNewlines)
      if message.isEmpty {
        // Nothing to print
        return true
      }
      
      if severity == .fatal {
        let filename = URL(fileURLWithPath: file).lastPathComponent
#if DEBUG
        // Prints a special runtime warning instead of crashing.
        os_log(
          .fault,
          dso: os_rw.dso,
          log: os_rw.log(category: "BraveCore"),
          "[%@:%ld] > %@", filename, line, message
        )
        return true
#else
        fatalError("Fatal BraveCore Error at \(filename):\(line).\n\(message)")
#endif
      }

      let level: XCGLogger.Level = {
        switch severity {
        case .fatal: return .severe
        case .error: return .error
        case .warning: return .warning
        case .info: return .info
        default: return .debug
        }
      }()

      Logger.braveCoreLogger.logln(
        level,
        fileName: file,
        lineNumber: Int(line),
        // Only print the actual message content, and drop the final character which is
        // a new line as it will be handled by logln
        closure: { message }
      )
      return true
    }

    migration = Migration(braveCore: braveCore)
    // Setup Adblock Stats and HTTPSE Stats.
    AdBlockStats.shared.startLoading()

    // TODO: Downgrade to 14.5 once api becomes available.
    if #unavailable(iOS 15.0) {
      HttpsEverywhereStats.shared.startLoading()
    }

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
    // Moving Brave VPN v1 users to v2 type of credentials.
    // This is a light operation, can be called at every launch without troubles.
    BraveVPN.migrateV1Credentials()

    // Start the keyboard helper to monitor and cache keyboard state.
    KeyboardHelper.defaultHelper.startObserving()
    DynamicFontHelper.defaultHelper.startObserving()
    ReaderModeFonts.registerCustomFonts()

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
        return try DiskImageStore(
          files: profile.files,
          namespace: "TabManagerScreenshots",
          quality: UIConstants.screenshotQuality)
      } catch {
        log.error("Failed to create an image store for files: \(profile.files) and namespace: \"TabManagerScreenshots\": \(error.localizedDescription)")
        assertionFailure()
      }
      return nil
    }()

    // Setup Scene Info
    sceneInfo = SceneInfoModel(
      profile: profile,
      diskImageStore: diskImageStore,
      migration: migration)

    // Perform migrations
    let profilePrefix = profile.prefs.getBranchPrefix()
    migration?.launchMigrations(keyPrefix: profilePrefix, profile: profile)

    // Setup GCD-WebServer
    setUpWebServer(profile)

    // Temporary fix for Bug 1390871 - NSInvalidArgumentException: -[WKContentView menuHelperFindInPage]: unrecognized selector
    if let clazz = NSClassFromString("WKCont" + "ent" + "View"), let swizzledMethod = class_getInstanceMethod(TabWebViewMenuHelper.self, #selector(TabWebViewMenuHelper.swizzledMenuHelperFindInPage)) {
      class_addMethod(clazz, MenuHelper.selectorFindInPage, method_getImplementation(swizzledMethod), method_getTypeEncoding(swizzledMethod))
    }

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

    SystemUtils.onFirstRun()

    // Schedule Brave Core Priority Tasks
    braveCore.scheduleLowPriorityStartupTasks()

    log.info("startApplication end")
    return true
  }

  func application(_ application: UIApplication, didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?) -> Bool {
    // IAPs can trigger on the app as soon as it launches,
    // for example when a previous transaction was not finished and is in pending state.
    SKPaymentQueue.default().add(BraveVPN.iapObserver)

    // Override point for customization after application launch.
    var shouldPerformAdditionalDelegateHandling = true

    AdblockEngine.setDomainResolver(AdblockEngine.defaultDomainResolver)

    UIView.applyAppearanceDefaults()

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
    if Preferences.General.isFirstLaunch.value, Preferences.General.isNewRetentionUser.value == nil {
      Preferences.General.isNewRetentionUser.value = true
    }

    if Preferences.DAU.appRetentionLaunchDate.value == nil {
      Preferences.DAU.appRetentionLaunchDate.value = Date()
    }
    
    // Starting Date for Brave Search Promotion to mark 15 days period
    if Preferences.BraveSearch.braveSearchPromotionLaunchDate.value == nil {
      Preferences.BraveSearch.braveSearchPromotionLaunchDate.value = Date()
    }
    
    // After user pressed 'maybe later' in promotion, the banner will not be shown user in same session
    if Preferences.BraveSearch.braveSearchPromotionCompletionState.value ==
        BraveSearchPromotionState.maybeLaterSameSession.rawValue {
      Preferences.BraveSearch.braveSearchPromotionCompletionState.value =
        BraveSearchPromotionState.maybeLaterUpcomingSession.rawValue
    }
    
    if isFirstLaunch {
      Preferences.PrivacyReports.ntpOnboardingCompleted.value = false
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
    CosmeticFiltersResourceDownloader.shared.startLoading()
    DebouncingResourceDownloader.shared.startLoading()

    return shouldPerformAdditionalDelegateHandling
  }

  func applicationWillTerminate(_ application: UIApplication) {
    // We have only five seconds here, so let's hope this doesn't take too long.
    sceneInfo?.profile.shutdown()

    SKPaymentQueue.default().remove(BraveVPN.iapObserver)

    // Clean up BraveCore
    braveCore.syncAPI.removeAllObservers()

    log.debug("Cleanly Terminated the Application")
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
      [
        (AboutHomeHandler.path, AboutHomeHandler()),
        (AboutLicenseHandler.path, AboutLicenseHandler()),
        (SessionRestoreHandler.path, SessionRestoreHandler()),
        (ErrorPageHandler.path, ErrorPageHandler()),
      ]
    responders.forEach { (path, responder) in
      InternalSchemeHandler.responders[path] = responder
    }

    ReaderModeHandlers.register(server, profile: profile)  // TODO: PORT TO InternalSchemeHandler
    BookmarksInterstitialPageHandler.register(server)  // TODO: PORT TO InternalSchemeHandler

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
    return UISceneConfiguration(
      name: connectingSceneSession.configuration.name,
      sessionRole: connectingSceneSession.role
    ).then {
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
