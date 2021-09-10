/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import BraveUI

@available(iOS 14.0, *)
struct RestoreWalletContainerView: View {
  @ObservedObject var keyringStore: KeyringStore
  
  var body: some View {
    UIKitScrollView(axis: .vertical) {
      RestoreWalletView(keyringStore: keyringStore)
        .background(Color(.braveBackground))
    }
    .background(Color(.braveBackground).edgesIgnoringSafeArea(.all))
    .navigationTitle("")
    .navigationBarTitleDisplayMode(.inline)
    .introspectViewController { vc in
      vc.navigationItem.backButtonTitle = "Restore Wallet" // NSLocalizedString
      vc.navigationItem.backButtonDisplayMode = .minimal
    }
  }
}

@available(iOS 14.0, *)
private struct RestoreWalletView: View {
  @ObservedObject var keyringStore: KeyringStore
  
  private enum RestoreWalletError: LocalizedError {
    case invalidPhrase
    case passwordTooShort
    case inputsDontMatch
    
    var errorDescription: String? {
      switch self {
      case .invalidPhrase:
        // TODO: Get real copy
        return "Phrase Invalid" // NSLocalizedString
      case .passwordTooShort:
        return "Password must be 7 or more characters" // NSLocalizedString
      case .inputsDontMatch:
        // TODO: Get real copy
        return "Passwords don't match" // NSLocalizedString
      }
    }
  }
  
  @State private var password: String = ""
  @State private var repeatedPassword: String = ""
  @State private var phrase: String = ""
  @State private var isEditingPhrase: Bool = false
  @State private var showingRecoveryPhase: Bool = false
  @State private var restoreError: RestoreWalletError?
  
  private func validate() -> Bool {
    if phrase.isEmpty {
      restoreError = .invalidPhrase
    } else if password.count < 7 {
      restoreError = .passwordTooShort
    } else if password != repeatedPassword {
      restoreError = .inputsDontMatch
    } else {
      restoreError = nil
    }
    return restoreError == nil
  }
  
  private func restore() {
    if !validate() {
      return
    }
    keyringStore.restoreWallet(phrase: phrase, password: password) { success in
      if !success {
        restoreError = .invalidPhrase
      } else {
        // If we're displaying this via onboarding, mark as completed.
        keyringStore.markOnboardingCompleted()
      }
    }
  }
  
  private func handlePhraseChanged(_ value: String) {
    if restoreError == .invalidPhrase {
      // Reset validation on user changing
      restoreError = nil
    }
  }
  
  private func handlePasswordChanged(_ value: String) {
    if restoreError == .passwordTooShort {
      // Reset validation on user changing
      restoreError = nil
    }
  }
  
  private func handleRepeatedPasswordChanged(_ value: String) {
    if restoreError == .inputsDontMatch {
      // Reset validation on user changing
      restoreError = nil
    }
  }
  
  var body: some View {
    VStack(spacing: 48) {
      VStack(spacing: 14) {
        Text("Restore Crypto Account") // NSLocalizedString
          .font(.headline)
          .foregroundColor(.primary)
        Text("Enter your recovery phrase to restore your Brave Wallet crypto account.") // NSLocalizedString
          .font(.subheadline)
          .foregroundColor(.secondary)
      }
      .multilineTextAlignment(.center)
      .fixedSize(horizontal: false, vertical: true)
      VStack(spacing: 10) {
        Group {
          if showingRecoveryPhase {
            TextField("Enter your recovery phrase", text: $phrase) // NSLocalizedString
          } else {
            SecureField("Enter your recovery phrase", text: $phrase) // NSLocalizedString
          }
        }
        .textFieldStyle(BraveValidatedTextFieldStyle(error: restoreError, when: .invalidPhrase))
        HStack {
          Toggle("Show Recovery Phase", isOn: $showingRecoveryPhase) // NSLocalizedString
            .labelsHidden()
            .scaleEffect(0.75)
            .padding(-6)
          Text("Show Recovery Phase") // NSLocalizedString
            .font(.footnote)
            .onTapGesture {
              withAnimation {
                showingRecoveryPhase.toggle()
              }
            }
          Spacer()
          Button(action: {
            if let string = UIPasteboard.general.string {
              phrase = string
            }
          }) {
            Image("brave.clipboard")
          }
        }
      }
      VStack {
        Text("New Password") // NSLocalizedString
          .font(.subheadline.weight(.medium))
        SecureField("Password", text: $password) // NSLocalizedString
          .textFieldStyle(BraveValidatedTextFieldStyle(error: restoreError, when: .passwordTooShort))
        SecureField("Re-type password", text: $repeatedPassword) // NSLocalizedString
          .textFieldStyle(BraveValidatedTextFieldStyle(error: restoreError, when: .inputsDontMatch))
      }
      .font(.subheadline)
      .padding(.horizontal, 48)
      Button(action: restore) {
        Text("Restore")
      }
      .buttonStyle(BraveFilledButtonStyle(size: .normal))
    }
    .padding()
    .onChange(of: phrase, perform: handlePhraseChanged)
    .onChange(of: password, perform: handlePasswordChanged)
    .onChange(of: repeatedPassword, perform: handleRepeatedPasswordChanged)
  }
}

#if DEBUG
@available(iOS 14.0, *)
struct RestoreWalletView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      RestoreWalletContainerView(keyringStore: .previewStore)
    }
    .previewLayout(.sizeThatFits)
    .previewColorSchemes()
  }
}
#endif
