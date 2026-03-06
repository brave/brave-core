// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings

@Observable
final class ManagePasswordDetailViewModel {

  enum Mode {
    // TODO: Implement add mode for creating new credentials
    // case add
    // TODO: Implement edit mode for modifying existing credentials
    // case edit(CWVPassword)
    case view(CWVPassword)
  }

  var site: String
  var username: String
  var passwordValue: String

  var isBlocked: Bool {
    switch mode {
    case .view(let password): return password.isBlocked
    }
  }

  let mode: Mode
  private let autofillDataManager: CWVAutofillDataManager

  init(mode: Mode, autofillDataManager: CWVAutofillDataManager) {
    self.mode = mode
    self.autofillDataManager = autofillDataManager
    switch mode {
    case .view(let password):
      site = password.site
      username = password.username ?? ""
      passwordValue = password.password ?? ""
    }
  }

  /// Saves edits to the password. Sends the update to CWVAutofillDataManager.
  /// - Returns: True if an update was performed.
  @discardableResult
  func savePassword() -> Bool {
    switch mode {
    case .view(let password):
      let hasChanges = username != password.username || passwordValue != (password.password ?? "")
      guard hasChanges else { return false }
      autofillDataManager.update(
        password,
        newUsername: username,
        newPassword: passwordValue,
        timestamp: Date()
      )
      return true
    }
  }

  /// Deletes the password via CWVAutofillDataManager.
  func deletePassword() {
    switch mode {
    case .view(let password):
      autofillDataManager.delete(password)
    }
  }
}
