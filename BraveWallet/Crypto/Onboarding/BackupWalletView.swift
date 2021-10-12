/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import BraveUI
import struct Shared.Strings

struct BackupWalletView: View {
  @ObservedObject var keyringStore: KeyringStore
  @State private var acknowledgedWarning: Bool = false
  
  var body: some View {
    ScrollView(.vertical) {
      VStack(spacing: 46) {
        Image("graphic-save")
          .padding(.vertical)
        VStack(spacing: 14) {
          Text(Strings.Wallet.backupWalletTitle)
            .font(.headline)
            .foregroundColor(.primary)
          Text(Strings.Wallet.backupWalletSubtitle)
            .font(.subheadline)
            .foregroundColor(.secondary)
        }
        .fixedSize(horizontal: false, vertical: true)
        .multilineTextAlignment(.center)
        Toggle(Strings.Wallet.backupWalletDisclaimer, isOn: $acknowledgedWarning)
          .foregroundColor(.secondary)
          .font(.footnote)
        VStack(spacing: 12) {
          NavigationLink(destination: BackupRecoveryPhraseView(keyringStore: keyringStore)) {
            Text(Strings.Wallet.continueButtonTitle)
          }
          .buttonStyle(BraveFilledButtonStyle(size: .normal))
          .disabled(!acknowledgedWarning)
          Button(action: {
            keyringStore.markOnboardingCompleted()
          }) {
            Text(Strings.Wallet.skipButtonTitle)
              .font(Font.subheadline.weight(.medium))
              .foregroundColor(Color(.braveLabel))
          }
        }
      }
      .padding(.horizontal, 30)
      .padding(.vertical)
    }
    .navigationBarBackButtonHidden(true)
    .navigationTitle(Strings.Wallet.cryptoTitle)
    .navigationBarTitleDisplayMode(.inline)
    .introspectViewController { vc in
      vc.navigationItem.backButtonTitle = Strings.Wallet.backupWalletBackButtonTitle
      vc.navigationItem.backButtonDisplayMode = .minimal
    }
    .background(Color(.braveBackground).edgesIgnoringSafeArea(.all))
  }
}

#if DEBUG
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
