// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import SwiftUI

struct ManagePasswordDetailEditableView: View {
  @Environment(\.dismiss) private var dismiss
  @Environment(\.editMode) private var editMode

  @Binding var isPasswordRevealed: Bool
  @FocusState private var focusedField: ManagePasswordCredentialFields.Field?
  @State private var username = ""
  @State private var passwordValue = ""
  @State private var isDeleteDialogPresented = false

  let viewModel: ManagePasswordsViewModel
  let password: CWVPassword

  private var isValid: Bool {
    viewModel.isValidCredential(username: username, password: passwordValue, site: password.site)
  }

  private var hasChanges: Bool {
    username != (password.username ?? "") || passwordValue != (password.password ?? "")
  }

  private func resetFields() {
    username = password.username ?? ""
    passwordValue = password.password ?? ""
  }

  private func deletePassword() {
    viewModel.deletePasswords([password])
    dismiss()
  }

  var body: some View {
    Form {
      ManagePasswordCredentialFields(
        site: .constant(password.site),
        username: $username,
        password: $passwordValue,
        isPasswordRevealed: $isPasswordRevealed,
        focusedField: $focusedField,
        isSiteDisabled: true
      )

      Section {
        Button(role: .destructive) {
          isDeleteDialogPresented = true
        } label: {
          HStack {
            Spacer()
            Text(Strings.Autofill.managePasswordsDeleteCredentialButtonTitle)
            Spacer()
          }
        }
        .confirmationDialog(
          Strings.Autofill.managePasswordsDeleteCredentialsAlertTitle,
          isPresented: $isDeleteDialogPresented
        ) {
          Button(Strings.CancelString, role: .cancel) {}
          Button(
            Strings.Autofill.managePasswordsDeleteCredentialConfirmButtonTitle,
            role: .destructive
          ) {
            deletePassword()
          }
        } message: {
          Text(Strings.Autofill.managePasswordDetailDeleteConfirmMessage)
        }
      }
    }
    .task {
      // SwiftUI's `Form` doesn't reliably accept focus the instant a screen presents — the
      // keyboard can be suppressed if `focusedField` is set before the view is fully mounted
      // and laid out. A short delay lets the presentation transition settle so the field
      // actually becomes first responder and the keyboard rises.
      try? await Task.sleep(for: .milliseconds(500))
      focusedField = .username
    }
    .onAppear {
      resetFields()
    }
    .onChange(of: editMode?.wrappedValue) { oldValue, newValue in
      // In edit mode there is no explicit Save button — the EditButton toggles between
      // read-only and editable as afforded by @Environment(\.editMode).
      guard oldValue == .active, newValue == .inactive, hasChanges, isValid else { return }
      viewModel.updatePassword(password, username: username, passwordValue: passwordValue)
    }
    .toolbar {
      ToolbarItem(placement: .topBarLeading) {
        Button {
          // Dismiss edit mode without applying any changes
          // undo all changes by resetting with the original values
          resetFields()
          editMode?.wrappedValue = .inactive
        } label: {
          Label(Strings.CancelString, braveSystemImage: "leo.close")
        }
      }
    }
  }
}
