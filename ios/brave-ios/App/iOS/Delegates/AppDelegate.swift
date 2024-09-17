// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import AIChat
import AVFoundation
import Brave
import BraveCore
import BraveNews
import BraveShared
import BraveShields
import BraveStore
import BraveVPN
import BraveWallet
import Combine
import CoreSpotlight
import Data
import Growth
import LocalAuthentication
import MessageUI
import Onboarding
import Playlist
import Preferences
import PrivateCDN
import RuntimeWarnings
import SDWebImage
@_spi(AppLaunch) import Shared
import Storage
import StoreKit
import UserAgent
import UserNotifications
import os

#if canImport(BraveTalk)
import BraveTalk
#endif

@main
class AppDelegate: UIResponder, UIApplicationDelegate {
  private let log = Logger(subsystem: Bundle.main.bundleIdentifier!, category: "app-delegate")

  var window: UIWindow?
  private weak var application: UIApplication?
  let appVersion = Bundle.main.infoDictionaryString(forKey: "CFBundleShortVersionString")
  var receivedURLs: [URL]?

  private var cancellables: Set<AnyCancellable> = []

  @discardableResult
  func application(
    _ application: UIApplication,
    willFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?
  ) -> Bool {
    // Hold references to willFinishLaunching parameters for delayed app launch
    self.application = application

    // Application Constants must be initialized first
    #if BRAVE_CHANNEL_RELEASE
    AppConstants.setBuildChannel(.release)
    #elseif BRAVE_CHANNEL_BETA
    AppConstants.setBuildChannel(.beta)
    #elseif BRAVE_CHANNEL_NIGHTLY
    AppConstants.setBuildChannel(.nightly)
    #elseif BRAVE_CHANNEL_DEBUG
    AppConstants.setBuildChannel(.debug)
    #endif

    #if OFFICIAL_BUILD
    AppConstants.setOfficialBuild(true)
    #endif

    AppState.shared.state = .launching(options: launchOptions ?? [:], active: false)

    // Set the Safari UA for browsing.
    setUserAgent()

    // Fetching details of GRDRegion for Automatic Region selection
    BraveVPN.fetchLastUsedRegionDetail()

    // Start the keyboard helper to monitor and cache keyboard state.
    KeyboardHelper.defaultHelper.startObserving()
    DynamicFontHelper.defaultHelper.startObserving()
    ReaderModeFonts.registerCustomFonts()
    FocusOnboardingFonts.registerCustomFonts()

    MenuHelper.defaultHelper.setItems()

    SDImageCodersManager.shared.addCoder(PrivateCDNImageCoder())

    // Temporary fix for Bug 1390871 - NSInvalidArgumentException: -[WKContentView menuHelperFindInPage]: unrecognized selector
    if let clazz = NSClassFromString("WKCont" + "ent" + "View"),
      let swizzledMethod = class_getInstanceMethod(
        TabWebViewMenuHelper.self,
        #selector(TabWebViewMenuHelper.swizzledMenuHelperFindInPage)
      )
    {
      class_addMethod(
        clazz,
        MenuHelper.selectorFindInPage,
        method_getImplementation(swizzledMethod),
        method_getTypeEncoding(swizzledMethod)
      )
    }

    if Preferences.BraveNews.isEnabled.value && !Preferences.BraveNews.userOptedIn.value {
      // Opt-out any user that has not explicitly opted-in
      Preferences.BraveNews.isEnabled.value = false
      // User now has to explicitly opt-in
      Preferences.BraveNews.isShowingOptIn.value = true
    }

    // If the user's language was checked but not included in the News supported languages list check it again
    // each launch since updates could add support for a new language. If a user previously opted in to News
    // however then we shouldn't show the opt-in card again.
    let shouldPerformLanguageCheck =
      !Preferences.BraveNews.languageChecked.value
      || Preferences.BraveNews.languageWasUnavailableDuringCheck.value == true
    let isNewsEnabledOrPreviouslyOptedIn =
      Preferences.BraveNews.isEnabled.value || Preferences.BraveNews.userOptedIn.value
    if shouldPerformLanguageCheck, !isNewsEnabledOrPreviouslyOptedIn,
      let languageCode = Locale.preferredLanguages.first?.prefix(2)
    {
      Preferences.BraveNews.languageChecked.value = true
      let languageShouldShowOptIn =
        FeedDataSource.supportedLanguages.contains(String(languageCode))
        || FeedDataSource.knownSupportedLocales.contains(Locale.current.identifier)
      Preferences.BraveNews.languageWasUnavailableDuringCheck.value = !languageShouldShowOptIn
      Preferences.BraveNews.isShowingOptIn.value = languageShouldShowOptIn
    }

    // Clean Logger for Secure content state
    DebugLogger.cleanLogger(for: .secureState)

    // Clean Logger for URP (no longer used)
    DebugLogger.cleanUrpLogs()

    return true
  }

  func application(
    _ application: UIApplication,
    didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?
  ) -> Bool {
    AppState.shared.state = .launching(options: launchOptions ?? [:], active: true)

    // Run migrations that need access to Data
    Migration.postDataLoadMigration()

    // IAPs can trigger on the app as soon as it launches,
    // for example when a previous transaction was not finished and is in pending state.
    SKPaymentQueue.default().add(BraveVPN.iapObserver)
    // Editing Product Promotion List
    Task { @MainActor in
      await BraveVPN.updateStorePromotionOrder()
      await BraveVPN.hideActiveStorePromotion()
    }

    // Brave Store SDK - Initialization
    BraveStoreSDK.shared.refreshAllSkusOrders()

    // Override point for customization after application launch.
    var shouldPerformAdditionalDelegateHandling = true
    AdblockEngine.setDomainResolver()

    UIView.applyAppearanceDefaults()

    if Preferences.Rewards.isUsingBAP.value == nil {
      Preferences.Rewards.isUsingBAP.value = Locale.current.region?.identifier == "JP"
    }

    // If a shortcut was launched, display its information and take the appropriate action
    if let shortcutItem = launchOptions?[UIApplication.LaunchOptionsKey.shortcutItem]
      as? UIApplicationShortcutItem
    {

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

    Preferences.AppState.dailyUserPingAwaitingUserConsent.value = isFirstLaunch

    Preferences.AppState.shouldDeferPromotedPurchase.value = isFirstLaunch

    if Preferences.Onboarding.basicOnboardingCompleted.value
      == OnboardingState.undetermined.rawValue
    {
      Preferences.Onboarding.basicOnboardingCompleted.value =
        isFirstLaunch ? OnboardingState.unseen.rawValue : OnboardingState.completed.rawValue
    }

    // Check if user has launched the application before and determine if it is a new retention user
    if Preferences.General.isFirstLaunch.value,
      Preferences.Onboarding.isNewRetentionUser.value == nil
    {
      Preferences.Onboarding.isNewRetentionUser.value = true
    }

    if Preferences.DAU.appRetentionLaunchDate.value == nil {
      Preferences.DAU.appRetentionLaunchDate.value = Date()
    }

    // Starting Date for Brave Search Promotion to mark 15 days period
    if Preferences.BraveSearch.braveSearchPromotionLaunchDate.value == nil {
      Preferences.BraveSearch.braveSearchPromotionLaunchDate.value = Date()
    }

    // After user pressed 'maybe later' in promotion, the banner will not be shown user in same session
    if Preferences.BraveSearch.braveSearchPromotionCompletionState.value
      == BraveSearchPromotionState.maybeLaterSameSession.rawValue
    {
      Preferences.BraveSearch.braveSearchPromotionCompletionState.value =
        BraveSearchPromotionState.maybeLaterUpcomingSession.rawValue
    }

    if isFirstLaunch {
      Preferences.PrivacyReports.ntpOnboardingCompleted.value = false
    }

    Preferences.General.isFirstLaunch.value = false

    Task {
      await AppState.shared.profile.searchEngines.loadSearchEngines()
      // Search engine setup must be checked outside of 'firstLaunch' loop because of #2770.
      // There was a bug that when you skipped onboarding, default search engine preference
      // was not set.
      if Preferences.Search.defaultEngineName.value == nil {
        AppState.shared.profile.searchEngines.searchEngineSetup()
      }
    }

    if isFirstLaunch {
      Preferences.DAU.installationDate.value = Date()

      // VPN credentials are kept in keychain and persist between app reinstalls.
      // To avoid unexpected problems we clear all vpn keychain items.
      // New set of keychain items will be created on purchase or iap restoration.
      BraveVPN.clearCredentials()

      // Always load YouTube in Brave for new users
      Preferences.General.keepYouTubeInBrave.value = true

      // Enable Search Suggestions for BraveSearch default countries
      Preferences.Search.showSuggestions.value =
        AppState.shared.profile.searchEngines.isBraveSearchDefaultRegion

      Preferences.Search.shouldShowSuggestionsOptIn.value =
        !AppState.shared.profile.searchEngines.isBraveSearchDefaultRegion
    }

    if Preferences.URP.referralLookupOutstanding.value == nil {
      // This preference has never been set, and this means it is a new or upgraded user.
      // That distinction must be made to know if a network request for ref-code look up should be made.

      // Setting this to an explicit value so it will never get overwritten on subsequent launches.
      // Upgrade users should not have ref code ping happening.
      Preferences.URP.referralLookupOutstanding.value = isFirstLaunch
    }

    SceneDelegate.shouldHandleUrpLookup = true
    SceneDelegate.shouldHandleInstallAttributionFetch = true

    #if canImport(BraveTalk)
    BraveTalkJitsiCoordinator.sendAppLifetimeEvent(
      .didFinishLaunching(options: launchOptions ?? [:])
    )
    #endif

    // DAU may not have pinged on the first launch so weekOfInstallation pref may not be set yet
    if let weekOfInstall = Preferences.DAU.weekOfInstallation.value
      ?? Preferences.DAU.installationDate.value?.mondayOfCurrentWeekFormatted,
      AppConstants.isOfficialBuild
    {
      AppState.shared.braveCore.initializeP3AService(
        forChannel: AppConstants.buildChannel.serverChannelParam,
        weekOfInstall: weekOfInstall
      )
    }

    LanguageMetrics.recordPrimaryLanguageP3A()

    Task(priority: .low) {
      await self.cleanUpLargeTemporaryDirectory()
    }

    Task(priority: .high) {
      // Start preparing the ad-block services right away
      // So it's ready a lot faster
      await LaunchHelper.shared.prepareAdBlockServices(
        adBlockService: AppState.shared.braveCore.adblockService
      )
    }

    // Setup Playlist
    // This restores the playlist incomplete downloads. So if a download was started
    // and interrupted on application death, we restart it on next launch.
    Task(priority: .low) { @MainActor in
      PlaylistManager.shared.setupPlaylistFolder()
      PlaylistManager.shared.restoreSession()
    }

    return shouldPerformAdditionalDelegateHandling
  }

  #if canImport(BraveTalk)
  func application(
    _ application: UIApplication,
    continue userActivity: NSUserActivity,
    restorationHandler: @escaping ([UIUserActivityRestoring]?) -> Void
  ) -> Bool {
    return BraveTalkJitsiCoordinator.sendAppLifetimeEvent(
      .continueUserActivity(userActivity, restorationHandler: restorationHandler)
    )
  }

  func application(
    _ app: UIApplication,
    open url: URL,
    options: [UIApplication.OpenURLOptionsKey: Any] = [:]
  ) -> Bool {
    return BraveTalkJitsiCoordinator.sendAppLifetimeEvent(.openURL(url, options: options))
  }
  #endif

  func applicationWillTerminate(_ application: UIApplication) {
    AppState.shared.profile.shutdown()

    SKPaymentQueue.default().remove(BraveVPN.iapObserver)

    // Clean up BraveCore
    AppState.shared.braveCore.syncAPI.removeAllObservers()

    log.debug("Cleanly Terminated the Application")
  }

  func application(
    _ application: UIApplication,
    supportedInterfaceOrientationsFor window: UIWindow?
  ) -> UIInterfaceOrientationMask {
    if let presentedViewController = window?.rootViewController?.presentedViewController {
      return presentedViewController.supportedInterfaceOrientations
    } else {
      return window?.rootViewController?.supportedInterfaceOrientations ?? .portraitUpsideDown
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
  }

  /// Dumps the temporary directory if the total size of the directory exceeds a size threshold (in bytes)
  private nonisolated func cleanUpLargeTemporaryDirectory(thresholdInBytes: Int = 100_000_000) async
  {
    let fileManager = FileManager.default
    let tmp = fileManager.temporaryDirectory
    guard
      let enumerator = fileManager.enumerator(
        at: tmp,
        includingPropertiesForKeys: [
          .isRegularFileKey, .totalFileAllocatedSizeKey, .totalFileSizeKey,
        ]
      )
    else { return }
    var totalSize: Int = 0
    while let fileURL = enumerator.nextObject() as? URL {
      guard
        let values = try? fileURL.resourceValues(forKeys: [
          .isRegularFileKey, .totalFileAllocatedSizeKey, .totalFileSizeKey,
        ]),
        let isRegularFile = values.isRegularFile,
        isRegularFile
      else {
        continue
      }
      totalSize += values.totalFileAllocatedSize ?? values.totalFileSize ?? 0
      if totalSize > thresholdInBytes {
        // Drop the tmp directory entirely and re-create it after
        do {
          try fileManager.removeItem(at: tmp)
          try fileManager.createDirectory(at: tmp, withIntermediateDirectories: false)
        } catch {
          log.warning(
            "Failed to delete & re-create tmp directory which exceeds size limit: \(error.localizedDescription)"
          )
        }
        return
      }
    }
  }
}

extension AppDelegate: MFMailComposeViewControllerDelegate {
  func mailComposeController(
    _ controller: MFMailComposeViewController,
    didFinishWith result: MFMailComposeResult,
    error: Error?
  ) {
    // Dismiss the view controller and start the app up
    controller.dismiss(animated: true, completion: nil)
  }
}

extension AppDelegate {
  // MARK: UISceneSession Lifecycle

  func application(
    _ application: UIApplication,
    configurationForConnecting connectingSceneSession: UISceneSession,
    options: UIScene.ConnectionOptions
  ) -> UISceneConfiguration {
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

  func application(
    _ application: UIApplication,
    didDiscardSceneSessions sceneSessions: Set<UISceneSession>
  ) {
    // Do not discard sessions on iPhones!
    // There is a bug in the OS where this will be called randomly
    // especially after launching and closing the app hundreds of times
    // while also locking the device.
    if !UIApplication.shared.supportsMultipleScenes {
      return
    }

    // Called when the user discards a scene session.
    // If any sessions were discarded while the application was not running, this will be called shortly after application:didFinishLaunchingWithOptions.
    // Use this method to release any resources that were specific to the discarded scenes, as they will not return.

    sceneSessions.forEach { session in
      if let windowIdString = BrowserState.getWindowInfo(from: session).windowId,
        let windowId = UUID(uuidString: windowIdString)
      {
        SessionWindow.delete(windowId: windowId)
      } else if let userActivity = session.scene?.userActivity,
        let windowIdString = BrowserState.getWindowInfo(from: userActivity).windowId,
        let windowId = UUID(uuidString: windowIdString)
      {
        SessionWindow.delete(windowId: windowId)
      }
    }
  }
}
