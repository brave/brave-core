// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI
import UIKit

struct BraveInputStyleModifier: ViewModifier {
  var strokeColor: Color?
  var lineWidthFactor: CGFloat?
  var backgroundColor: Color?
  var cornerRadius: CGFloat?

  private var borderShape: some InsettableShape {
    RoundedRectangle(cornerRadius: cornerRadius ?? 4, style: .continuous)
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

extension View {
  public func braveInputStyle() -> some View {
    return self.modifier(
      BraveInputStyleModifier(
        strokeColor: .clear,
        backgroundColor: Color(braveSystemName: .containerBackground),
        cornerRadius: 8
      )
    )
  }
}
