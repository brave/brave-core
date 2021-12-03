/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import LocalAuthentication
import BraveUI
import struct Shared.Strings

struct CreateWalletContainerView: View {
  @ObservedObject var keyringStore: KeyringStore
  
  var body: some View {
    ScrollView(.vertical) {
      CreateWalletView(keyringStore: keyringStore)
        .background(Color(.braveBackground))
    }
    .background(Color(.braveBackground).edgesIgnoringSafeArea(.all))
    .navigationTitle(Strings.Wallet.cryptoTitle)
    .navigationBarTitleDisplayMode(.inline)
    .introspectViewController { vc in
      vc.navigationItem.backButtonTitle = Strings.Wallet.createWalletBackButtonTitle
      vc.navigationItem.backButtonDisplayMode = .minimal
    }
  }
}

private struct CreateWalletView: View {
  @ObservedObject var keyringStore: KeyringStore
  
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
  
  @State private var password: String = ""
  @State private var repeatedPassword: String = ""
  @State private var validationError: ValidationError?
  @State private var isShowingBiometricsPrompt: Bool = false
  @State private var isSkippingBiometricsPrompt: Bool = false
  
  private func createWallet() {
    validate { success in
      if !success {
        return
      }
      keyringStore.createWallet(password: password) { mnemonic in
        if !mnemonic.isEmpty {
          if isBiometricsAvailable {
            isShowingBiometricsPrompt = true
          } else {
            isSkippingBiometricsPrompt = true
          }
        }
      }
    }
  }
  
  private var isBiometricsAvailable: Bool {
    LAContext().canEvaluatePolicy(.deviceOwnerAuthenticationWithBiometrics, error: nil)
  }
  
  private func validate(_ completion: @escaping (Bool) -> Void) {
    keyringStore.isStrongPassword(password) { isValidPassword in
      if !isValidPassword {
        validationError = .requirementsNotMet
      } else if password != repeatedPassword {
        validationError = .inputsDontMatch
      } else {
        validationError = nil
      }
      completion(validationError == nil)
    }
  }
  
  private func handlePasswordChanged(_ value: String) {
    if validationError == .requirementsNotMet {
      // Reset validation on user changing
      validationError = nil
    }
  }
  
  private func handleRepeatedPasswordChanged(_ value: String) {
    if validationError == .inputsDontMatch {
      // Reset validation on user changing
      validationError = nil
    }
  }
  
  var body: some View {
    VStack(spacing: 0) {
      VStack(spacing: 46) {
        Image("graphic-lock")
          .padding(.bottom)
        VStack {
          Text(Strings.Wallet.createWalletTitle)
            .font(.headline)
            .padding(.bottom)
            .multilineTextAlignment(.center)
            .fixedSize(horizontal: false, vertical: true)
          VStack {
            SecureField(Strings.Wallet.passwordPlaceholder, text: $password)
              .textContentType(.newPassword)
              .textFieldStyle(BraveValidatedTextFieldStyle(error: validationError, when: .requirementsNotMet))
            SecureField(Strings.Wallet.repeatedPasswordPlaceholder, text: $repeatedPassword, onCommit: createWallet)
              .textContentType(.newPassword)
              .textFieldStyle(BraveValidatedTextFieldStyle(error: validationError, when: .inputsDontMatch))
          }
          .font(.subheadline)
          .padding(.horizontal, 48)
        }
        Button(action: createWallet) {
          Text(Strings.Wallet.continueButtonTitle)
        }
        .buttonStyle(BraveFilledButtonStyle(size: .normal))
      }
      .frame(maxHeight: .infinity, alignment: .top)
      .padding()
      .padding(.vertical)
      .background(BiometricsPromptView(isPresented: $isShowingBiometricsPrompt) { enabled, navController in
        // Store password in keychain
        if enabled, !KeyringStore.storePasswordInKeychain(password) {
          let alert = UIAlertController(
            title: Strings.Wallet.biometricsSetupErrorTitle,
            message: Strings.Wallet.biometricsSetupErrorMessage,
            preferredStyle: .alert
          )
          alert.addAction(.init(title: Strings.OKString, style: .default, handler: nil))
          navController?.presentedViewController?.present(alert, animated: true)
          return false
        }
        let controller = UIHostingController(rootView: BackupWalletView(keyringStore: keyringStore))
        navController?.pushViewController(controller, animated: true)
        return true
      })
      .background(
        NavigationLink(
          destination: BackupWalletView(keyringStore: keyringStore),
          isActive: $isSkippingBiometricsPrompt
        ) {
          EmptyView()
        }
      )
      .onChange(of: password, perform: handlePasswordChanged)
      .onChange(of: repeatedPassword, perform: handleRepeatedPasswordChanged)
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
