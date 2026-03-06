// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings

@Observable
final class ManagePasswordDetailViewModel {
  /// The password being viewed/edited.
  let password: CWVPassword
  var username: String
  var passwordValue: String

  private let autofillDataManager: CWVAutofillDataManager

  init(password: CWVPassword, autofillDataManager: CWVAutofillDataManager) {
    self.password = password
    self.username = password.username ?? ""
    self.passwordValue = password.password ?? ""
    self.autofillDataManager = autofillDataManager
  }

  /// Site URL for the password.
  var site: String {
    password.site
  }


  /// Whether this credential is blocked (never-saved).
  var isBlocked: Bool {
    password.isBlocked
  }

  /// Saves edits to the password. Sends the update to CWVAutofillDataManager.
  /// - Parameters:
  ///   - username: New username. Pass nil to leave unchanged.
  ///   - password: New password. Pass nil to leave unchanged.
  /// - Returns: True if an update was performed.
  @discardableResult
  func savePassword() -> Bool {
    let hasChanges =
    username != password.username || passwordValue != (password.password ?? "")
    guard hasChanges else { return false }
    autofillDataManager.update(password, newUsername: username, newPassword: passwordValue, timestamp: Date())
    return true
  }

  /// Deletes the password via CWVAutofillDataManager.
  func deletePassword() {
    autofillDataManager.delete(password)
  }
}
