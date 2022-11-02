// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

struct ChannelLabel: View {
  var title: String
  var subtitle: String?
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
          Text(title)
            .font(.footnote.bold())
            .foregroundColor(Color(.braveLabel))
          if let subtitle {
            Text(subtitle)
              .font(.footnote)
              .foregroundColor(Color(.secondaryBraveLabel))
          }
        }
      } icon: {
        Group {
          if let glyph = channelGlyphMap[title] {
            Image(braveSystemName: glyph)
              .imageScale(.small)
          } else {
            Text("\(title.first?.uppercased() ?? "")")
              .font(.callout.weight(.medium))
          }
        }
        .foregroundColor(Color(.braveLabel))
        .frame(width: imageSize, height: imageSize)
        .background(Color(.secondaryBraveBackground).clipShape(Circle()))
      }
      .labelStyle(NewsLabelStyle())
    }
  }
}

private let channelGlyphMap: [String: String] = [
  "Health": "brave.heart.and.hands",
  "Home": "brave.house",
  "Crypto": "brave.bitcoinsign",
  "Technology": "brave.desktop.and.phone",
  "Entertainment": "brave.star",
  "Top News": "brave.trophy",
  "Culture": "brave.book.alt",
  "Travel": "brave.mappin",
  "Business": "brave.building.2",
  "Sports": "brave.basketball",
  "Gaming": "brave.gamecontroller",
  "Science": "brave.erlenmeyerflask",
  "Fashion": "brave.tshirt",
  "Food": "brave.bread.and.carrot",
  "Weather": "brave.moon.cloud",
  "Top Sources": "brave.newspaper",
  "Brave": "brave.logo",
  "Politics": "brave.building.columns",
  "Fun": "brave.face.smiling",
  "Cars": "brave.car",
  "World News": "brave.globe",
]

#if DEBUG
struct ChannelLabel_PreviewProvider: PreviewProvider {
  static var previews: some View {
    VStack {
      ChannelLabel(title: "Brave", isFollowing: .constant(false))
      ChannelLabel(title: "Top Sources", subtitle: "Canada", isFollowing: .constant(true))
    }
  }
}
#endif
