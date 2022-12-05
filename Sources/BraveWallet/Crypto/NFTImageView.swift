/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI
import BraveUI
import SDWebImageSwiftUI

struct NFTImageView<Placeholder: View>: View {
  private let urlString: String
  private var placeholder: () -> Placeholder
  
  init(
    urlString: String,
    @ViewBuilder placeholder: @escaping () -> Placeholder
  ) {
    self.urlString = urlString
    self.placeholder = placeholder
  }
  
  var body: some View {
    if urlString.hasPrefix("data:image/") {
      WebImageReader(url: URL(string: urlString)) { image, isFinished in
        if let image = image {
          Image(uiImage: image)
            .resizable()
            .aspectRatio(contentMode: .fit)
        } else {
          placeholder()
        }
      }
    } else {
      if urlString.hasSuffix(".svg") {
        WebImageReader(url: URL(string: urlString)) { image, isFinished in
          if let image = image {
            Image(uiImage: image)
              .resizable()
              .aspectRatio(contentMode: .fit)
          } else {
            placeholder()
          }
        }
      } else {
        WebImage(url: URL(string: urlString))
          .resizable()
          .placeholder { placeholder() }
          .indicator(.activity)
          .aspectRatio(contentMode: .fit)
      }
    }
  }
}
