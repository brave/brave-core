// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import Introspect
import SwiftUI

/// A `TextEditor` made to look in the same way as `BraveTextFieldStyle` and adds a similar looking "prompt" (placeholder) to the view.
public struct BraveTextEditor: View {
  @Binding private var text: String
  private let prompt: String

  private var isPromptHidden: Bool {
    text.isEmpty ? false : true
  }

  public init(text: Binding<String>, prompt: String) {
    _text = text
    self.prompt = prompt
  }

  public var body: some View {
    TextEditor(text: $text)
      .braveInputStyle()
      .introspectTextView { textView in
        textView.textContainerInset = .zero
        textView.textContainer.lineFragmentPadding = 0
      }
      .scrollContentBackground(.hidden)
      .overlay(
        Text(prompt)
          .multilineTextAlignment(.leading)
          .disabled(true)
          .allowsHitTesting(false)
          .font(.callout)
          .padding(12)
          .frame(maxWidth: .infinity, alignment: .leading)
          .foregroundColor(Color(.placeholderText))
          .opacity(isPromptHidden ? 0 : 1)
          .accessibilityHidden(isPromptHidden),
        alignment: .topLeading
      )
  }
}
