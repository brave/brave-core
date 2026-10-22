// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import ExternalLinks
import Preferences
import SwiftUI
import UIKit

extension BrowserViewController {
  /// Handles a link opened from another app based on the user's setting. Returns `false`
  /// when the link should instead open in the current browsing mode.
  func handleExternalLink(
    _ url: URL,
    isPrivate: Bool,
    open: @escaping (_ isPrivate: Bool) -> Void
  ) -> Bool {
    guard url.isWebPage(includeDataURIs: false),
      !Preferences.Privacy.privateBrowsingOnly.value
    else {
      return false
    }
    let settings = profileController.defaultHostContentSettings
    switch settings.defaultExternalLinkMode {
    case .current:
      guard !Preferences.General.externalLinkOnboardingShown.value else { return false }
      Preferences.General.externalLinkOnboardingShown.value = true
      presentExternalLinkOnboarding(url: url, isPrivate: isPrivate, open: open)
    case .ask:
      if let remembered = ExternalLinkChoice(settings.externalLinkMode(for: url)) {
        performExternalLinkChoice(remembered, url: url, open: open)
      } else {
        presentExternalLinkPrompt(url: url, open: open)
      }
    case let mode:
      guard let choice = ExternalLinkChoice(mode) else { return false }
      performExternalLinkChoice(choice, url: url, open: open)
    }
    return true
  }

  private func presentExternalLinkOnboarding(
    url: URL,
    isPrivate: Bool,
    open: @escaping (_ isPrivate: Bool) -> Void
  ) {
    weak var controller: UIViewController?
    var didEnable = false
    let onboardingController = SelfSizingSheetController(
      rootView: ExternalLinkOnboardingView(
        onEnable: { [weak self] in
          guard let self else { return }
          didEnable = true
          self.profileController.defaultHostContentSettings.defaultExternalLinkMode = .ask
          controller?.dismiss(animated: true) {
            self.presentExternalLinkPrompt(url: url, open: open)
          }
        },
        onDecline: {
          controller?.dismiss(animated: true)
        }
      )
      // Covers decline, the close button, and swipe-to-dismiss
      .onDisappear {
        if !didEnable {
          open(isPrivate)
        }
      }
    )
    controller = onboardingController
    presentOnTop(onboardingController)
  }

  private func presentExternalLinkPrompt(url: URL, open: @escaping (_ isPrivate: Bool) -> Void) {
    weak var controller: UIViewController?
    let promptController = SelfSizingSheetController(
      rootView: ExternalLinkPromptView(url: url) { [weak self] choice, remember in
        guard let self else { return }
        if remember {
          self.profileController.defaultHostContentSettings.setExternalLinkMode(
            choice.contentSettingMode,
            for: url
          )
        }
        controller?.dismiss(animated: true) {
          self.performExternalLinkChoice(choice, url: url, open: open)
        }
      }
    )
    controller = promptController
    presentOnTop(promptController)
  }

  private func performExternalLinkChoice(
    _ choice: ExternalLinkChoice,
    url: URL,
    open: (_ isPrivate: Bool) -> Void
  ) {
    switch choice {
    case .regularMode:
      open(false)
    case .privateMode:
      open(true)
    case .quickView:
      presentExternalLinkQuickView(url: url)
    }
  }

  private func presentExternalLinkQuickView(url: URL) {
    guard Preferences.General.externalLinkQuickViewPrivateStorage.value else {
      presentQuickView(url: url, profile: profileController.profile, showsConfirmationAlert: false)
      return
    }
    let present = { [weak self] in
      guard let self else { return }
      self.presentQuickView(
        url: url,
        profile: self.profileController.profile.offTheRecordProfile,
        showsConfirmationAlert: false
      )
    }
    // Skip private-tab auth when app passcode lock is on; the user must unlock the app first.
    guard Preferences.Privacy.privateBrowsingLock.value,
      !Preferences.Privacy.lockWithPasscode.value
    else {
      present()
      return
    }
    guard let windowProtection else { return }
    askForLocalAuthentication(using: windowProtection, viewType: .external) { success, _ in
      if success {
        present()
      }
    }
  }

  func presentOnTop(_ viewController: UIViewController) {
    var presenter: UIViewController = self
    while let presented = presenter.presentedViewController {
      presenter = presented
    }
    presenter.present(viewController, animated: true)
  }
}
