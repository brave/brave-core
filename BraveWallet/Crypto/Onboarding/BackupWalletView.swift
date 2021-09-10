/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import BraveUI

@available(iOS 14.0, *)
struct BackupWalletView: View {
  @ObservedObject var keyringStore: KeyringStore
  @State private var acknowledgedWarning: Bool = false
  
  var body: some View {
    ScrollView(.vertical) {
      VStack(spacing: 46) {
        Image("graphic-save")
          .padding(.vertical)
        VStack(spacing: 14) {
          Text("Backup your wallet now!") // NSLocalizedString
            .font(.headline)
            .foregroundColor(.primary)
          Text("In the next step you will see 12 words that allows you to recover your crypto wallet.") // NSLocalizedString
            .font(.subheadline)
            .foregroundColor(.secondary)
        }
        .fixedSize(horizontal: false, vertical: true)
        .multilineTextAlignment(.center)
        Toggle("I understand that if I lose my recovery words, I will not be able to access my crypto wallet.", isOn: $acknowledgedWarning) // NSLocalizedString
          .foregroundColor(.secondary)
          .font(.footnote)
        VStack(spacing: 12) {
          NavigationLink(destination: BackupRecoveryPhraseView(keyringStore: keyringStore)) {
            Text("Continue") // NSLocalizedString
          }
          .buttonStyle(BraveFilledButtonStyle(size: .normal))
          .disabled(!acknowledgedWarning)
          Button(action: {
            keyringStore.markOnboardingCompleted()
          }) {
            Text("Skip") // NSLocalizedString
              .font(Font.subheadline.weight(.medium))
              .foregroundColor(Color(.braveLabel))
          }
        }
      }
      .padding(.horizontal, 30)
      .padding(.vertical)
    }
    .navigationBarBackButtonHidden(true)
    .navigationTitle("Crypto") // NSLocalizedString
    .navigationBarTitleDisplayMode(.inline)
    .introspectViewController { vc in
      vc.navigationItem.backButtonTitle = "Backup Wallet" // NSLocalizedString
      vc.navigationItem.backButtonDisplayMode = .minimal
    }
    .background(Color(.braveBackground).edgesIgnoringSafeArea(.all))
  }
}

#if DEBUG
@available(iOS 14.0, *)
struct BackupWalletView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      BackupWalletView(keyringStore: .previewStore)
    }
    .previewLayout(.sizeThatFits)
    .previewColorSchemes()
  }
}
#endif
