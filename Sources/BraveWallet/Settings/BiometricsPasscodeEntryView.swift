// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import DesignSystem
import Strings
import SwiftUI

struct BiometricsPasscodeEntryView: View {
  
  @ObservedObject var keyringStore: KeyringStore
  
  private enum UnlockError: LocalizedError {
    case incorrectPassword
    
    var errorDescription: String? {
      switch self {
      case .incorrectPassword:
        return Strings.Wallet.incorrectPasswordErrorMessage
      }
    }
  }
  
  @State private var password: String = ""
  /// Error occured when user entered their password
  @State private var unlockError: UnlockError?
  /// Flag used to determine if we should show an error alert because we failed to store the password in the keychain
  @State private var isShowingKeychainError: Bool = false
  @Environment(\.presentationMode) @Binding private var presentationMode
  
  private var isPasswordValid: Bool {
    !password.isEmpty
  }
  
  private func unlock() {
    keyringStore.validate(password: password) { isValid in
      if isValid {
        // store password in keychain
        if case let status = keyringStore.storePasswordInKeychain(password),
           status != errSecSuccess {
          self.isShowingKeychainError = true
        } else {
          // password stored in keychain, dismiss modal
          presentationMode.dismiss()
        }
      } else {
        // Conflict with the keyboard submit/dismissal that causes a bug
        // with SwiftUI animating the screen away...
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.05) {
          unlockError = .incorrectPassword
          UIImpactFeedbackGenerator(style: .medium).bzzt()
        }
      }
    }
  }
  
  var body: some View {
    NavigationView {
      ScrollView(.vertical) {
        VStack(spacing: 36) {
          Image("graphic-lock", bundle: .current)
            .accessibilityHidden(true)
          VStack {
            Text(Strings.Wallet.enterPasswordForBiometricsTitle)
              .font(.headline)
              .padding(.bottom)
              .multilineTextAlignment(.center)
              .fixedSize(horizontal: false, vertical: true)
            SecureField(Strings.Wallet.passwordPlaceholder, text: $password, onCommit: unlock)
              .textContentType(.password)
              .font(.subheadline)
              .introspectTextField(customize: { tf in
                tf.becomeFirstResponder()
              })
              .textFieldStyle(BraveValidatedTextFieldStyle(error: unlockError))
              .padding(.horizontal, 48)
          }
          Button(action: unlock) {
            Text(Strings.Wallet.saveButtonTitle)
          }
          .buttonStyle(BraveFilledButtonStyle(size: .normal))
          .disabled(!isPasswordValid)
          .frame(maxWidth: .infinity)
          .listRowInsets(.zero)
        }
        .padding()
        .padding(.vertical)
        .navigationBarTitleDisplayMode(.inline)
        .navigationTitle(Strings.Wallet.enterPasswordForBiometricsNavTitle)
        .alert(isPresented: $isShowingKeychainError) {
          Alert(
            title: Text(Strings.Wallet.biometricsSetupErrorTitle),
            message: Text(Strings.Wallet.biometricsSetupErrorMessage),
            dismissButton: .default(Text(Strings.OKString))
          )
        }
      }
    }
  }
}

#if DEBUG
struct BiometricsPasscodeEntryView_Previews: PreviewProvider {
  static var previews: some View {
    BiometricsPasscodeEntryView(
      keyringStore: .previewStore
    )
  }
}
#endif
