// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import Combine
import Strings
import SwiftUI
import UIKit

struct TabGridSearchBar: View {
  @Binding var text: String
  @Binding var isFocused: Bool
  var scrollView: UIScrollView

  var body: some View {
    HStack {
      Representable(text: $text, isFocused: $isFocused)
      if isFocused {
        Button {
          isFocused = false
          text = ""
        } label: {
          Text(Strings.CancelString)
            .foregroundStyle(Color(braveSystemName: .textInteractive))
        }
        .transition(.move(edge: .trailing).combined(with: .opacity))
      }
    }
    .animation(.toolbarsSizeAnimation, value: isFocused)
    .dynamicTypeSize(.xSmall..<DynamicTypeSize.accessibility1)
  }
}

extension TabGridSearchBar {
  private struct Representable: UIViewRepresentable {
    @Binding var text: String
    @Binding var isFocused: Bool

    public init(
      text: Binding<String>,
      isFocused: Binding<Bool>,
      onSubmit: (() -> Void)? = nil
    ) {
      self._text = text
      self._isFocused = isFocused
    }

    public func makeUIView(context: Context) -> UISearchTextField {
      let searchTextField = UISearchTextField()
      searchTextField.text = text
      searchTextField.placeholder = Strings.tabTraySearchBarTitle
      searchTextField.delegate = context.coordinator
      searchTextField.clipsToBounds = true
      searchTextField.addTarget(
        context.coordinator,
        action: #selector(Coordinator.textFieldTextDidChange(_:)),
        for: .editingChanged
      )
      return searchTextField
    }

    public func updateUIView(_ uiView: UISearchTextField, context: Context) {
      uiView.text = text
      if isFocused {
        if !uiView.isFirstResponder {
          uiView.becomeFirstResponder()
        }
      } else {
        if uiView.isFirstResponder {
          // Thread-hop to avoid a AttributeGraph cycle
          DispatchQueue.main.async {
            uiView.resignFirstResponder()
          }
        }
      }
    }

    func sizeThatFits(
      _ proposal: ProposedViewSize,
      uiView: UISearchTextField,
      context: Context
    ) -> CGSize? {
      return .init(width: proposal.replacingUnspecifiedDimensions().width, height: 36.0)
    }

    public func makeCoordinator() -> Coordinator {
      Coordinator(text: $text, isFocused: $isFocused)
    }

    public class Coordinator: NSObject, UISearchTextFieldDelegate {
      @Binding var text: String
      @Binding var isFocused: Bool

      init(text: Binding<String>, isFocused: Binding<Bool>) {
        self._text = text
        self._isFocused = isFocused
      }

      @objc func textFieldTextDidChange(_ sender: UISearchTextField) {
        text = sender.text ?? ""
      }

      public func textFieldDidBeginEditing(_ textField: UITextField) {
        isFocused = true
      }

      public func textFieldDidEndEditing(_ textField: UITextField) {
        isFocused = false
      }

      public func textFieldShouldReturn(_ textField: UITextField) -> Bool {
        textField.resignFirstResponder()
        return true
      }
    }
  }
}
