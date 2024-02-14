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
  "Health": "leo.news.health",
  "Home": "leo.news.home",
  "Crypto": "leo.bitcoin",
  "Technology": "leo.news.technology", // Deprecated
  "Entertainment": "leo.news.filmandtv", // Deprecated
  "Top News": "leo.news.topnews",
  "Culture": "leo.news.culture",
  "Travel": "leo.news.travel",
  "Business": "leo.news.business",
  "Sports": "leo.news.sports",
  "Gaming": "leo.news.gaming",
  "Science": "leo.news.science",
  "Fashion": "leo.news.fashion",
  "Food": "leo.news.food",
  "Weather": "leo.news.weather",
  "Top Sources": "leo.news.default",
  "Brave": "leo.news.brave",
  "Politics": "leo.news.politics",
  "Fun": "leo.news.fun",
  "Cars": "leo.news.car",
  "World News": "leo.news.worldnews",
  // Added Later
  "Entertainment News": "leo.news.entertainment",
  "Film and TV": "leo.news.filmandtv",
  "Music": "leo.news.music",
  "Tech News": "leo.news.technology",
  "Tech Reviews": "leo.news.technology-reviews",
  "US News": "leo.news.regional",
  "UK News": "leo.news.regional",
]

#if DEBUG
struct ChannelLabel_PreviewProvider: PreviewProvider {
  static var previews: some View {
    VStack {
      ChannelLabel(title: "Brave", isFollowing: .constant(false))
      ChannelLabel(title: "Top Sources", subtitle: "Canada", isFollowing: .constant(true))
    }
    .padding()
  }
}
#endif
