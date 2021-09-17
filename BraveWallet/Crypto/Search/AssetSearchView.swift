/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import BraveCore

@available(iOS 14.0, *)
struct AssetSearchView: View {
  var tokenRegistry: BraveWalletERCTokenRegistry
  @State private var allTokens: [BraveWallet.ERCToken] = []
  @StateObject private var searchDelegate = SearchDelegate()
  
  private var tokens: [BraveWallet.ERCToken] {
    let query = searchDelegate.query.lowercased()
    if query.isEmpty {
      return allTokens
    }
    return allTokens.filter {
      $0.symbol.lowercased().contains(query) ||
      $0.name.lowercased().contains(query)
    }
  }
  
  var body: some View {
    NavigationView {
      Form {
        Section(
          header: WalletListHeaderView(
            title: Text("Assets") // NSLocalizedString
          )
          .osAvailabilityModifiers { content in
            if #available(iOS 15.0, *) {
              content // Padding already applied
            } else {
              content
                .padding(.top)
            }
          }
        ) {
          let tokens = self.tokens
          if tokens.isEmpty {
            Group {
              if searchDelegate.query.isEmpty {
                Text("No assets found") // NSLocalizedString
              } else {
                Text("No assets found for \"\(searchDelegate.query)\" query") // NSLocalizedString
              }
            }
            .font(.footnote)
            .foregroundColor(Color(.secondaryBraveLabel))
            .multilineTextAlignment(.center)
            .frame(maxWidth: .infinity)
          } else {
            ForEach(tokens) { token in
              Text(token.name)
            }
          }
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      .navigationBarTitleDisplayMode(.inline)
      .navigationTitle("Search") // NSLocalizedString
      .onAppear {
        tokenRegistry.allTokens { tokens in
          self.allTokens = tokens
        }
      }
      .listStyle(InsetGroupedListStyle())
      .introspectViewController { vc in
        // TODO: In iOS 15, use `searchable`
        if vc.navigationItem.searchController == nil {
          vc.navigationItem.searchController = UISearchController(searchResultsController: nil).then {
            $0.automaticallyShowsCancelButton = true
            $0.hidesNavigationBarDuringPresentation = false
            $0.obscuresBackgroundDuringPresentation = false
            $0.searchBar.delegate = searchDelegate
          }
          vc.navigationItem.hidesSearchBarWhenScrolling = false
          // TODO: In iOS 15, use `toolbar` & `presentationMode` now that PresentationMode is passed to
          //       SwiftUI when presented from UIKit
          vc.navigationItem.leftBarButtonItem = .init(systemItem: .cancel, primaryAction: .init(handler: { [unowned vc] _ in
            vc.dismiss(animated: true)
          }))
        }
      }
    }
    .navigationViewStyle(StackNavigationViewStyle())
  }
}

private class SearchDelegate: NSObject, UISearchBarDelegate, ObservableObject {
  @Published var query: String = ""
  
  func searchBar(_ searchBar: UISearchBar, textDidChange searchText: String) {
    query = searchText
  }
  func searchBarCancelButtonClicked(_ searchBar: UISearchBar) {
    query = ""
  }
}
