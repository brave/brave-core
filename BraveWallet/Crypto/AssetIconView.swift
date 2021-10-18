// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveCore

/// Displays an asset's icon from the token registry
///
/// By default, creating an `AssetIconView` will result in a dynamically sized icon based
/// on the users size category. If you for some reason need to obtain a fixed size asset icon,
/// wrap this view in another frame of your desired size, for example:
///
///     AssetIconView(token: .eth)
///       .frame(width: 20, height: 20)
///
struct AssetIconView: View {
  var token: BraveWallet.ERCToken
  @ScaledMetric var length: CGFloat = 40
  
  private var image: Image {
    if token.isETH {
      // Use bundled ETH icon
      return Image("eth-icon")
    }
    let fallbackImage = Image(uiImage: UIImage()) // TODO: Obtain a fallback image
    guard let baseURL = BraveWallet.TokenRegistryUtils.tokenLogoBaseURL,
          case let imageURL = baseURL.appendingPathComponent(token.logo),
          let image = UIImage(contentsOfFile: imageURL.path) else {
            return fallbackImage
          }
    return Image(uiImage: image)
  }
  
  var body: some View {
    image
      .resizable()
      .aspectRatio(contentMode: .fit)
      .frame(width: length, height: length)
  }
}

struct AssetIconView_Previews: PreviewProvider {
  static var previews: some View {
    AssetIconView(token: .eth)
      .previewLayout(.sizeThatFits)
      .padding()
      .previewSizeCategories()
  }
}
