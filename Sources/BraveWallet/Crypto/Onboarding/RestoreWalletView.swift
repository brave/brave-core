/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import DesignSystem
import Strings
import struct Shared.AppConstants
import Preferences

struct RestoreWalletContainerView: View {
  @ObservedObject var keyringStore: KeyringStore

  var body: some View {
    ScrollView(.vertical) {
      RestoreWalletView(keyringStore: keyringStore)
        .background(Color(.braveBackground))
    }
    .background(Color(.braveBackground).edgesIgnoringSafeArea(.all))
    .transparentNavigationBar(backButtonTitle: Strings.Wallet.restoreWalletBackButtonTitle, backButtonDisplayMode: .generic)
  }
}

private struct RestoreWalletView: View {
  @ObservedObject var keyringStore: KeyringStore
  
  @Environment(\.sizeCategory) private var sizeCategory
  @Environment(\.dismiss) private var dismiss
  
  @State private var isBraveLegacyWallet: Bool = false
  @State private var isRevealRecoveryWords: Bool = true
  @State private var recoveryWords: [String] = .init(repeating: "", count: 12)
  @State private var newPassword: String?
  @State private var isShowingCreateNewPassword: Bool = false
  @State private var isShowingPhraseError: Bool = false
  @State private var isShowingCompleteState: Bool = false
  
  private var numberOfColumns: Int {
    sizeCategory.isAccessibilityCategory ? 2 : 3
  }
  
  private var isLegacyWallet: Bool {
    recoveryWords.count == 24
  }
  
  private var isContinueDisabled: Bool {
    !recoveryWords.allSatisfy({ !$0.isEmpty }) || keyringStore.isRestoringWallet
  }
  
  private var errorLabel: some View {
    HStack(spacing: 12) {
      Image(braveSystemName: "leo.warning.circle-filled")
        .renderingMode(.template)
        .foregroundColor(Color(.braveLighterOrange))
      Text(Strings.Wallet.restoreWalletPhraseInvalidError)
        .multilineTextAlignment(.leading)
        .font(.callout)
      Spacer()
    }
    .padding(12)
    .background(
      Color(.braveErrorBackground)
        .clipShape(RoundedRectangle(cornerRadius: 10, style: .continuous))
    )
  }
  
  private func handleRecoveryWordsChanged(oldValue: [String], newValue: [String]) {
    let indexOnDifference = zip(oldValue, newValue).enumerated().first(where: { $1.0 != $1.1 }).map { $0.0 }
    if let indexOnDifference,
       let oldInput = oldValue[safe: indexOnDifference],
       let newInput = newValue[safe: indexOnDifference] { // there is a new input on `indexOnDifference`
      if abs(newInput.count - oldInput.count) > 1 { // we consider this is a copy and paste from `UIPasteboard`
        let phrases = newInput.split(separator: " ")
        if (!isLegacyWallet && phrases.count == 12) || (isLegacyWallet && phrases.count == 24) { // user copies and pastes the entire recovery phrases, we will auto-fill in all the recovery phrases
          let currentLength = recoveryWords.count
          var newPhrases = Array(repeating: "", count: currentLength)
          for (index, pastedWord) in phrases.enumerated() {
            newPhrases[index] = String(pastedWord)
          }
          recoveryWords = newPhrases
        } else { // user copy and paste some phrases, we will auto-fill in from the `indexOfDifference` (where user pastes) to the last input field. This also means, if user passtes more phrases than number of input fields remaining, we won't exceed and will stop pasting at the last input field.
          var startIndex = indexOnDifference
          var recoveryWordsCopy = recoveryWords
          for phrase in phrases {
            if startIndex < recoveryWordsCopy.count {
              recoveryWordsCopy[startIndex] = String(phrase)
              startIndex += 1
            } else {
              break
            }
          }
          recoveryWords = recoveryWordsCopy
        }
        resignFirstResponder()
      }
    }
    
  }

  var body: some View {
    ScrollView {
      VStack(spacing: 48) {
        VStack(spacing: 14) {
          Text(Strings.Wallet.restoreWalletTitle)
            .font(.title)
            .foregroundColor(Color(uiColor: WalletV2Design.textPrimary))
          Text(Strings.Wallet.restoreWalletSubtitle)
            .font(.subheadline)
            .foregroundColor(Color(uiColor: WalletV2Design.textSecondary))
        }
        .multilineTextAlignment(.center)
        .fixedSize(horizontal: false, vertical: true)
        let columns: [GridItem] = (0..<numberOfColumns).map { _ in .init(.flexible()) }
        LazyVGrid(columns: columns, spacing: 8) {
          ForEach(self.recoveryWords.indices, id: \.self) { index in
            VStack(alignment: .leading, spacing: 10) {
              if isRevealRecoveryWords {
                TextField(String.localizedStringWithFormat(Strings.Wallet.restoreWalletPhrasePlaceholder, (index + 1)), text: $recoveryWords[index])
                  .customPrivacySensitive()
                  .autocapitalization(.none)
                  .disableAutocorrection(true)
                  .foregroundColor(Color(.braveLabel))
              } else {
                SecureField(String.localizedStringWithFormat(Strings.Wallet.restoreWalletPhrasePlaceholder, (index + 1)), text: $recoveryWords[index])
                  .customPrivacySensitive()
                  .textContentType(.newPassword)
              }
              Divider()
            }
          }
        }
        .padding(.horizontal)
        if isShowingPhraseError {
          errorLabel
        }
        HStack {
          Spacer()
          Button {
            // Regular wallet has `12` recovery-phrase
            // Legacy wallet has `24` recovery-phrase
            // This button is to toggle the current wallet type
            // to the other type, meaning:
            // regular(12) to legacy(24)
            // or legacy(24) to regular(12)
            resignFirstResponder()
            recoveryWords = .init(repeating: "", count: isLegacyWallet ? .regularWalletRecoveryPhraseNumber : .legacyWalletRecoveryPhraseNumber)
            isShowingPhraseError = false
          } label: {
            Text(isLegacyWallet ? Strings.Wallet.restoreWalletImportFromRegularBraveWallet : Strings.Wallet.restoreWalletImportFromLegacyBraveWallet)
              .fontWeight(.medium)
              .foregroundColor(Color(.braveBlurpleTint))
          }
          Spacer()
          Button {
            isRevealRecoveryWords.toggle()
          } label: {
            Image(braveSystemName: isRevealRecoveryWords ? "leo.eye.off" : "leo.eye.on")
              .foregroundColor(Color(.braveLabel))
          }
        }
        Button {
          if let newPassword, !newPassword.isEmpty {
            keyringStore.restoreWallet(words: recoveryWords, password: newPassword, isLegacyBraveWallet: isLegacyWallet) { isMnemonicValid in
              if isMnemonicValid {
                isShowingPhraseError = false
                keyringStore.resetKeychainStoredPassword()
                if keyringStore.isOnboardingVisible {
                  Preferences.Wallet.isOnboardingCompleted.value = true
                }
              } else {
                isShowingPhraseError = true
              }
            }
          } else {
            isShowingCreateNewPassword = true
          }
        } label: {
          Text(Strings.Wallet.continueButtonTitle)
            .frame(maxWidth: .infinity)
        }
        .buttonStyle(BraveFilledButtonStyle(size: .large))
        .disabled(isContinueDisabled)
      }
    }
    .padding()
    .onChange(of: recoveryWords) { [recoveryWords] newValue in
      handleRecoveryWordsChanged(oldValue: recoveryWords, newValue: newValue)
    }
    .sheet(isPresented: $isShowingCreateNewPassword) {
      NavigationView {
        CreateWalletContainerView(
          keyringStore: keyringStore,
          restorePackage: RestorePackage(
            recoveryWords: recoveryWords,
            onRestoreCompleted: { success, password in
              if success {
                isShowingPhraseError = false
                keyringStore.resetKeychainStoredPassword()
                if keyringStore.isOnboardingVisible {
                  Preferences.Wallet.isOnboardingCompleted.value = true
                }
              } else {
                newPassword = password
                isShowingPhraseError = true
              }
              isShowingCreateNewPassword = false
            }
          )
        )
        .toolbar {
          ToolbarItemGroup(placement: .destructiveAction) {
            Button(Strings.CancelString) {
              isShowingCreateNewPassword = false
            }
          }
        }
      }
    }
  }
  
  private func resignFirstResponder() {
    UIApplication.shared.sendAction(#selector(UIResponder.resignFirstResponder), to: nil, from: nil, for: nil)
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
