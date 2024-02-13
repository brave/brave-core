// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI
import BraveStrings
import BraveUI

struct SourceListContainerView: View {
  @ObservedObject var dataSource: FeedDataSource
  
  var localizedSourcesByRank: [FeedItem.Source] {
    let locale = dataSource.selectedLocale
    let sources = dataSource.sources.filter {
      $0.localeDetails?.contains(where: { $0.locale == locale }) ?? false
    }
    return sources.sorted(by: {
      $0.rank(of: locale) < $1.rank(of: locale)
    })
  }
  
  var body: some View {
    return SourceListView(
      sources: localizedSourcesByRank,
      isFollowing: { dataSource.isFollowingSourceBinding(source: $0) }
    )
  }
}

private struct SourceListView: View {
  var sources: [FeedItem.Source]
  var isFollowing: (FeedItem.Source) -> Binding<Bool>
  
  var body: some View {
    List {
      Section {
        ForEach(sources) { source in
          SourceLabel(source: source, isFollowing: isFollowing(source))
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
    }
    .environment(\.defaultMinListRowHeight, 44)
    .listBackgroundColor(Color(.braveGroupedBackground))
    .listStyle(.grouped)
    .navigationTitle(Strings.BraveNews.popularSourcesTitle)
    .navigationBarTitleDisplayMode(.inline)
  }
}

#if DEBUG
struct SourceListView_PreviewProvider: PreviewProvider {
  static var previews: some View {
    NavigationView {
      SourceListView(sources: Mock.sources, isFollowing: { _ in .constant(false) })
    }
    .navigationViewStyle(.stack)
    .accentColor(Color(.braveBlurpleTint))
  }
}
#endif
