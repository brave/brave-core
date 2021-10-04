// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI
import Introspect

@available(iOS, introduced: 14.0, deprecated: 15.0)
struct FilterableViewModifier: ViewModifier {
  @Binding var query: String
  var prompt: String?
  var onSubmit: (() -> Void)?
  
  @StateObject private var searchDelegate = SearchDelegate()
  
  func body(content: Content) -> some View {
    content
      .introspectViewController { vc in
        if vc.navigationItem.searchController == nil {
          vc.navigationItem.searchController = UISearchController(searchResultsController: nil).then {
            $0.automaticallyShowsCancelButton = true
            $0.hidesNavigationBarDuringPresentation = false
            $0.obscuresBackgroundDuringPresentation = false
            $0.searchBar.delegate = searchDelegate
          }
        }
        vc.navigationItem.hidesSearchBarWhenScrolling = false
        vc.navigationItem.searchController?.searchBar.placeholder = prompt
        searchDelegate.onSubmit = onSubmit
      }
      .onChange(of: searchDelegate.query) { newValue in
        query = newValue
      }
  }
}

private class SearchDelegate: NSObject, UISearchBarDelegate, ObservableObject {
  @Published var query: String = ""
  var onSubmit: (() -> Void)?
  
  func searchBar(_ searchBar: UISearchBar, textDidChange searchText: String) {
    query = searchText
  }
  func searchBarCancelButtonClicked(_ searchBar: UISearchBar) {
    query = ""
  }
  func searchBarSearchButtonClicked(_ searchBar: UISearchBar) {
    onSubmit?()
  }
}

extension View {
  /// Adds a search bar to the parent navigation controller to simply filter the contents of the View
  ///
  /// Optionally you may only apply a filter when the user taps search/hits enter on the keyboard
  @ViewBuilder public func filterable(
    text: Binding<String>,
    prompt: String? = nil,
    onSubmit: (() -> Void)? = nil
  ) -> some View {
    if #available(iOS 15.0, *) {
      searchable(
        text: text,
        placement: .navigationBarDrawer(displayMode: .always),
        prompt: prompt.map(Text.init)
      )
      .onSubmit(of: .search) {
        onSubmit?()
      }
    } else {
      modifier(FilterableViewModifier(query: text, prompt: prompt, onSubmit: onSubmit))
    }
  }
}
