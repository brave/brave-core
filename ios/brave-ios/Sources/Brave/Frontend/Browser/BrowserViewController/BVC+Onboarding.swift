// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Preferences
import BraveUI
import Shared
import BraveCore
import UIKit
import Onboarding
import BraveShields
import BraveVPN
import StoreKit

// MARK: - Onboarding

extension BrowserViewController {

  func presentOnboardingIntro() {
    if Preferences.DebugFlag.skipOnboardingIntro == true { return }

    Preferences.AppState.isOnboardingActive.value = true
    presentOnboardingWelcomeScreen(on: self)
  }

  private func presentOnboardingWelcomeScreen(on parentController: UIViewController) {
    if Preferences.DebugFlag.skipOnboardingIntro == true { return }

    // 1. Existing user.
    // 2. User already completed onboarding.
    if Preferences.Onboarding.basicOnboardingCompleted.value == OnboardingState.completed.rawValue {
      Preferences.AppState.isOnboardingActive.value = false
      return
    }

    // 1. User is brand new
    // 2. User hasn't completed onboarding
    if Preferences.Onboarding.basicOnboardingCompleted.value != OnboardingState.completed.rawValue,
       Preferences.Onboarding.isNewRetentionUser.value == true {
      let onboardingController = WelcomeViewController(p3aUtilities: braveCore.p3aUtils, attributionManager: attributionManager)
      onboardingController.modalPresentationStyle = .fullScreen
      parentController.present(onboardingController, animated: false)
      isOnboardingOrFullScreenCalloutPresented = true
    }
  }

  private func addNTPTutorialPage() {
    let basicOnboardingNotCompleted =
      Preferences.Onboarding.basicOnboardingProgress.value != OnboardingProgress.newTabPage.rawValue
    
    if basicOnboardingNotCompleted, showNTPEducation().isEnabled, let url = showNTPEducation().url {
      tabManager.addTab(
        PrivilegedRequest(url: url) as URLRequest,
        afterTab: self.tabManager.selectedTab,
        isPrivate: privateBrowsingManager.isPrivateBrowsing)
    }
  }

  func showNTPOnboarding() {
    Preferences.AppState.isOnboardingActive.value = false
    iapObserver.savedPayment = nil
    
    if !topToolbar.inOverlayMode,
       topToolbar.currentURL == nil,
       Preferences.DebugFlag.skipNTPCallouts != true {
      
      if !Preferences.FullScreenCallout.omniboxCalloutCompleted.value,
          Preferences.Onboarding.isNewRetentionUser.value == true {
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
      from: topToolbar.locationView).insetBy(dx: -1.0, dy: -1.0)
    
    // Present the popover
    let controller = WelcomeOmniBoxOnboardingController()
    controller.setText(
      title: Strings.Onboarding.omniboxOnboardingPopOverTitle,
      details: Strings.Onboarding.omniboxOnboardingPopOverDescription)

    presentPopoverContent(
      using: controller,
      with: frame, cornerRadius: topToolbar.locationContainer.layer.cornerRadius,
      didDismiss: { [weak self] in
        guard let self = self else { return }

        Preferences.FullScreenCallout.omniboxCalloutCompleted.value = true
        Preferences.AppState.isOnboardingActive.value = false
        
        self.triggerPromotedInAppPurchase(savedPayment: self.iapObserver.savedPayment)
      },
      didClickBorderedArea: { [weak self] in
        guard let self = self else { return }
        
        Preferences.FullScreenCallout.omniboxCalloutCompleted.value = true
        Preferences.AppState.isOnboardingActive.value = false
        
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.25) {
          self.topToolbar.tabLocationViewDidTapLocation(self.topToolbar.locationView)
        }
      }
    )
  }
  
  private func triggerPromotedInAppPurchase(savedPayment: SKPayment?) {
    guard let productPayment = savedPayment else {
      return
    }
     
    navigationHelper.openVPNBuyScreen(iapObserver: iapObserver)
    BraveVPN.activatePaymentTypeForStoredPromotion(savedPayment: productPayment)
  }

  private func showPrivacyReportsOnboardingIfNeeded() {
    if Preferences.PrivacyReports.ntpOnboardingCompleted.value || privateBrowsingManager.isPrivateBrowsing {
      return
    }

    let trackerCountThresholdForOnboarding = AppConstants.buildChannel.isPublic ? 250 : 20
    let trackerAdsTotal = BraveGlobalShieldStats.shared.adblock + BraveGlobalShieldStats.shared.trackingProtection

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
      with: frame, cornerRadius: 12.0,
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
  
  private func presentPopoverContent(
    using contentController: UIViewController & PopoverContentComponent,
    with frame: CGRect,
    cornerRadius: CGFloat,
    didDismiss: @escaping () -> Void,
    didClickBorderedArea: @escaping () -> Void,
    didButtonClick: (() -> Void)? = nil) {
    let popover = PopoverController(
      contentController: contentController,
      contentSizeBehavior: .autoLayout(.phoneWidth))
    popover.arrowDistance = 10.0
      
    // Create a border / placeholder view
    let borderView = BorderView(frame: frame, cornerRadius: cornerRadius, colouredBorder: true)
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
      guard !maskFrame.isNull &&
            !maskFrame.isInfinite &&
            !maskFrame.isEmpty &&
            !popover.backgroundOverlayView.bounds.isNull &&
            !popover.backgroundOverlayView.bounds.isInfinite &&
            !popover.backgroundOverlayView.bounds.isEmpty else {
        return
      }

      guard maskFrame.origin.x.isFinite &&
            maskFrame.origin.y.isFinite &&
            maskFrame.size.width.isFinite &&
            maskFrame.size.height.isFinite &&
            maskFrame.size.width > 0 &&
            maskFrame.size.height > 0 else {
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
        
      popover?.dismissPopover() {
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
  
  func notifyTrackersBlocked(domain: String, displayTrackers: [AdBlockTrackerType], trackerCount: Int) {
    let controller = WelcomeBraveBlockedAdsController().then {
      $0.setData(displayTrackers: displayTrackers.map(\.rawValue), trackerCount: trackerCount)
    }

    let popover = PopoverController(contentController: controller)
    popover.previewForOrigin = .init(view: topToolbar.shieldsButton, action: { [weak self] popover in
      popover.dismissPopover() {
        self?.presentBraveShieldsViewController()
      }
    })
    popover.present(from: topToolbar.shieldsButton, on: self)

    popover.popoverDidDismiss = { [weak self] _ in
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
    return (Locale.current.regionCode == "JP", .brave.ntpTutorialPage)
  }

  func completeOnboarding(_ controller: UIViewController) {
    Preferences.Onboarding.basicOnboardingCompleted.value = OnboardingState.completed.rawValue
    Preferences.AppState.isOnboardingActive.value = false
    controller.dismiss(animated: true)
  }
}

// MARK: BorderView

private class BorderView: UIView {
  
  public var didClickBorderedArea: (() -> Void)?

  init(frame: CGRect, cornerRadius: CGFloat, colouredBorder: Bool = false) {
    let borderLayer = CAShapeLayer().then {
      let frame = frame.with { $0.origin = .zero }
      $0.strokeColor = colouredBorder ? UIColor.braveLighterBlurple.cgColor : UIColor.white.cgColor
      $0.fillColor = UIColor.clear.cgColor
      $0.lineWidth = 2.0
      $0.strokeEnd = 1.0
      $0.path = UIBezierPath(roundedRect: frame, cornerRadius: cornerRadius).cgPath
    }

    super.init(frame: frame)
    layer.addSublayer(borderLayer)

    addGestureRecognizer(UITapGestureRecognizer(target: self, action: #selector(onClickBorder(_:))))
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  @objc
  private func onClickBorder(_ tap: UITapGestureRecognizer) {
    didClickBorderedArea?()
  }
}
