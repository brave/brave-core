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

      showTranslateOnboarding(tab: tab) { [weak tab] translateEnabled in
        let tabHelper =
          tab?.getTabHelper(named: BraveTranslateTabHelper.tabHelperName)
          as? BraveTranslateTabHelper
        if let tabHelper {
          tabHelper.startTranslation(canShowToast: true)
        }
      }
    }
  }

  func showTranslateOnboarding(tab: Tab?, completion: @escaping (_ translateEnabled: Bool) -> Void)
  {
    // Do NOT show the translate onboarding popup if the tab isn't visible
    guard Preferences.Translate.translateEnabled.value,
      let selectedTab = tabManager.selectedTab,
      selectedTab === tab,
      selectedTab.translationState == .available
    else {
      return
    }

    if Preferences.Translate.translateURLBarOnboardingCount.value < 2,
      shouldShowTranslationOnboardingThisSession,
      presentedViewController == nil
    {
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

              let tabHelper =
                tab.getTabHelper(named: BraveTranslateTabHelper.tabHelperName)
                as? BraveTranslateTabHelper

              tabHelper?.presentUI(on: self)
            },
            onDisableFeature: { [weak self, weak tab] in
              guard let tab = tab, let self = self else { return }

              self.topToolbar.locationView.translateButton.setOnboardingState(enabled: false)

              Preferences.Translate.translateEnabled.value = false
              tab.translationState = .unavailable
              self.topToolbar.updateTranslateButtonState(.unavailable)
            }
          ),
          autoLayoutConfiguration: .init(preferredWidth: self.view.bounds.width - (32.0 * 2.0))
        )

        popover.arrowDistance = 10.0

        popover.previewForOrigin = .init(
          view: self.topToolbar.locationView.translateButton.then {
            $0.setOnboardingState(enabled: true)
          },
          action: { popover in
            popover.previewForOrigin = nil
            popover.dismissPopover()
            completion(Preferences.Translate.translateEnabled.value)
          }
        )

        popover.popoverDidDismiss = { [weak self, weak tab] _ in
          guard let tab = tab, let self = self else { return }

          self.topToolbar.locationView.translateButton.setOnboardingState(enabled: false)

          if Preferences.Translate.translateEnabled.value {
            let tabHelper =
              tab.getTabHelper(named: BraveTranslateTabHelper.tabHelperName)
              as? BraveTranslateTabHelper

            tabHelper?.presentUI(on: self)

            completion(true)
            return
          }

          completion(false)
        }
        popover.present(from: self.topToolbar.locationView.translateButton, on: self)
      }

      shouldShowTranslationOnboardingThisSession = false
    }
  }

  func presentToast(tab: Tab?, languageInfo: BraveTranslateLanguageInfo) {
    let popover = PopoverController(
      content: TranslateToast(languageInfo: languageInfo) { [weak tab] languageInfo in

        let tabHelper =
          tab?.getTabHelper(named: BraveTranslateTabHelper.tabHelperName)
          as? BraveTranslateTabHelper

        tabHelper?.startTranslation(canShowToast: false)
      },
      autoLayoutConfiguration: nil
    )
    popover.present(from: self.topToolbar.locationView.translateButton, on: self)
  }
}
