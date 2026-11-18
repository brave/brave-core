// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import UIKit

/// A WebUI page presented from settings. When its page asks for another
/// screen, it presents it over itself or dismisses itself to make way for it.
final class BraveAccountWebUIController: ChromeWebUIController,
  BraveAccountScreenOpenerBridge
{
  private let profile: any Profile
  private let url: URL
  private let dialogMode: BraveAccount.DialogMode?

  init?(screen: Screen, profile: any Profile) {
    guard let destination = screen.destination else { return nil }
    self.profile = profile
    url = destination.url
    if case .dialog(let dialogMode) = screen {
      self.dialogMode = dialogMode
    } else {
      dialogMode = nil
    }
    super.init(profile: profile)
    title = destination.title
    navigationItem.rightBarButtonItem = .doneButton { [unowned self] in
      dismiss(animated: true)
    }
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    if let dialogMode {
      webView.braveAccountDialogMode = dialogMode
    }
    webView.braveAccountScreenOpener = self
    webView.load(URLRequest(url: url))
  }

  enum Screen {
    case dialog(BraveAccount.DialogMode)
    case settingsPage(BraveAccount.SettingsPage)

    var destination: (url: URL, title: String)? {
      switch self {
      case .dialog:
        return (
          URL(string: "brave://account")!,
          L10nUtils.string(messageId: .BRAVE_ACCOUNT_TITLE)
        )
      case .settingsPage(let page):
        switch page {
        case .braveAccount:
          return (
            URL(string: "brave://account/\(BraveAccountSettingsPath)")!,
            L10nUtils.string(messageId: .BRAVE_ACCOUNT_TITLE)
          )
        @unknown default:
          return nil
        }
      }
    }
  }

  // MARK: - BraveAccountScreenOpenerBridge

  // Called from the WebUI serving the account rows, which asks for the dialog
  // to be opened over it.
  func openBraveAccountDialog(dialogMode: BraveAccount.DialogMode) {
    guard
      let dialog = BraveAccountWebUIController(
        screen: .dialog(dialogMode),
        profile: profile
      )
    else { return }
    present(UINavigationController(rootViewController: dialog), animated: true)
  }

  // Called from a WebUI page that only links to `page`, which opens in its
  // place.
  func openSettingsPage(page: BraveAccount.SettingsPage) {
    guard let presenter = presentingViewController,
      let next = BraveAccountWebUIController(
        screen: .settingsPage(page),
        profile: profile
      )
    else { return }
    dismiss(animated: true) {
      presenter.present(UINavigationController(rootViewController: next), animated: true)
    }
  }
}
