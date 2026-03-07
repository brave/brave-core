// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore

@MainActor
@Observable
final class ManagePasswordGroupViewModel {
  let domain: String
  private(set) var passwords: [CWVPassword]
  let autofillDataManager: CWVAutofillDataManager

  var isEmpty: Bool {
    passwords.isEmpty
  }

  init(domain: String, passwords: [CWVPassword], autofillDataManager: CWVAutofillDataManager) {
    self.domain = domain
    self.passwords = passwords
    self.autofillDataManager = autofillDataManager
  }

  /// Deletes a single credential from the group.
  func delete(_ credential: CWVPassword) {
    autofillDataManager.delete(credential)
    passwords.removeAll { $0.identifier == credential.identifier }
  }

  /// Deletes all credentials matching the given set of identifiers.
  func deleteCredentials(forIds ids: Set<String>) {
    let toDelete = passwords.filter { ids.contains($0.identifier) }
    toDelete.forEach { autofillDataManager.delete($0) }
    passwords.removeAll { ids.contains($0.identifier) }
  }
}
