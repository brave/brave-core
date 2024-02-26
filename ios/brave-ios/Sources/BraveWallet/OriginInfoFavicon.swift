// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import SwiftUI

/// Displays the favicon for the OriginInfo, or the Brave Wallet logo for BraveWallet origin.
struct OriginInfoFavicon: View {

  let originInfo: BraveWallet.OriginInfo

  @ScaledMetric var faviconSize: CGFloat = 48
  let maxFaviconSize: CGFloat = 96

  var body: some View {
    Group {
      if originInfo.isBraveWalletOrigin {
        Image("wallet-brave-icon", bundle: .module)
          .resizable()
          .aspectRatio(contentMode: .fit)
          .padding(4)
          .frame(maxWidth: .infinity, maxHeight: .infinity)
          .background(Color(.braveDisabled))
      } else {
        if let url = URL(string: originInfo.originSpec) {
          FaviconReader(url: url) { image in
            if let image = image {
              Image(uiImage: image)
                .resizable()
            } else {
              globeFavicon
            }
          }
        } else {
          globeFavicon
        }
      }
    }
    .frame(width: min(faviconSize, maxFaviconSize), height: min(faviconSize, maxFaviconSize))
    .clipShape(RoundedRectangle(cornerRadius: 4, style: .continuous))
  }

  private var globeFavicon: some View {
    Image(systemName: "globe")
      .resizable()
      .aspectRatio(contentMode: .fit)
      .padding(8)
      .background(Color(.braveDisabled))
  }
}
