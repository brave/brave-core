/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import Introspect
import BraveUI
import struct Shared.Strings

struct SetupCryptoView: View {
  @ObservedObject var keyringStore: KeyringStore
  
  var body: some View {
    VStack(spacing: 46) {
      Image("setup-welcome")
        .padding(.bottom)
      VStack(spacing: 14) {
        Text(Strings.Wallet.setupCryptoTitle)
          .foregroundColor(.primary)
          .font(.headline)
        Text(Strings.Wallet.setupCryptoSubtitle)
          .foregroundColor(.secondary)
          .font(.subheadline)
      }
      .fixedSize(horizontal: false, vertical: true)
      .multilineTextAlignment(.center)
      VStack(spacing: 26) {
        NavigationLink(destination: CreateWalletContainerView(keyringStore: keyringStore)) {
          Text(Strings.Wallet.setupCryptoButtonTitle)
        }
        .buttonStyle(BraveFilledButtonStyle(size: .normal))
        NavigationLink(destination: RestoreWalletContainerView(keyringStore: keyringStore)) {
          Text(Strings.Wallet.restoreWalletButtonTitle)
            .font(.subheadline.weight(.medium))
            .foregroundColor(Color(.braveLabel))
        }
      }
    }
    .padding()
    .frame(maxHeight: .infinity)
    .accessibilityEmbedInScrollView()
    .navigationTitle(Strings.Wallet.cryptoTitle)
    .navigationBarTitleDisplayMode(.inline)
    .introspectViewController { vc in
      vc.navigationItem.backButtonTitle = Strings.Wallet.setupCryptoButtonBackButtonTitle
      vc.navigationItem.backButtonDisplayMode = .minimal
    }
    .background(Color(.braveBackground).edgesIgnoringSafeArea(.all))
  }
}

#if DEBUG
struct SetupCryptoView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      SetupCryptoView(keyringStore: .previewStore)
    }
    .previewLayout(.sizeThatFits)
    .previewColorSchemes()
  }
}
#endif
