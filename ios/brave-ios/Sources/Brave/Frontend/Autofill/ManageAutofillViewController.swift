// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import Preferences
import SwiftUI
import UIKit

enum AutofillType {
  case passwords
}

/// Hosts an autofill management view (passwords, credit cards, etc.) based on `AutofillType`.
class ManageAutofillViewController: UIHostingController<AnyView> {

  init(
    autofillDataManager: CWVAutofillDataManager,
    context: AutofillManagementContext,
    type: AutofillType
  ) {
    let content: AnyView
    switch type {
    case .passwords:
      let viewModel = ManagePasswordsViewModel(autofillDataManager: autofillDataManager)
      content = AnyView(ManagePasswordsView(viewModel: viewModel))
    }

    let rootView = AnyView(content.environment(\.autofillManagementContext, context))
    super.init(rootView: rootView)
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
