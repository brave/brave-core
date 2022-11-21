// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

struct RSSFeedLabel: View {
  var feed: RSSFeedLocation
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
          if let title = feed.title {
            Text(title)
              .font(.footnote.bold())
              .foregroundColor(Color(.braveLabel))
          }
          Text(feed.url.absoluteString)
            .font(.footnote)
            .foregroundColor(Color(.secondaryBraveLabel))
        }
      } icon: {
        Group {
          Text("\((feed.title ?? feed.url.host ?? "").first?.uppercased() ?? "")")
            .font(.callout.weight(.medium))
        }
        .foregroundColor(Color(.braveLabel))
        .frame(width: imageSize, height: imageSize)
        .background(Color(.secondaryBraveBackground).clipShape(Circle()))
      }
      .labelStyle(NewsLabelStyle())
    }
  }
}

#if DEBUG
struct RSSFeedLabel_PreviewProvider: PreviewProvider {
  static var previews: some View {
    VStack {
      RSSFeedLabel(feed: .init(url: URL(string: "https://brave.com/blog/index.xml")!), isFollowing: .constant(true))
      RSSFeedLabel(feed: .init(title: "Brave Blog", url: URL(string: "https://brave.com/blog/index.xml")!), isFollowing: .constant(true))
    }
    .padding()
  }
}
#endif
