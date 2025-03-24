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
import Web

extension BrowserViewController: BraveTranslateScriptHandlerDelegate {
  func updateTranslateURLBar(tab: Web.Tab, state: TranslateURLBarButton.TranslateState) {
    tab.translationState = state

    if tab === tabManager.selectedTab {
      topToolbar.updateTranslateButtonState(state)
    }
  }

  func canShowTranslateOnboarding(tab: Web.Tab) -> Bool {
    guard let selectedTab = tabManager.selectedTab, tab === selectedTab else {
      return false
    }

    return Preferences.Translate.translateEnabled.value
      && !topToolbar.inOverlayMode
      && topToolbar.secureContentState == .secure
      && Preferences.Translate.translateURLBarOnboardingCount.value < 2
      && shouldShowTranslationOnboardingThisSession && presentedViewController == nil
  }

  func showTranslateOnboarding(
    tab: Web.Tab,
    completion: @escaping (_ translateEnabled: Bool) -> Void
  ) {
    topToolbar.layoutIfNeeded()
    view.layoutIfNeeded()

    // Ensure url bar is expanded before presenting a popover on it
    toolbarVisibilityViewModel.toolbarState = .expanded

    DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
      // Do NOT show the translate onboarding popup if the tab isn't visible
      guard self.canShowTranslateOnboarding(tab: tab)
      else {
        completion(Preferences.Translate.translateEnabled.value)
        return
      }

      Preferences.Translate.translateURLBarOnboardingCount.value += 1

      let popover = PopoverController(
        content: OnboardingTranslateView(),
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
        completion(Preferences.Translate.translateEnabled.value)
      }
      popover.present(from: self.topToolbar.locationView.translateButton, on: self)
      self.shouldShowTranslationOnboardingThisSession = false
    }
  }

  func presentTranslateToast(tab: Web.Tab, languageInfo: BraveTranslateLanguageInfo) {
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

  func presentTranslateError(tab: Web.Tab) {
    if presentedViewController != nil || topToolbar.inOverlayMode || tab !== tabManager.selectedTab
    {
      return
    }

    let alert = UIAlertController(
      title: Strings.genericErrorTitle,
      message: Strings.genericErrorBody,
      preferredStyle: .alert
    )
    alert.addAction(UIAlertAction(title: Strings.OBErrorOkay, style: .default))
    self.present(alert, animated: true)
  }
}
