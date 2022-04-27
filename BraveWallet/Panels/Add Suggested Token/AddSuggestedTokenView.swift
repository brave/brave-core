// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import struct Shared.Strings
import BraveCore
import BraveUI

struct AddSuggestedTokenView: View {
  var token: BraveWallet.BlockchainToken
  
  var body: some View {
    NavigationView {
      ScrollView(.vertical) {
        VStack(spacing: 44) {
          Text("Would you like to import this token?")
            .foregroundColor(Color(.braveLabel))
            .font(.subheadline)
          VStack {
            AssetIconView(token: token, length: 64)
            Text(token.symbol)
              .font(.headline)
              .foregroundColor(Color(.bravePrimary))
            Text("0 \(token.symbol)")
              .font(.subheadline)
              .foregroundColor(Color(.secondaryBraveLabel))
          }
          Button {
            
          } label: {
            Text(Strings.Wallet.add)
          }
          .buttonStyle(BraveFilledButtonStyle(size: .large))
        }
        .frame(maxWidth: .infinity)
        .padding(.top, 64)
        .padding([.leading, .trailing, .bottom])
      }
      .background(Color(.braveGroupedBackground).ignoresSafeArea())
      .toolbar {
        ToolbarItemGroup(placement: .cancellationAction) {
          Button {
            
          } label: {
            Text(Strings.cancelButtonTitle)
          }
        }
      }
      .navigationTitle("Add Suggested Token")
      .navigationBarTitleDisplayMode(.inline)
    }
    .navigationViewStyle(.stack)
  }
}

#if DEBUG
struct AddSuggestedTokenView_Previews: PreviewProvider {
  static var previews: some View {
    AddSuggestedTokenView(token: .previewToken)
  }
}
#endif
