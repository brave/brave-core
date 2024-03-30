// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

struct SearchBar: UIViewRepresentable {

  @Binding var text: String
  var placeholder = ""

  func makeUIView(context: Context) -> UISearchBar {
    let searchBar = UISearchBar(frame: .zero)
    searchBar.text = text
    searchBar.placeholder = placeholder
    // remove black divider lines above/below field
    searchBar.searchBarStyle = .minimal
    // don't disable 'Search' when field empty
    searchBar.enablesReturnKeyAutomatically = false
    return searchBar
  }

  func updateUIView(_ uiView: UISearchBar, context: Context) {
    uiView.text = text
    uiView.placeholder = placeholder
    uiView.delegate = context.coordinator
  }

  func makeCoordinator() -> Coordinator {
    Coordinator(text: $text)
  }

  class Coordinator: NSObject, UISearchBarDelegate {
    @Binding var text: String

    init(text: Binding<String>) {
      _text = text
    }

    func searchBar(_ searchBar: UISearchBar, textDidChange searchText: String) {
      text = searchText
    }

    func searchBarSearchButtonClicked(_ searchBar: UISearchBar) {
      // dismiss keyboard when 'Search' / return key tapped
      searchBar.resignFirstResponder()
    }
  }
}
