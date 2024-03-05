// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Data
import Onboarding
import Preferences
import Shared
import UIKit

// MARK: - ProductNotification

extension BrowserViewController {

  /// The educational product notifications are presented
  /// when no onboarding or  full screen callout is presented
  /// There are 2 types one is Adblock and the other one DataSaved (JP Only)
  @objc func showEducationalNotifications() {
    if Preferences.DebugFlag.skipEduPopups == true { return }

    // Adding slight delay here for 2 reasons
    // First the content Blocker stats will be updated in current tab
    // after receiving notification from Global Stats
    // Second the popover notification will be shown after page loaded
    DispatchQueue.main.asyncAfter(deadline: .now() + 1) { [weak self] in
      guard let self = self else { return }

      var isAboutHomeUrl = false
      if let url = tabManager.selectedTab?.url, let internalURL = InternalURL(url) {
        isAboutHomeUrl = internalURL.isAboutHomeURL
      }

      // The conditions guarding presentation for educational notifications
      guard let selectedTab = tabManager.selectedTab,
        presentedViewController == nil,
        !isOnboardingOrFullScreenCalloutPresented,
        !Preferences.AppState.backgroundedCleanly.value,
        !topToolbar.inOverlayMode,
        !isTabTrayActive,
        selectedTab.webView?.scrollView.isDragging == false,
        isAboutHomeUrl == false
      else {
        return
      }

      self.presentOnboardingAdblockNotifications(selectedTab: selectedTab)
      self.presentEducationalProductNotifications(selectedTab: selectedTab)
    }
  }

  private func presentOnboardingAdblockNotifications(selectedTab: Tab) {
    guard !Preferences.General.onboardingAdblockPopoverShown.value,
      Preferences.Onboarding.isNewRetentionUser.value == true
    else {
      return
    }

    // If JP locale the ad block product notification should be shown
    // after 3 days period
    var showAdblockNotifications = true
    if Locale.current.regionCode == "JP",
      let appRetentionLaunchDate = Preferences.DAU.appRetentionLaunchDate.value
    {

      let showDate = appRetentionLaunchDate.addingTimeInterval(
        FullScreenCalloutManager.delayAmountJpOnboarding
      )

      // Delay period should pass before showing any educational product notification
      // This will be the case as long as new onboarding is active for JAPAN
      showAdblockNotifications = Date() > showDate
    }

    let blockedRequestURLs = selectedTab.contentBlocker.blockedRequests
    if showAdblockNotifications, !blockedRequestURLs.isEmpty, let url = selectedTab.url {

      let domain = url.baseDomain ?? url.host ?? url.schemelessAbsoluteString
      guard currentBenchmarkWebsite != domain else {
        return
      }

      currentBenchmarkWebsite = domain

      guard
        let trackersDetail = BlockedTrackerParser.parse(
          blockedRequestURLs: blockedRequestURLs,
          selectedTabURL: url
        )
      else {
        return
      }

      notifyTrackersBlocked(
        domain: domain,
        displayTrackers: trackersDetail.displayTrackers,
        trackerCount: trackersDetail.trackerCount
      )

      Preferences.General.onboardingAdblockPopoverShown.value = true
    }
  }

  private func notifyTrackersBlocked(
    domain: String,
    displayTrackers: [AdBlockTrackerType],
    trackerCount: Int
  ) {
    let controller = WelcomeBraveBlockedAdsController().then {
      $0.setData(displayTrackers: displayTrackers.map(\.rawValue), trackerCount: trackerCount)
    }

    let popover = PopoverController(contentController: controller)
    popover.previewForOrigin = .init(
      view: topToolbar.shieldsButton,
      action: { [weak self] popover in
        popover.dismissPopover {
          self?.presentBraveShieldsViewController()
        }
      }
    )
    popover.present(from: topToolbar.shieldsButton, on: self)
    adblockProductNotificationPresented = true

    popover.popoverDidDismiss = { [weak self] _ in
      DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
        guard let self = self else { return }

        if self.shouldShowPlaylistOnboardingThisSession {
          self.showPlaylistOnboarding(tab: self.tabManager.selectedTab)
        }
      }
    }
  }

  private func presentEducationalProductNotifications(selectedTab: Tab) {
    // Data Saved Pop-Over only exist in JP locale
    guard Locale.current.regionCode == "JP",
      !Preferences.ProductNotificationBenchmarks.showingSpecificDataSavedEnabled.value,
      let appRetentionLaunchDate = Preferences.DAU.appRetentionLaunchDate.value
    else {
      return
    }

    let rightNow = Date()
    let showDate = appRetentionLaunchDate.addingTimeInterval(
      FullScreenCalloutManager.delayAmountJpOnboarding
    )

    // 3 days period should pass before showing any educational product notification
    // This will be the case as long as new onboarding is active for JAPAN
    if rightNow > showDate {
      if !adblockProductNotificationPresented {
        guard let currentURL = selectedTab.url,
          DataSaved.get(with: currentURL.absoluteString) == nil,
          let domainFetchedSiteSavings = benchmarkBlockingDataSource?.fetchDomainFetchedSiteSavings(
            currentURL
          )
        else {
          return
        }

        notifyDomainSpecificDataSaved(domainFetchedSiteSavings)

        DataSaved.insert(
          savedUrl: currentURL.absoluteString,
          amount: domainFetchedSiteSavings
        )
        return
      }
    }
  }

  private func notifyDomainSpecificDataSaved(_ dataSaved: String) {
    let shareTrackersViewController = ShareTrackersController(
      trackingType: .domainSpecificDataSaved(dataSaved: dataSaved)
    )
    dismiss(animated: true)

    shareTrackersViewController.actionHandler = { [weak self] action in
      guard let self = self, action == .dontShowAgainTapped else { return }

      Preferences.ProductNotificationBenchmarks.showingSpecificDataSavedEnabled.value = true
      self.dismiss(animated: true)
    }

    showBenchmarkNotificationPopover(controller: shareTrackersViewController)
  }

  private func showBenchmarkNotificationPopover(
    controller: (UIViewController & PopoverContentComponent)
  ) {
    adblockProductNotificationPresented = true

    let popover = PopoverController(contentController: controller)
    popover.addsConvenientDismissalMargins = false
    popover.previewForOrigin = .init(view: topToolbar.shieldsButton)
    popover.present(from: topToolbar.shieldsButton, on: self)
  }
}
