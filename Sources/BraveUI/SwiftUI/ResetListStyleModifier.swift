// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI

private struct ResetListHeaderStyleModifier: ViewModifier {
  @Environment(\.font) private var font
  @Environment(\.sizeCategory) private var sizeCategory
  var insets: EdgeInsets?

  func body(content: Content) -> some View {
    content
      .textCase(.none)
      .font(font)
      .foregroundColor(.primary)
      .listRowInsets(
        insets.map {
          // When using an accessibility font size, inset grouped tables automatically change to regular
          // grouped tables which do not have a horizontal default padding. This ensures headers which opt into
          // inset removal still get some horizontal padding from the edge of the screen.
          if $0 == .zero && sizeCategory.isAccessibilityCategory {
            return .init(top: 0, leading: 16, bottom: 0, trailing: 16)
          }
          return $0
        }
      )
      .accessibilityRemoveTraits(.isHeader)
  }
}

extension View {
  /// Resets many default styles used on a `List` or `Form`'s header as if it were a standard control
  ///
  /// You may optionally choose not to reset the list row insets by providing a value
  public func resetListHeaderStyle(insets: EdgeInsets? = .zero) -> some View {
    modifier(ResetListHeaderStyleModifier(insets: insets))
  }
}
