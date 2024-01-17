// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import Data
import Shared
import BraveShared
import Preferences
import BraveUI
import Onboarding
import Storage
import WebKit
import os.log
import Favicon

// TODO: Move this log to the Rewards/Ads target once we move Rewards/Ads files.
public let adsRewardsLog = Logger(subsystem: Bundle.main.bundleIdentifier!, category: "ads-rewards")

extension BrowserViewController {
  func updateRewardsButtonState() {
    if !isViewLoaded { return }
    if !BraveRewards.isAvailable {
      self.topToolbar.rewardsButton.isHidden = true
      return
    }
    self.topToolbar.rewardsButton.isHidden = Preferences.Rewards.hideRewardsIcon.value || privateBrowsingManager.isPrivateBrowsing
    self.topToolbar.rewardsButton.iconState = rewards.isEnabled || rewards.isTurningOnRewards ? .enabled : (Preferences.Rewards.rewardsToggledOnce.value ? .disabled : .initial)
  }

  func showBraveRewardsPanel() {
    if !Preferences.FullScreenCallout.rewardsCalloutCompleted.value,
      Preferences.Onboarding.isNewRetentionUser.value == true,
      !Preferences.Rewards.rewardsToggledOnce.value {

      let controller = OnboardingRewardsAgreementViewController()
      controller.onOnboardingStateChanged = { [weak self] controller, state in
        self?.completeOnboarding(controller)
      }
      controller.onRewardsStatusChanged = { [weak self] status in
        self?.rewards.isEnabled = status
      }

      Preferences.FullScreenCallout.rewardsCalloutCompleted.value = true
      present(controller, animated: true)
      topToolbar.rewardsButton.iconState = Preferences.Rewards.rewardsToggledOnce.value ? (rewards.isEnabled || rewards.isTurningOnRewards ? .enabled : .disabled) : .initial
      return
    }

    updateRewardsButtonState()

    guard let tab = tabManager.selectedTab else { return }
    
    if #available(iOS 16.0, *) {
      // System components sit on top so we want to dismiss it
      tab.webView?.findInteraction?.dismissFindNavigator()
    }
    
    let braveRewardsPanel = BraveRewardsViewController(
      tab: tab,
      rewards: rewards
    )
    braveRewardsPanel.actionHandler = { [weak self] action in
      switch action {
      case .unverifiedPublisherLearnMoreTapped:
        self?.loadNewTabWithRewardsURL(.brave.rewardsUnverifiedPublisherLearnMoreURL)
      }
    }

    let popover = PopoverController(contentController: braveRewardsPanel)
    popover.addsConvenientDismissalMargins = false
    popover.present(from: topToolbar.rewardsButton, on: self)
    popover.popoverDidDismiss = { [weak self] _ in
      guard let self = self else { return }
      if let tabId = self.tabManager.selectedTab?.rewardsId, self.rewards.rewardsAPI?.selectedTabId == 0 {
        // Show the tab currently visible
        self.rewards.rewardsAPI?.selectedTabId = tabId
      }
    }
    // Hide the current tab
    rewards.rewardsAPI?.selectedTabId = 0
  }

  func claimPendingPromotions() {
    guard
      let rewardsAPI = rewards.rewardsAPI,
      case let promotions = rewardsAPI.pendingPromotions.filter({ $0.status == .active }),
      !promotions.isEmpty else {
      return
    }
    Task {
      for promo in promotions {
        let success = await rewardsAPI.claimPromotion(promo)
        adsRewardsLog.info("[BraveRewards] Auto-Claim Promotion - \(success) for \(promo.approximateValue)")
      }
    }
  }

  @objc func resetNTPNotification() {
    Preferences.NewTabPage.brandedImageShowed.value = false
    Preferences.NewTabPage.atleastOneNTPNotificationWasShowed.value = false
  }

  private func loadNewTabWithRewardsURL(_ url: URL) {
    self.presentedViewController?.dismiss(animated: true)

    if let tab = tabManager.getTabForURL(url, isPrivate: privateBrowsingManager.isPrivateBrowsing) {
      tabManager.selectTab(tab)
    } else {
      let request = URLRequest(url: url)
      let isPrivate = privateBrowsingManager.isPrivateBrowsing
      tabManager.addTabAndSelect(request, isPrivate: isPrivate)
    }
  }

  /// Removes any scheduled or delivered ad grant reminders which may have been added prior to
  /// removal of those reminders.
  public func removeScheduledAdGrantReminders() {
    let idPrefix = "rewards.notification.monthly-claim"
    let center = UNUserNotificationCenter.current()
    center.getPendingNotificationRequests { requests in
      let ids =
        requests
        .filter { $0.identifier.hasPrefix(idPrefix) }
        .map(\.identifier)
      if ids.isEmpty { return }
      center.removeDeliveredNotifications(withIdentifiers: ids)
      center.removePendingNotificationRequests(withIdentifiers: ids)
    }
  }

  func setupLedger() {
    guard let rewardsAPI = rewards.rewardsAPI else { return }
    // Update defaults
    rewardsAPI.setMinimumVisitDuration(8)
    rewardsAPI.setMinimumNumberOfVisits(1)
    rewardsAPI.setContributionAmount(Double.greatestFiniteMagnitude)

    // Create rewards observer
    let rewardsObserver = RewardsObserver(rewardsAPI: rewardsAPI)
    rewardsAPI.add(rewardsObserver)
    self.rewardsObserver = rewardsObserver

    rewardsObserver.walletInitalized = { [weak self] result in
      guard let self = self, let client = self.deviceCheckClient else { return }
      if result == .ok, !DeviceCheckClient.isDeviceEnrolled() {
        rewardsAPI.setupDeviceCheckEnrollment(client) {}
        self.updateRewardsButtonState()
      }
    }
    rewardsObserver.promotionsAdded = { [weak self] promotions in
      self?.claimPendingPromotions()
    }
    rewardsObserver.fetchedPanelPublisher = { [weak self] publisher, tabId in
      DispatchQueue.main.async {
        guard let self = self, self.isViewLoaded, let tab = self.tabManager.selectedTab, tab.rewardsId == tabId else { return }
        self.publisher = publisher
      }
    }

    promotionFetchTimer = Timer.scheduledTimer(
      withTimeInterval: 1.hours,
      repeats: true,
      block: { [weak self, weak rewardsAPI] _ in
        guard let self = self, let rewardsAPI = rewardsAPI else { return }
        if self.rewards.isEnabled {
          rewardsAPI.fetchPromotions(nil)
        }
      }
    )
  }
}

extension Tab {
  func reportPageLoad(to rewards: BraveRewards, redirectionURLs urls: [URL]) {
    guard let webView = webView, let url = webView.url else { return }
    if url.isLocal || self.isPrivate { return }

    var htmlBlob: String?
    var classifierText: String?

    let group = DispatchGroup()
    group.enter()

    webView.evaluateSafeJavaScript(functionName: "new XMLSerializer().serializeToString", args: ["document"], contentWorld: WKContentWorld.defaultClient, escapeArgs: false) { html, _ in
      htmlBlob = html as? String
      group.leave()
    }

    if shouldClassifyLoadsForAds {
      group.enter()
      webView.evaluateSafeJavaScript(functionName: "document?.body?.innerText", contentWorld: .defaultClient, asFunction: false) { text, _ in
        classifierText = text as? String
        group.leave()
      }
    }

    group.notify(queue: .main) {
      if self.displayFavicon == nil {
        adsRewardsLog.warning("No favicon found in \(self) to report to rewards panel")
      }
      rewards.reportLoadedPage(
        url: url, redirectionURLs: urls.isEmpty ? [url] : urls,
        tabId: Int(self.rewardsId),
        html: htmlBlob ?? "", adsInnerText: classifierText)
    }
  }

  func reportPageNavigation(to rewards: BraveRewards) {
    rewards.reportTabNavigation(tabId: self.rewardsId)
  }
}

extension BrowserViewController: BraveAdsCaptchaHandler {
  public func handleAdaptiveCaptcha(forPaymentId paymentId: String, captchaId: String) {
    if Preferences.Rewards.adaptiveCaptchaFailureCount.value >= 10 {
      adsRewardsLog.info("Skipping adaptive captcha request as failure count exceeds threshold.")
      return
    }
    Task {
      do {
        try await rewards.rewardsAPI?.solveAdaptiveCaptcha(paymentId: paymentId, captchaId: captchaId)
      } catch {
        // Increase failure count, stop attempting attestation altogether passed a specific count
        Preferences.Rewards.adaptiveCaptchaFailureCount.value += 1
        adsRewardsLog.error("Failed to solve adaptive captcha: \(error.localizedDescription)")
      }
    }
  }
}
