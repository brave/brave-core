// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveUI
import BraveVPN
import DataImporter
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
    case .bottomBar:
      presentBottomBarCallout(skipSafeGuards: skipSafeGuards)
    case .defaultBrowser:
      presentDefaultBrowserScreenCallout(skipSafeGuards: skipSafeGuards)
    case .rewards:
      presentBraveRewardsScreenCallout(skipSafeGuards: skipSafeGuards)
    case .vpnLinkReceipt:
      presentVPNLinkReceiptCallout(skipSafeGuards: skipSafeGuards)
    }
  }

  // MARK: Conditional Callout Methods

  private func presentP3AScreenCallout() {
    if braveCore.p3aUtils.isP3APreferenceManaged {
      return
    }

    let controller = OnboardingController(
      environment: .init(
        p3aUtils: braveCore.p3aUtils,
        attributionManager: attributionManager
      ),
      steps: [.p3aOptIn],
      showSplashScreen: false,
      showDismissButton: false
    ).then {
      $0.isModalInPresentation = true
      $0.modalPresentationStyle = .overFullScreen
    }

    present(controller, animated: true)
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

  func presentDefaultBrowserScreenCallout(skipSafeGuards: Bool = false) {
    if !skipSafeGuards {
      defaultBrowserHelper.performAccurateDefaultCheckIfNeeded()
      let isLikelyDefault =
        defaultBrowserHelper.status == .defaulted || defaultBrowserHelper.status == .likely
      if isLikelyDefault {
        return
      }
    }

    let defaultBrowserCallout = OnboardingController(
      environment: .init(
        p3aUtils: braveCore.p3aUtils,
        attributionManager: attributionManager
      ),
      steps: [.defaultBrowsing],
      showSplashScreen: false,
      showDismissButton: false
    ).then {
      $0.isModalInPresentation = true
      $0.modalPresentationStyle = .overFullScreen
    }

    present(defaultBrowserCallout, animated: true)
  }

  private func presentBraveRewardsScreenCallout(skipSafeGuards: Bool = false) {
    if !skipSafeGuards {
      guard BraveRewards.isSupported(prefService: profileController.profile.prefs),
        !Preferences.Rewards.rewardsToggledOnce.value
      else {
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

    if calloutType == .defaultBrowser, defaultBrowserHelper.isAccurateDefaultCheckAvailable {
      // Use DefaultBrowserHelper's logic when more accurate API is available
      return defaultBrowserHelper.shouldPerformAccurateDefaultCheck
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
    // If the onboarding has not completed or VPN is not available we do not show any promo screens.
    // This will most likely be the case for users who have not installed the app yet.
    if profileController.profile.prefs.isBraveVPNAvailable,
      Preferences.Onboarding.basicOnboardingCompleted.value != OnboardingState.completed.rawValue
    {
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

  func presentDataImporter() {
    // DataImportView is typically presented from inside settings, so we need to:
    //   1. Set up a UINavigationController container
    //   2. Add a done button to the toolbar
    let controller = UIHostingController(
      rootView: DataImportView(
        model: .init(
          coordinator: SafariDataImporterCoordinatorImpl(profile: profileController.profile)
        ),
        openURL: { [unowned self] url in
          dismiss(animated: true)
          openURLInNewTab(
            url,
            isPrivate: privateBrowsingManager.isPrivateBrowsing,
            isPrivileged: url.scheme == InternalURL.scheme
          )
        },
        dismiss: { [unowned self] in
          dismiss(animated: true)
        },
        onDismiss: {}
      )
    )
    controller.navigationItem.rightBarButtonItem = .init(
      systemItem: .done,
      primaryAction: .init(handler: { [unowned self] _ in
        dismiss(animated: true)
      })
    )
    let container = UINavigationController(rootViewController: controller)
    present(container, animated: true)
  }
}
