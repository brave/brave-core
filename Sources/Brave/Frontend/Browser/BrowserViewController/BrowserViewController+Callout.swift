// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveShared
import Preferences
import BraveCore
import BraveUI
import Shared
import SwiftKeychainWrapper
import SwiftUI
import BraveVPN
import Onboarding
import SafariServices

// MARK: - Callouts

extension BrowserViewController {
  
  // Priority: P3A - Bottom Bar - VPN - Default Browser - Rewards - Cookie Notification - Link Receipt
  func presentFullScreenCallouts() {
    for type in FullScreenCalloutType.allCases {
      presentScreenCallout(for: type)
    }
  }
  
  private func presentScreenCallout(for type: FullScreenCalloutType) {
    switch type {
    case .p3a:
      presentP3AScreenCallout()
    case .bottomBar:
      presentBottomBarCallout()
    case .vpn:
      presentVPNAlertCallout()
    case .defaultBrowser:
      presentDefaultBrowserScreenCallout()
    case .rewards:
      presentBraveRewardsScreenCallout()
    case .blockCookieConsentNotices:
      presentCookieNotificationBlockingCallout()
    case .linkReceipt:
      presentLinkReceiptCallout()
    }
  }
  
  // MARK: Conditional Callout Methods
  
  private func presentP3AScreenCallout() {
    // Check the blockCookieConsentNotices callout can be shown
    guard shouldShowCallout(calloutType: .p3a) else {
      return
    }
    
    let onboardingP3ACalloutController = Welcome3PAViewController().then {
      $0.isModalInPresentation = true
      $0.modalPresentationStyle = .overFullScreen
    }

    let state = WelcomeViewCalloutState.p3a(
      info: WelcomeViewCalloutState.WelcomeViewDefaultBrowserDetails(
        title: Strings.Callout.p3aCalloutTitle,
        toggleTitle: Strings.Callout.p3aCalloutToggleTitle,
        details: Strings.Callout.p3aCalloutDescription,
        linkDescription: Strings.Callout.p3aCalloutLinkTitle,
        primaryButtonTitle: Strings.done,
        toggleAction: { [weak self] isOn in
          self?.braveCore.p3aUtils.isP3AEnabled = isOn
        },
        linkAction: { url in
          let p3aLearnMoreController = SFSafariViewController(url: .brave.p3aHelpArticle, configuration: .init())
          p3aLearnMoreController.modalPresentationStyle = .currentContext
          
          onboardingP3ACalloutController.present(p3aLearnMoreController, animated: true)
        },
        primaryButtonAction: { [weak self] in
          Preferences.Onboarding.p3aOnboardingShown.value = true

          self?.isOnboardingOrFullScreenCalloutPresented = true
          self?.dismiss(animated: false)
        }
      )
    )

    onboardingP3ACalloutController.setLayoutState(state: state)
    
    if !isOnboardingOrFullScreenCalloutPresented {
      braveCore.p3aUtils.isNoticeAcknowledged = true
      present(onboardingP3ACalloutController, animated: false)
    }
  }
  
  private func presentBottomBarCallout() {
    guard traitCollection.userInterfaceIdiom == .phone else { return }
    
    // Check the blockCookieConsentNotices callout can be shown
    guard shouldShowCallout(calloutType: .blockCookieConsentNotices) else {
      return
    }

    // Onboarding should be completed to show callouts
    if Preferences.Onboarding.basicOnboardingCompleted.value != OnboardingState.completed.rawValue {
      return
    }
    
    // Show if bottom bar is not enabled
    if Preferences.General.isUsingBottomBar.value {
      return
    }

    var bottomBarView = OnboardingBottomBarView()
    bottomBarView.switchBottomBar = { [weak self] in
      guard let self else { return }
    
      self.dismiss(animated: false) {
        Preferences.General.isUsingBottomBar.value = true
      }
    }
    bottomBarView.dismiss = { [weak self] in
      guard let self = self else { return }
      
      self.dismiss(animated: false)
    }
    
    let popup = PopupViewController(rootView: bottomBarView, isDismissable: true)

    isOnboardingOrFullScreenCalloutPresented = true
    present(popup, animated: false)
  }
  
  private func presentVPNAlertCallout() {
    // Check the vpn alert callout can be shown
    guard shouldShowCallout(calloutType: .vpn) else {
      return
    }

    let onboardingNotCompleted =
      Preferences.Onboarding.basicOnboardingCompleted.value != OnboardingState.completed.rawValue

    let showedPopup = Preferences.VPN.popupShowed

    if onboardingNotCompleted
      || showedPopup.value
      || !VPNProductInfo.isComplete {
      FullScreenCalloutType.vpn.preferenceValue.value = false
      return
    }

    var vpnDetailsView = OnboardingVPNDetailsView()
    vpnDetailsView.learnMore = { [weak self] in
      guard let self = self else { return }
    
      self.dismiss(animated: false) {
        self.presentCorrespondingVPNViewController()
      }
    }
    
    let popup = PopupViewController(rootView: vpnDetailsView, isDismissable: true)

    isOnboardingOrFullScreenCalloutPresented = true
    showedPopup.value = true
    present(popup, animated: false)
  }
  
  private func presentDefaultBrowserScreenCallout() {
    // Check the blockCookieConsentNotices callout can be shown
    guard shouldShowCallout(calloutType: .defaultBrowser) else {
      return
    }

    let onboardingController = WelcomeViewController(
      state: WelcomeViewCalloutState.defaultBrowserCallout(
        info: WelcomeViewCalloutState.WelcomeViewDefaultBrowserDetails(
          title: Strings.Callout.defaultBrowserCalloutTitle,
          details: Strings.Callout.defaultBrowserCalloutDescription,
          primaryButtonTitle: Strings.Callout.defaultBrowserCalloutPrimaryButtonTitle,
          secondaryButtonTitle: Strings.Callout.defaultBrowserCalloutSecondaryButtonTitle,
          primaryButtonAction: { [weak self] in
            guard let settingsUrl = URL(string: UIApplication.openSettingsURLString) else {
              return
            }

            Preferences.General.defaultBrowserCalloutDismissed.value = true
            self?.isOnboardingOrFullScreenCalloutPresented = true

            UIApplication.shared.open(settingsUrl)
            self?.dismiss(animated: false)
          },
          secondaryButtonAction: { [weak self] in
            self?.isOnboardingOrFullScreenCalloutPresented = true

            self?.dismiss(animated: false)
          }
        )
      ), p3aUtilities: braveCore.p3aUtils
    )

    if !isOnboardingOrFullScreenCalloutPresented {
      present(onboardingController, animated: true)
    }
  }
  
  private func presentBraveRewardsScreenCallout() {
    // Check the blockCookieConsentNotices callout can be shown
    guard shouldShowCallout(calloutType: .rewards) else {
      return
    }

    if BraveRewards.isAvailable, !Preferences.Rewards.rewardsToggledOnce.value {
      let controller = OnboardingRewardsAgreementViewController()
      controller.onOnboardingStateChanged = { [weak self] controller, state in
        self?.completeOnboarding(controller)
      }
      controller.onRewardsStatusChanged = { [weak self] status in
        self?.rewards.isEnabled = status
      }
      
      present(controller, animated: true)
      isOnboardingOrFullScreenCalloutPresented = true
    }
  }
  
  private func presentCookieNotificationBlockingCallout() {
    // Check the blockCookieConsentNotices callout can be shown
    guard shouldShowCallout(calloutType: .blockCookieConsentNotices) else {
      return
    }
    
    // Don't show this if we already enabled the setting
    guard !FilterListStorage.shared.isEnabled(for: FilterList.cookieConsentNoticesComponentID) else { return }
    // Don't show this if we are presenting another popup already
    guard !isOnboardingOrFullScreenCalloutPresented else { return }
    // We only show the popup on second launch
    guard !Preferences.General.isFirstLaunch.value else { return }
    // Ensure we successfully shown basic onboarding first
    guard Preferences.FullScreenCallout.omniboxCalloutCompleted.value else { return }

    let popover = PopoverController(
      contentController: CookieNotificationBlockingConsentViewController(),
      contentSizeBehavior: .preferredContentSize)
    popover.addsConvenientDismissalMargins = false
    popover.present(from: topToolbar.locationView.shieldsButton, on: self)
  }
  
  private func presentLinkReceiptCallout(skipSafeGuards: Bool = false) {
    if !skipSafeGuards {
      // Show this onboarding only if the VPN has been purchased
      guard case .purchased = BraveVPN.vpnState else { return }
      
      guard shouldShowCallout(calloutType: .linkReceipt) else {
        return
      }
      
      if Preferences.Onboarding.basicOnboardingCompleted.value != OnboardingState.completed.rawValue {
        return
      }
    }
    
    var linkReceiptView = OnboardingLinkReceiptView()
    linkReceiptView.linkReceiptAction = {
      self.openURLInNewTab(.brave.braveVPNLinkReceiptProd, isPrivate: PrivateBrowsingManager.shared.isPrivateBrowsing, isPrivileged: false)
    }
    let popup = PopupViewController(rootView: linkReceiptView, isDismissable: true)
    isOnboardingOrFullScreenCalloutPresented = true
    present(popup, animated: false)
  }
  
  private func shouldShowCallout(calloutType: FullScreenCalloutType) -> Bool {
    if Preferences.DebugFlag.skipNTPCallouts == true || isOnboardingOrFullScreenCalloutPresented || topToolbar.inOverlayMode {
      return false
    }

    if presentedViewController != nil || !FullScreenCalloutManager.shouldShowDefaultBrowserCallout(calloutType: calloutType) {
      return false
    }
    
    return true
  }
  
  // MARK: Non-Conditional Callouts Methods
  
  func presentPassCodeMigration() {
    if KeychainWrapper.sharedAppContainerKeychain.authenticationInfo() != nil {
      let controller = PasscodeMigrationViewController()
      controller.rootView.dismiss = { [unowned controller] enableBrowserLock in
        KeychainWrapper.sharedAppContainerKeychain.setAuthenticationInfo(nil)
        Preferences.Privacy.lockWithPasscode.value = enableBrowserLock
        controller.dismiss(animated: true)
      }
      controller.modalPresentationStyle = .overFullScreen
      // No animation to ensure we don't leak the users tabs
      present(controller, animated: false)
    }
  }
  
  func presentVPNInAppEventCallout() {
    // If the onboarding has not completed we do not show any promo screens.
    // This will most likely be the case for users who have not installed the app yet.
    if Preferences.Onboarding.basicOnboardingCompleted.value != OnboardingState.completed.rawValue {
      return
    }
    
    switch BraveVPN.vpnState {
    case .purchased:
      presentLinkReceiptCallout(skipSafeGuards: true)
    case .expired, .notPurchased:
      if VPNProductInfo.isComplete {
        presentCorrespondingVPNViewController()
      } else {
        // This is flaky. We fetch VPN prices from Apple asynchronously and it makes no sense to
        // show anything if there's no price data. We try to wait one second and see if the price data is there.
        // If not we do not show anything.
        // This can happen if the app is not in memory and we have to fresh launch it upon tapping on the in app event.
        DispatchQueue.main.asyncAfter(deadline: .now() + 1) { [self] in
          if VPNProductInfo.isComplete {
            presentCorrespondingVPNViewController()
          }
        }
      }
    }
  }
}
