// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import BraveUI
import SwiftUI
import UIKit

/// How the password detail screen is opened: view/edit an existing login, or add a new one.
enum ManagePasswordDetailContext {
  /// Opens the detail screen to view or edit an existing saved login.
  case edit(CWVPassword)
  /// Opens the detail screen to add a new login, optionally pre-seeded with a draft.
  case add(ManagePasswordDraft)
}

struct ManagePasswordDetailView: View {
  @Environment(\.dismiss) private var dismiss
  @Environment(\.editMode) private var editMode
  @Environment(\.redactionReasons) private var redactionReasons

  @State private var isPasswordRevealed = false
  @State private var passwordDraft: ManagePasswordDraft = .init()

  let viewModel: ManagePasswordsViewModel
  let context: ManagePasswordDetailContext
  let redactedTitle = Strings.Autofill.managePasswordsTitle

  var password: CWVPassword? {
    guard case .edit(let password) = context else { return nil }
    return password
  }

  private var navigationTitle: String {
    guard !redactionReasons.contains(.privacy) else { return redactedTitle }

    switch context {
    case .edit(let password):
      return URL(string: password.site)?.baseDomain ?? password.title
    case .add:
      return Strings.Autofill.managePasswordDetailAddCredentialTitle
    }
  }

  private var isAddMode: Bool {
    guard case .add = context else { return false }
    return true
  }

  private var isEditMode: Bool {
    editMode?.wrappedValue == .active
  }

  private var shouldDisableSiteField: Bool {
    guard case .add(let draft) = context else { return isEditMode }
    return !draft.site.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty
  }

  private func deletePassword() {
    guard case .edit(let password) = context else { return }
    viewModel.deletePasswords([password])
    passwordDraft.reset()
    dismiss()
  }

  var body: some View {
    Group {
      if isEditMode || isAddMode {
        ManagePasswordDetailAddEditView(
          isPasswordRevealed: $isPasswordRevealed,
          site: $passwordDraft.site,
          username: $passwordDraft.username,
          passwordValue: $passwordDraft.password,
          prefilledSite: shouldDisableSiteField,
          deleteAction: isEditMode ? deletePassword : nil
        )
        .onAppear {
          if case .edit(let password) = context {
            passwordDraft.populate(with: password)
          } else if case .add(let draft) = context {
            passwordDraft = draft
          }
        }
        .onDisappear {
          if case .edit(let password) = context, passwordDraft.hasChanges(from: password) {
            viewModel.updatePassword(password, with: passwordDraft)
          } else if isAddMode, passwordDraft.isValid {
            viewModel.addPassword(passwordDraft)
          }
          passwordDraft.reset()
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
            passwordDraft.reset()
            dismiss()
          } label: {
            Label(Strings.CancelString, braveSystemImage: "leo.close")
          }
        }

        ToolbarItem(placement: .confirmationAction) {
          Button {
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

      if isEditMode, case .edit(let password) = context {
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

struct ManagePasswordDetailReadOnlyView: View {
  @Environment(\.openURL) private var openURL
  @Binding var isPasswordRevealed: Bool
  let password: CWVPassword

  var body: some View {
    Form {
      Section {
        LabeledContent {
          Menu {
            Button {
              UIPasteboard.general.string = password.site
            } label: {
              Text(Strings.menuItemCopyTitle)
            }
            Button {
              if let url = URL(string: password.site), url.isWebPage() {
                openURL(url)
              }
            } label: {
              Text(Strings.openWebsite)
            }
          } label: {
            Text(password.site).lineLimit(1)
              .contentShape(.rect)
              .foregroundStyle(Color(braveSystemName: .textSecondary))
          }
        } label: {
          Text(Strings.Login.loginInfoDetailsWebsiteFieldTitle)
            .foregroundStyle(Color(braveSystemName: .textPrimary))
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))

        LabeledContent {
          Menu {
            Button {
              UIPasteboard.general.setSecureString(password.username ?? "")
            } label: {
              Text(Strings.menuItemCopyTitle)
            }
          } label: {
            Text(password.username ?? "")
              .lineLimit(1)
              .contentShape(.rect)
              .foregroundStyle(Color(braveSystemName: .textSecondary))
          }
        } label: {
          Text(Strings.Login.loginInfoDetailsUsernameFieldTitle)
            .foregroundStyle(Color(braveSystemName: .textPrimary))
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))

        LabeledContent {
          HStack(spacing: 8) {
            Menu {
              Button {
                UIPasteboard.general.setSecureString(password.password ?? "")
              } label: {
                Text(Strings.menuItemCopyTitle)
              }
            } label: {
              HStack {
                Spacer()
                if isPasswordRevealed {
                  Text(password.password ?? "")
                    .lineLimit(1)
                } else {
                  Text(String(repeating: "•", count: 8))
                    .lineLimit(1)
                    .allowsHitTesting(false)
                    .accessibility(hidden: true)
                    .multilineTextAlignment(.trailing)
                    .frame(maxWidth: .infinity, alignment: .trailing)
                }
              }
              .contentShape(.rect)
              .foregroundStyle(Color(braveSystemName: .textSecondary))
            }

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
    }
  }
}

struct ManagePasswordDetailAddEditView: View {
  enum Field { case site, username, password }
  typealias DeleteExistingPasswordAction = () -> Void

  @FocusState private var focusedField: Field?
  @Binding var isPasswordRevealed: Bool
  @Binding var site: String
  @Binding var username: String
  @Binding var passwordValue: String
  @State private var isDeleteDialogPresented = false

  var prefilledSite: Bool = false
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
            .disabled(prefilledSite)
            .multilineTextAlignment(.trailing)
            .focused($focusedField, equals: .site)
            .foregroundStyle(Color(braveSystemName: prefilledSite ? .textTertiary : .textSecondary))
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
      try? await Task.sleep(for: .milliseconds(500))
      focusedField = prefilledSite ? .username : .site
    }
  }
}
