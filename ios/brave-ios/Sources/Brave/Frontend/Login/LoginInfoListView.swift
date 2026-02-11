// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import BraveUI
import LocalAuthentication
import SwiftUI

/// SwiftUI view displaying all login credentials for a selected domain.
/// Each credential is shown on a separate row (username + masked password).
struct LoginInfoListView: View {
  let domain: String
  let credentials: [PasswordForm]
  let passwordAPI: BravePasswordAPI
  let windowProtection: WindowProtection?
  var settingsDelegate: SettingsDelegate?

  var body: some View {
    ZStack {
      Color(.braveGroupedBackground)
        .ignoresSafeArea()
      List {
        Section {
          ForEach(credentials, id: \.credentialRowId) { credential in
            NavigationLink {
              LoginInfoDetailView(
                credential: credential,
                passwordAPI: passwordAPI,
                windowProtection: windowProtection,
                settingsDelegate: settingsDelegate,
                askForAuthentication: makeAskForAuthentication(windowProtection: windowProtection)
              )
            } label: {
              LoginInfoListRow(credential: credential)
            }
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
          }
        }
      }
      .listStyle(.insetGrouped)
      .scrollContentBackground(.hidden)
    }
    .navigationTitle(domain)
    .navigationBarTitleDisplayMode(.inline)
    .toolbarBackground(.visible, for: .navigationBar)
    .toolbar {
      ToolbarItem(placement: .navigationBarTrailing) {
        Button {
          // TODO: Handle Add New credential for this domain
        } label: {
          Image(braveSystemName: "leo.plus.add")
            .foregroundColor(Color(braveSystemName: .primary40))
        }
      }
    }
  }
}

/// A single row displaying username and masked password for a credential.
private struct LoginInfoListRow: View {
  let credential: PasswordForm

  var body: some View {
    VStack(alignment: .leading, spacing: 4) {
      Text(credential.usernameValue ?? "")
        .font(.body)
        .foregroundColor(Color(.braveLabel))

      Text(maskedPassword)
        .font(.subheadline)
        .foregroundColor(Color(.secondaryBraveLabel))
    }
    .frame(maxWidth: .infinity, alignment: .leading)
  }

  private var maskedPassword: String {
    let length = credential.passwordValue?.count ?? 0
    return String(repeating: "•", count: max(8, min(length, 16)))
  }
}

// MARK: - Auth closure for LoginInfoDetailView

private func makeAskForAuthentication(
  windowProtection: WindowProtection?
) -> (@escaping (Bool, LAError.Code?) -> Void) -> Void {
  return { completion in
    guard let windowProtection else {
      completion(false, nil)
      return
    }
    if !windowProtection.isPassCodeAvailable {
      completion(false, .passcodeNotSet)
      return
    }
    windowProtection.presentAuthenticationForViewController(
      determineLockWithPasscode: false,
      viewType: .passwords
    ) { status, error in
      completion(status, error)
    }
  }
}

// MARK: - Credential row identity

extension PasswordForm {
  /// Stable identifier for use in ForEach when displaying credentials in a list.
  fileprivate var credentialRowId: String {
    "\(signOnRealm)_\(usernameValue ?? "")_\(usernameElement ?? "")"
  }
}
