/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import BraveUI
import struct Shared.Strings

struct BackupRecoveryPhraseView: View {
  @ObservedObject var keyringStore: KeyringStore
  
  @State private var confirmedPhraseBackup: Bool = false
  @State private var recoveryWords: [RecoveryWord] = []
  
  private var warningView: some View {
    HStack(alignment: .firstTextBaseline) {
      Image(systemName: "exclamationmark.circle")
        .font(.subheadline.weight(.semibold))
        .foregroundColor(.red)
      
      let warningPartOne = Text(Strings.Wallet.backupRecoveryPhraseWarningPartOne)
        .fontWeight(.semibold)
        .foregroundColor(.red)
      let warningPartTwo = Text(Strings.Wallet.backupRecoveryPhraseWarningPartTwo)
      Text(verbatim: "\(warningPartOne) \(warningPartTwo)")
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
    UIPasteboard.general.string = recoveryWords.map(\.value).joined(separator: " ")
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
          Text(verbatim: word.value)
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
          Text(verbatim: "\(Strings.Wallet.copyToPasteboard) \(Image(systemName: "brave.clipboard"))")
            .font(.subheadline.bold())
            .foregroundColor(.primary)
        }
        Toggle(Strings.Wallet.backupRecoveryPhraseDisclaimer, isOn: $confirmedPhraseBackup)
          .fixedSize(horizontal: false, vertical: true)
          .font(.footnote)
          .padding(.vertical, 30)
          .padding(.horizontal, 20)
        NavigationLink(destination: VerifyRecoveryPhraseView(keyringStore: keyringStore)) {
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
    .onAppear {
      keyringStore.recoveryPhrase { words in
        recoveryWords = words
      }
    }
    .modifier(ToolbarModifier(isShowingCancel: !keyringStore.isOnboardingVisible))
    .navigationTitle(Strings.Wallet.cryptoTitle)
    .navigationBarTitleDisplayMode(.inline)
    .introspectViewController { vc in
      vc.navigationItem.backButtonTitle = Strings.Wallet.backupRecoveryPhraseBackButtonTitle
      vc.navigationItem.backButtonDisplayMode = .minimal
    }
    .background(Color(.braveBackground).edgesIgnoringSafeArea(.all))
  }
  
  struct ToolbarModifier: ViewModifier {
    var isShowingCancel: Bool
    
    @Environment(\.presentationMode) @Binding private var presentationMode
   
    func body(content: Content) -> some View {
      if isShowingCancel {
        content
          .toolbar {
            ToolbarItemGroup(placement: .cancellationAction) {
              Button(action: {
                presentationMode.dismiss()
              }) {
                Text(Strings.CancelString)
                  .foregroundColor(Color(.braveOrange))
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
struct BackupRecoveryPhraseView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      BackupRecoveryPhraseView(keyringStore: .previewStore)
    }
    .previewLayout(.sizeThatFits)
    .previewColorSchemes()
  }
}
#endif
