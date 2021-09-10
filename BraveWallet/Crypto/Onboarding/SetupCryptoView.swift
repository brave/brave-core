/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import Introspect
import BraveUI

@available(iOS 14.0, *)
struct SetupCryptoView: View {
  @ObservedObject var keyringStore: KeyringStore
  @Environment(\.presentationMode) @Binding private var presentationMode
  
  var body: some View {
    VStack(spacing: 46) {
      Image("setup-welcome")
        .padding(.bottom)
      VStack(spacing: 14) {
        Text("DeFi & Secure Crypto Storage") // NSLocalizedString
          .foregroundColor(.primary)
          .font(.headline)
        Text("Hold crypto in your custody. Trade assets. View Prices and portfolio performance. Invest, Borrow, and lend with DeFi.")  // NSLocalizedString
          .foregroundColor(.secondary)
          .font(.subheadline)
      }
      .fixedSize(horizontal: false, vertical: true)
      .multilineTextAlignment(.center)
      VStack(spacing: 26) {
        NavigationLink(destination: CreateWalletContainerView(keyringStore: keyringStore)) {
          Text("Setup Crypto") // NSLocalizedString
        }
        .buttonStyle(BraveFilledButtonStyle(size: .normal))
        NavigationLink(destination: RestoreWalletContainerView(keyringStore: keyringStore)) {
          Text("Restore") // NSLocalizedString
            .font(.subheadline.weight(.medium))
            .foregroundColor(Color(.braveLabel))
        }
      }
    }
    .padding()
    .frame(maxHeight: .infinity)
    .accessibilityEmbedInScrollView()
    .navigationTitle("Crypto") // NSLocalizedString
    .navigationBarTitleDisplayMode(.inline)
    .introspectViewController { vc in
      vc.navigationItem.backButtonTitle = "Welcome" // NSLocalizedString
      vc.navigationItem.backButtonDisplayMode = .minimal
    }
    .toolbar {
      ToolbarItemGroup(placement: .cancellationAction) {
        Button(action: { presentationMode.dismiss() }) {
          Image("wallet-dismiss")
            .renderingMode(.template)
        }
      }
    }
    .background(Color(.braveBackground).edgesIgnoringSafeArea(.all))
  }
}

#if DEBUG
@available(iOS 14.0, *)
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
