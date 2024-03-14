// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShields
import BraveUI
import BraveVPN
import Onboarding
import Preferences
import Shared
import StoreKit
import SwiftUI
import UIKit

// MARK: - Onboarding

extension BrowserViewController {

  func presentOnboardingIntro() {
    if Preferences.DebugFlag.skipOnboardingIntro == true { return }

    // If locale is JP, start the new onboarding process
    // This will be the case as long as new onboarding is active for JAPAN
    if Locale.current.regionCode == "JP" {
      presentFocusOnboarding()
    } else {
      presentOnboardingWelcomeScreen()
    }
  }

  // MARK: Welcome Onboarding - Regions except JAPAN

  private func presentOnboardingWelcomeScreen() {
    // 1. Existing user.
    // 2. User already completed onboarding.
    if Preferences.Onboarding.basicOnboardingCompleted.value == OnboardingState.completed.rawValue {
      Preferences.AppState.shouldDeferPromotedPurchase.value = false
      return
    }

    // 1. User is brand new
    // 2. User hasn't completed onboarding
    if Preferences.Onboarding.basicOnboardingCompleted.value != OnboardingState.completed.rawValue,
      Preferences.Onboarding.isNewRetentionUser.value == true
    {
      let onboardingController = WelcomeViewController(
        p3aUtilities: braveCore.p3aUtils,
        attributionManager: attributionManager
      )
      onboardingController.modalPresentationStyle = .fullScreen
      present(onboardingController, animated: false)
      isOnboardingOrFullScreenCalloutPresented = true
    }
  }

  func showNTPOnboarding() {
    Preferences.AppState.shouldDeferPromotedPurchase.value = false
    iapObserver.savedPayment = nil

    if !topToolbar.inOverlayMode,
      topToolbar.currentURL == nil,
      Preferences.DebugFlag.skipNTPCallouts != true
    {

      if !Preferences.FullScreenCallout.omniboxCalloutCompleted.value,
        Preferences.Onboarding.isNewRetentionUser.value == true
      {
        presentOmniBoxOnboarding()
        addNTPTutorialPage()
      }

      if !Preferences.FullScreenCallout.ntpCalloutCompleted.value {
        showPrivacyReportsOnboardingIfNeeded()
      }
    }
  }

  private func presentOmniBoxOnboarding() {
    // If a controller is already presented (such as menu), do not show onboarding
    guard presentedViewController == nil else {
      return
    }

    let frame = view.convert(
      topToolbar.locationView.frame,
      from: topToolbar.locationView
    ).insetBy(dx: -1.0, dy: -1.0)

    // Present the popover
    let controller = WelcomeOmniBoxOnboardingController()
    controller.setText(
      title: Strings.Onboarding.omniboxOnboardingPopOverTitle,
      details: Strings.Onboarding.omniboxOnboardingPopOverDescription
    )

    presentPopoverContent(
      using: controller,
      with: frame,
      cornerRadius: topToolbar.locationContainer.layer.cornerRadius,
      didDismiss: { [weak self] in
        guard let self = self else { return }

        Preferences.FullScreenCallout.omniboxCalloutCompleted.value = true
        Preferences.AppState.shouldDeferPromotedPurchase.value = false

        self.triggerPromotedInAppPurchase(savedPayment: self.iapObserver.savedPayment)
      },
      didClickBorderedArea: { [weak self] in
        guard let self = self else { return }

        Preferences.FullScreenCallout.omniboxCalloutCompleted.value = true
        Preferences.AppState.shouldDeferPromotedPurchase.value = false

        DispatchQueue.main.asyncAfter(deadline: .now() + 0.25) {
          self.topToolbar.tabLocationViewDidTapLocation(self.topToolbar.locationView)
        }
      }
    )
  }

  private func addNTPTutorialPage() {
    let basicOnboardingNotCompleted =
      Preferences.Onboarding.basicOnboardingProgress.value != OnboardingProgress.newTabPage.rawValue

    // NTP Education screen should load after onboarding is finished and user is on locale JP
    let (educationPermitted, url) = (Locale.current.regionCode == "JP", URL.brave.ntpTutorialPage)

    if basicOnboardingNotCompleted, educationPermitted {
      tabManager.addTab(
        PrivilegedRequest(url: url) as URLRequest,
        afterTab: self.tabManager.selectedTab,
        isPrivate: privateBrowsingManager.isPrivateBrowsing
      )
    }
  }

  private func triggerPromotedInAppPurchase(savedPayment: SKPayment?) {
    guard let productPayment = savedPayment else {
      return
    }

    navigationHelper.openVPNBuyScreen(iapObserver: iapObserver)
    BraveVPN.activatePaymentTypeForStoredPromotion(savedPayment: productPayment)
  }

  private func showPrivacyReportsOnboardingIfNeeded() {
    if Preferences.PrivacyReports.ntpOnboardingCompleted.value
      || privateBrowsingManager.isPrivateBrowsing
    {
      return
    }

    let trackerCountThresholdForOnboarding = AppConstants.isOfficialBuild ? 250 : 20
    let trackerAdsTotal =
      BraveGlobalShieldStats.shared.adblock + BraveGlobalShieldStats.shared.trackingProtection

    if trackerAdsTotal < trackerCountThresholdForOnboarding {
      return
    }

    // If a controller is already presented (such as menu), do not show onboarding
    // It also includes the case for overlay mode and tabtray opened
    guard presentedViewController == nil, !topToolbar.inOverlayMode, !isTabTrayActive else {
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

    // Present the popover
    let controller = WelcomeNTPOnboardingController()
    controller.setText(details: Strings.Onboarding.ntpOnboardingPopOverTrackerDescription)

    controller.buttonText = Strings.PrivacyHub.onboardingButtonTitle

    topToolbar.isURLBarEnabled = false

    presentPopoverContent(
      using: controller,
      with: frame,
      cornerRadius: 12.0,
      didDismiss: { [weak self] in
        self?.topToolbar.isURLBarEnabled = true
        Preferences.PrivacyReports.ntpOnboardingCompleted.value = true
      },
      didClickBorderedArea: { [weak self] in
        self?.topToolbar.isURLBarEnabled = true
        Preferences.PrivacyReports.ntpOnboardingCompleted.value = true
      },
      didButtonClick: { [weak self] in
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.25) {
          self?.topToolbar.isURLBarEnabled = true
          self?.openPrivacyReport()
        }
      }
    )
  }

  func completeOnboarding(_ controller: UIViewController) {
    Preferences.Onboarding.basicOnboardingCompleted.value = OnboardingState.completed.rawValue
    Preferences.AppState.shouldDeferPromotedPurchase.value = false
    controller.dismiss(animated: true)
  }

  private func presentPopoverContent(
    using contentController: UIViewController & PopoverContentComponent,
    with frame: CGRect,
    cornerRadius: CGFloat,
    didDismiss: @escaping () -> Void,
    didClickBorderedArea: @escaping () -> Void,
    didButtonClick: (() -> Void)? = nil
  ) {
    let popover = PopoverController(
      contentController: contentController,
      contentSizeBehavior: .autoLayout(.phoneWidth)
    )
    popover.arrowDistance = 10.0

    // Create a border / placeholder view
    let borderView = NotificationBorderView(
      frame: frame,
      cornerRadius: cornerRadius,
      colouredBorder: true
    )
    let placeholderView = UIView(frame: frame).then {
      $0.alpha = 0.0
      $0.frame = frame
    }

    view.addSubview(placeholderView)
    popover.view.insertSubview(borderView, aboveSubview: popover.view)

    let maskShape = CAShapeLayer().then {
      $0.fillRule = .evenOdd
      $0.fillColor = UIColor.white.cgColor
      $0.strokeColor = UIColor.clear.cgColor
    }

    popover.present(from: placeholderView, on: self) { [weak popover, weak self] in
      guard let popover = popover, let self = self else { return }

      // Mask the shadow
      let maskFrame = self.view.convert(frame, to: popover.backgroundOverlayView)
      guard
        !maskFrame.isNull && !maskFrame.isInfinite && !maskFrame.isEmpty
          && !popover.backgroundOverlayView.bounds.isNull
          && !popover.backgroundOverlayView.bounds.isInfinite
          && !popover.backgroundOverlayView.bounds.isEmpty
      else {
        return
      }

      guard
        maskFrame.origin.x.isFinite && maskFrame.origin.y.isFinite && maskFrame.size.width.isFinite
          && maskFrame.size.height.isFinite && maskFrame.size.width > 0 && maskFrame.size.height > 0
      else {
        return
      }
    }

    popover.backgroundOverlayView.layer.mask = maskShape

    popover.popoverDidDismiss = { _ in
      maskShape.removeFromSuperlayer()
      borderView.removeFromSuperview()

      didDismiss()
    }

    borderView.didClickBorderedArea = { [weak popover] in
      maskShape.removeFromSuperlayer()
      borderView.removeFromSuperview()

      popover?.dismissPopover {
        didClickBorderedArea()
      }
    }

    if let controller = contentController as? WelcomeNTPOnboardingController {
      controller.buttonTapped = {
        maskShape.removeFromSuperlayer()
        borderView.removeFromSuperview()
        didButtonClick?()
      }
    }

    DispatchQueue.main.async {
      maskShape.path = {
        let path = CGMutablePath()
        path.addRect(popover.backgroundOverlayView.bounds)
        return path
      }()
    }
  }
}

extension BrowserViewController {

  // MARK: Day 0 Focus Onboarding - JAPAN Region

  func presentFocusOnboarding() {

    // Check user has never seen onboarding - new user
    guard Preferences.Onboarding.basicOnboardingCompleted.value == OnboardingState.unseen.rawValue
    else {
      Preferences.AppState.shouldDeferPromotedPurchase.value = false
      return
    }

    let welcomeView = WelcomeFocusView()
    let onboardingController = UIHostingController(rootView: welcomeView).then {
      $0.modalPresentationStyle = .fullScreen
    }

    present(onboardingController, animated: false)
  }
}
