// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import SDWebImageSwiftUI
import SwiftUI

struct NFTImageView<Placeholder: View>: View {
  private let urlString: String
  private var placeholder: () -> Placeholder
  var isLoading: Bool

  init(
    urlString: String,
    isLoading: Bool = false,
    @ViewBuilder placeholder: @escaping () -> Placeholder
  ) {
    self.urlString = urlString
    self.isLoading = isLoading
    self.placeholder = placeholder
  }

  var body: some View {
    if isLoading {
      LoadingNFTView()
    } else {
      if let url = URL(string: urlString) {
        if url.absoluteString.hasPrefix("data:image/") {
          WebImageReader(url: url) { image in
            if let image = image {
              Image(uiImage: image)
                .resizable()
                .aspectRatio(contentMode: .fit)
            } else {
              placeholder()
            }
          }
        } else if url.isSecureWebPage() {
          if url.absoluteString.hasSuffix(".gif") {
            WebImage(url: url)
              .resizable()
              .placeholder { placeholder() }
              .aspectRatio(contentMode: .fit)
          } else {
            WebImageReader(url: url) { image in
              if let image = image {
                Image(uiImage: image)
                  .resizable()
                  .aspectRatio(contentMode: .fit)
              } else {
                placeholder()
              }
            }
          }
        } else {
          placeholder()
        }
      } else {
        placeholder()
      }
    }
  }
}

struct LoadingNFTView: View {
  var shimmer: Bool = true
  @State var viewSize: CGSize = .zero

  private var fontSizeForNFTImage: CGFloat {
    guard viewSize.width > 0 else { return 12 }
    return floor(viewSize.width / 3)
  }

  var body: some View {
    Color(braveSystemName: .containerHighlight)
      .cornerRadius(4)
      .redacted(reason: .placeholder)
      .shimmer(shimmer)
      .overlay {
        Image(braveSystemName: "leo.nft")
          .foregroundColor(Color(braveSystemName: .containerBackground))
          .font(.system(size: fontSizeForNFTImage))
          .aspectRatio(contentMode: .fit)
      }
      .background(
        GeometryReader { geometryProxy in
          Color.clear
            .preference(key: SizePreferenceKey.self, value: geometryProxy.size)
        }
      )
      .aspectRatio(1, contentMode: .fit)
      .onPreferenceChange(SizePreferenceKey.self) { newSize in
        viewSize = newSize
      }
  }
}
