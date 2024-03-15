// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import Strings
import SwiftUI

struct TokenList<Item: Identifiable, Header: View, Content: View, EmptyStateView: View>: View {
  var tokens: [Item]
  var searchRules: (_ query: String, _ token: Item) -> Bool
  var header: Header
  var emptyStateView: EmptyStateView
  var content: (Item) -> Content

  @State private var query: String

  private var filteredTokens: [Item] {
    let normalizedQuery = query.lowercased()
    if normalizedQuery.isEmpty {
      return tokens
    }
    return tokens.filter { searchRules(normalizedQuery, $0) }
  }

  init(
    tokens: [Item],
    prefilledQuery: String? = nil,
    searchRules: @escaping (_ query: String, _ token: Item) -> Bool,
    @ViewBuilder header: @escaping () -> Header,
    @ViewBuilder emptyStateView: @escaping () -> EmptyStateView,
    @ViewBuilder content: @escaping (Item) -> Content
  ) {
    self.tokens = tokens
    self.searchRules = searchRules
    self.header = header()
    self.emptyStateView = emptyStateView()
    self.content = content
    self._query = .init(initialValue: prefilledQuery ?? "")
  }

  var body: some View {
    List {
      Section(header: header) {
        if tokens.isEmpty {
          emptyStateView
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
        } else {
          Group {
            if filteredTokens.isEmpty {
              Text(Strings.Wallet.assetSearchEmpty)
                .font(.footnote)
                .foregroundColor(Color(.secondaryBraveLabel))
                .multilineTextAlignment(.center)
                .frame(maxWidth: .infinity)
            } else {
              ForEach(filteredTokens) { token in
                content(token)
                  .listRowSeparator(.hidden)
              }
            }
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
      }
    }
    .listStyle(PlainListStyle())
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
    .animation(nil, value: query)
    .searchable(
      text: $query,
      placement: .navigationBarDrawer(displayMode: .always)
    )
  }
}

extension TokenList where Header == EmptyView {
  init(
    tokens: [Item],
    prefilledQuery: String? = nil,
    searchRules: @escaping (_ query: String, _ token: Item) -> Bool,
    @ViewBuilder emptyStateView: @escaping () -> EmptyStateView,
    @ViewBuilder content: @escaping (Item) -> Content
  ) {
    self.tokens = tokens
    self.searchRules = searchRules
    self.header = EmptyView()
    self.emptyStateView = emptyStateView()
    self.content = content
    self._query = .init(initialValue: prefilledQuery ?? "")
  }
}

extension TokenList where EmptyStateView == EmptyView {
  init(
    tokens: [Item],
    prefilledQuery: String? = nil,
    searchRules: @escaping (_ query: String, _ token: Item) -> Bool,
    @ViewBuilder header: @escaping () -> Header,
    @ViewBuilder content: @escaping (Item) -> Content
  ) {
    self.tokens = tokens
    self.searchRules = searchRules
    self.header = header()
    self.emptyStateView = EmptyView()
    self.content = content
    self._query = .init(initialValue: prefilledQuery ?? "")
  }
}

extension TokenList where Header == EmptyView, EmptyStateView == EmptyView {
  init(
    tokens: [Item],
    prefilledQuery: String? = nil,
    searchRules: @escaping (_ query: String, _ token: Item) -> Bool,
    @ViewBuilder content: @escaping (Item) -> Content
  ) {
    self.tokens = tokens
    self.searchRules = searchRules
    self.header = EmptyView()
    self.emptyStateView = EmptyView()
    self.content = content
    self._query = .init(initialValue: prefilledQuery ?? "")
  }
}

#if DEBUG
struct TokenListView_Previews: PreviewProvider {
  static var previews: some View {
    TokenList(
      tokens: MockBlockchainRegistry.testTokens
    ) { _, _ in
      return true
    } content: { token in
      Text(token.name)
    }
  }
}
#endif
