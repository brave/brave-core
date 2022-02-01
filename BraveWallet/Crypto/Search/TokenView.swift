// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveCore

struct TokenView: View {
  var token: BraveWallet.BlockchainToken
  
  var body: some View {
    HStack(spacing: 8) {
      AssetIconView(token: token)
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
      TokenView(token: MockBlockchainRegistry.testTokens.first!)
    }
}
#endif
