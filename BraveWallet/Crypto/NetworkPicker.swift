/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI
import BraveCore
import struct Shared.Strings

extension BraveWallet.EthereumChain {
  fileprivate var shortChainName: String {
    chainName.split(separator: " ").first?.capitalized ?? chainName
  }
}

struct NetworkPicker: View {
  var networks: [BraveWallet.EthereumChain]
  @Binding var selectedNetwork: BraveWallet.EthereumChain
  
  var body: some View {
    Menu {
      Picker(
        Strings.Wallet.selectedNetworkAccessibilityLabel,
        selection: $selectedNetwork
      ) {
        ForEach(networks) {
          Text($0.chainName).tag($0)
        }
      }
    } label: {
      HStack {
        Text(selectedNetwork.shortChainName)
          .fontWeight(.bold)
        Image(systemName: "chevron.down.circle")
      }
      .foregroundColor(Color(.bravePrimary))
      .font(.caption.weight(.semibold))
      .padding(.init(top: 6, leading: 12, bottom: 6, trailing: 12))
      .background(
        Color(.secondaryButtonTint)
          .clipShape(Capsule().inset(by: 0.5).stroke())
      )
      .clipShape(Capsule())
      .contentShape(Capsule())
    }
  }
}

#if DEBUG
struct NetworkPicker_Previews: PreviewProvider {
  static var previews: some View {
    NetworkPicker(networks: [.mainnet, .rinkeby, .ropsten], selectedNetwork: .constant(.mainnet))
      .padding()
      .previewLayout(.sizeThatFits)
      .previewColorSchemes()
  }
}
#endif
