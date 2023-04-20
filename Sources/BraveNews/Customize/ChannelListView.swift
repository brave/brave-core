// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI
import BraveUI
import BraveStrings

struct ChannelListContainerView: View {
  @ObservedObject var dataSource: FeedDataSource
  
  var body: some View {
    ChannelListView(
      channels: Array(dataSource.channels),
      isFollowingChannel: {
        dataSource.isFollowingChannelBinding(
          channel: .init(localeIdentifier: dataSource.selectedLocale, name: $0)
        )
      }
    )
  }
}

private struct ChannelListView: View {
  var channels: [String]
  var isFollowingChannel: (String) -> Binding<Bool>
  
  var body: some View {
    List {
      ForEach(channels.sorted(), id: \.self) { channel in
        ChannelLabel(
          title: channel,
          isFollowing: isFollowingChannel(channel)
        )
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    .listStyle(.grouped)
    .listBackgroundColor(Color(.braveGroupedBackground))
    .environment(\.defaultMinListRowHeight, 0)
    .navigationBarTitleDisplayMode(.inline)
    .navigationTitle(Strings.BraveNews.channelsTitle)
  }
}

#if DEBUG
struct ChannelListView_PreviewProvider: PreviewProvider {
  static var previews: some View {
    NavigationView {
      ChannelListView(
        channels: Mock.channels,
        isFollowingChannel: { _ in .constant(false) }
      )
    }
  }
}
#endif
