// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import BraveUI
import Onboarding
import Preferences

extension BrowserViewController: BraveTranslateScriptHandlerDelegate {
  func updateTranslateURLBar(tab: Tab?, state: TranslateURLBarButton.TranslateState) {
    guard let tab = tab else { return }

    if tab === tabManager.selectedTab {
      tab.translationState = state

//      translateActivity(info: state == .existingItem ? item : nil)
      topToolbar.updateTranslateButtonState(state)
    }
  }

  func showTranslateToast(tab: Tab?, state: TranslateURLBarButton.TranslateState) {
    updateTranslateURLBar(tab: tab, state: state)

    guard let selectedTab = tabManager.selectedTab,
          selectedTab === tab,
          selectedTab.translationState == .available
    else {
      return
    }

    if (pendingToast as? TranslateToast) != nil {
      return
    }

    pendingToast = TranslateToast(
      completion: { [weak self] _ in
        guard let self = self else {
          return
         }
        
        self.pendingToast = nil
      }
    )

    if let pendingToast = pendingToast {
      show(toast: pendingToast, afterWaiting: .milliseconds(250), duration: .seconds(5))
    }
  }

  func showTranslateOnboarding(tab: Tab?, completion: @escaping (_ translateEnabled: Bool) -> Void) {
    // Do NOT show the translate onboarding popup if the tab isn't visible
    guard Preferences.Translate.translateEnabled.value,
          let selectedTab = tabManager.selectedTab,
          selectedTab === tab,
          selectedTab.translationState == .available else {
      completion(Preferences.Translate.translateEnabled.value)
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
        let popover = PopoverController(content: OnboardingTranslateView(onContinueButtonPressed: { [weak self, weak tab] in
          guard let tab = tab, let self = self else { return }
          
          if let scriptHandler = tab.getContentScript(name: BraveTranslateScriptHandler.scriptName)
            as? BraveTranslateScriptHandler
          {
            self.showTranslateOnboarding(tab: tab) { [weak scriptHandler] translateEnabled in
              scriptHandler?.presentUI(on: self)
            }
          }
        }, onDisableFeature: { [weak self, weak tab] in
          guard let tab = tab, let self = self else { return }
          
          Preferences.Translate.translateEnabled.value = false
          tab.translationState = .unavailable
          self.topToolbar.updateTranslateButtonState(.unavailable)
        }))
        
        popover.previewForOrigin = .init(
          view: self.topToolbar.locationView.translateButton,
          action: { popover in
            popover.previewForOrigin = nil
            popover.dismissPopover()
            completion(Preferences.Translate.translateEnabled.value)
          }
        )
        
        popover.popoverDidDismiss = { _ in
          completion(Preferences.Translate.translateEnabled.value)
        }
        popover.present(from: self.topToolbar.locationView.translateButton, on: self)
      }

      shouldShowTranslationOnboardingThisSession = false
    } else if Preferences.Translate.translateEnabled.value {
      completion(true)
    }
  }
}
