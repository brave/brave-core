/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import BraveUI

@available(iOS 14.0, *)
struct BackupRecoveryPhraseView: View {
  @ObservedObject var keyringStore: KeyringStore
  
  @State private var confirmedPhraseBackup: Bool = false
  @State private var recoveryWords: [RecoveryWord] = []
  
  private var warningView: some View {
    HStack(alignment: .firstTextBaseline) {
      Image(systemName: "exclamationmark.circle")
        .font(.subheadline.weight(.semibold))
        .foregroundColor(.red)
      Group {
        Text("WARNING:") // NSLocalizedString
          .font(.subheadline.weight(.semibold))
          .foregroundColor(.red) +
          Text(" Never disclose your backup phrase. Anyone with this phrase can take your funds forever.") // NSLocalizedString
          .font(.subheadline)
          .foregroundColor(.secondary)
      }
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
          Text("Your recovery phrase") // NSLocalizedString
            .font(.headline)
          Text("Write down or copy these words in the right order and save them somewhere safe.") // NSLocalizedString
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
          Text("Copy")
            .font(.subheadline.bold())
            .foregroundColor(.primary)
        }
        Toggle("I have backed up my phrase somewhere safe", isOn: $confirmedPhraseBackup) // NSLocalizedString
          .fixedSize(horizontal: false, vertical: true)
          .font(.footnote)
          .padding(.vertical, 30)
          .padding(.horizontal, 20)
        NavigationLink(destination: VerifyRecoveryPhraseView(keyringStore: keyringStore)) {
          Text("Continue") // NSLocalizedString
        }
        .buttonStyle(BraveFilledButtonStyle(size: .normal))
        .disabled(!confirmedPhraseBackup)
        if keyringStore.isOnboardingVisible {
          Button(action: {
            keyringStore.markOnboardingCompleted()
          }) {
            Text("Skip") // NSLocalizedString
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
    .navigationTitle("Crypto") // NSLocalizedString
    .navigationBarTitleDisplayMode(.inline)
    .introspectViewController { vc in
      vc.navigationItem.backButtonTitle = "Recovery Phrase" // NSLocalizedString
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
                Text("Cancel")
                  .foregroundColor(.accentColor)
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
@available(iOS 14.0, *)
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
