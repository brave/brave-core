// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import SwiftUI

struct ManagePasswordDetailContainerView: View {
  /// How the password detail screen is opened: view/edit an existing login, or add a new one.
  enum Mode {
    /// Opens the detail screen to view or edit an existing saved login.
    case edit(CWVPassword)
    /// Opens the detail screen to add a new login, optionally pre-seeded with a draft.
    case add(ManagePasswordDraft)
  }

  @Environment(\.dismiss) private var dismiss
  @Environment(\.editMode) private var editMode
  @Environment(\.redactionReasons) private var redactionReasons

  @State private var isPasswordRevealed = false
  @State private var passwordDraft: ManagePasswordDraft = .init()

  let viewModel: ManagePasswordsViewModel
  let mode: Mode
  let redactedTitle = Strings.Autofill.managePasswordsTitle

  /// The existing password being edited, or `nil` in add mode.
  var password: CWVPassword? {
    if case .edit(let password) = mode { return password }
    return nil
  }

  /// The pre-seeded draft for a new login, or `nil` in edit mode.
  private var initialDraft: ManagePasswordDraft? {
    if case .add(let draft) = mode { return draft }
    return nil
  }

  private var navigationTitle: String {
    guard !redactionReasons.contains(.privacy) else { return redactedTitle }

    switch mode {
    case .edit(let password):
      return URL(string: password.site)?.baseDomain ?? password.title
    case .add:
      return Strings.Autofill.managePasswordDetailAddCredentialTitle
    }
  }

  private var isAddMode: Bool {
    initialDraft != nil
  }

  private var isEditMode: Bool {
    editMode?.wrappedValue == .active
  }

  private var shouldDisableSiteField: Bool {
    guard let initialDraft else { return isEditMode }
    return !initialDraft.site.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty
  }

  private func deletePassword() {
    guard let password else { return }
    viewModel.deletePasswords([password])
    passwordDraft.reset()
    dismiss()
  }

  var body: some View {
    Group {
      if isEditMode || isAddMode {
        ManagePasswordDetailEditableView(
          isPasswordRevealed: $isPasswordRevealed,
          site: $passwordDraft.site,
          username: $passwordDraft.username,
          passwordValue: $passwordDraft.password,
          isSitePrefilled: shouldDisableSiteField,
          deleteAction: (isEditMode && password != nil) ? deletePassword : nil
        )
        .onAppear {
          if let password {
            passwordDraft.populate(with: password)
          } else if let initialDraft {
            passwordDraft = initialDraft
          }
        }
        .onDisappear {
          // Navigating away (swipe-back, pop, sheet dismiss) discards any in-flight
          // edits. Accidental edits should never reach storage just because the user left the screen.
          passwordDraft.reset()
        }
        .onChange(of: editMode?.wrappedValue) { oldValue, newValue in
          // In edit mode there is no explicit Save button — the EditButton toggles between
          // read-only and editable as afforded by @Environment(\.editMode).
          guard oldValue == .active, newValue == .inactive,
            let password,
            passwordDraft.hasChanges(from: password),
            passwordDraft.isValid
          else { return }
          viewModel.updatePassword(password, with: passwordDraft)
        }
      } else if let password {
        ManagePasswordDetailReadOnlyView(
          isPasswordRevealed: $isPasswordRevealed,
          password: password
        )
      }
    }
    .scrollContentBackground(.hidden)
    .background((Color(.braveGroupedBackground)))
    .foregroundStyle(Color(braveSystemName: .textPrimary))
    .accessibility(hidden: redactionReasons.contains(.privacy) ? true : false)
    .navigationTitle(navigationTitle)
    .navigationBarTitleDisplayMode(.inline)
    .navigationBarBackButtonHidden(isEditMode)
    .toolbar {
      if isAddMode {
        ToolbarItem(placement: .cancellationAction) {
          Button {
            dismiss()
          } label: {
            Label(Strings.CancelString, braveSystemImage: "leo.close")
          }
        }

        ToolbarItem(placement: .confirmationAction) {
          Button {
            viewModel.addPassword(passwordDraft)
            dismiss()
          } label: {
            Label(Strings.saveButtonTitle, braveSystemImage: "leo.check.normal")
          }
          .disabled(!passwordDraft.isValid)
        }
      } else {
        ToolbarItem(placement: .topBarTrailing) {
          EditButton()
        }
      }

      if isEditMode, let password {
        ToolbarItem(placement: .topBarLeading) {
          Button {
            // Dismiss edit mode without applying any changes
            // undo all changes by resetting with the original values
            passwordDraft.populate(with: password)
            editMode?.wrappedValue = .inactive
          } label: {
            Label(Strings.CancelString, braveSystemImage: "leo.close")
          }
        }
      }
    }
    .overlay {
      if redactionReasons.contains(.privacy) { Color(.braveGroupedBackground).ignoresSafeArea() }
    }
  }
}
