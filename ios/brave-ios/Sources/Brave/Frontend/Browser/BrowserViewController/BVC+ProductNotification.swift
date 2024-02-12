// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Preferences
import BraveUI
import Shared
import Data
import UIKit
import Onboarding

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

      self.presentOnboardingAdblockNotifications()
      self.presentEducationalProductNotifications()
    }
  }

  private func presentOnboardingAdblockNotifications() {
    if Preferences.DebugFlag.skipEduPopups == true { return }

    var isAboutHomeUrl = false
    if let selectedTab = tabManager.selectedTab,
       let url = selectedTab.url,
       let internalURL = InternalURL(url) {
      isAboutHomeUrl = internalURL.isAboutHomeURL
    }

    guard let selectedTab = tabManager.selectedTab,
          !Preferences.General.onboardingAdblockPopoverShown.value,
          !benchmarkNotificationPresented,
          !Preferences.AppState.backgroundedCleanly.value,
          Preferences.Onboarding.isNewRetentionUser.value == true,
          !topToolbar.inOverlayMode,
          !isTabTrayActive,
          selectedTab.webView?.scrollView.isDragging == false,
          isAboutHomeUrl == false
    else {
      return
    }

    let blockedRequestURLs = selectedTab.contentBlocker.blockedRequests

    if !blockedRequestURLs.isEmpty, let url = selectedTab.url {
      
      let domain = url.baseDomain ?? url.host ?? url.schemelessAbsoluteString
      
      guard currentBenchmarkWebsite != domain else {
        return
      }
      
      currentBenchmarkWebsite = domain
      
      guard let trackersDetail = BlockedTrackerParser.parse(
        blockedRequestURLs: blockedRequestURLs,
        selectedTabURL: url) else {
        return
      }
      
      notifyTrackersBlocked(
        domain: domain,
        displayTrackers: trackersDetail.displayTrackers,
        trackerCount: trackersDetail.trackerCount)

      Preferences.General.onboardingAdblockPopoverShown.value = true
      
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
          presentedViewController == nil,
          !benchmarkNotificationPresented,
          !isOnboardingOrFullScreenCalloutPresented,
          !Preferences.AppState.backgroundedCleanly.value,
          !topToolbar.inOverlayMode,
          !isTabTrayActive,
          selectedTab.webView?.scrollView.isDragging == false,
          isAboutHomeUrl == false
    else {
      return
    }

    // Data Saved Pop-Over only exist in JP locale
    if Locale.current.regionCode == "JP" {
      if !benchmarkNotificationPresented,
         !Preferences.ProductNotificationBenchmarks.showingSpecificDataSavedEnabled.value {
        guard let currentURL = selectedTab.url,
              DataSaved.get(with: currentURL.absoluteString) == nil,
              let domainFetchedSiteSavings = benchmarkBlockingDataSource?.fetchDomainFetchedSiteSavings(currentURL)
        else {
          return
        }

        notifyDomainSpecificDataSaved(domainFetchedSiteSavings)

        DataSaved.insert(
          savedUrl: currentURL.absoluteString,
          amount: domainFetchedSiteSavings)
        return
      }
    }
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

    let popover = PopoverController(contentController: controller)
    popover.addsConvenientDismissalMargins = false
    popover.previewForOrigin = .init(view: topToolbar.shieldsButton)
    popover.present(from: topToolbar.shieldsButton, on: self)
  }
}
