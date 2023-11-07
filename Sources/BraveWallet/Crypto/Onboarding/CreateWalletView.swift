/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import DesignSystem
import Strings
import struct Shared.AppConstants

struct RestorePackage {
  let recoveryWords: [String]
  let onRestoreCompleted: (_ status: Bool, _ validPassword: String) -> Void
  var isLegacyWallet: Bool {
    recoveryWords.count == .legacyWalletRecoveryPhraseNumber
  }
}

struct CreateWalletContainerView: View {
  @ObservedObject var keyringStore: KeyringStore
  var restorePackage: RestorePackage?
  
  init(keyringStore: KeyringStore, restorePackage: RestorePackage? = nil) {
    self.keyringStore = keyringStore
    self.restorePackage = restorePackage
  }
  
  var body: some View {
    ScrollView(.vertical) {
      CreateWalletView(
        keyringStore: keyringStore,
        restorePackage: restorePackage
      )
      .background(Color(.braveBackground))
    }
    .background(Color(.braveBackground).edgesIgnoringSafeArea(.all))
    .transparentNavigationBar(backButtonTitle: Strings.Wallet.createWalletBackButtonTitle, backButtonDisplayMode: .generic)
  }
}

private enum ValidationError: LocalizedError, Equatable {
  case requirementsNotMet
  case inputsDontMatch
  
  var errorDescription: String? {
    switch self {
    case .requirementsNotMet:
      return Strings.Wallet.passwordDoesNotMeetRequirementsError
    case .inputsDontMatch:
      return Strings.Wallet.passwordsDontMatchError
    }
  }
}

private struct CreateWalletView: View {
  @ObservedObject var keyringStore: KeyringStore
  var restorePackage: RestorePackage?

  @State private var password: String = ""
  @State private var repeatedPassword: String = ""
  @State private var validationError: ValidationError?
  @State private var isNewWalletCreated: Bool = false
  @State private var passwordStatus: PasswordStatus = .none
  @State private var isInputsMatch: Bool = false
  
  @FocusState private var isFieldFocused: Bool

  private func createWallet() {
    if let restorePackage {
      // restore wallet with recovery phrases and a new password
      keyringStore.restoreWallet(
        words: restorePackage.recoveryWords,
        password: password,
        isLegacyBraveWallet: restorePackage.isLegacyWallet
      ) { success in
        restorePackage.onRestoreCompleted(success, password)
      }
    } else {
      keyringStore.createWallet(password: password) { mnemonic in
        if let mnemonic, !mnemonic.isEmpty {
          isNewWalletCreated = true
        }
      }
    }
  }

  private func validatePassword() {
    keyringStore.validatePassword(password) { status in
      passwordStatus = status
      if status == .none {
        validationError = nil
      } else if status == .invalid {
        validationError = .requirementsNotMet
      } else {
        if !password.isEmpty && !repeatedPassword.isEmpty {
          if password == repeatedPassword {
            isInputsMatch = true
            validationError = nil
          } else {
            isInputsMatch = false
            validationError = .inputsDontMatch
          }
        } else {
          validationError = nil
        }
      }
    }
  }
  
  private func handleInputChange(_ value: String) {
    validationError = nil
    isInputsMatch = false
    validatePassword()
  }
  
  @ViewBuilder func passwordStatusView(_ status: PasswordStatus) -> some View {
    HStack(spacing: 4) {
      ProgressView(value: status.percentage)
        .tint(status.tintColor)
        .background(status.tintColor.opacity(0.1))
        .frame(width: 52, height: 4)
      Text(status.description)
        .foregroundColor(status.tintColor)
        .font(.footnote)
        .padding(.leading, 20)
    }
  }
  
  func errorLabel(_ error: ValidationError?) -> some View {
    HStack(spacing: 12) {
      Image(braveSystemName: "leo.warning.circle-filled")
        .renderingMode(.template)
        .foregroundColor(Color(.braveLighterOrange))
      Text(error?.errorDescription ?? "")
        .multilineTextAlignment(.leading)
        .font(.callout)
      Spacer()
    }
    .padding(12)
    .background(
      Color(.braveErrorBackground)
        .clipShape(RoundedRectangle(cornerRadius: 10, style: .continuous))
    )
    .hidden(isHidden: error == nil)
  }
  
  private var isContinueDisabled: Bool {
    validationError != nil || password.isEmpty || repeatedPassword.isEmpty ||
    keyringStore.isCreatingWallet || keyringStore.isRestoringWallet
  }

  var body: some View {
    VStack(spacing: 16) {
      VStack {
        Text(Strings.Wallet.createWalletTitle)
          .font(.title)
          .padding(.bottom)
          .multilineTextAlignment(.center)
          .foregroundColor(Color(uiColor: WalletV2Design.textPrimary))
        Text(Strings.Wallet.createWalletSubTitle)
          .font(.subheadline)
          .padding(.bottom)
          .multilineTextAlignment(.center)
          .foregroundColor(Color(uiColor: WalletV2Design.textSecondary))
      }
      VStack(alignment: .leading, spacing: 20) {
        VStack(spacing: 30) {
          VStack(alignment: .leading, spacing: 10) {
            Text(Strings.Wallet.newPasswordPlaceholder)
              .foregroundColor(Color(uiColor: WalletV2Design.textPrimary))
            HStack(spacing: 8) {
              SecureField(Strings.Wallet.newPasswordPlaceholder, text: $password)
                .textContentType(.newPassword)
                .focused($isFieldFocused)
              Spacer()
              if passwordStatus != .none {
                passwordStatusView(passwordStatus)
              }
            }
            Divider()
          }
          VStack(alignment: .leading, spacing: 12) {
            Text(Strings.Wallet.repeatedPasswordPlaceholder)
              .foregroundColor(.primary)
            HStack(spacing: 8) {
              SecureField(Strings.Wallet.repeatedPasswordPlaceholder, text: $repeatedPassword, onCommit: createWallet)
                .textContentType(.newPassword)
              Spacer()
              if isInputsMatch {
                Text("\(Image(braveSystemName: "leo.check.normal")) \(Strings.Wallet.repeatedPasswordMatch)")
                  .multilineTextAlignment(.trailing)
                  .font(.footnote)
                  .foregroundColor(.secondary)
              }
            }
            Divider()
          }
        }
        .font(.subheadline)
        errorLabel(validationError)
      }
      Button(action: createWallet) {
        Text(Strings.Wallet.continueButtonTitle)
          .frame(maxWidth: .infinity)
      }
      .buttonStyle(BraveFilledButtonStyle(size: .large))
      .disabled(isContinueDisabled)
      .padding(.top, 60)
    }
    .padding(.horizontal, 20)
    .padding(.bottom, 20)
    .background(Color(.braveBackground).edgesIgnoringSafeArea(.all))
    .background(
      NavigationLink(
        destination: BackupRecoveryPhraseView(
          password: password,
          keyringStore: keyringStore
        ).navigationBarBackButtonHidden(),
        isActive: $isNewWalletCreated
      ) {
        EmptyView()
      }
    )
    .onChange(of: password, perform: handleInputChange)
    .onChange(of: repeatedPassword, perform: handleInputChange)
    .onAppear {
      isFieldFocused = true
    }
  }
}

#if DEBUG
struct CreateWalletView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      CreateWalletContainerView(keyringStore: .previewStore)
    }
    .previewLayout(.sizeThatFits)
    .previewColorSchemes()
  }
}
#endif
