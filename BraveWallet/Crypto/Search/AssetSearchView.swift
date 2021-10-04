/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import BraveCore

private struct TokenView: View {
  var token: BraveWallet.ERCToken
  
  var body: some View {
    HStack(spacing: 8) {
      Circle()
        .frame(width: 40, height: 40)
        .overlay(
          Image(uiImage: .init())
        )
        .clipShape(Circle())
      VStack(alignment: .leading) {
        Text(token.name)
          .fontWeight(.semibold)
          .foregroundColor(Color(.bravePrimary))
        Text(token.symbol.uppercased())
          .foregroundColor(Color(.secondaryBraveLabel))
      }
      .font(.footnote)
    }
    .padding(.vertical, 8)
  }
}

struct AssetSearchView: View {
  var tokenRegistry: BraveWalletERCTokenRegistry
  var keyringStore: KeyringStore
  var networkStore: NetworkStore
  
  @State private var allTokens: [BraveWallet.ERCToken] = []
  @State private var query = ""
  
  private var tokens: [BraveWallet.ERCToken] {
    let query = query.lowercased()
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
      List {
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
              if query.isEmpty {
                Text("No assets found") // NSLocalizedString
              } else {
                Text("No assets found for \"\(query)\" query") // NSLocalizedString
              }
            }
            .font(.footnote)
            .foregroundColor(Color(.secondaryBraveLabel))
            .multilineTextAlignment(.center)
            .frame(maxWidth: .infinity)
          } else {
            ForEach(tokens) { token in
              NavigationLink(
                destination: AssetDetailView(
                  keyringStore: keyringStore,
                  networkStore: networkStore,
                  token: token
                )
              ) {
                TokenView(token: token)
              }
            }
          }
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      .navigationBarTitleDisplayMode(.inline)
      .navigationTitle("Search") // NSLocalizedString
      .listStyle(InsetGroupedListStyle())
      .animation(nil, value: query)
      .filterable(text: $query)
      .introspectViewController { vc in
        // TODO: In iOS 15, use `toolbar` & `presentationMode` now that PresentationMode is passed to
        //       SwiftUI when presented from UIKit
        vc.navigationItem.leftBarButtonItem = .init(systemItem: .cancel, primaryAction: .init(handler: { [unowned vc] _ in
          vc.dismiss(animated: true)
        }))
      }
    }
    .navigationViewStyle(StackNavigationViewStyle())
    .onAppear {
      tokenRegistry.allTokens { tokens in
        self.allTokens = tokens.sorted(by: { $0.symbol < $1.symbol })
      }
    }
  }
}
