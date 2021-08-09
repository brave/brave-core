// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import BraveShared
import Combine

class BraveRewards: NSObject {
    
    /// Whether or not Brave Rewards is available/can be enabled
    public static var isAvailable: Bool {
        #if MOZ_CHANNEL_DEBUG
        return true
        #else
        return DCDevice.current.isSupported
        #endif
    }
    
    private(set) var ads: BraveAds
    private(set) var ledger: BraveLedger?
    var ledgerServiceDidStart: ((BraveLedger) -> Void)?
    
    private let configuration: Configuration
    
    init(configuration: Configuration, buildChannel: BraveAdsBuildChannel?) {
        self.configuration = configuration
        
        BraveAds.isDebug = configuration.environment != .production
        BraveAds.environment = Int32(configuration.environment.rawValue)
        if let channel = buildChannel {
            BraveAds.buildChannel = channel
        }
        
        BraveLedger.isDebug = configuration.environment != .production
        BraveLedger.environment = configuration.environment
        BraveLedger.isTesting = configuration.isTesting
        BraveLedger.useShortRetries = configuration.useShortRetries
        BraveLedger.reconcileInterval = Int32(configuration.overridenNumberOfSecondsBetweenReconcile)
        
        ads = BraveAds(stateStoragePath: configuration.storageURL.appendingPathComponent("ads").path)
        
        super.init()
        
        braveNewsObservation = Preferences.BraveNews.isEnabled.$value
            .receive(on: DispatchQueue.main)
            .sink { [weak self] value in
                if !value {
                    self?.proposeAdsShutdown()
                }
            }
    }
    
    func startLedgerService(_ completion: (() -> Void)?) {
        if ledger != nil {
            // Already started
            completion?()
            return
        }
        let storagePath = configuration.storageURL.appendingPathComponent("ledger").path
        ledger = BraveLedger(stateStoragePath: storagePath)
        ledger?.initializeLedgerService { [weak self] in
            guard let self = self, let ledger = self.ledger else { return }
            if self.ads.isEnabled {
                if self.ads.isAdsServiceRunning() {
                    self.updateAdsWithWalletInfo()
                } else {
                    self.ads.initialize { success in
                        if success {
                            self.updateAdsWithWalletInfo()
                        }
                    }
                }
            }
            self.ledgerServiceDidStart?(ledger)
            completion?()
        }
    }
    
    private func updateAdsWithWalletInfo() {
        guard let ledger = ledger else { return }
        ledger.currentWalletInfo { wallet in
            guard let wallet = wallet else { return }
            let seed = wallet.recoverySeed.map(\.uint8Value)
            self.ads.updateWalletInfo(
                wallet.paymentId,
                base64Seed: Data(seed).base64EncodedString()
            )
        }
    }
    
    private var braveNewsObservation: AnyCancellable?
    
    private var shouldShutdownAds: Bool {
        ads.isAdsServiceRunning() && !ads.isEnabled && !Preferences.BraveNews.isEnabled.value
    }
    
    /// Propose that the ads service should be shutdown based on whether or not that all features
    /// that use it are disabled
    private func proposeAdsShutdown() {
        if !shouldShutdownAds { return }
        ads.shutdown {
            self.ads = BraveAds(stateStoragePath: self.configuration.storageURL.appendingPathComponent("ads").path)
        }
    }
    
    // MARK: - State
    
    /// Whether or not rewards is enabled
    @objc public var isEnabled: Bool {
        get {
            ads.isEnabled
        }
        set {
            willChangeValue(for: \.isEnabled)
            Preferences.Rewards.rewardsToggledOnce.value = true
            createWalletIfNeeded { [weak self] in
                guard let self = self else { return }
                self.ledger?.isAutoContributeEnabled = newValue
                self.ads.isEnabled = newValue
                if !newValue {
                    self.proposeAdsShutdown()
                } else {
                    if self.ads.isAdsServiceRunning() {
                        self.updateAdsWithWalletInfo()
                    } else {
                        self.ads.initialize { success in
                            if success {
                                self.updateAdsWithWalletInfo()
                            }
                        }
                    }
                }
                self.didChangeValue(for: \.isEnabled)
            }
        }
    }
    
    private(set) var isCreatingWallet: Bool = false
    private func createWalletIfNeeded(_ completion: (() -> Void)? = nil) {
        if isCreatingWallet {
            // completion block will be hit by previous call
            return
        }
        isCreatingWallet = true
        startLedgerService {
            guard let ledger = self.ledger else { return }
            ledger.createWalletAndFetchDetails { [weak self] success in
                self?.isCreatingWallet = false
                completion?()
            }
        }
    }
    
    func reset() {
        try? FileManager.default.removeItem(
            at: configuration.storageURL.appendingPathComponent("ledger")
        )
        if ads.isAdsServiceRunning(), !Preferences.BraveNews.isEnabled.value {
            ads.shutdown { [self] in
                try? FileManager.default.removeItem(
                    at: configuration.storageURL.appendingPathComponent("ads")
                )
                if ads.isEnabled {
                    ads.initialize { _ in }
                }
            }
        }
    }
    
    // MARK: - Reporting
    
    /// Report that a tab with a given id was updated
    func reportTabUpdated(
        _ tabId: Int,
        url: URL,
        faviconURL: URL?,
        isSelected: Bool,
        isPrivate: Bool
    ) {
        if isSelected {
            ledger?.selectedTabId = UInt32(tabId)
            tabRetrieved(tabId, url: url, faviconURL: faviconURL, html: nil)
        }
        ads.reportTabUpdated(tabId, url: url, isSelected: isSelected, isPrivate: isPrivate)
    }
    
    /// Report that a page has loaded in the current browser tab, and the HTML is available for analysis
    ///
    /// - note: Send nil for `adsInnerText` if the load happened due to tabs restoring
    ///         after app launch
    func reportLoadedPage(
        url: URL,
        redirectionURLs: [URL]?,
        faviconURL: URL?,
        tabId: Int,
        html: String,
        adsInnerText: String?
    ) {
        tabRetrieved(tabId, url: url, faviconURL: faviconURL, html: html)
        if let innerText = adsInnerText {
            ads.reportLoadedPage(
                with: url,
                redirectedFrom: redirectionURLs ?? [],
                html: html,
                innerText: innerText,
                tabId: tabId
            )
        }
        ledger?.reportLoadedPage(url: url, tabId: UInt32(tabId))
    }
    
    /// Report any XHR load happening in the page
    func reportXHRLoad(url: URL, tabId: Int, firstPartyURL: URL, referrerURL: URL?) {
        ledger?.reportXHRLoad(
            url,
            tabId: UInt32(tabId),
            firstPartyURL: firstPartyURL,
            referrerURL: referrerURL
        )
    }
    
    /// Report posting data to a form
    func reportPostData(_ data: Data, url: URL, tabId: Int, firstPartyURL: URL, referrerURL: URL?) {
        ledger?.reportPost(
            data,
            url: url,
            tabId: UInt32(tabId),
            firstPartyURL: firstPartyURL,
            referrerURL: referrerURL
        )
    }
    
    /// Report that media has started on a tab with a given id
    func reportMediaStarted(tabId: Int) {
        ads.reportMediaStarted(tabId: tabId)
    }
    
    /// Report that media has stopped on a tab with a given id
    func reportMediaStopped(tabId: Int) {
        ads.reportMediaStopped(tabId: tabId)
    }
    
    /// Report that a tab with a given id navigated to a new page in the same tab
    func reportTabNavigation(tabId: UInt32) {
        ledger?.reportTabNavigationOrClosed(tabId: tabId)
    }
    
    /// Report that a tab with a given id was closed by the user
    func reportTabClosed(tabId: Int) {
        ads.reportTabClosed(tabId: tabId)
        ledger?.reportTabNavigationOrClosed(tabId: UInt32(tabId))
    }
    
    private func tabRetrieved(_ tabId: Int, url: URL, faviconURL: URL?, html: String?) {
        ledger?.fetchPublisherActivity(from: url, faviconURL: faviconURL, publisherBlob: html, tabId: UInt64(tabId))
    }
}

extension BraveRewards {
    
    struct Configuration {
        var storageURL: URL
        var environment: BATEnvironment
        var adsBuildChannel: BraveAdsBuildChannel = .init()
        var isTesting: Bool = false
        var overridenNumberOfSecondsBetweenReconcile: Int = 0
        var useShortRetries: Bool = false
        
        static var `default`: Configuration {
            .init(
                storageURL: FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask).first!,
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
                isTesting: true,
                overridenNumberOfSecondsBetweenReconcile: 30,
                useShortRetries: true
            )
        }
    }
}
