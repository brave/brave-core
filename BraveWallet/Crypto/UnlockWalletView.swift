/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI
import BraveUI
import struct Shared.Strings
import LocalAuthentication

struct UnlockWalletView: View {
  @ObservedObject var keyringStore: KeyringStore
  
  @State private var password: String = ""
  @State private var unlockError: UnlockError?
  @State private var attemptedBiometricsUnlock: Bool = false
  
  private enum UnlockError: LocalizedError {
    case incorrectPassword
    
    var errorDescription: String? {
      switch self {
      case .incorrectPassword:
        return Strings.Wallet.incorrectPasswordErrorMessage
      }
    }
  }
  
  private var isPasswordValid: Bool {
    !password.isEmpty
  }
  
  private func unlock() {
    // Conflict with the keyboard submit/dismissal that causes a bug
    // with SwiftUI animating the screen away...
    DispatchQueue.main.asyncAfter(deadline: .now() + 0.05) {
      keyringStore.unlock(password: password) { unlocked in
        if !unlocked {
          unlockError = .incorrectPassword
          UIImpactFeedbackGenerator(style: .medium).bzzt()
        }
      }
    }
  }
  
  private func fillPasswordFromKeychain() {
    if let password = KeyringStore.retrievePasswordFromKeychain() {
      self.password = password
      unlock()
    }
  }
  
  private var biometricsIcon: Image? {
    let context = LAContext()
    if context.canEvaluatePolicy(.deviceOwnerAuthenticationWithBiometrics, error: nil) {
      switch context.biometryType {
      case .faceID:
        return Image(systemName: "faceid")
      case .touchID:
        return Image(systemName: "touchid")
      case .none:
        return nil
      @unknown default:
        return nil
      }
    }
    return nil
  }
  
  var body: some View {
    ScrollView(.vertical) {
      VStack(spacing: 46) {
        Image("graphic-lock")
          .padding(.bottom)
        VStack {
          Text(Strings.Wallet.unlockWalletTitle)
            .font(.headline)
            .padding(.bottom)
            .multilineTextAlignment(.center)
            .fixedSize(horizontal: false, vertical: true)
          HStack {
            SecureField(Strings.Wallet.passwordPlaceholder, text: $password, onCommit: unlock)
              .textContentType(.password)
              .font(.subheadline)
              .introspectTextField(customize: { tf in
                tf.becomeFirstResponder()
              })
              .textFieldStyle(BraveValidatedTextFieldStyle(error: unlockError))
            if KeyringStore.isKeychainPasswordStored, let icon = biometricsIcon {
              Button(action: fillPasswordFromKeychain) {
                icon
                  .imageScale(.large)
                  .font(.headline)
              }
            }
          }
          .padding(.horizontal, 48)
        }
        VStack(spacing: 30) {
          Button(action: unlock) {
            Text(Strings.Wallet.unlockWalletButtonTitle)
          }
          .buttonStyle(BraveFilledButtonStyle(size: .normal))
          .disabled(!isPasswordValid)
          NavigationLink(destination: RestoreWalletContainerView(keyringStore: keyringStore)) {
            Text(Strings.Wallet.restoreWalletButtonTitle)
              .font(.subheadline.weight(.medium))
          }
          .foregroundColor(Color(.braveLabel))
        }
      }
      .frame(maxHeight: .infinity, alignment: .top)
      .padding()
      .padding(.vertical)
    }
    .navigationTitle(Strings.Wallet.cryptoTitle)
    .navigationBarTitleDisplayMode(.inline)
    .background(Color(.braveBackground).edgesIgnoringSafeArea(.all))
    .onChange(of: password) { _ in
      unlockError = nil
    }
    .onAppear {
      DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) { [self] in
        if !keyringStore.lockedManually && !attemptedBiometricsUnlock && keyringStore.keyring.isLocked {
          attemptedBiometricsUnlock = true
          fillPasswordFromKeychain()
        }
      }
    }
  }
}

#if DEBUG
struct CryptoUnlockView_Previews: PreviewProvider {
  static var previews: some View {
    UnlockWalletView(keyringStore: .previewStore)
      .previewLayout(.sizeThatFits)
      .previewColorSchemes()
  }
}
#endif
