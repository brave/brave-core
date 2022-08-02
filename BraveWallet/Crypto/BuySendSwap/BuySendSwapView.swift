/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI
import BraveCore
import Strings

struct BuySendSwapView: View {
  var network: BraveWallet.NetworkInfo
  var action: (BuySendSwapDestination) -> Void
  var destinations: [BuySendSwapDestination]

  init(
    network: BraveWallet.NetworkInfo,
    isCustomNetwork: Bool,
    action: @escaping (BuySendSwapDestination) -> Void
  ) {
    self.network = network
    self.action = action
    if isCustomNetwork || network.coin != .eth {
      destinations = [BuySendSwapDestination(kind: .send)]
    } else {
      destinations = [
        BuySendSwapDestination(kind: .buy),
        BuySendSwapDestination(kind: .send),
        BuySendSwapDestination(kind: .swap),
      ]
    }
  }

  var body: some View {
    VStack(alignment: .leading, spacing: 16) {
      ForEach(destinations, id: \.self) { destination in
        Button(action: { self.action(destination) }) {
          VStack(alignment: .leading, spacing: 3) {
            Text(destination.kind.localizedTitle)
              .foregroundColor(Color(.bravePrimary))
              .font(.headline)
              .multilineTextAlignment(.leading)
            Text(destination.kind.localizedDescription)
              .foregroundColor(Color(.braveLabel))
              .font(.footnote)
              .multilineTextAlignment(.leading)
          }
          .frame(maxWidth: .infinity, alignment: .leading)
          .padding([.leading, .trailing], 20)
        }
        if destination != destinations.last {
          Divider()
            .padding(.leading, 20)
        }
      }
    }
    .padding(.vertical, 20)
    .background(Color(.braveBackground))
  }
}

#if DEBUG
struct BuySendSwapView_Previews: PreviewProvider {
  static var previews: some View {
    BuySendSwapView(network: .init(), isCustomNetwork: false, action: { _ in })
      .previewLayout(.sizeThatFits)
      //      .previewColorSchemes()
      .previewSizeCategories([.large, .accessibilityLarge])
  }
}
#endif
