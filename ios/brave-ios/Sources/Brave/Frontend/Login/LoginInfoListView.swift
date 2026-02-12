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
  @Environment(\.dismiss) private var dismiss
  let domain: String
  let credentials: [PasswordForm]
  let passwordAPI: BravePasswordAPI
  let windowProtection: WindowProtection?
  var settingsDelegate: SettingsDelegate?

  @State private var isEditMode: Bool = false
  @State private var selectedCredentialIds: Set<String> = []
  @State private var showDeleteConfirmation: Bool = false

  var body: some View {
    ZStack {
      Color(.braveGroupedBackground)
        .ignoresSafeArea()
      List {
        Section {
          ForEach(credentials, id: \.credentialRowId) { credential in
            LoginInfoCredentialRow(
              credential: credential,
              isEditMode: isEditMode,
              isSelected: selectedCredentialIds.contains(credential.credentialRowId),
              onToggleSelection: { toggleSelection(credential.credentialRowId) },
              passwordAPI: passwordAPI,
              windowProtection: windowProtection,
              settingsDelegate: settingsDelegate
            )
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
      toolbarContent
    }
    .alert(Strings.Autofill.deleteLoginAlertTitle, isPresented: $showDeleteConfirmation) {
      Button(Strings.Autofill.deleteLoginAlertCancelActionTitle, role: .cancel) {}
      Button(Strings.Autofill.deleteLoginButtonTitle, role: .destructive) {
        performDeletion()
      }
    } message: {
      Text(
        String.localizedStringWithFormat(
          Strings.Autofill.deleteLoginAlertLocalMessage,
          domain
        )
      )
    }
  }

  @ToolbarContentBuilder
  var toolbarContent: some ToolbarContent {
    ToolbarItem(placement: .navigationBarTrailing) {
      Button {
        // TODO: Handle Add New credential for this domain
      } label: {
        Image(braveSystemName: "leo.plus.add")
          .foregroundColor(Color(braveSystemName: .primary40))
      }
      .disabled(isEditMode)
    }

    ToolbarItemGroup(placement: .bottomBar) {
      if isEditMode {
        Button(Strings.Autofill.deleteLoginButtonTitle) {
          showDeleteConfirmation = true
        }
        .foregroundColor(selectedCredentialIds.isEmpty ? Color(.braveDisabled) : .red)
        .disabled(selectedCredentialIds.isEmpty)
        Spacer()
        Button(Strings.done) {
          updateEditMode(false)
          selectedCredentialIds.removeAll()
        }
        .foregroundColor(Color(.braveBlurpleTint))
      } else {
        Spacer()
        Button(Strings.edit) {
          updateEditMode(true)
        }
      }
    }
  }

  private func updateEditMode(_ isEnabled: Bool) {
    withAnimation(.easeIn(duration: 0.5)) {
      self.isEditMode = isEnabled
    }
  }

  private func toggleSelection(_ credentialId: String) {
    if selectedCredentialIds.contains(credentialId) {
      selectedCredentialIds.remove(credentialId)
    } else {
      selectedCredentialIds.insert(credentialId)
    }
  }

  private func performDeletion() {
    let toRemove = credentials.filter { selectedCredentialIds.contains($0.credentialRowId) }
    for credential in toRemove {
      passwordAPI.removeLogin(credential)
    }
    selectedCredentialIds.removeAll()
    updateEditMode(false)
    dismiss()
  }
}

/// A single credential row: either selectable in edit mode or a navigation link to detail.
private struct LoginInfoCredentialRow: View {
  let credential: PasswordForm
  let isEditMode: Bool
  let isSelected: Bool
  let onToggleSelection: () -> Void
  let passwordAPI: BravePasswordAPI
  let windowProtection: WindowProtection?
  var settingsDelegate: SettingsDelegate?

  private var rowLabel: some View {
    LoginInfoListRow(credential: credential)
  }

  @ViewBuilder
  var body: some View {
    if isEditMode {
      editModeRow
    } else {
      navigationRow
    }
  }

  private var editModeRow: some View {
    Button {
      onToggleSelection()
    } label: {
      HStack(spacing: 12) {
        Image(systemName: isSelected ? "checkmark.circle.fill" : "circle")
          .font(.title2)
          .foregroundColor(isSelected ? Color(.braveBlurpleTint) : Color(.secondaryBraveLabel))
        rowLabel
      }
      .contentShape(Rectangle())
    }
    .buttonStyle(.plain)
  }

  private var navigationRow: some View {
    NavigationLink {
      LoginInfoDetailView(
        credential: credential,
        passwordAPI: passwordAPI,
        windowProtection: windowProtection,
        settingsDelegate: settingsDelegate,
        askForAuthentication: makeAskForAuthentication(windowProtection: windowProtection)
      )
    } label: {
      rowLabel
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
