// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import LocalAuthentication
import SwiftUI

/// Context required by autofill data management views (e.g. `ManagePasswordDetailView`) for authentication,
/// URL opening, and passcode. Inject via `.environment(\.autofillContext, context)` at
/// the root of the autofill flow (e.g. `ManagePasswordsView`).
struct AutofillManagementContext {
  let settingsDelegate: SettingsDelegate?
  let askForAuthentication: (@escaping (Bool, LAError.Code?) -> Void) -> Void

  /// Opens the given URL in a new tab via the settings delegate.
  func openURLInNewTab(_ url: URL) {
    settingsDelegate?.settingsOpenURLInNewTab(url)
  }
}

private struct AutofillManagementContextKey: EnvironmentKey {
  static var defaultValue: AutofillManagementContext? { nil }
}

extension EnvironmentValues {
  /// Context for autofill views (auth, URL opening, etc.).
  /// Set at the root of the autofill flow so child views can access it.
  var autofillManagementContext: AutofillManagementContext? {
    get { self[AutofillManagementContextKey.self] }
    set { self[AutofillManagementContextKey.self] = newValue }
  }
}
