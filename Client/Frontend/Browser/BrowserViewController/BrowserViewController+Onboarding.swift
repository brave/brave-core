// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveShared
import BraveUI
import Shared
import BraveCore

// MARK: - Onboarding

extension BrowserViewController {

  func presentOnboardingIntro() {
    if Preferences.DebugFlag.skipOnboardingIntro == true { return }

    presentOnboardingWelcomeScreen(on: self)
  }

  func presentOnboardingWelcomeScreen(on parentController: UIViewController) {
    if Preferences.DebugFlag.skipOnboardingIntro == true { return }

    // 1. Existing user.
    // 2. User already completed onboarding.
    if Preferences.General.basicOnboardingCompleted.value == OnboardingState.completed.rawValue {
      return
    }

    // 1. User is brand new
    // 2. User hasn't completed onboarding
    if Preferences.General.basicOnboardingCompleted.value != OnboardingState.completed.rawValue,
       Preferences.General.isNewRetentionUser.value == true {
      let onboardingController = WelcomeViewController(
        profile: profile,
        rewards: rewards)
      onboardingController.modalPresentationStyle = .fullScreen
      onboardingController.onAdsWebsiteSelected = { [weak self] url in
        guard let self = self else { return }

        if let url = url {
          self.topToolbar.leaveOverlayMode()
          self.addNTPTutorialPage()

          let tab = self.tabManager.addTab(
            PrivilegedRequest(url: url) as URLRequest,
            afterTab: self.tabManager.selectedTab,
            isPrivate: PrivateBrowsingManager.shared.isPrivateBrowsing)
          self.tabManager.selectTab(tab)
        } else {
          self.addNTPTutorialPage()
          
          DispatchQueue.main.asyncAfter(deadline: .now() + 0.25) {
            self.topToolbar.tabLocationViewDidTapLocation(self.topToolbar.locationView)
          }
        }
      }
      onboardingController.onSkipSelected = { [weak self] in
        guard let self = self else { return }
        self.addNTPTutorialPage()
      }

      parentController.present(onboardingController, animated: false)
      isOnboardingOrFullScreenCalloutPresented = true
    }
  }

  private func addNTPTutorialPage() {
    if showNTPEducation().isEnabled, let url = showNTPEducation().url {
      tabManager.addTab(
        PrivilegedRequest(url: url) as URLRequest,
        afterTab: self.tabManager.selectedTab,
        isPrivate: PrivateBrowsingManager.shared.isPrivateBrowsing)
    }
  }

  func showNTPOnboarding() {
    if !topToolbar.inOverlayMode,
       topToolbar.currentURL == nil,
       Preferences.DebugFlag.skipNTPCallouts != true {
      showPrivacyReportsOnboardingIfNeeded()
    }
  }

  func showPrivacyReportsOnboardingIfNeeded() {
    if Preferences.PrivacyReports.ntpOnboardingCompleted.value || PrivateBrowsingManager.shared.isPrivateBrowsing {
      return
    }

    let trackerCountThresholdForOnboarding = AppConstants.buildChannel.isPublic ? 250 : 20
    let trackerAdsTotal = BraveGlobalShieldStats.shared.adblock + BraveGlobalShieldStats.shared.trackingProtection

    if trackerAdsTotal < trackerCountThresholdForOnboarding {
      return
    }

    // If a controller is already presented (such as menu), do not show onboarding
    guard presentedViewController == nil else {
      return
    }

    // We can only show this onboarding on the NTP
    guard let ntpController = tabManager.selectedTab?.newTabPageViewController,
          let statsFrame = ntpController.ntpStatsOnboardingFrame
    else {
      return
    }

    // Project the statsFrame to the current frame
    let frame = view.convert(statsFrame, from: ntpController.view)

    // Create a border view
    let borderView = UIView().then {
      let borderLayer = CAShapeLayer().then {
        let frame = frame.with { $0.origin = .zero }
        $0.strokeColor = UIColor.white.cgColor
        $0.fillColor = UIColor.clear.cgColor
        $0.lineWidth = 2.0
        $0.strokeEnd = 1.0
        $0.path = UIBezierPath(roundedRect: frame, cornerRadius: 12.0).cgPath
      }
      $0.layer.addSublayer(borderLayer)
    }

    view.addSubview(borderView)
    borderView.frame = frame

    // Present the popover and mark onboarding as complete
    Preferences.PrivacyReports.ntpOnboardingCompleted.value = true

    let controller = WelcomeNTPOnboardingController()
    controller.setText(details: Strings.Onboarding.ntpOnboardingPopOverTrackerDescription)
    controller.buttonText = Strings.PrivacyHub.onboardingButtonTitle

    let popover = PopoverController(contentController: controller)
    popover.arrowDistance = 10.0
    popover.present(from: borderView, on: self) { [weak popover, weak self] in
      guard let popover = popover,
            let self = self
      else { return }

      // Mask the shadow
      let maskFrame = self.view.convert(frame, to: popover.backgroundOverlayView)
      guard !maskFrame.isNull && !maskFrame.isInfinite && !maskFrame.isEmpty && !popover.backgroundOverlayView.bounds.isNull && !popover.backgroundOverlayView.bounds.isInfinite && !popover.backgroundOverlayView.bounds.isEmpty else {
        return
      }

      guard maskFrame.origin.x.isFinite && maskFrame.origin.y.isFinite && maskFrame.size.width.isFinite && maskFrame.size.height.isFinite && maskFrame.size.width > 0 && maskFrame.size.height > 0 else {
        return
      }

      let maskShape = CAShapeLayer().then {
        $0.fillRule = .evenOdd
        $0.fillColor = UIColor.white.cgColor
        $0.strokeColor = UIColor.clear.cgColor
      }

      popover.backgroundOverlayView.layer.mask = maskShape
      popover.popoverDidDismiss = { _ in
        maskShape.removeFromSuperlayer()
        borderView.removeFromSuperview()
      }

      controller.buttonTapped = { [weak self] in
        maskShape.removeFromSuperlayer()
        borderView.removeFromSuperview()
        DispatchQueue.main.async {
          self?.openPrivacyReport()
        }
      }

      DispatchQueue.main.async {
        maskShape.path = {
          let path = CGMutablePath()
          path.addRect(popover.backgroundOverlayView.bounds)
          path.addRoundedRect(
            in: maskFrame,
            cornerWidth: 12.0,
            cornerHeight: 12.0)
          return path
        }()
      }
    }
  }

  func notifyTrackersBlocked(domain: String, trackerName: String, remainingTrackersCount: Int) {
    let controller = WelcomeBraveBlockedAdsController().then {
      $0.setData(domain: domain, trackerBlocked: trackerName, trackerCount: remainingTrackersCount)
    }

    let popover = PopoverController(contentController: controller)
    popover.present(from: topToolbar.locationView.shieldsButton, on: self)

    let pulseAnimationView = RadialPulsingAnimation(ringCount: 3)
    pulseAnimationView.present(
      icon: topToolbar.locationView.shieldsButton.imageView?.image,
      from: topToolbar.locationView.shieldsButton,
      on: popover,
      browser: self)

    pulseAnimationView.animationViewPressed = { [weak self] in
      popover.dismissPopover() {
        self?.presentBraveShieldsViewController()
      }
    }

    popover.popoverDidDismiss = { [weak self] _ in
      pulseAnimationView.removeFromSuperview()

      DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
        guard let self = self else { return }

        if self.shouldShowPlaylistOnboardingThisSession {
          self.showPlaylistOnboarding(tab: self.tabManager.selectedTab)
        }
      }
    }
  }

  /// New Tab Page Education screen should load after onboarding is finished and user is on locale JP
  /// - Returns: A tuple which shows NTP Education is enabled and URL to be loaded
  func showNTPEducation() -> (isEnabled: Bool, url: URL?) {
    guard let url = BraveUX.ntpTutorialPageURL else {
      return (false, nil)
    }

    return (Locale.current.regionCode == "JP", url)
  }

  func completeOnboarding(_ controller: UIViewController) {
    Preferences.General.basicOnboardingCompleted.value = OnboardingState.completed.rawValue
    controller.dismiss(animated: true)
  }
}
