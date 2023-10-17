// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI
import UIKit

struct BraveTextInputStyleModifier: ViewModifier {
  var strokeColor: Color?
  var lineWidthFactor: CGFloat?
  var backgroundColor: Color?

  private var borderShape: some InsettableShape {
    RoundedRectangle(cornerRadius: 4, style: .continuous)
  }

  func body(content: Content) -> some View {
    content
      .font(.callout)
      .padding(.vertical, 10)
      .padding(.horizontal, 12)
      .overlay(
        borderShape
          // * 2 + clipShape below = pixel perfect hairline border
          .stroke(strokeColor ?? Color(.secondaryButtonTint), lineWidth: 2 * (lineWidthFactor ?? 1))
      )
      .background(
        backgroundColor ?? Color(.braveBackground)
      )
      .clipShape(borderShape)
  }
}

public extension TextEditor {
  func braveTextEditorStyle() -> some View {
    return self.modifier(BraveTextInputStyleModifier())
  }
}
