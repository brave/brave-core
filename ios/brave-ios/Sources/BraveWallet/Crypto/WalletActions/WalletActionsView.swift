// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Strings
import SwiftUI

struct WalletActionsView: View {
  var networkStore: NetworkStore
  var action: (WalletActionDestination) -> Void
  var destinations: [WalletActionDestination] = []

  init(
    networkStore: NetworkStore,
    action: @escaping (WalletActionDestination) -> Void
  ) {
    self.networkStore = networkStore
    self.action = action
    self.destinations.append(WalletActionDestination(kind: .buy))
    self.destinations.append(WalletActionDestination(kind: .send))
    self.destinations.append(WalletActionDestination(kind: .swap))
    self.destinations.append(WalletActionDestination(kind: .deposit(query: nil)))
  }

  var body: some View {
    VStack(alignment: .leading, spacing: 16) {
      ForEach(destinations, id: \.self) { destination in
        Button {
          self.action(destination)
        } label: {
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
struct BuySendSwapDepositView_Previews: PreviewProvider {
  static var previews: some View {
    WalletActionsView(networkStore: .previewStore, action: { _ in })
      .previewLayout(.sizeThatFits)
      //      .previewColorSchemes()
      .previewSizeCategories([.large, .accessibilityLarge])
  }
}
#endif
