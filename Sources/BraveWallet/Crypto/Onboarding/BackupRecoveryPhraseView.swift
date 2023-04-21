/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import DesignSystem
import Strings

struct BackupRecoveryPhraseView: View {
  @ObservedObject var keyringStore: KeyringStore

  @State private var confirmedPhraseBackup: Bool = false
  @State private var recoveryWords: [RecoveryWord]
  
  init(
    recoveryWords: [RecoveryWord],
    keyringStore: KeyringStore
  ) {
    self.recoveryWords = recoveryWords
    self.keyringStore = keyringStore
  }

  private var warningView: some View {
    HStack(alignment: .firstTextBaseline) {
      Image(systemName: "exclamationmark.circle")
        .font(.subheadline.weight(.semibold))
        .foregroundColor(.red)
        .accessibilityHidden(true)

      let warningPartOne = Text(Strings.Wallet.backupRecoveryPhraseWarningPartOne)
        .fontWeight(.semibold)
        .foregroundColor(.red)
      let warningPartTwo = Text(Strings.Wallet.backupRecoveryPhraseWarningPartTwo)
      Text("\(warningPartOne) \(warningPartTwo)")
        .font(.subheadline)
        .foregroundColor(Color(.secondaryBraveLabel))
        .fixedSize(horizontal: false, vertical: true)
    }
    .frame(maxWidth: .infinity, alignment: .leading)
    .padding()
    .background(
      Color(.secondaryBraveBackground)
        .clipShape(RoundedRectangle(cornerRadius: 4, style: .continuous))
    )
  }

  private func copyRecoveryPhrase() {
    UIPasteboard.general.setSecureString(
      recoveryWords.map(\.value).joined(separator: " ")
    )
  }

  var body: some View {
    ScrollView(.vertical) {
      VStack(spacing: 16) {
        Group {
          Text(Strings.Wallet.backupRecoveryPhraseTitle)
            .font(.headline)
          Text(Strings.Wallet.backupRecoveryPhraseSubtitle)
            .font(.subheadline)
        }
        .fixedSize(horizontal: false, vertical: true)
        .multilineTextAlignment(.center)
        warningView
          .padding(.vertical)
        RecoveryPhraseGrid(data: recoveryWords, id: \.self) { word in
          Text(verbatim: "\(word.index + 1). \(word.value)")
            .customPrivacySensitive()
            .font(.footnote.bold())
            .padding(8)
            .frame(maxWidth: .infinity)
            .background(
              Color(.braveDisabled)
                .clipShape(RoundedRectangle(cornerRadius: 4, style: .continuous))
            )
            .fixedSize(horizontal: false, vertical: true)
        }
        .padding(.horizontal)
        Button(action: copyRecoveryPhrase) {
          Text("\(Strings.Wallet.copyToPasteboard) \(Image(braveSystemName: "leo.copy.plain-text"))")
            .font(.subheadline.bold())
            .foregroundColor(Color(.braveBlurpleTint))
        }
        Toggle(Strings.Wallet.backupRecoveryPhraseDisclaimer, isOn: $confirmedPhraseBackup)
          .fixedSize(horizontal: false, vertical: true)
          .toggleStyle(SwitchToggleStyle(tint: .accentColor))
          .font(.footnote)
          .padding(.vertical, 30)
          .padding(.horizontal, 20)
        NavigationLink(
          destination: VerifyRecoveryPhraseView(
            recoveryWords: recoveryWords,
            keyringStore: keyringStore
          )
        ) {
          Text(Strings.Wallet.continueButtonTitle)
        }
        .buttonStyle(BraveFilledButtonStyle(size: .normal))
        .disabled(!confirmedPhraseBackup)
        if keyringStore.isOnboardingVisible {
          Button(action: {
            keyringStore.markOnboardingCompleted()
          }) {
            Text(Strings.Wallet.skipButtonTitle)
              .font(Font.subheadline.weight(.medium))
              .foregroundColor(Color(.braveLabel))
          }
        }
      }
      .padding()
    }
    .navigationTitle(Strings.Wallet.cryptoTitle)
    .navigationBarTitleDisplayMode(.inline)
    .introspectViewController { vc in
      vc.navigationItem.backButtonTitle = Strings.Wallet.backupRecoveryPhraseBackButtonTitle
      vc.navigationItem.backButtonDisplayMode = .minimal
    }
    .background(Color(.braveBackground).edgesIgnoringSafeArea(.all))
    .alertOnScreenshot {
      Alert(
        title: Text(Strings.Wallet.screenshotDetectedTitle),
        message: Text(Strings.Wallet.recoveryPhraseScreenshotDetectedMessage),
        dismissButton: .cancel(Text(Strings.OKString))
      )
    }
  }
}

#if DEBUG
struct BackupRecoveryPhraseView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      BackupRecoveryPhraseView(
        recoveryWords: [],
        keyringStore: .previewStore
      )
    }
    .previewLayout(.sizeThatFits)
    .previewColorSchemes()
  }
}
#endif
