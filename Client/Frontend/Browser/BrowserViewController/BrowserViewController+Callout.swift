// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveShared
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

  func presentPassCodeMigration() {
    if KeychainWrapper.sharedAppContainerKeychain.authenticationInfo() != nil {
      let controller = UIHostingController(rootView: PasscodeMigrationContainerView())
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
  
  func presentP3AScreenCallout() {
    if Preferences.DebugFlag.skipNTPCallouts == true || isOnboardingOrFullScreenCalloutPresented { return }

    if presentedViewController != nil || !FullScreenCalloutManager.shouldShowDefaultBrowserCallout(calloutType: .p3a) {
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
          let p3aLearnMoreController = SFSafariViewController(url: BraveUX.braveP3ALearnMoreURL, configuration: .init())
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

  func presentVPNAlertCallout() {
    if Preferences.DebugFlag.skipNTPCallouts == true || isOnboardingOrFullScreenCalloutPresented { return }

    if presentedViewController != nil || !FullScreenCalloutManager.shouldShowDefaultBrowserCallout(calloutType: .vpn) {
      return
    }

    let onboardingNotCompleted =
      Preferences.Onboarding.basicOnboardingCompleted.value != OnboardingState.completed.rawValue

    let showedPopup = Preferences.VPN.popupShowed

    if onboardingNotCompleted
      || showedPopup.value
      || !VPNProductInfo.isComplete {
      FullScreenCalloutManager.FullScreenCalloutType.vpn.preferenceValue.value = false
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

  func presentDefaultBrowserScreenCallout() {
    if Preferences.DebugFlag.skipNTPCallouts == true || isOnboardingOrFullScreenCalloutPresented { return }

    if presentedViewController != nil || !FullScreenCalloutManager.shouldShowDefaultBrowserCallout(calloutType: .defaultBrowser) {
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

  func presentBraveRewardsScreenCallout() {
    if Preferences.DebugFlag.skipNTPCallouts == true || isOnboardingOrFullScreenCalloutPresented { return }

    if presentedViewController != nil || !FullScreenCalloutManager.shouldShowDefaultBrowserCallout(calloutType: .rewards) {
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
  
  func presentTabReceivedCallout(url: URL) {
    // 'Tab Received' indicator will only be shown in normal browsing
    if !PrivateBrowsingManager.shared.isPrivateBrowsing {
      let toast = ButtonToast(
        labelText: Strings.Callout.tabReceivedCalloutTitle,
        image: UIImage(braveSystemNamed: "brave.tablet.and.phone"),
        buttonText: Strings.goButtonTittle,
        completion: { [weak self] buttonPressed in
          guard let self = self else { return }
          
          if buttonPressed {
            self.tabManager.addTabAndSelect(URLRequest(url: url), isPrivate: false)
          }
      })
      
      show(toast: toast, duration: ButtonToastUX.toastDismissAfter)
    }
  }
}
