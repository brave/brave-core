// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI
import UIKit

/// A representable around `UISearchBar` using the minimal style
public struct SearchBar: UIViewRepresentable {
  @Binding var text: String
  var placeholder: String?
  var isFocused: Binding<Bool>?
  var onSubmit: (() -> Void)?

  public init(
    text: Binding<String>,
    placeholder: String? = nil,
    isFocused: Binding<Bool>? = nil,
    onSubmit: (() -> Void)? = nil
  ) {
    self._text = text
    self.placeholder = placeholder
    self.isFocused = isFocused
    self.onSubmit = onSubmit
  }

  public func makeUIView(context: Context) -> UISearchBar {
    let searchBar = UISearchBar(frame: .zero)
    searchBar.text = text
    searchBar.placeholder = placeholder
    searchBar.searchBarStyle = .minimal
    return searchBar
  }

  public func updateUIView(_ uiView: UISearchBar, context: Context) {
    uiView.text = text
    uiView.placeholder = placeholder
    uiView.delegate = context.coordinator
    if let isFocused {
      if isFocused.wrappedValue {
        uiView.becomeFirstResponder()
      } else {
        uiView.resignFirstResponder()
      }
    }
  }

  public func makeCoordinator() -> Coordinator {
    Coordinator(text: $text, isFocused: isFocused, onSubmit: onSubmit)
  }

  public class Coordinator: NSObject, UISearchBarDelegate {
    @Binding var text: String
    var isFocused: Binding<Bool>?
    var onSubmit: (() -> Void)?

    init(text: Binding<String>, isFocused: Binding<Bool>?, onSubmit: (() -> Void)?) {
      self._text = text
      self.isFocused = isFocused
      self.onSubmit = onSubmit
    }

    public func searchBar(_ searchBar: UISearchBar, textDidChange searchText: String) {
      text = searchText
    }

    public func searchBarTextDidBeginEditing(_ searchBar: UISearchBar) {
      isFocused?.wrappedValue = true
    }

    public func searchBarTextDidEndEditing(_ searchBar: UISearchBar) {
      isFocused?.wrappedValue = false
    }

    public func searchBarSearchButtonClicked(_ searchBar: UISearchBar) {
      searchBar.resignFirstResponder()
      onSubmit?()
    }
  }
}
