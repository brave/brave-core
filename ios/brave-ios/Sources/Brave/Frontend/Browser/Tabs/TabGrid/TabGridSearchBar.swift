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
  static let defaultHeight: CGFloat = 36.0
  static let padding: CGFloat = 8.0

  @Binding var text: String
  @Binding var isFocused: Bool
  var scrollView: UIScrollView

  @State private var height: CGFloat = Self.defaultHeight

  var body: some View {
    HStack {
      Representable(text: $text, isFocused: $isFocused, height: height)
        .onReceive(scrollView.publisher(for: \.contentOffset)) { offset in
          let defaultHeight = Self.defaultHeight
          let newHeight = -(offset.y + scrollView.contentInset.top - defaultHeight)
          height = isFocused ? defaultHeight : min(defaultHeight, max(0, newHeight))
        }
      if isFocused {
        Button {
          isFocused = false
          text = ""
        } label: {
          Text(Strings.CancelString)
            .foregroundStyle(Color(braveSystemName: .textInteractive))
        }
        .frame(height: height)
        .transition(.move(edge: .trailing).combined(with: .opacity))
      }
    }
    .animation(.snappy, value: isFocused)
    .dynamicTypeSize(.xSmall..<DynamicTypeSize.accessibility1)
  }
}

extension TabGridSearchBar {
  private struct Representable: UIViewRepresentable {
    @Binding var text: String
    @Binding var isFocused: Bool
    var height: CGFloat

    public init(
      text: Binding<String>,
      isFocused: Binding<Bool>,
      onSubmit: (() -> Void)? = nil,
      height: CGFloat,
    ) {
      self._text = text
      self._isFocused = isFocused
      self.height = height
    }

    public func makeUIView(context: Context) -> TabCollectionViewSearchTextField {
      let searchTextField = TabCollectionViewSearchTextField()
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

    public func updateUIView(_ uiView: TabCollectionViewSearchTextField, context: Context) {
      uiView.text = text
      if isFocused {
        uiView.becomeFirstResponder()
      } else {
        uiView.resignFirstResponder()
      }
    }

    func sizeThatFits(
      _ proposal: ProposedViewSize,
      uiView: TabCollectionViewSearchTextField,
      context: Context
    ) -> CGSize? {
      return .init(width: proposal.replacingUnspecifiedDimensions().width, height: height)
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

  class TabCollectionViewSearchTextField: UISearchTextField {
    override var frame: CGRect {
      didSet {
        if oldValue == frame { return }
        let translationToZeroAlpha = 12.0
        let alpha =
          ((frame.height - (TabGridSearchBar.defaultHeight - translationToZeroAlpha))
            / translationToZeroAlpha)
        let clampedAlpha = max(0.0, min(1.0, alpha))
        subviews.filter { $0 is UILabel || $0 is UIImageView }.forEach {
          $0.alpha = clampedAlpha
        }
      }
    }
  }
}
