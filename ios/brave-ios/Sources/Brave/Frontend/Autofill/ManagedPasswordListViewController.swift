// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import Preferences
import SwiftUI
import UIKit

class ManagedPasswordListViewController: UIHostingController<ManagedPasswordListView> {
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

    let managedPasswordListView = ManagedPasswordListView(
      passwordAPI: passwordAPI,
      windowProtection: windowProtection,
      settingsDelegate: nil
    )

    super.init(rootView: managedPasswordListView)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    // UINavigationController hides the bottom toolbar by default
    navigationController?.setToolbarHidden(false, animated: animated)
  }
}
