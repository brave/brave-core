/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI
import BraveUI
import struct Shared.Strings

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
          // TODO: Add haptic buzz here when it fails
        }
      }
    }
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
          SecureField(Strings.Wallet.passwordPlaceholder, text: $password, onCommit: unlock)
            .font(.subheadline)
            .textFieldStyle(BraveValidatedTextFieldStyle(error: unlockError))
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
        if !attemptedBiometricsUnlock && keyringStore.keyring.isLocked {
          attemptedBiometricsUnlock = true
          if let password = KeyringStore.retrievePasswordFromKeychain() {
            self.password = password
            unlock()
          }
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
  }
}
#endif
