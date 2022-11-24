// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI
import BraveUI
import Introspect

struct FollowingListContainerView: View {
  @ObservedObject var dataSource: FeedDataSource
  
  var body: some View {
    FollowingListView(
      fetchSources: { Array(dataSource.followedSources) },
      isFollowingSource: { dataSource.isFollowingSourceBinding(source: $0) },
      fetchChannels: { Array(dataSource.followedChannels) },
      isFollowingChannel: { dataSource.isFollowingChannelBinding(channel: $0)}
    )
  }
}

struct FollowingListView: View {
  var fetchSources: () -> [FeedItem.Source]
  var isFollowingSource: (FeedItem.Source) -> Binding<Bool>
  var fetchChannels: () -> [FeedChannel]
  var isFollowingChannel: (FeedChannel) -> Binding<Bool>
  
  @State private var followedSources: [FeedItem.Source] = []
  @State private var followedChannels: [FeedChannel] = []
  
  var body: some View {
    List {
      if !followedSources.isEmpty {
        Section {
          ForEach(followedSources) { source in
            SourceLabel(source: source, isFollowing: isFollowingSource(source))
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        } header: {
          Text("Sources")
        }
      }
      if !followedChannels.isEmpty {
        Section {
          ForEach(followedChannels) { channel in
            let shouldShowRegionSubtitle = followedChannels.filter {
              $0.name == channel.name
            }.count > 1
            ChannelLabel(
              title: channel.name,
              subtitle: shouldShowRegionSubtitle ? channel.localeDescription : nil,
              isFollowing: isFollowingChannel(channel)
            )
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
            .padding(.vertical, 4)
          }
        } header: {
          Text("Channels")
        }
      }
    }
    .listBackgroundColor(Color(.braveGroupedBackground))
    .listStyle(.insetGrouped)
    .environment(\.defaultMinListRowHeight, 0)
    .navigationTitle("Following")
    .navigationBarTitleDisplayMode(.inline)
    .onAppear {
      // Since we dont want to remove entries from the list immediately, we will not be using
      // the followed sources & channel list directly and instead copying them
      followedSources = fetchSources()
      followedChannels = fetchChannels().sorted(by: { $0.name < $1.name })
    }
    .listInitialOffsetWorkaround()
  }
}

#if DEBUG
struct FollowingListView_PreviewProvider: PreviewProvider {
  static var previews: some View {
    NavigationView {
      FollowingListView(
        fetchSources: { Mock.sources },
        isFollowingSource: { _ in .constant(true) },
        fetchChannels: {
          [
            .init(localeIdentifier: "en_US", name: "Top Sources"),
            .init(localeIdentifier: "en_CA", name: "Top Sources"),
            .init(localeIdentifier: "en_US", name: "Brave"),
            .init(localeIdentifier: "en_US", name: "Weather"),
          ]
        },
        isFollowingChannel: { _ in .constant(true) }
      )
    }
  }
}
#endif
