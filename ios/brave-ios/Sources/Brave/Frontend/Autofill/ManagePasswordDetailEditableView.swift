// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import SwiftUI

struct ManagePasswordDetailEditableView: View {
  @Environment(\.dismiss) private var dismiss

  @Binding var isPasswordRevealed: Bool
  @FocusState private var focusedField: ManagePasswordCredentialFields.Field?
  @State private var site: String
  @State private var username: String
  @State private var passwordValue: String
  @State private var isDeleteDialogPresented = false

  let viewModel: ManagePasswordsViewModel
  let password: CWVPassword
  let onFinishEditing: () -> Void

  init(
    isPasswordRevealed: Binding<Bool>,
    viewModel: ManagePasswordsViewModel,
    password: CWVPassword,
    onFinishEditing: @escaping () -> Void
  ) {
    _isPasswordRevealed = isPasswordRevealed
    self.viewModel = viewModel
    self.password = password
    self.onFinishEditing = onFinishEditing
    _site = State(initialValue: password.site)
    _username = State(initialValue: password.username ?? "")
    _passwordValue = State(initialValue: password.password ?? "")
  }

  private var isValid: Bool {
    viewModel.isValidCredential(username: username, password: passwordValue, site: password.site)
  }

  private var hasChanges: Bool {
    username != (password.username ?? "") || passwordValue != (password.password ?? "")
  }

  private func resetFields() {
    site = password.site
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
        site: $site,
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
    .toolbar {
      ToolbarItem(placement: .cancellationAction) {
        Button {
          resetFields()
          onFinishEditing()
        } label: {
          Label(Strings.CancelString, braveSystemImage: "leo.close")
        }
      }

      ToolbarItem(placement: .confirmationAction) {
        Button {
          if hasChanges, isValid {
            viewModel.updatePassword(password, username: username, passwordValue: passwordValue)
          }
          onFinishEditing()
        } label: {
          Label(Strings.saveButtonTitle, braveSystemImage: "leo.check.normal")
        }
        .disabled(!isValid)
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
  }
}
