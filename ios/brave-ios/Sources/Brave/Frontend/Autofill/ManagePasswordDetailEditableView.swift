// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import SwiftUI

struct ManagePasswordDetailEditableView: View {
  enum Field { case site, username, password }
  typealias DeleteExistingPasswordAction = () -> Void

  @FocusState private var focusedField: Field?
  @Binding var isPasswordRevealed: Bool
  @Binding var site: String
  @Binding var username: String
  @Binding var passwordValue: String
  @State private var isDeleteDialogPresented = false

  var isSitePrefilled: Bool = false
  var deleteAction: DeleteExistingPasswordAction? = nil
  var body: some View {
    Form {
      Section {
        LabeledContent {
          TextField("", text: $site)
            .textContentType(.URL)
            .textInputAutocapitalization(.never)
            .autocorrectionDisabled()
            .accessibilityLabel(Strings.Login.loginInfoDetailsWebsiteFieldTitle)
            .disabled(isSitePrefilled)
            .multilineTextAlignment(.trailing)
            .focused($focusedField, equals: .site)
            .foregroundStyle(
              Color(braveSystemName: isSitePrefilled ? .textTertiary : .textSecondary)
            )
        } label: {
          Text(Strings.Login.loginInfoDetailsWebsiteFieldTitle)
            .foregroundStyle(Color(braveSystemName: .textPrimary))
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))

        LabeledContent {
          TextField("", text: $username)
            .textContentType(.username)
            .textInputAutocapitalization(.never)
            .autocorrectionDisabled()
            .accessibilityLabel(Strings.Login.loginInfoDetailsUsernameFieldTitle)
            .multilineTextAlignment(.trailing)
            .focused($focusedField, equals: .username)
            .foregroundStyle(Color(braveSystemName: .textSecondary))
        } label: {
          Text(Strings.Login.loginInfoDetailsUsernameFieldTitle)
            .foregroundStyle(Color(braveSystemName: .textPrimary))
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))

        LabeledContent {
          HStack(spacing: 8) {
            Group {
              if isPasswordRevealed {
                TextField(
                  Strings.Autofill.managePasswordDetailInputPasswordPlaceholder,
                  text: $passwordValue
                )
                .textContentType(.password)
                .textInputAutocapitalization(.never)
                .autocorrectionDisabled()
                .accessibilityLabel(Strings.Login.loginInfoDetailsPasswordFieldTitle)
              } else {
                SecureField(
                  Strings.Autofill.managePasswordDetailInputPasswordPlaceholder,
                  text: $passwordValue
                )
                .textContentType(.password)
                .accessibilityLabel(Strings.Login.loginInfoDetailsPasswordFieldTitle)
              }
            }
            .multilineTextAlignment(.trailing)
            .focused($focusedField, equals: .password)
            .foregroundStyle(Color(braveSystemName: .textSecondary))

            Button {
              isPasswordRevealed.toggle()
            } label: {
              Label(
                Strings.Autofill.managePasswordDetailRevealPassword,
                braveSystemImage: isPasswordRevealed ? "leo.eye.on" : "leo.eye.off"
              )
              .foregroundStyle(Color(braveSystemName: .iconInteractive))
              .labelStyle(.iconOnly)
            }
            .buttonStyle(.plain)
          }
        } label: {
          Text(Strings.Login.loginInfoDetailsPasswordFieldTitle)
            .foregroundStyle(Color(braveSystemName: .textPrimary))
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      if let deleteAction {
        Section {
          Button {
            isDeleteDialogPresented = true
          } label: {
            Label(
              Strings.Autofill.managePasswordsDeleteCredentialButtonTitle,
              braveSystemImage: "leo.trash"
            )
            .labelStyle(.titleOnly)
          }
          .foregroundStyle(
            Color(
              braveSystemName: .systemfeedbackErrorVibrant
            )
          )
          .confirmationDialog(
            Strings.Autofill.managePasswordsDeleteCredentialsAlertTitle,
            isPresented: $isDeleteDialogPresented
          ) {
            Button(Strings.CancelString, role: .cancel) {}
            Button(
              Strings.Autofill.managePasswordsDeleteCredentialButtonTitle,
              role: .destructive
            ) {
              deleteAction()
            }
          } message: {
            Text(Strings.Autofill.managePasswordDetailDeleteConfirmMessage)
          }
        }
      }
    }
    .task {
      // SwiftUI's `Form` doesn't reliably accept focus the instant a screen presents — the
      // keyboard can be suppressed if `focusedField` is set before the view is fully mounted
      // and laid out. A short delay lets the presentation transition settle so the field
      // actually becomes first responder and the keyboard rises.
      try? await Task.sleep(for: .milliseconds(500))
      focusedField = isSitePrefilled ? .username : .site
    }
  }
}
