// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveUI

struct SearchResultsView: View {
  var results: SearchResults
  var isFollowingSource: (FeedItem.Source) -> Binding<Bool>
  var isFollowingChannel: (FeedChannel) -> Binding<Bool>
  
  private var isResultsEmpty: Bool {
    results.sources.isEmpty && results.channels.isEmpty
  }
  
  var body: some View {
    List {
      if !results.channels.isEmpty {
        Section {
          ForEach(results.channels, id: \.self) { channel in
            let shouldShowRegionSubtitle = results.channels.filter {
              $0.name == channel.name
            }.count > 1
            ChannelLabel(
              title: channel.name,
              subtitle: shouldShowRegionSubtitle ? channel.localeDescription : nil,
              isFollowing: isFollowingChannel(channel)
            )
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        } header: {
          Text("Channels") // TODO: Localize
        }
      }
      if !results.sources.isEmpty {
        Section {
          ForEach(results.sources) { source in
            SourceLabel(source: source, isFollowing: isFollowingSource(source))
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        } header: {
          Text("Sources") // TODO: Localize
        }
      }
    }
    .listStyle(.insetGrouped)
    .listBackgroundColor(Color(.braveGroupedBackground))
    .overlay(Group {
      if isResultsEmpty {
        Text("No Results Found") // TODO: Localize
          .foregroundColor(Color(.secondaryBraveLabel))
      }
    })
  }
}

#if DEBUG
struct SearchResultsView_PreviewProvider: PreviewProvider {
  static var previews: some View {
    Group {
      SearchResultsView(
        results: .init(sources: [], channels: []),
        isFollowingSource: { _ in .constant(false) },
        isFollowingChannel: { _ in .constant(false) }
      )
      .previewDisplayName("No Results")
      SearchResultsView(
        results: .init(sources: Mock.sources, channels: [.init(localeIdentifier: "en_US", name: "Business")]),
        isFollowingSource: { _ in .constant(false) },
        isFollowingChannel: { _ in .constant(true) }
      )
      .previewDisplayName("Results")
    }
  }
}
#endif
