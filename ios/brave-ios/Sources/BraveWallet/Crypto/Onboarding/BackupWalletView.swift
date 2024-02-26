// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import Foundation
import Strings
import SwiftUI

struct BackupWalletView: View {
  @ObservedObject var keyringStore: KeyringStore
  @State private var password: String
  @State private var passwordError: PasswordEntryError?
  @State private var acknowledgedWarning: Bool = false
  @State private var recoveryWords: [RecoveryWord] = []
  private let requirePasswordEntry: Bool

  init(
    password: String?,
    keyringStore: KeyringStore
  ) {
    self.requirePasswordEntry = password == nil
    self.password = password ?? ""
    self.keyringStore = keyringStore
  }

  private var isContinueButtonDisabled: Bool {
    !acknowledgedWarning || password.isEmpty
  }

  private func continueToBackupPhrase() {
    guard acknowledgedWarning else {
      // user can press return in field to execute when continue button is disabled
      return
    }

    // Dismiss keyboard for password field before leaving this view #8445.
    // Without dismissal, it's possible the scroll view in the detail view
    // has incorrect frame (sitting above the area the keyboard used to be)
    // which can show the scroll view contents out of the scroll bounds
    // but will block tap interactions.
    resignFirstResponder()

    keyringStore.recoveryPhrase(password: password) { words in
      if words.isEmpty {
        passwordError = .incorrectPassword
      } else {
        recoveryWords = words
      }
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

  var body: some View {
    ScrollView(.vertical) {
      VStack(spacing: 24) {
        Image("graphic-save", bundle: .module)

        VStack(spacing: 14) {
          Text(Strings.Wallet.backupWalletTitle)
            .font(.headline)
            .foregroundColor(Color(.bravePrimary))
          Text(Strings.Wallet.backupWalletSubtitle)
            .font(.subheadline)
            .foregroundColor(Color(.braveLabel))
        }
        .multilineTextAlignment(.center)

        Toggle(Strings.Wallet.backupWalletDisclaimer, isOn: $acknowledgedWarning)
          .font(.footnote)
          .toggleStyle(SwitchToggleStyle(tint: .accentColor))
          .foregroundColor(Color(.braveLabel))
          .padding(.horizontal, 20)
          .padding(.vertical, 10)

        if requirePasswordEntry {
          PasswordEntryField(
            password: $password,
            error: $passwordError,
            placeholder: Strings.Wallet.backupWalletPasswordPlaceholder,
            shouldShowBiometrics: true,
            keyringStore: keyringStore,
            onCommit: continueToBackupPhrase
          )
        }

        Button(action: continueToBackupPhrase) {
          Text(Strings.Wallet.continueButtonTitle)
        }
        .buttonStyle(BraveFilledButtonStyle(size: .normal))
        .disabled(isContinueButtonDisabled)

        if keyringStore.isOnboardingVisible {
          Button {
            keyringStore.markOnboardingCompleted()
          } label: {
            Text(Strings.Wallet.skipButtonTitle)
              .font(Font.subheadline.weight(.medium))
              .foregroundColor(Color(.braveLabel))
          }
        }
      }
      .padding()
      .padding()
    }
    .navigationBarBackButtonHidden(true)
    .navigationTitle(Strings.Wallet.cryptoTitle)
    .navigationBarTitleDisplayMode(.inline)
    .introspectViewController { vc in
      vc.navigationItem.backButtonTitle = Strings.Wallet.backupWalletBackButtonTitle
      vc.navigationItem.backButtonDisplayMode = .minimal
    }
    .modifier(ToolbarModifier(isShowingCancel: !keyringStore.isOnboardingVisible))
    .background(Color(.braveBackground).edgesIgnoringSafeArea(.all))
    .background(
      NavigationLink(
        isActive: Binding(
          get: { !recoveryWords.isEmpty },
          set: { if !$0 { recoveryWords = [] } }
        ),
        destination: {
          BackupRecoveryPhraseView(
            password: password,
            keyringStore: keyringStore
          )
        },
        label: {
          EmptyView()
        }
      )
    )
  }

  struct ToolbarModifier: ViewModifier {
    var isShowingCancel: Bool

    @Environment(\.presentationMode) @Binding private var presentationMode

    func body(content: Content) -> some View {
      if isShowingCancel {
        content
          .toolbar {
            ToolbarItemGroup(placement: .cancellationAction) {
              Button {
                presentationMode.dismiss()
              } label: {
                Text(Strings.cancelButtonTitle)
                  .foregroundColor(Color(.braveBlurpleTint))
              }
            }
          }
      } else {
        content
      }
    }
  }
}

#if DEBUG
struct BackupWalletView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      BackupWalletView(
        password: "",
        keyringStore: .previewStore
      )
    }
    .previewLayout(.sizeThatFits)
    .previewColorSchemes()
  }
}
#endif
