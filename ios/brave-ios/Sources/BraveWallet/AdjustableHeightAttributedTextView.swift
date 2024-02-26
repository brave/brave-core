// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

/// A `AttributedTextView` with adjustable height to fit the given text. No .frame modifier needed
struct AdjustableHeightAttributedTextView: View {
  var attributedString: NSAttributedString
  var openLink: ((URL?) -> Void)?

  @State private var height: CGFloat = .zero

  var body: some View {
    AttributedTextView(
      attributedString: attributedString,
      dynamicHeight: $height,
      openLink: openLink
    )
    .frame(height: height)
  }
}

/// A `UIViewPresentable` of `UITextView`,
struct AttributedTextView: UIViewRepresentable {
  private var attributedString: NSAttributedString
  @Binding var dynamicHeight: CGFloat
  private var openLink: ((URL?) -> Void)?

  init(
    attributedString: NSAttributedString,
    dynamicHeight: Binding<CGFloat>,
    openLink: ((URL?) -> Void)?
  ) {
    self.attributedString = attributedString
    self.openLink = openLink
    _dynamicHeight = dynamicHeight
  }

  func makeUIView(context: Context) -> UITextView {
    let textView = UITextView().then {
      $0.backgroundColor = .clear
      $0.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
      $0.isEditable = false
      $0.isSelectable = true
      $0.delegate = context.coordinator
      $0.isScrollEnabled = false
      $0.textAlignment = .center
      $0.textContainer.lineFragmentPadding = 0
      $0.textContainerInset = .zero
    }
    return textView
  }

  func updateUIView(_ uiView: UITextView, context: Context) {
    uiView.attributedText = attributedString
    DispatchQueue.main.async {
      dynamicHeight =
        uiView.sizeThatFits(
          CGSize(width: uiView.bounds.width, height: CGFloat.greatestFiniteMagnitude)
        ).height
    }
  }

  func makeCoordinator() -> Coordinator {
    Coordinator(parent: self)
  }

  class Coordinator: NSObject, UITextViewDelegate {
    var parent: AttributedTextView

    init(parent: AttributedTextView) {
      self.parent = parent
    }

    func textView(
      _ textView: UITextView,
      shouldInteractWith url: URL,
      in characterRange: NSRange,
      interaction: UITextItemInteraction
    ) -> Bool {
      parent.openLink?(url)
      return false
    }
  }
}
