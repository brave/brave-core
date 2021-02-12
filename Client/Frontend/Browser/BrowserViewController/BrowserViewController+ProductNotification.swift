// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveShared
import BraveUI
import Shared

// MARK: - ProductNotification

extension BrowserViewController {
    
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
        guard let selectedTab = tabManager.selectedTab,
              !benchmarkNotificationPresented,
              !topToolbar.inOverlayMode else {
            return
        }
        
        let contentBlockerStats = selectedTab.contentBlocker.stats
        
        // Step 1: First Time Block Notification
        if !Preferences.ProductNotificationBenchmarks.firstTimeBlockingShown.value,
           contentBlockerStats.total > 0 {
            
            notifyFirstTimeBlock(theme: Theme.of(selectedTab))
            Preferences.ProductNotificationBenchmarks.firstTimeBlockingShown.value = true
            
            return
        }
        
        // Step 2: Load a video on a streaming site
        if !Preferences.ProductNotificationBenchmarks.videoAdBlockShown.value,
           selectedTab.url?.isVideoSteamingSiteURL == true {

            notifyVideoAdsBlocked(theme: Theme.of(selectedTab))
            Preferences.ProductNotificationBenchmarks.videoAdBlockShown.value = true

            return
        }
        
        // Step 3: Pre-determined # of Trackers and Ads Blocked
        if !Preferences.ProductNotificationBenchmarks.privacyProtectionBlockShown.value,
           contentBlockerStats.total > benchmarkNumberOfTrackers {
            
            notifyPrivacyProtectBlock(theme: Theme.of(selectedTab))
            Preferences.ProductNotificationBenchmarks.privacyProtectionBlockShown.value = true

            return
        }
        
        // Step 4: Https Upgrade
        if !Preferences.ProductNotificationBenchmarks.httpsUpgradeShown.value,
           contentBlockerStats.httpsCount > 0 {

            notifyHttpsUpgrade(theme: Theme.of(selectedTab))
            Preferences.ProductNotificationBenchmarks.httpsUpgradeShown.value = true

            return
        }
    }
    
    private func notifyFirstTimeBlock(theme: Theme) {
        let shareTrackersViewController = ShareTrackersController(theme: theme, trackingType: .trackerAdWarning)
        
        shareTrackersViewController.actionHandler = { [weak self] action in
            guard let self = self else { return }
            
            switch action {
                case .takeALookTapped:
                    self.showShieldsScreen()
                default:
                    break
            }
        }
        
        showBenchmarkNotificationPopover(controller: shareTrackersViewController)
    }
    
    private func notifyVideoAdsBlocked(theme: Theme) {
        let shareTrackersViewController = ShareTrackersController(theme: theme, trackingType: .videoAdBlock)
        
        dismiss(animated: true)
        showBenchmarkNotificationPopover(controller: shareTrackersViewController)
    }
    
    private func notifyPrivacyProtectBlock(theme: Theme) {
        let shareTrackersViewController = ShareTrackersController(theme: theme, trackingType: .trackerAdCountBlock(count: benchmarkNumberOfTrackers))
        dismiss(animated: true)
        showBenchmarkNotificationPopover(controller: shareTrackersViewController)
    }
    
    private func notifyHttpsUpgrade(theme: Theme) {
        let shareTrackersViewController = ShareTrackersController(theme: theme, trackingType: .encryptedConnectionWarning)
        dismiss(animated: true)
        showBenchmarkNotificationPopover(controller: shareTrackersViewController)
    }
    
    private func showBenchmarkNotificationPopover(controller: (UIViewController & PopoverContentComponent)) {
        benchmarkNotificationPresented = true

        let popover = PopoverController(contentController: controller, contentSizeBehavior: .autoLayout)
        popover.addsConvenientDismissalMargins = false
        popover.present(from: self.topToolbar.locationView.shieldsButton, on: self)
    }
    
    // MARK: Actions
    
    func showShieldsScreen() {
        dismiss(animated: true) {
            self.presentBraveShieldsViewController()
        }
    }
}
