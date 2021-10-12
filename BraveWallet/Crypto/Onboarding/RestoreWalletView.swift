/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import BraveUI
import struct Shared.Strings

struct RestoreWalletContainerView: View {
  @ObservedObject var keyringStore: KeyringStore
  
  var body: some View {
    ScrollView(.vertical) {
      RestoreWalletView(keyringStore: keyringStore)
        .background(Color(.braveBackground))
    }
    .background(Color(.braveBackground).edgesIgnoringSafeArea(.all))
    .navigationTitle("")
    .navigationBarTitleDisplayMode(.inline)
    .introspectViewController { vc in
      vc.navigationItem.backButtonTitle = Strings.Wallet.restoreWalletBackButtonTitle
      vc.navigationItem.backButtonDisplayMode = .minimal
    }
  }
}

private struct RestoreWalletView: View {
  @ObservedObject var keyringStore: KeyringStore
  
  private enum RestoreWalletError: LocalizedError {
    case invalidPhrase
    case passwordTooShort
    case inputsDontMatch
    
    var errorDescription: String? {
      switch self {
      case .invalidPhrase:
        return Strings.Wallet.restoreWalletPhraseInvalidError
      case .passwordTooShort:
        return Strings.Wallet.passwordDoesNotMeetRequirementsError
      case .inputsDontMatch:
        return Strings.Wallet.passwordsDontMatchError
      }
    }
  }
  
  @State private var password: String = ""
  @State private var repeatedPassword: String = ""
  @State private var phrase: String = ""
  @State private var isEditingPhrase: Bool = false
  @State private var showingRecoveryPhase: Bool = false
  @State private var restoreError: RestoreWalletError?
  @State private var isShowingLegacyWalletToggle: Bool = false
  @State private var isBraveLegacyWallet: Bool = false
  
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
    keyringStore.restoreWallet(
      phrase: phrase,
      password: password,
      isLegacyBraveWallet: isBraveLegacyWallet
    ) { success in
      if !success {
        restoreError = .invalidPhrase
      } else {
        // If we're displaying this via onboarding, mark as completed.
        keyringStore.markOnboardingCompleted()
      }
    }
  }
  
  private func handlePhraseChanged(_ value: String) {
    isShowingLegacyWalletToggle = value.split(separator: " ").count == 24
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
        Text(Strings.Wallet.restoreWalletTitle)
          .font(.headline)
          .foregroundColor(.primary)
        Text(Strings.Wallet.restoreWalletSubtitle)
          .font(.subheadline)
          .foregroundColor(.secondary)
      }
      .multilineTextAlignment(.center)
      .fixedSize(horizontal: false, vertical: true)
      VStack(spacing: 10) {
        Group {
          if showingRecoveryPhase {
            TextField(Strings.Wallet.restoreWalletPhrasePlaceholder, text: $phrase)
          } else {
            SecureField(Strings.Wallet.restoreWalletPhrasePlaceholder, text: $phrase)
          }
        }
        .textFieldStyle(BraveValidatedTextFieldStyle(error: restoreError, when: .invalidPhrase))
        if isShowingLegacyWalletToggle {
          HStack {
            Toggle(Strings.Wallet.restoreWalletImportFromLegacyBraveWallet, isOn: $isBraveLegacyWallet)
              .labelsHidden()
              .scaleEffect(0.75)
              .padding(-6)
            Text(Strings.Wallet.restoreWalletImportFromLegacyBraveWallet)
              .font(.footnote)
              .onTapGesture {
                withAnimation {
                  showingRecoveryPhase.toggle()
                }
              }
          }
          .frame(maxWidth: .infinity, alignment: .leading)
        }
        HStack {
          Toggle(Strings.Wallet.restoreWalletShowRecoveryPhrase, isOn: $showingRecoveryPhase)
            .labelsHidden()
            .scaleEffect(0.75)
            .padding(-6)
          Text(Strings.Wallet.restoreWalletShowRecoveryPhrase)
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
        Text(Strings.Wallet.restoreWalletNewPasswordTitle)
          .font(.subheadline.weight(.medium))
        SecureField(Strings.Wallet.passwordPlaceholder, text: $password)
          .textFieldStyle(BraveValidatedTextFieldStyle(error: restoreError, when: .passwordTooShort))
        SecureField(Strings.Wallet.repeatedPasswordPlaceholder, text: $repeatedPassword)
          .textFieldStyle(BraveValidatedTextFieldStyle(error: restoreError, when: .inputsDontMatch))
      }
      .font(.subheadline)
      .padding(.horizontal, 48)
      Button(action: restore) {
        Text(Strings.Wallet.restoreWalletButtonTitle)
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
