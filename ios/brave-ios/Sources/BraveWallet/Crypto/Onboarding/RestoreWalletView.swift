// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import Foundation
import Preferences
import Strings
import SwiftUI

import struct Shared.AppConstants

struct RestoreWalletView: View {
  @ObservedObject var keyringStore: KeyringStore
  // Used to dismiss all of Wallet
  let dismissAction: () -> Void

  @Environment(\.sizeCategory) private var sizeCategory

  @State private var isLegacyEthSeedFormat: Bool = false
  @State private var isRevealRecoveryWords: Bool = true
  @State private var recoveryWords: [String] = .init(repeating: "", count: 12)
  @State private var newPassword: String?
  @State private var isShowingCreateNewPassword: Bool = false
  @State private var isShowingPhraseError: Bool = false
  @State private var isShowingCompleteState: Bool = false

  private var isTwelvePhrase: Bool {
    return recoveryWords.count == .walletTwelveRecoveryPhraseNumber
  }

  private var numberOfColumns: Int {
    sizeCategory.isAccessibilityCategory ? 2 : 3
  }

  private var isContinueDisabled: Bool {
    !recoveryWords.allSatisfy({ !$0.isEmpty }) || keyringStore.isRestoringWallet
  }

  private var isShowingCreatingWallet: Bool {
    keyringStore.isCreatingWallet || keyringStore.isRestoringWallet
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
    let indexOnDifference = zip(oldValue, newValue).enumerated().first(where: { $1.0 != $1.1 }).map
    { $0.0 }
    if let indexOnDifference,
      let oldInput = oldValue[safe: indexOnDifference],
      let newInput = newValue[safe: indexOnDifference]
    {  // there is a new input on `indexOnDifference`
      // we consider this is a copy and paste from `UIPasteboard`
      if abs(newInput.count - oldInput.count) > 1 {
        let phrases = newInput.split(separator: " ")
        // user copies and pastes the entire recovery phrases, we will auto-fill in all the recovery phrases
        if (isTwelvePhrase && phrases.count == 12) || (!isTwelvePhrase && phrases.count == 24) {
          let currentLength = recoveryWords.count
          var newPhrases = Array(repeating: "", count: currentLength)
          for (index, pastedWord) in phrases.enumerated() {
            newPhrases[index] = String(pastedWord)
          }
          recoveryWords = newPhrases
        } else {
          // user copy and paste some phrases, we will auto-fill in from the `indexOfDifference`
          // (where user pastes) to the last input field. This also means, if user passtes more
          // phrases than number of input fields remaining, we won't exceed and will stop pasting
          // at the last input field.
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
      VStack(spacing: 40) {
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
                TextField(
                  String.localizedStringWithFormat(
                    Strings.Wallet.restoreWalletPhrasePlaceholder,
                    (index + 1)
                  ),
                  text: $recoveryWords[index]
                )
                .customPrivacySensitive()
                .autocapitalization(.none)
                .disableAutocorrection(true)
                .foregroundColor(Color(.braveLabel))
              } else {
                SecureField(
                  String.localizedStringWithFormat(
                    Strings.Wallet.restoreWalletPhrasePlaceholder,
                    (index + 1)
                  ),
                  text: $recoveryWords[index]
                )
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
        VStack(spacing: 20) {
          HStack {
            Button {
              // Regular wallet has `12` or `24` recovery-phrase
              // Legacy wallet has `24` recovery-phrase
              // This button is to toggle the current wallet type
              // to the other type, meaning:
              // regular(12) to legacy(24)
              // or legacy(24) to regular(12)
              isLegacyEthSeedFormat = false
              resignFirstResponder()
              recoveryWords = .init(
                repeating: "",
                count: !isTwelvePhrase
                  ? .walletTwelveRecoveryPhraseNumber : .walletTwentyFourRecoveryPhraseNumber
              )
              isShowingPhraseError = false
            } label: {
              Text(
                !isTwelvePhrase
                  ? Strings.Wallet.restoreWalletImportWithTwelvePhrases
                  : Strings.Wallet.restoreWalletImportFromTwentyFourPhrases
              )
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
          if !isTwelvePhrase {
            Toggle(isOn: $isLegacyEthSeedFormat) {
              Text(Strings.Wallet.restoreLegacyBraveWalletToggleLabel)
                .fontWeight(.medium)
                .foregroundColor(Color(.braveBlurpleTint))
            }
            .tint(Color(.braveBlurpleTint))
          }
        }
        .padding(.horizontal, 16)
        Button {
          if let newPassword, !newPassword.isEmpty {
            keyringStore.restoreWallet(
              words: recoveryWords,
              password: newPassword,
              isLegacyEthSeedFormat: isLegacyEthSeedFormat
            ) { isMnemonicValid in
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
    .navigationBarBackButtonHidden(isShowingCreatingWallet)
    .frame(maxWidth: .infinity, maxHeight: .infinity)
    .background(Color(.braveBackground).edgesIgnoringSafeArea(.all))
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
    .sheet(isPresented: $isShowingCreateNewPassword) {
      NavigationView {
        CreateWalletView(
          keyringStore: keyringStore,
          setupOption: .restore,
          onValidPasswordEntered: restoreWallet,
          dismissAction: dismissAction
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
    .transparentUnlessScrolledNavigationAppearance()
    .onAppear {
      keyringStore.reportP3AOnboarding(action: .startRestore)
    }
  }

  private func resignFirstResponder() {
    UIApplication.shared.sendAction(
      #selector(UIResponder.resignFirstResponder),
      to: nil,
      from: nil,
      for: nil
    )
  }

  private func restoreWallet(_ password: String) {
    newPassword = password
    isShowingCreateNewPassword = false
    keyringStore.restoreWallet(
      words: recoveryWords,
      password: password,
      isLegacyEthSeedFormat: isLegacyEthSeedFormat
    ) { success in
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
    }
  }
}

#if DEBUG
struct RestoreWalletView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      RestoreWalletView(
        keyringStore: .previewStore,
        dismissAction: {}
      )
    }
    .previewLayout(.sizeThatFits)
    .previewColorSchemes()
  }
}
#endif
