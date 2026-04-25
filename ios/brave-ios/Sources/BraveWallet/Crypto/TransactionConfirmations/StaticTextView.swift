// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

/// We needed a `TextEditor` that couldn't be edited and had a clear background color
/// so we have to fallback to UIKit for this
struct StaticTextView: UIViewRepresentable {
  var text: String
  var attributedText: NSAttributedString?
  var isMonospaced: Bool = true

  func makeUIView(context: Context) -> UITextView {
    let textView = UITextView()
    if let attributedText {
      textView.attributedText = attributedText
    } else {
      textView.text = text
    }
    textView.isEditable = false
    textView.backgroundColor = .tertiaryBraveGroupedBackground
    textView.font = {
      let metrics = UIFontMetrics(forTextStyle: .body)
      let desc = UIFontDescriptor.preferredFontDescriptor(withTextStyle: .body)
      let font =
        isMonospaced
        ? UIFont.monospacedSystemFont(ofSize: desc.pointSize, weight: .regular)
        : UIFont.systemFont(ofSize: desc.pointSize, weight: .regular)
      return metrics.scaledFont(for: font)
    }()
    textView.adjustsFontForContentSizeCategory = true
    textView.textContainerInset = .init(top: 12, left: 8, bottom: 12, right: 8)
    return textView
  }
  func updateUIView(_ uiView: UITextView, context: Context) {
    if let attributedText {
      uiView.attributedText = attributedText
    } else {
      uiView.text = text
    }
  }
}
