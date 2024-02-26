// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import Foundation
import Preferences
import Strings
import SwiftUI

struct VerifyRecoveryPhraseView: View {
  @ObservedObject var keyringStore: KeyringStore

  @State private var input: String = ""
  @State private var isShowingError = false
  @State private var activeCheckIndex: Int = 0
  @State private var isShowingSkipWarning: Bool = false

  @Environment(\.modalPresentationMode) @Binding private var modalPresentationMode
  @FocusState private var isFieldFocused: Bool

  private var recoveryWords: [RecoveryWord]
  private let targetedRecoveryWordIndexes: [Int]
  private let password: String

  init(
    keyringStore: KeyringStore,
    recoveryWords: [RecoveryWord],
    targetedRecoveryWordIndexes: [Int],
    password: String
  ) {
    self.keyringStore = keyringStore
    self.recoveryWords = recoveryWords
    self.targetedRecoveryWordIndexes = targetedRecoveryWordIndexes
    self.password = password
  }

  var body: some View {
    ScrollView {
      VStack {
        HStack(spacing: 16) {
          Text(Strings.Wallet.verifyRecoveryPhraseTitle)
            .font(.title.weight(.medium))
            .foregroundColor(Color(uiColor: WalletV2Design.textPrimary))
            .fixedSize(horizontal: false, vertical: true)
          RecoveryPhrasePager(activeIndex: $activeCheckIndex)
        }
        Text(
          LocalizedStringKey(
            String.localizedStringWithFormat(
              Strings.Wallet.verifyRecoveryPhraseSubTitle,
              targetedRecoveryWordIndexes[activeCheckIndex] + 1
            )
          )
        )
        .font(.subheadline)
        .foregroundColor(Color(uiColor: WalletV2Design.textPrimary))
        .fixedSize(horizontal: false, vertical: true)
        .padding(.bottom, 40)
        VStack(alignment: .leading) {
          TextField("", text: $input)
            .font(.body)
            .autocorrectionDisabled()
            .autocapitalization(.none)
            .focused($isFieldFocused)
          Divider()
        }
        HStack(spacing: 12) {
          Image(braveSystemName: "leo.warning.circle-filled")
            .renderingMode(.template)
            .foregroundColor(Color(.braveLighterOrange))
          Text(Strings.Wallet.verifyRecoveryPhraseError)
            .multilineTextAlignment(.leading)
            .font(.callout)
          Spacer()
        }
        .padding(12)
        .background(
          Color(.braveErrorBackground)
            .clipShape(RoundedRectangle(cornerRadius: 10, style: .continuous))
        )
        .hidden(isHidden: !isShowingError)
        Button {
          let targetIndex = targetedRecoveryWordIndexes[activeCheckIndex]
          if input.trimmingCharacters(in: .whitespaces) == recoveryWords[safe: targetIndex]?.value {
            isShowingError = false
            if activeCheckIndex == targetedRecoveryWordIndexes.count - 1 {  // finished all checks
              if keyringStore.isOnboardingVisible {
                // check if biometric is available
                keyringStore.notifyWalletBackupComplete()
                Preferences.Wallet.isOnboardingCompleted.value = true
              } else {  // coming from BackUpWalletView
                keyringStore.notifyWalletBackupComplete()
                modalPresentationMode = false
              }
            } else {
              // next check
              activeCheckIndex += 1
              input = ""
            }
          } else {
            isShowingError = true
          }
        } label: {
          Text(Strings.Wallet.continueButtonTitle)
            .frame(maxWidth: .infinity)
        }
        .buttonStyle(BraveFilledButtonStyle(size: .large))
        .disabled(input.isEmpty)
        .padding(.top, 86)
        if keyringStore.isOnboardingVisible {
          Button {
            isShowingSkipWarning = true
          } label: {
            Text(Strings.Wallet.skipButtonTitle)
              .font(Font.subheadline.weight(.medium))
              .foregroundColor(Color(.braveLabel))
          }
          .padding(.top, 16)
        }
      }
    }
    .padding(.horizontal, 20)
    .padding(.bottom, 20)
    .background(Color(.braveBackground).edgesIgnoringSafeArea(.all))
    .background(
      WalletPromptView(
        isPresented: $isShowingSkipWarning,
        primaryButton: WalletPromptButton(
          title: Strings.Wallet.editTransactionErrorCTA,
          action: { _ in
            isShowingSkipWarning = false
          }
        ),
        secondaryButton: WalletPromptButton(
          title: Strings.Wallet.backupSkipButtonTitle,
          action: { _ in
            isShowingSkipWarning = false
            Preferences.Wallet.isOnboardingCompleted.value = true
          }
        ),
        showCloseButton: false,
        content: {
          VStack(alignment: .leading, spacing: 20) {
            Text(Strings.Wallet.backupSkipPromptTitle)
              .font(.subheadline.weight(.medium))
              .foregroundColor(.primary)
            Text(Strings.Wallet.backupSkipPromptSubTitle)
              .font(.subheadline)
              .foregroundColor(.secondary)
          }
          .multilineTextAlignment(.leading)
          .padding(.vertical, 20)
        }
      )
    )
    .transparentNavigationBar(backButtonDisplayMode: .generic)
    .onChange(
      of: input,
      perform: { newValue in
        if newValue.isEmpty {
          isShowingError = false
        }
      }
    )
    .onAppear {
      isFieldFocused = true
    }
  }
}

#if DEBUG
struct VerifyRecoveryPhraseView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      VerifyRecoveryPhraseView(
        keyringStore: .previewStore,
        recoveryWords: [
          .init(value: "First", index: 0),
          .init(value: "Second", index: 1),
          .init(value: "Third", index: 2),
        ],
        targetedRecoveryWordIndexes: [0, 1, 2],
        password: ""
      )
    }
    .previewLayout(.sizeThatFits)
    .previewColorSchemes()
  }
}
#endif
