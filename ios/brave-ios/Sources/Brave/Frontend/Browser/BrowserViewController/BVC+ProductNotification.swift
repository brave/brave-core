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

      self.presentEducationalProductNotifications(selectedTab: selectedTab)
    }
  }

  private func presentEducationalProductNotifications(selectedTab: Tab) {
    // Data Saved Pop-Over only exist in JP locale
    guard Locale.current.region?.identifier == "JP",
      !Preferences.ProductNotificationBenchmarks.showingSpecificDataSavedEnabled.value
    else {
      return
    }

    // Delay period 3 days should pass before showing any educational product notification
    // This will be the case as long as new onboarding is active for JAPAN
    var showAdblockNotifications = true
    if let appRetentionLaunchDate = Preferences.DAU.appRetentionLaunchDate.value {
      let showDate = appRetentionLaunchDate.addingTimeInterval(
        FullScreenCalloutManager.delayAmountJpOnboarding
      )

      showAdblockNotifications = Date() > showDate
    }

    if showAdblockNotifications {
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
