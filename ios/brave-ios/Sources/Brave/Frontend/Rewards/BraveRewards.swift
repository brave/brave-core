// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Combine
import DeviceCheck
import Foundation
import Growth
import Preferences
import Shared

public class BraveRewards: PreferencesObserver {

  /// Whether or not Brave Rewards is available/can be enabled
  public static var isAvailable: Bool {
    #if DEBUG
    return true
    #else
    return DCDevice.current.isSupported
    #endif
  }

  private(set) var ads: BraveAds
  private(set) var rewardsAPI: BraveRewardsAPI?
  var rewardsServiceDidStart: ((BraveRewardsAPI) -> Void)?

  private let configuration: Configuration

  public init(configuration: Configuration) {
    self.configuration = configuration

    ads = BraveAds(stateStoragePath: configuration.storageURL.appendingPathComponent("ads").path)

    braveNewsObservation = Preferences.BraveNews.isEnabled.$value
      .receive(on: DispatchQueue.main)
      .sink { [weak self] value in
        if !value {
          self?.proposeAdsShutdown()
        }
      }

    if Preferences.Rewards.adsEnabledTimestamp.value == nil, ads.isEnabled {
      Preferences.Rewards.adsEnabledTimestamp.value = Date()
    }

    ads.notifyBraveNewsIsEnabledPreferenceDidChange(Preferences.BraveNews.isEnabled.value)
    Preferences.BraveNews.isEnabled.observe(from: self)
  }

  public func preferencesDidChange(for key: String) {
    ads.notifyBraveNewsIsEnabledPreferenceDidChange(Preferences.BraveNews.isEnabled.value)
  }

  func startRewardsService(_ completion: (() -> Void)?) {
    if rewardsAPI != nil {
      // Already started
      completion?()
      return
    }
    let storagePath = configuration.storageURL.appendingPathComponent("ledger").path
    rewardsAPI = BraveRewardsAPI(stateStoragePath: storagePath)
    rewardsAPI?.initializeRewardsService { [weak self] in
      guard let self = self, let rewardsAPI = self.rewardsAPI else { return }
      if self.ads.isEnabled {
        self.fetchWalletAndInitializeAds()
      }
      self.rewardsServiceDidStart?(rewardsAPI)
      completion?()
    }
  }

  private func fetchWalletAndInitializeAds(toggleAds: Bool? = nil) {
    guard let rewardsAPI = rewardsAPI else { return }
    rewardsAPI.currentWalletInfo { wallet in
      var walletInfo: BraveAds.WalletInfo?
      if let wallet {
        let seed = wallet.recoverySeed.map(\.uint8Value)
        walletInfo = .init(
          paymentId: wallet.paymentId,
          recoverySeedBase64: Data(seed).base64EncodedString()
        )
      }
      // If ads is already initialized just toggle rewards ads and update the wallet info
      if self.ads.isServiceRunning() {
        if let walletInfo {
          self.ads.notifyRewardsWalletDidUpdate(
            walletInfo.paymentId,
            base64Seed: walletInfo.recoverySeedBase64
          )
        }
        if let toggleAds {
          self.ads.isEnabled = toggleAds
        }
        self.isTurningOnRewards = false
        return
      }
      self.ads.initialize(walletInfo: walletInfo) { success in
        if success, let toggleAds {
          self.ads.isEnabled = toggleAds
        }
        self.isTurningOnRewards = false
      }
    }
  }

  private var braveNewsObservation: AnyCancellable?

  private var shouldShutdownAds: Bool {
    ads.isServiceRunning() && !shouldStartAds
  }

  /// Propose that the ads service should be shutdown based on whether or not that all features
  /// that use it are disabled
  private func proposeAdsShutdown() {
    if !shouldShutdownAds { return }
    ads.shutdownService {
      self.ads = BraveAds(
        stateStoragePath: self.configuration.storageURL.appendingPathComponent("ads").path
      )
    }
  }

  // MARK: - State

  /// Whether or not rewards is enabled
  @objc public var isEnabled: Bool {
    get {
      ads.isEnabled
    }
    set {
      createWalletIfNeeded { [weak self] in
        guard let self = self else { return }
        Preferences.Rewards.rewardsToggledOnce.value = true
        let wasEnabled = self.ads.isEnabled
        if !wasEnabled && newValue {
          Preferences.Rewards.adsEnabledTimestamp.value = Date()
        } else if wasEnabled && !newValue {
          Preferences.Rewards.adsDisabledTimestamp.value = Date()
        }
        if !newValue {
          self.ads.isEnabled = newValue
          self.proposeAdsShutdown()
          self.isTurningOnRewards = false
        } else {
          self.fetchWalletAndInitializeAds(toggleAds: true)
        }
      }
    }
  }

  public var shouldStartAds: Bool {
    // Start Brave Ads if one of the following is true:
    // - Brave Rewards is enabled.
    // - Brave News is enabled.
    // - `ShouldAlwaysRunBraveAdsService` feature is enabled.
    return ads.isEnabled || Preferences.BraveNews.isEnabled.value
      || BraveAds.shouldAlwaysRunService()
  }

  private(set) var isTurningOnRewards: Bool = false

  private func createWalletIfNeeded(_ completion: (() -> Void)? = nil) {
    if isTurningOnRewards {
      // completion block will be hit by previous call
      return
    }
    isTurningOnRewards = true
    startRewardsService {
      guard let rewardsAPI = self.rewardsAPI else { return }
      rewardsAPI.createWalletAndFetchDetails { success in
        completion?()
      }
    }
  }

  func reset() async {
    try? await AsyncFileManager.default.removeItem(
      at: configuration.storageURL.appendingPathComponent("ledger")
    )
    if ads.isServiceRunning(), !Preferences.BraveNews.isEnabled.value {
      await withCheckedContinuation { continuation in
        ads.shutdownService {
          continuation.resume()
        }
      }
      try? await AsyncFileManager.default.removeItem(
        at: configuration.storageURL.appendingPathComponent("ads")
      )
      if ads.isEnabled {
        await withCheckedContinuation { continuation in
          ads.initialize { _ in
            continuation.resume()
          }
        }
      }
    }
  }

  /// Notifies Brave Ads that the given tab did change
  func maybeNotifyTabDidChange(
    tab: Tab,
    isSelected: Bool
  ) {
    guard !tab.redirectChain.isEmpty, !tab.isPrivate, ads.isServiceRunning() else {
      // Don't notify `DidChange` for tabs that haven't finished loading, private tabs,
      // or when the ads service is not running.
      return
    }

    ads.notifyTabDidChange(
      Int(tab.rewardsId),
      redirectChain: tab.redirectChain,
      isNewNavigation: tab.rewardsReportingState.isNewNavigation,
      isRestoring: tab.rewardsReportingState.wasRestored,
      isSelected: isSelected
    )
  }

  /// Notifies Brave Ads that the given tab did load
  func maybeNotifyTabDidLoad(tab: Tab) {
    guard !tab.redirectChain.isEmpty, !tab.isPrivate, ads.isServiceRunning() else {
      // Don't notify `DidLoad` for tabs that haven't finished loading, private tabs,
      // or when the ads service is not running.
      return
    }

    ads.notifyTabDidLoad(
      Int(tab.rewardsId),
      httpStatusCode: tab.rewardsReportingState.httpStatusCode
    )
  }

  // MARK: - Brave Ads Data

  /// Clear Brave Ads Data.
  @MainActor func clearAdsData() async {
    await withCheckedContinuation { continuation in
      ads.clearData {
        continuation.resume()
      }
    }
  }

  // MARK: - Reporting

  /// Report that a tab with a given id was updated
  func reportTabUpdated(
    tab: Tab,
    isSelected: Bool,
    isPrivate: Bool
  ) {
    guard let url = tab.redirectChain.last else {
      // Don't report update for tabs that haven't finished loading.
      return
    }

    let tabId = Int(tab.rewardsId)
    if isSelected {
      rewardsAPI?.selectedTabId = UInt32(tabId)
      tabRetrieved(tabId, url: url, html: nil)
    }
  }

  /// Report that a page has loaded in the current browser tab, and the
  /// text/HTML content is available for analysis.
  func reportLoadedPage(
    tab: Tab,
    htmlContent: String?,
    textContent: String?
  ) {
    guard let url = tab.redirectChain.last else {
      // Don't report update for tabs that haven't finished loading.
      return
    }

    let tabId = Int(tab.rewardsId)

    tabRetrieved(tabId, url: url, html: htmlContent)

    // Don't notify about content changes if the ads service is not available, the
    // tab was restored, was a previously committed navigation, or an error page was displayed.
    if ads.isServiceRunning() {
      let kHttpClientErrorResponseStatusCodeClass = 4
      let kHttpServerErrorResponseStatusCodeClass = 5
      let responseStatusCodeClass = tab.rewardsReportingState.httpStatusCode / 100

      if !tab.rewardsReportingState.wasRestored
        && tab.rewardsReportingState.isNewNavigation
        && responseStatusCodeClass != kHttpClientErrorResponseStatusCodeClass
        && responseStatusCodeClass != kHttpServerErrorResponseStatusCodeClass
      {
        // HTML is not required because verifiable conversions are only supported
        // for Brave Rewards users. However, we must notify that the tab content has
        // changed with empty HTML to ensure that regular conversions are processed.
        ads.notifyTabHtmlContentDidChange(
          tabId,
          redirectChain: tab.redirectChain,
          html: htmlContent ?? ""
        )
        if let textContent {
          ads.notifyTabTextContentDidChange(
            tabId,
            redirectChain: tab.redirectChain,
            text: textContent
          )
        }
      }
    }
    rewardsAPI?.reportLoadedPage(url: url, tabId: UInt32(tabId))
  }

  /// Report any XHR load happening in the page
  func reportXHRLoad(url: URL, tabId: Int, firstPartyURL: URL, referrerURL: URL?) {
    rewardsAPI?.reportXHRLoad(
      url,
      tabId: UInt32(tabId),
      firstPartyURL: firstPartyURL,
      referrerURL: referrerURL
    )
  }

  /// Report that media has started on a tab with a given id
  func reportMediaStarted(tabId: Int) {
    if !ads.isServiceRunning() { return }
    ads.notifyTabDidStartPlayingMedia(tabId)
  }

  /// Report that media has stopped on a tab with a given id
  func reportMediaStopped(tabId: Int) {
    if !ads.isServiceRunning() { return }
    ads.notifyTabDidStopPlayingMedia(tabId)
  }

  /// Report that a tab with a given id navigated to a new page in the same tab
  func reportTabNavigation(tabId: UInt32) {
    rewardsAPI?.reportTabNavigationOrClosed(tabId: tabId)
  }

  /// Report that a tab with a given id was closed by the user
  func reportTabClosed(tabId: Int) {
    if ads.isServiceRunning() {
      ads.notifyDidCloseTab(tabId)
    }
    rewardsAPI?.reportTabNavigationOrClosed(tabId: UInt32(tabId))
  }

  private func tabRetrieved(_ tabId: Int, url: URL, html: String?) {
    rewardsAPI?.fetchPublisherActivity(
      from: url,
      faviconURL: nil,
      publisherBlob: html,
      tabId: UInt64(tabId)
    )
  }
}

extension BraveRewards {

  public struct Configuration {
    public enum Environment {
      case development
      case production
      case staging
    }

    public var storageURL: URL
    public var environment: Environment
    public var adsBuildChannel: BraveAds.BuildChannelInfo = .init()
    public var isDebug: Bool?
    public var overridenNumberOfSecondsBetweenReconcile: Int?
    public var retryInterval: Int?

    public static func current(
      buildChannel: AppBuildChannel = AppConstants.buildChannel,
      isDebugFlag: Bool? = Preferences.Rewards.debugFlagIsDebug.value,
      retryInterval: Int? = Preferences.Rewards.debugFlagRetryInterval.value,
      reconcileInterval: Int? = Preferences.Rewards.debugFlagReconcileInterval.value
    ) -> Self {
      var configuration: BraveRewards.Configuration = .production
      if let override = Preferences.Rewards.EnvironmentOverride(
        rawValue: Preferences.Rewards.environmentOverride.value
      ), override != .none {
        switch override {
        case .dev:
          configuration = .default
        case .staging:
          configuration = .staging
        case .prod:
          configuration = .production
        default:
          configuration = .staging
        }
      } else {
        configuration = AppConstants.isOfficialBuild ? .production : .staging
      }
      configuration.isDebug = isDebugFlag
      configuration.retryInterval = retryInterval
      configuration.overridenNumberOfSecondsBetweenReconcile = reconcileInterval
      return configuration
    }

    static var `default`: Configuration {
      .init(
        storageURL: FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)
          .first!,
        environment: .development
      )
    }
    static var staging: Configuration {
      var config = Configuration.default
      config.environment = .staging
      return config
    }
    static var production: Configuration {
      var config = Configuration.default
      config.environment = .production
      return config
    }
    static var testing: Configuration {
      .init(
        storageURL: URL(fileURLWithPath: NSTemporaryDirectory()),
        environment: .development,
        overridenNumberOfSecondsBetweenReconcile: 30,
        retryInterval: 30
      )
    }

    public var flags: String {
      var flags: [String: String] = [:]
      switch environment {
      case .production:
        flags["staging"] = "false"
      case .staging:
        flags["staging"] = "true"
      case .development:
        flags["development"] = "true"
      }
      if let debug = isDebug {
        flags["debug"] = String(debug)
      }
      if let reconcileInterval = overridenNumberOfSecondsBetweenReconcile {
        flags["reconcile-interval"] = "\(reconcileInterval)"
      }
      if let retryInterval = retryInterval {
        flags["retry-interval"] = "\(retryInterval)"
      }
      return flags.map({ "\($0.key)=\($0.value)" }).joined(separator: ",")
    }
  }
}
