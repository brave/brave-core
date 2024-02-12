// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI
import BraveStrings

struct SourceSuggestionsContainerView: View {
  @ObservedObject var dataSource: FeedDataSource
  
  private var suggestionsForFollowedSources: [Suggestion] {
    let followedSources = dataSource.followedSources.sorted(by: {
      $0.rank(of: dataSource.selectedLocale) < $1.rank(of: dataSource.selectedLocale)
    })
    var allSuggestions: [Suggestion] = []
    for source in followedSources {
      guard let sourceSuggestions = dataSource.sourceSuggestions[source.id] else { continue }
      allSuggestions.append(contentsOf: sourceSuggestions
        .sorted(by: { $0.relativeScore > $1.relativeScore })
        .compactMap { sourceSimilarity in
          guard let suggestedSource = dataSource.sources.first(where: { $0.id == sourceSimilarity.sourceID }) else {
            return nil
          }
          return Suggestion(source: suggestedSource, similarToSource: source)
        }
      )
    }
    return allSuggestions
  }
  
  var body: some View {
    SourceSuggestionsView(
      suggestions: suggestionsForFollowedSources,
      isFollowing: { dataSource.isFollowingSourceBinding(source: $0) }
    )
  }
}

private struct Suggestion: Identifiable {
  var source: FeedItem.Source
  var similarToSource: FeedItem.Source
  var id: String {
    source.id
  }
}

private struct SourceSuggestionsView: View {
  var suggestions: [Suggestion]
  var isFollowing: (FeedItem.Source) -> Binding<Bool>
  
  var body: some View {
    List {
      ForEach(suggestions) { suggestion in
        SourceLabel(
          source: suggestion.source,
          similarSource: suggestion.similarToSource,
          isFollowing: isFollowing(suggestion.source)
        )
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    .environment(\.defaultMinListRowHeight, 44)
    .listBackgroundColor(Color(.braveGroupedBackground))
    .listStyle(.grouped)
    .navigationTitle(Strings.BraveNews.suggestedTitle)
    .navigationBarTitleDisplayMode(.inline)
  }
}

#if DEBUG
struct SourceSuggestions_PreviewProvider: PreviewProvider {
  static var previews: some View {
    SourceSuggestionsView(suggestions: [], isFollowing: { _ in .constant(false) })
  }
}
#endif
