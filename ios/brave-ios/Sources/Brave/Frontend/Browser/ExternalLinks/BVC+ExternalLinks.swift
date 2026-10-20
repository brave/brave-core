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
  func handleExternalLink(_ url: URL, open: @escaping (_ isPrivate: Bool) -> Void) -> Bool {
    guard url.isWebPage(includeDataURIs: false),
      !Preferences.Privacy.privateBrowsingOnly.value
    else {
      return false
    }
    let settings = profileController.defaultHostContentSettings
    switch settings.defaultExternalLinkMode {
    case .current:
      return false
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

  private func presentExternalLinkPrompt(url: URL, open: @escaping (_ isPrivate: Bool) -> Void) {
    weak var controller: UIViewController?
    let promptController = ExternalLinkPromptViewController(url: url) {
      [weak self] choice, remember in
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
      presentQuickView(url: url, profile: profileController.profile, showsConfirmationAlert: false)
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
