// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import BraveUI
import LocalAuthentication
import SwiftUI
import UIKit

/// Displays Sites (URL), Username, Password (with reveal toggle), Created date, and Delete.
struct LoginInfoDetailView: View {
  let windowProtection: WindowProtection?
  var settingsDelegate: SettingsDelegate?
  var askForAuthentication: (@escaping (Bool, LAError.Code?) -> Void) -> Void

  @State private var viewModel: LoginInfoDetailViewModel
  @State private var isEditing = false
  @State private var editedUsername: String = ""
  @State private var editedPassword: String = ""
  @State private var isPasswordRevealed = false
  @State private var showDeleteAlert = false
  @State private var showSetPasscodeAlert = false
  @Environment(\.dismiss) private var dismiss

  init(
    credential: PasswordForm,
    passwordAPI: BravePasswordAPI,
    windowProtection: WindowProtection?,
    settingsDelegate: SettingsDelegate?,
    askForAuthentication: @escaping (@escaping (Bool, LAError.Code?) -> Void) -> Void
  ) {
    self.windowProtection = windowProtection
    self.settingsDelegate = settingsDelegate
    self.askForAuthentication = askForAuthentication
    self._viewModel = State(
      initialValue: LoginInfoDetailViewModel(credential: credential, passwordAPI: passwordAPI)
    )
  }

  private var currentCredential: PasswordForm {
    viewModel.credential
  }

  private var domainTitle: String {
    URL(string: currentCredential.signOnRealm)?.baseDomain ?? ""
  }

  private var formattedCreationDate: String {
    let formatter = DateFormatter()
    formatter.locale = .current
    formatter.dateFormat = "EEEE, MMM d, yyyy"
    return formatter.string(from: currentCredential.dateCreated ?? Date())
  }

  var body: some View {
    ZStack {
      Color(.braveGroupedBackground)
        .ignoresSafeArea()

      ScrollView {
        VStack(spacing: 0) {
          VStack(alignment: .leading, spacing: 0) {
            detailRow(
              label: Strings.Login.loginInfoDetailsWebsiteFieldTitle,
              value: currentCredential.signOnRealm,
              editable: false
            )
            .contentShape(Rectangle())
            .contextMenu {
              Button {
                UIPasteboard.general.string = currentCredential.signOnRealm
              } label: {
                Label(Strings.menuItemCopyTitle, braveSystemImage: "leo.copy")
              }
              Button {
                settingsDelegate?.settingsOpenURLInNewTab(currentCredential.url)
                dismiss()
              } label: {
                Label(Strings.openWebsite, braveSystemImage: "leo.discover")
              }
            }
            divider

            detailRow(
              label: Strings.Login.loginInfoDetailsUsernameFieldTitle,
              value: isEditing ? editedUsername : (currentCredential.usernameValue ?? ""),
              editable: isEditing,
              textBinding: $editedUsername,
              keyboardType: .emailAddress
            )
            .contentShape(Rectangle())
            .contextMenu {
              if !isEditing, let username = currentCredential.usernameValue {
                Button {
                  UIPasteboard.general.string = username
                } label: {
                  Label(Strings.menuItemCopyTitle, braveSystemImage: "leo.copy")
                }
              }
            }
            divider

            detailRow(
              label: Strings.Login.loginInfoDetailsPasswordFieldTitle,
              value: isEditing
                ? editedPassword
                : (isPasswordRevealed
                  ? (currentCredential.passwordValue ?? "")
                  : maskedPassword(currentCredential.passwordValue)),
              editable: isEditing,
              textBinding: $editedPassword,
              isSecure: !isEditing && !isPasswordRevealed,
              showRevealToggle: !isEditing,
              onRevealToggle: { togglePasswordReveal() }
            )
            .contentShape(Rectangle())
            .contextMenu {
              if !isEditing {
                Button {
                  askForAuthentication { success, error in
                    if success, let password = currentCredential.passwordValue {
                      let expireDate = Date().addingTimeInterval(5 * 60)
                      UIPasteboard.general.setItems(
                        [[UIPasteboard.typeAutomatic: password]],
                        options: [UIPasteboard.OptionsKey.expirationDate: expireDate]
                      )
                    } else if error == .passcodeNotSet {
                      showSetPasscodeAlert = true
                    }
                  }
                } label: {
                  Label(Strings.menuItemCopyTitle, braveSystemImage: "leo.copy")
                }
              }
            }
          }
          .background(Color(.secondaryBraveGroupedBackground))
          .clipShape(RoundedRectangle(cornerRadius: 12, style: .continuous))

          // Created date
          Text(String(format: Strings.Login.loginInfoCreatedHeaderTitle, formattedCreationDate))
            .font(.subheadline)
            .foregroundColor(Color(.secondaryBraveLabel))
            .frame(maxWidth: .infinity)
            .padding(.vertical, 20)
        }
        .padding()
      }
    }
    .navigationTitle(domainTitle)
    .navigationBarTitleDisplayMode(.inline)
    .toolbarBackground(.visible, for: .navigationBar)
    .toolbar {
      ToolbarItem(placement: .navigationBarTrailing) {
        if currentCredential.isBlockedByUser {
          EmptyView()
        } else if isEditing {

          Button {
            doneEditing()
          } label: {
            Image(braveSystemName: "leo.check.normal")
              .resizable()
              .scaledToFit()
              .foregroundColor(Color(braveSystemName: .primary40))
          }
        } else {
          Button(Strings.edit) {
            startEditing()
          }
          .foregroundColor(Color(braveSystemName: .primary40))
        }
      }
    }
    .onAppear {
      editedUsername = currentCredential.usernameValue ?? ""
      editedPassword = currentCredential.passwordValue ?? ""
      viewModel.startObserving()
    }
    .onDisappear {
      viewModel.stopObserving()
    }
    .alert(Strings.deleteLoginAlertTitle, isPresented: $showDeleteAlert) {
      Button(Strings.deleteLoginButtonTitle, role: .destructive) {
        viewModel.passwordAPI.removeLogin(currentCredential)
        dismiss()
      }
      Button(Strings.cancelButtonTitle, role: .cancel) {}
    } message: {
      Text(Strings.Login.loginEntryDeleteAlertMessage)
    }
    .alert(Strings.Login.loginInfoSetPasscodeAlertTitle, isPresented: $showSetPasscodeAlert) {
      Button(Strings.OKString, role: .cancel) {}
    } message: {
      Text(Strings.Login.loginInfoSetPasscodeAlertDescription)
    }
  }

  private var divider: some View {
    Divider()
      .background(Color(.braveSeparator))
      .padding(.leading, 16)
  }

  private func detailRow(
    label: String,
    value: String,
    editable: Bool,
    textBinding: Binding<String>? = nil,
    keyboardType: UIKeyboardType = .default,
    isSecure: Bool = false,
    showRevealToggle: Bool = false,
    onRevealToggle: (() -> Void)? = nil
  ) -> some View {
    HStack(alignment: .top, spacing: 12) {
      VStack(alignment: .leading, spacing: 4) {
        Text(label)
          .font(.footnote)
          .foregroundColor(Color(.secondaryBraveLabel))

        if editable, let binding = textBinding {
          TextField("", text: binding)
            .font(.callout)
            .foregroundColor(Color(.braveLabel))
            .keyboardType(keyboardType)
            .textInputAutocapitalization(.never)
            .autocorrectionDisabled()
        } else {
          Text(value)
            .font(.callout)
            .foregroundColor(Color(.braveLabel))
        }
      }
      .frame(maxWidth: .infinity, alignment: .leading)

      if showRevealToggle {
        Button {
          onRevealToggle?()
        } label: {
          Image(braveSystemName: isSecure ? "leo.eye.off" : "leo.eye.on")
            .foregroundColor(Color(braveSystemName: .primary40))
        }
      }
    }
    .frame(maxWidth: .infinity, minHeight: 44, alignment: .leading)
    .padding(16)
    .opacity(isEditing && !editable ? 0.5 : 1)
  }

  private func maskedPassword(_ value: String?) -> String {
    let count = value?.count ?? 0
    return String(repeating: "•", count: max(8, min(count, 20)))
  }

  private func startEditing() {
    askForAuthentication { [self] success, error in
      guard success else {
        if error == .passcodeNotSet {
          showSetPasscodeAlert = true
        }
        return
      }
      isEditing = true
      editedUsername = currentCredential.usernameValue ?? ""
      editedPassword = currentCredential.passwordValue ?? ""
    }
  }

  private func doneEditing() {
    let username = editedUsername
    let password = editedPassword
    guard username != currentCredential.usernameValue || password != currentCredential.passwordValue
    else {
      isEditing = false
      return
    }
    if let oldCredentials = currentCredential.copy() as? PasswordForm {
      viewModel.credential.update(username, passwordValue: password)
      viewModel.passwordAPI.updateLogin(viewModel.credential, oldPasswordForm: oldCredentials)
    }
    isEditing = false
  }

  private func togglePasswordReveal() {
    if isPasswordRevealed {
      isPasswordRevealed = false
    } else {
      askForAuthentication { [self] success, error in
        if success {
          isPasswordRevealed = true
        } else if error == .passcodeNotSet {
          showSetPasscodeAlert = true
        }
      }
    }
  }
}

// MARK: - View model (credential + password store observer)

@Observable
final class LoginInfoDetailViewModel {
  var credential: PasswordForm
  let passwordAPI: BravePasswordAPI
  private var passwordStoreListener: PasswordStoreListener?
  private var isEditing: Bool = false

  init(credential: PasswordForm, passwordAPI: BravePasswordAPI) {
    self.credential = credential
    self.passwordAPI = passwordAPI
  }

  func startObserving() {
    guard passwordStoreListener == nil else { return }
    passwordStoreListener = passwordAPI.add(
      PasswordStoreStateObserver { [weak self] stateChange in
        guard let self else { return }
        switch stateChange {
        case .passwordFormsChanged(let formList):
          let observed = formList.first {
            $0.signOnRealm == self.credential.signOnRealm
              && $0.usernameElement == self.credential.usernameElement
          }
          if let form = observed {
            DispatchQueue.main.async {
              self.credential = form
            }
          }
        default:
          break
        }
      }
    )
  }

  func stopObserving() {
    if let listener = passwordStoreListener {
      passwordAPI.removeObserver(listener)
      passwordStoreListener = nil
    }
  }
}
