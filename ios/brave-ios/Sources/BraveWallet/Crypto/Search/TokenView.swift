// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveCore

struct TokenView<ImageView: View>: View {
  var token: BraveWallet.BlockchainToken
  var network: BraveWallet.NetworkInfo
  var image: () -> ImageView
  
  init(
    token: BraveWallet.BlockchainToken,
    network: BraveWallet.NetworkInfo,
    @ViewBuilder image: @escaping () -> ImageView
  ) {
    self.token = token
    self.network = network
    self.image = image
  }
  
  var body: some View {
    HStack(spacing: 8) {
      image()
      VStack(alignment: .leading) {
        Text(token.name)
          .fontWeight(.semibold)
          .foregroundColor(Color(.bravePrimary))
        Text(token.symbol.uppercased())
          .foregroundColor(Color(.secondaryBraveLabel))
      }
      .font(.footnote)
    }
    .padding(.vertical, 8)
  }
}

#if DEBUG
struct TokenView_Previews: PreviewProvider {
  static var previews: some View {
    TokenView(token: MockBlockchainRegistry.testTokens.first!, network: .mockMainnet) {
      AssetIconView(token: MockBlockchainRegistry.testTokens.first!, network: .mockMainnet)
    }
  }
}
#endif
