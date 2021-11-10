// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveShared
import BraveUI
import Shared
import Data

// MARK: - ProductNotification

extension BrowserViewController {
    
    // MARK: BenchmarkTrackerCountTier
        
    enum BenchmarkTrackerCountTier: Int, Equatable, CaseIterable {
        case specialTier = 1000
        case newbieExclusiveTier = 5000
        case casualExclusiveTier = 10_000
        case regularExclusiveTier = 25_000
        case expertExclusiveTier = 75_000
        case professionalTier = 100_000
        case primeTier = 250_000
        case grandTier = 500_000
        case legendaryTier = 1_000_000

        var title: String {
            switch self {
                case .specialTier:
                    return Strings.ShieldEducation.benchmarkSpecialTierTitle
                case .newbieExclusiveTier, .casualExclusiveTier, .regularExclusiveTier, .expertExclusiveTier:
                    return Strings.ShieldEducation.benchmarkExclusiveTierTitle
                case .professionalTier:
                    return Strings.ShieldEducation.benchmarkProfessionalTierTitle
                case .primeTier:
                    return Strings.ShieldEducation.benchmarkPrimeTierTitle
                case .grandTier:
                    return Strings.ShieldEducation.benchmarkGrandTierTitle
                case .legendaryTier:
                    return Strings.ShieldEducation.benchmarkLegendaryTierTitle
            }
        }

        var nextTier: BenchmarkTrackerCountTier? {
            guard let indexOfSelf = Self.allCases.firstIndex(where: { self == $0 }) else {
                return nil
            }

            return Self.allCases[safe: indexOfSelf + 1]
        }
        
        var value: Int {
            AppConstants.buildChannel.isPublic ? self.rawValue : self.rawValue / 100
        }
    }
    
    // MARK: Internal
    
    @objc func updateShieldNotifications() {
        // Adding slight delay here for 2 reasons
        // First the content Blocker stats will be updated in current tab
        // after receiving notification from Global Stats
        // Second the popover notification will be shown after page loaded
        DispatchQueue.main.asyncAfter(deadline: .now() + 1) { [weak self] in
            guard let self = self else { return }
            
            self.presentEducationalProductNotifications()
        }
    }
    
    private func presentEducationalProductNotifications() {
        if Preferences.DebugFlag.skipEduPopups == true { return }
        
        var isAboutHomeUrl = false
        if let selectedTab = tabManager.selectedTab,
           let url = selectedTab.url,
           let internalURL = InternalURL(url) {
            isAboutHomeUrl = internalURL.isAboutHomeURL
        }
        
        guard let selectedTab = tabManager.selectedTab,
              !benchmarkNotificationPresented,
              !Preferences.AppState.backgroundedCleanly.value,
              !topToolbar.inOverlayMode,
              !isTabTrayActive,
              selectedTab.webView?.scrollView.isDragging == false,
              isAboutHomeUrl == false else {
            return
        }

        // Step 1: Load a video on a streaming site
        if !Preferences.ProductNotificationBenchmarks.videoAdBlockShown.value,
           selectedTab.url?.isVideoSteamingSiteURL == true {

            notifyVideoAdsBlocked()
            Preferences.ProductNotificationBenchmarks.videoAdBlockShown.value = true

            return
        }
        
        // Step 2: Share Brave Benchmark Tiers
        let numOfTrackerAds = BraveGlobalShieldStats.shared.adblock + BraveGlobalShieldStats.shared.trackingProtection
        if numOfTrackerAds > benchmarkCurrentSessionAdCount + 20 {
            let existingTierList = BenchmarkTrackerCountTier.allCases.filter {
                Preferences.ProductNotificationBenchmarks.trackerTierCount.value < $0.value}
            
            if !existingTierList.isEmpty {
                guard let firstExistingTier = existingTierList.first else { return }
                
                Preferences.ProductNotificationBenchmarks.trackerTierCount.value = numOfTrackerAds
                
                if numOfTrackerAds > firstExistingTier.value {
                    notifyTrackerAdsCount(firstExistingTier.value, description: firstExistingTier.title)
                }
            }
        }
        
        // Step 3: Domain Specific Data Saved
        // Data Saved Pop-Over only exist in JP locale
        if Locale.current.regionCode == "JP" {
            if !benchmarkNotificationPresented,
               !Preferences.ProductNotificationBenchmarks.showingSpecificDataSavedEnabled.value {
                guard let currentURL = selectedTab.url,
                      DataSaved.get(with: currentURL.absoluteString) == nil,
                      let domainFetchedSiteSavings = benchmarkBlockingDataSource?.fetchDomainFetchedSiteSavings(currentURL) else {
                    return
                }
                
                notifyDomainSpecificDataSaved(domainFetchedSiteSavings)
                
                DataSaved.insert(savedUrl: currentURL.absoluteString,
                                 amount: domainFetchedSiteSavings)
                return
            }
        }
    }
    
    private func notifyVideoAdsBlocked() {
        let shareTrackersViewController = ShareTrackersController(trackingType: .videoAdBlock)
        
        dismiss(animated: true)
        showBenchmarkNotificationPopover(controller: shareTrackersViewController)
    }
    
    private func notifyTrackerAdsCount(_ count: Int, description: String) {
        let shareTrackersViewController = ShareTrackersController(trackingType: .trackerCountShare(count: count, description: description))
        dismiss(animated: true)

        shareTrackersViewController.actionHandler = { [weak self] action in
            guard let self = self, action == .shareTheNewsTapped else { return }

            self.showShareScreen()
        }

        showBenchmarkNotificationPopover(controller: shareTrackersViewController)
    }
    
    private func notifyDomainSpecificDataSaved(_ dataSaved: String) {
        let shareTrackersViewController = ShareTrackersController(trackingType: .domainSpecificDataSaved(dataSaved: dataSaved))
        dismiss(animated: true)

        shareTrackersViewController.actionHandler = { [weak self] action in
            guard let self = self, action == .dontShowAgainTapped else { return }

            Preferences.ProductNotificationBenchmarks.showingSpecificDataSavedEnabled.value = true
            self.dismiss(animated: true)
        }

        showBenchmarkNotificationPopover(controller: shareTrackersViewController)
    }
    
    private func showBenchmarkNotificationPopover(controller: (UIViewController & PopoverContentComponent)) {
        benchmarkNotificationPresented = true

        let popover = PopoverController(contentController: controller, contentSizeBehavior: .autoLayout)
        popover.addsConvenientDismissalMargins = false
        popover.present(from: self.topToolbar.locationView.shieldsButton, on: self)
    }
    
    // MARK: Actions
    
    func showShareScreen() {
        dismiss(animated: true) {
            let globalShieldsActivityController =
                ShieldsActivityItemSourceProvider.shared.setupGlobalShieldsActivityController()
            globalShieldsActivityController.popoverPresentationController?.sourceView = self.view
    
            self.present(globalShieldsActivityController, animated: true, completion: nil)
        }
    }
}
