// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import Preferences
import SwiftUI
import UIKit

class LoginListViewController: UIHostingController<LoginListView> {
  weak var settingsDelegate: SettingsDelegate? {
    didSet {
      rootView.settingsDelegate = settingsDelegate
    }
  }

  private let passwordAPI: BravePasswordAPI
  private let windowProtection: WindowProtection?

  init(passwordAPI: BravePasswordAPI, windowProtection: WindowProtection?) {
    self.passwordAPI = passwordAPI
    self.windowProtection = windowProtection

    let loginView = LoginListView(
      passwordAPI: passwordAPI,
      windowProtection: windowProtection,
      settingsDelegate: nil,
      onCredentialSelected: nil
    )

    super.init(rootView: loginView)

    // Set up the navigation handler after super.init
    rootView.onCredentialSelected = { [weak self] credential in
      guard let self = self else { return }
      let loginDetailsViewController = LoginInfoViewController(
        passwordAPI: self.passwordAPI,
        credentials: credential,
        windowProtection: self.windowProtection
      )
      loginDetailsViewController.settingsDelegate = self.settingsDelegate
      self.navigationController?.pushViewController(loginDetailsViewController, animated: true)
    }
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
}
