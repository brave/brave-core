// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import DesignSystem
import Foundation
import Strings
import SwiftUI

import struct Shared.AppConstants

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

struct CreateWalletView: View {
  @ObservedObject var keyringStore: KeyringStore
  let setupOption: OnboardingSetupOption
  let onValidPasswordEntered: ((_ validPassword: String) -> Void)?
  // Used to dismiss all of Wallet
  let dismissAction: () -> Void

  @State private var password: String = ""
  @State private var repeatedPassword: String = ""
  @State private var validationError: ValidationError?
  @State private var isNewWalletCreated: Bool = false
  @State private var passwordStatus: PasswordStatus = .none
  @State private var isInputsMatch: Bool = false
  /// If this view is showing `Creating Wallet...` overlay, blocking input fields.
  /// Using a local flag for the view instead of `keyringStore.isCreatingWallet` so we
  /// only show `CreatingWalletView` on the `RestoreWalletView` when restoring.
  @State private var isShowingCreatingWallet: Bool = false

  @FocusState private var isFieldFocused: Bool

  init(
    keyringStore: KeyringStore,
    setupOption: OnboardingSetupOption,
    onValidPasswordEntered: ((_ validPassword: String) -> Void)? = nil,
    dismissAction: @escaping () -> Void
  ) {
    self.keyringStore = keyringStore
    self.setupOption = setupOption
    self.onValidPasswordEntered = onValidPasswordEntered
    self.dismissAction = dismissAction
  }

  private func createWallet() {
    switch setupOption {
    case .new:
      isShowingCreatingWallet = true
      keyringStore.createWallet(password: password) { mnemonic in
        defer { self.isShowingCreatingWallet = false }
        if let mnemonic, !mnemonic.isEmpty {
          isNewWalletCreated = true
        }
      }
    case .restore:
      if isInputsMatch {
        onValidPasswordEntered?(password)
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

  private func errorLabel(_ error: ValidationError?) -> some View {
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
    validationError != nil || password.isEmpty || repeatedPassword.isEmpty
      || keyringStore.isCreatingWallet || keyringStore.isRestoringWallet
  }

  var body: some View {
    ScrollView(.vertical) {
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
                SecureField(
                  Strings.Wallet.repeatedPasswordPlaceholder,
                  text: $repeatedPassword,
                  onCommit: createWallet
                )
                .textContentType(.newPassword)
                Spacer()
                if isInputsMatch {
                  Text(
                    "\(Image(braveSystemName: "leo.check.normal")) \(Strings.Wallet.repeatedPasswordMatch)"
                  )
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
    }
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
    .navigationBarBackButtonHidden(isShowingCreatingWallet)
    .frame(maxWidth: .infinity, maxHeight: .infinity)
    .overlay {
      if isShowingCreatingWallet {
        CreatingWalletView()
          .ignoresSafeArea()
          .frame(maxWidth: .infinity, maxHeight: .infinity)
      }
    }
    .toolbar(content: {
      ToolbarItem(placement: .navigationBarLeading) {
        if isShowingCreatingWallet {
          Button(action: dismissAction) {  // dismiss all of wallet
            Image("wallet-dismiss", bundle: .module)
              .renderingMode(.template)
              .foregroundColor(Color(.braveBlurpleTint))
          }
        }
      }
    })
    .onAppear {
      isFieldFocused = true
      keyringStore.reportP3AOnboarding(action: .legalAndPassword)
    }
    .transparentNavigationBar(
      backButtonTitle: Strings.Wallet.createWalletBackButtonTitle,
      backButtonDisplayMode: .generic
    )
  }
}

#if DEBUG
struct CreateWalletView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      CreateWalletView(
        keyringStore: .previewStore,
        setupOption: .new,
        dismissAction: {}
      )
    }
    .previewLayout(.sizeThatFits)
    .previewColorSchemes()
  }
}
#endif

/// View shown as an overlay over `CreateWalletView` or `RestoreWalletView`
/// when waiting for Wallet to be created & wallet data files downloaded.
struct CreatingWalletView: View {

  var body: some View {
    VStack(spacing: 24) {
      Spacer()
      ProgressView()
        .progressViewStyle(.braveCircular(size: .normal, tint: .braveBlurpleTint))
      Text(Strings.Wallet.creatingWallet)
        .font(.title)
      Spacer()
    }
    .frame(maxWidth: .infinity, maxHeight: .infinity)
    .background(Color(braveSystemName: .containerBackground))
  }
}
