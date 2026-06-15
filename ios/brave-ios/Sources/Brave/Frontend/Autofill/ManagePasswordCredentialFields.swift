// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import SwiftUI

struct ManagePasswordCredentialFields: View {
  enum Field { case site, username, password }

  @Binding var site: String
  @Binding var username: String
  @Binding var password: String
  @Binding var isPasswordRevealed: Bool
  @FocusState.Binding var focusedField: Field?

  var isSiteDisabled: Bool

  var body: some View {
    Section {
      LabeledContent {
        Group {
          if isSiteDisabled {
            Text(site)
              .multilineTextAlignment(.trailing)
              .frame(maxWidth: .infinity, alignment: .trailing)
              .foregroundStyle(.tertiary)
          } else {
            TextField("", text: $site)
              .textContentType(.URL)
              .textInputAutocapitalization(.never)
              .autocorrectionDisabled()
              .multilineTextAlignment(.trailing)
              .focused($focusedField, equals: .site)
              .foregroundStyle(.secondary)
          }
        }
        .accessibilityLabel(Strings.Login.loginInfoDetailsWebsiteFieldTitle)
      } label: {
        Text(Strings.Login.loginInfoDetailsWebsiteFieldTitle)
      }

      LabeledContent {
        TextField("", text: $username)
          .textContentType(.username)
          .textInputAutocapitalization(.never)
          .autocorrectionDisabled()
          .accessibilityLabel(Strings.Login.loginInfoDetailsUsernameFieldTitle)
          .multilineTextAlignment(.trailing)
          .focused($focusedField, equals: .username)
          .foregroundStyle(.secondary)
      } label: {
        Text(Strings.Login.loginInfoDetailsUsernameFieldTitle)
      }

      LabeledContent {
        HStack(spacing: 8) {
          Group {
            if isPasswordRevealed {
              TextField(
                Strings.Autofill.managePasswordDetailInputPasswordPlaceholder,
                text: $password
              )
              .textContentType(.password)
              .textInputAutocapitalization(.never)
            } else {
              SecureField(
                Strings.Autofill.managePasswordDetailInputPasswordPlaceholder,
                text: $password
              )
              .textContentType(.password)
            }
          }
          .autocorrectionDisabled()
          .accessibilityLabel(Strings.Login.loginInfoDetailsPasswordFieldTitle)
          .multilineTextAlignment(.trailing)
          .focused($focusedField, equals: .password)
          .foregroundStyle(.secondary)

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
      }
    }
  }
}
