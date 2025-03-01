// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import DesignSystem
import Foundation
import Onboarding
import Preferences
import SwiftUI
import UIKit

extension BrowserViewController: BraveTranslateScriptHandlerDelegate {
  func updateTranslateURLBar(tab: Tab?, state: TranslateURLBarButton.TranslateState) {
    guard let tab = tab else { return }

    tab.translationState = state

    if tab === tabManager.selectedTab {
      topToolbar.updateTranslateButtonState(state)
    }
  }

  func canShowTranslateOnboarding(tab: Tab?) -> Bool {
    guard let tab = tab, let selectedTab = tabManager.selectedTab, tab === selectedTab else {
      return false
    }

    return Preferences.Translate.translateEnabled.value == nil
      && !topToolbar.inOverlayMode
      && topToolbar.secureContentState == .secure
      && Preferences.Translate.translateURLBarOnboardingCount.value < 2
      && shouldShowTranslationOnboardingThisSession && presentedViewController == nil
  }

  func showTranslateOnboarding(tab: Tab?, completion: @escaping (_ translateEnabled: Bool?) -> Void)
  {
    // Do NOT show the translate onboarding popup if the tab isn't visible
    guard canShowTranslateOnboarding(tab: tab)
    else {
      completion(nil)
      return
    }

    Preferences.Translate.translateURLBarOnboardingCount.value += 1

    topToolbar.layoutIfNeeded()
    view.layoutIfNeeded()

    // Ensure url bar is expanded before presenting a popover on it
    toolbarVisibilityViewModel.toolbarState = .expanded

    DispatchQueue.main.async {
      let popover = PopoverController(
        content: OnboardingTranslateView(
          onContinueButtonPressed: { [weak self, weak tab] in
            guard let tab = tab, let self = self else { return }

            self.topToolbar.locationView.translateButton.setOnboardingState(enabled: false)
            Preferences.Translate.translateEnabled.value = true

            tab.translateHelper?.presentUI(on: self)
            completion(true)
          },
          onDisableFeature: { [weak self, weak tab] in
            guard let tab = tab, let self = self else { return }

            self.topToolbar.locationView.translateButton.setOnboardingState(enabled: false)

            Preferences.Translate.translateEnabled.value = false
            tab.translationState = .unavailable
            self.topToolbar.updateTranslateButtonState(.unavailable)
            completion(false)
          }
        ),
        autoLayoutConfiguration: .phoneWidth
      )

      popover.arrowDistance = 10.0
      popover.outerMargins = UIEdgeInsets(equalInset: 16.0)

      popover.previewForOrigin = .init(
        view: self.topToolbar.locationView.translateButton.then {
          $0.setOnboardingState(enabled: true)
        },
        action: { popover in
          popover.previewForOrigin = nil
          popover.dismissPopover()
        }
      )

      popover.popoverDidDismiss = { [weak self] _ in
        self?.topToolbar.locationView.translateButton.setOnboardingState(enabled: false)
        completion(nil)
      }
      popover.present(from: self.topToolbar.locationView.translateButton, on: self)
    }

    shouldShowTranslationOnboardingThisSession = false
  }

  func presentToast(tab: Tab?, languageInfo: BraveTranslateLanguageInfo) {
    if presentedViewController != nil || topToolbar.inOverlayMode || tab !== tabManager.selectedTab
    {
      return
    }

    let popover = PopoverController(
      content: TranslateToast(languageInfo: languageInfo) { [weak tab] _ in
        tab?.translateHelper?.startTranslation(canShowToast: false)
      },
      autoLayoutConfiguration: nil
    )
    popover.present(from: self.topToolbar.locationView.translateButton, on: self)
  }
}
