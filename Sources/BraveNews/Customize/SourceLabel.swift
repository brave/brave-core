// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI
import BraveUI
import BraveStrings

private struct SourceImageView: View {
  var source: FeedItem.Source
  
  @ScaledMetric private var cornerRadius = 4
  @ScaledMetric private var imageHeight: CGFloat = 24
  @Environment(\.pixelLength) private var pixelLength
  
  private var backgroundColor: Color {
    if let bgColor = source.backgroundColor, !bgColor.isEmpty {
      return Color(UIColor(colorString: bgColor))
    }
    return Color.clear
  }
  
  private var containerShape: some InsettableShape {
    RoundedRectangle(cornerRadius: cornerRadius, style: .continuous)
  }
  
  var body: some View {
    backgroundColor
      .frame(width: imageHeight*2, height: imageHeight)
      .overlay(
        source.coverURL.map {
          WebImageReader(url: $0) { image in
            if let image {
              Image(uiImage: image)
                .resizable()
                .aspectRatio(contentMode: .fit)
            } else {
              ProgressView()
                .progressViewStyle(.braveCircular(size: .small, tint: .black))
                .opacity(0.2)
            }
          }
        }
      )
      .overlay(
        containerShape.strokeBorder(Color.black.opacity(0.1), lineWidth: pixelLength)
      )
      .clipShape(containerShape)
  }
}

struct SourceLabel: View {
  var source: FeedItem.Source
  var similarSource: FeedItem.Source?
  @Binding var isFollowing: Bool
  
  @ScaledMetric private var imageSize: CGFloat = 32.0
  @Environment(\.sizeCategory) private var sizeCategory
  
  @ViewBuilder private func containerView(
    @ViewBuilder content: () -> some View
  ) -> some View {
    if sizeCategory > .accessibilityLarge {
      VStack(alignment: .leading, spacing: 0) {
        content()
        FollowToggle(isFollowing: $isFollowing)
          .frame(maxWidth: .infinity, alignment: .trailing)
      }
    } else {
      HStack {
        content()
        Spacer()
        FollowToggle(isFollowing: $isFollowing)
      }
    }
  }
  
  var body: some View {
    containerView {
      Label {
        VStack(alignment: .leading, spacing: 2) {
          Text(source.name)
            .font(.footnote.bold())
            .foregroundColor(Color(.braveLabel))
            .frame(maxWidth: .infinity, alignment: .leading)
          if let similarSource {
            Text(String.localizedStringWithFormat(Strings.BraveNews.similarToSourceSubtitle, similarSource.name))
              .font(.footnote)
              .foregroundColor(Color(.secondaryBraveLabel))
          }
        }
        .padding(.vertical, 6)
      } icon: {
        SourceImageView(source: source)
          .foregroundColor(Color(.braveLabel))
      }
      .labelStyle(NewsLabelStyle())
    }
  }
}

#if DEBUG
struct SourceImageView_PreviewProvider: PreviewProvider {
  static var previews: some View {
    VStack {
      SourceLabel(
        source: Mock.sources[0],
        isFollowing: .constant(false)
      )
      Divider()
      SourceLabel(
        source: Mock.sources[0],
        similarSource: Mock.sources[1],
        isFollowing: .constant(false)
      )
    }
    .padding()
  }
}
#endif
