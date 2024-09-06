// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveUI
import BraveVPN
import Onboarding
import Preferences
import SafariServices
import Shared
import StoreKit
import SwiftUI

// MARK: - Callouts

extension BrowserViewController {
  /// Check FullScreenCalloutType to make alterations to priority of pop-over variation
  ///
  /// Priority:
  /// - P3A
  /// - VPN Update Billing
  /// - Bottom Bar
  /// - VPN Promotion
  /// - Default Browser
  /// - Rewards
  /// - VPN Link Receipt
  func presentFullScreenCallouts() {
    for type in FullScreenCalloutType.allCases {
      presentScreenCallout(for: type)
    }
  }

  private func presentScreenCallout(for type: FullScreenCalloutType, skipSafeGuards: Bool = false) {
    // Check the type custom callout can be shown
    guard shouldShowCallout(calloutType: type, skipSafeGuards: skipSafeGuards) else {
      return
    }

    switch type {
    case .p3a:
      presentP3AScreenCallout()
    case .vpnUpdateBilling:
      presentVPNUpdateBillingCallout(skipSafeGuards: skipSafeGuards)
    case .bottomBar:
      presentBottomBarCallout(skipSafeGuards: skipSafeGuards)
    case .defaultBrowser:
      presentDefaultBrowserScreenCallout(skipSafeGuards: skipSafeGuards)
    case .rewards:
      presentBraveRewardsScreenCallout(skipSafeGuards: skipSafeGuards)
    case .vpnPromotion:
      presentVPNPromotionCallout(skipSafeGuards: skipSafeGuards)
    case .vpnLinkReceipt:
      presentVPNLinkReceiptCallout(skipSafeGuards: skipSafeGuards)
    }
  }

  // MARK: Conditional Callout Methods

  private func presentP3AScreenCallout() {
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
        primaryButtonTitle: Strings.P3A.continueButton,
        toggleAction: { [weak self] isOn in
          self?.braveCore.p3aUtils.isP3AEnabled = isOn
        },
        linkAction: { url in
          let p3aLearnMoreController = SFSafariViewController(
            url: .brave.p3aHelpArticle,
            configuration: .init()
          )
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

    braveCore.p3aUtils.isNoticeAcknowledged = true
    present(onboardingP3ACalloutController, animated: false)
  }

  private func presentBottomBarCallout(skipSafeGuards: Bool = false) {
    if !skipSafeGuards {
      guard traitCollection.userInterfaceIdiom == .phone else {
        return
      }

      // Show if bottom bar is not enabled
      if Preferences.General.isUsingBottomBar.value {
        return
      }
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

  private func presentDefaultBrowserScreenCallout(skipSafeGuards: Bool = false) {
    if !Locale.current.isNewOnboardingRegion {
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
        ),
        p3aUtilities: braveCore.p3aUtils,
        attributionManager: attributionManager
      )
      present(onboardingController, animated: true)
      return
    }

    if !skipSafeGuards {
      let isLikelyDefault = DefaultBrowserHelper.isBraveLikelyDefaultBrowser()

      guard !isLikelyDefault else {
        return
      }
    }

    let defaultBrowserCallout = UIHostingController(
      rootView: FocusSystemSettingsView(
        screenType: .callout,
        shouldDismiss: Binding.constant(false)
      )
    ).then {
      $0.isModalInPresentation = true
      $0.modalPresentationStyle = .overFullScreen
    }

    present(defaultBrowserCallout, animated: true)
  }

  private func presentBraveRewardsScreenCallout(skipSafeGuards: Bool = false) {
    if !skipSafeGuards {
      guard BraveRewards.isAvailable, !Preferences.Rewards.rewardsToggledOnce.value else {
        return
      }
    }

    let controller = OnboardingRewardsAgreementViewController()
    controller.onOnboardingStateChanged = { [weak self] controller, state in
      self?.completeOnboarding(controller)
    }
    controller.onRewardsStatusChanged = { [weak self] status in
      self?.rewards.isEnabled = status
    }

    isOnboardingOrFullScreenCalloutPresented = true
    present(controller, animated: true)
  }

  private func presentVPNPromotionCallout(skipSafeGuards: Bool = false) {
    if !skipSafeGuards {
      // Onboarding should be completed to show callouts
      if Preferences.Onboarding.basicOnboardingCompleted.value != OnboardingState.completed.rawValue
      {
        return
      }

      if Preferences.VPN.popupShowed.value
        || !BraveVPNProductInfo.isComplete
      {
        FullScreenCalloutType.vpnPromotion.preferenceValue.value = false
        return
      }
    }

    var vpnDetailsView = OnboardingVPNDetailsView()
    vpnDetailsView.learnMore = { [weak self] in
      guard let self = self else { return }

      self.dismiss(animated: false) {
        self.presentCorrespondingVPNViewController()
      }
    }

    let popup = PopupViewController(rootView: vpnDetailsView, isDismissable: true)
    Preferences.VPN.popupShowed.value = true

    isOnboardingOrFullScreenCalloutPresented = true
    present(popup, animated: false)
  }

  private func presentVPNLinkReceiptCallout(skipSafeGuards: Bool = false) {
    if !skipSafeGuards {
      // Show this onboarding only if the VPN has been purchased
      guard case .purchased = BraveVPN.vpnState else {
        return
      }

      if Preferences.Onboarding.basicOnboardingCompleted.value != OnboardingState.completed.rawValue
      {
        return
      }
    }

    var linkReceiptView = VPNLinkReceiptView()
    linkReceiptView.linkReceiptAction = {
      self.openURLInNewTab(
        .brave.braveVPNLinkReceiptProd,
        isPrivate: self.privateBrowsingManager.isPrivateBrowsing,
        isPrivileged: false
      )
    }
    let popup = PopupViewController(rootView: linkReceiptView, isDismissable: true)

    isOnboardingOrFullScreenCalloutPresented = true
    present(popup, animated: false)
  }

  private func presentVPNUpdateBillingCallout(skipSafeGuards: Bool = false) {
    if !skipSafeGuards {
      // Checking subscription receipt is in retry period
      guard let receiptStatus = Preferences.VPN.vpnReceiptStatus.value else {
        return
      }

      if receiptStatus != BraveVPN.ReceiptResponse.Status.retryPeriod.rawValue {
        return
      }
    }

    #if compiler(>=5.8)
    if #available(iOS 16.4, *) {
      Task { @MainActor in
        for await message in StoreKit.Message.messages {
          guard let windowScene = currentScene else {
            return
          }

          try? message.display(in: windowScene)
        }
      }
    } else {
      presentVPNChurnBilling()
    }
    #else
    presentVPNChurnBilling()
    #endif

    func presentVPNChurnBilling() {
      presentVPNChurnPromoCallout(for: .updateBillingExpired) {
        // Opens Apple's 'manage subscription' screen
        guard let url = URL.apple.manageSubscriptions else { return }

        if UIApplication.shared.canOpenURL(url) {
          UIApplication.shared.open(url, options: [:])
        }
      }
    }
  }

  // MARK: Helper Methods for Presentation

  private func presentVPNChurnPromoCallout(
    for type: VPNChurnPromoType,
    completion: @escaping () -> Void
  ) {
    var vpnChurnPromoView = VPNChurnPromoView(churnPromoType: type)

    vpnChurnPromoView.renewAction = {
      completion()
    }

    let popup = PopupViewController(rootView: vpnChurnPromoView, isDismissable: true)

    isOnboardingOrFullScreenCalloutPresented = true
    present(popup, animated: false)
  }

  private func shouldShowCallout(calloutType: FullScreenCalloutType, skipSafeGuards: Bool) -> Bool {
    if skipSafeGuards {
      return true
    }

    if Preferences.DebugFlag.skipNTPCallouts == true || isOnboardingOrFullScreenCalloutPresented
      || topToolbar.inOverlayMode
    {
      return false
    }

    if presentedViewController != nil
      || !FullScreenCalloutManager.shouldShowCallout(calloutType: calloutType)
    {
      return false
    }

    return true
  }

  // MARK: Non-Conditional Callouts Methods

  func presentVPNInAppEventCallout() {
    // If the onboarding has not completed we do not show any promo screens.
    // This will most likely be the case for users who have not installed the app yet.
    if Preferences.Onboarding.basicOnboardingCompleted.value != OnboardingState.completed.rawValue {
      return
    }

    switch BraveVPN.vpnState {
    case .purchased:
      presentVPNLinkReceiptCallout(skipSafeGuards: true)
    case .expired, .notPurchased:
      if BraveVPNProductInfo.isComplete {
        presentCorrespondingVPNViewController()
      } else {
        // This is flaky. We fetch VPN prices from Apple asynchronously and it makes no sense to
        // show anything if there's no price data. We try to wait one second and see if the price data is there.
        // If not we do not show anything.
        // This can happen if the app is not in memory and we have to fresh launch it upon tapping on the in app event.
        DispatchQueue.main.asyncAfter(deadline: .now() + 1) { [self] in
          if BraveVPNProductInfo.isComplete {
            presentCorrespondingVPNViewController()
          }
        }
      }
    }
  }

  func presentBraveLeoDeepLink() {
    // If the onboarding has not completed we do not show any promo screens.
    // This will most likely be the case for users who have not installed the app yet.
    if Preferences.Onboarding.basicOnboardingCompleted.value != OnboardingState.completed.rawValue {
      return
    }

    openBraveLeo()
  }
}
