// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import DesignSystem
import SwiftUI

/// Displays the favicon for a dApp origin, or a globe if it cannot be loaded.
struct OriginInfoFavicon: View {

  let originInfo: BraveWallet.OriginInfo

  @ScaledMetric var faviconSize: CGFloat = 48
  let maxFaviconSize: CGFloat = 96

  var body: some View {
    Group {
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
    .frame(width: min(faviconSize, maxFaviconSize), height: min(faviconSize, maxFaviconSize))
    .clipShape(RoundedRectangle(cornerRadius: 4, style: .continuous))
  }

  private var globeFavicon: some View {
    Image(braveSystemName: "leo.internet")
      .resizable()
      .aspectRatio(contentMode: .fit)
      .padding(8)
      .background(Color(braveSystemName: .neutral20))
  }
}
