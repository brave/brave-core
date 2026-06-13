// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import SwiftUI

/// Drives add password sheet presentation so each presentation carries its own prefilled site.
struct ManagePasswordAddPresentation: Identifiable {
  let id = UUID()
  let prefilledSite: String
}

struct ManagePasswordAddView: View {
  @Environment(\.dismiss) private var dismiss
  @Environment(\.redactionReasons) private var redactionReasons

  @FocusState private var focusedField: ManagePasswordCredentialFields.Field?
  @State private var isPasswordRevealed = false
  @State private var site = ""
  @State private var username = ""
  @State private var password = ""

  let viewModel: ManagePasswordsViewModel
  let prefilledSite: String

  private var isSitePrefilled: Bool {
    !prefilledSite.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty
  }

  private var isValid: Bool {
    viewModel.isValidCredential(username: username, password: password, site: site)
  }

  init(viewModel: ManagePasswordsViewModel, prefilledSite: String = "") {
    self.viewModel = viewModel
    self.prefilledSite = prefilledSite
  }

  var body: some View {
    Form {
      ManagePasswordCredentialFields(
        site: $site,
        username: $username,
        password: $password,
        isPasswordRevealed: $isPasswordRevealed,
        focusedField: $focusedField,
        isSiteDisabled: isSitePrefilled
      )
    }
    .navigationTitle(Strings.Autofill.managePasswordDetailAddCredentialTitle)
    .navigationBarTitleDisplayMode(.inline)
    .toolbar {
      ToolbarItem(placement: .cancellationAction) {
        Button {
          dismiss()
        } label: {
          Label(Strings.CancelString, braveSystemImage: "leo.close")
        }
      }

      ToolbarItem(placement: .confirmationAction) {
        Button {
          viewModel.addPassword(username: username, password: password, site: site)
          dismiss()
        } label: {
          Label(Strings.saveButtonTitle, braveSystemImage: "leo.check.normal")
        }
        .disabled(!isValid)
      }
    }
    .overlay {
      if redactionReasons.contains(.privacy) { Color(.braveGroupedBackground).ignoresSafeArea() }
    }
    .task(id: prefilledSite) {
      site = prefilledSite
      // SwiftUI's `Form` doesn't reliably accept focus the instant a screen presents — the
      // keyboard can be suppressed if `focusedField` is set before the view is fully mounted
      // and laid out. A short delay lets the presentation transition settle so the field
      // actually becomes first responder and the keyboard rises.
      try? await Task.sleep(for: .milliseconds(500))
      focusedField = isSitePrefilled ? .username : .site
    }
  }
}
