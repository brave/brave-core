/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI

private struct PreviewColorSchemeIteratorModifier: ViewModifier {
  var schemes: [ColorScheme]

  func body(content: Content) -> some View {
    ForEach(schemes, id: \.self) { scheme in
      content
        .background(Color(.braveBackground))
        .preferredColorScheme(scheme)
        .previewDisplayName(String(describing: scheme).capitalized)
    }
  }
}

private struct PreviewContentSizeCategoriesIteratorModifier: ViewModifier {
  var categories: [ContentSizeCategory]

  func body(content: Content) -> some View {
    ForEach(categories, id: \.self) { category in
      content
        .environment(\.sizeCategory, category)
        .previewDisplayName(String(describing: category))
    }
  }
}

extension View {
  /// Creates a preview for each ColorScheme provided. Defaults to previewing all colors schemes
  public func previewColorSchemes(
    _ schemes: [ColorScheme] = ColorScheme.allCases
  ) -> some View {
    modifier(PreviewColorSchemeIteratorModifier(schemes: schemes))
  }
  /// Creates a preview for each size category provided. Defaults to previewing `small`, `large` and
  /// `accessibilityLarge` categories.
  public func previewSizeCategories(
    _ categories: [ContentSizeCategory] = [.small, .large, .accessibilityLarge]
  ) -> some View {
    modifier(PreviewContentSizeCategoriesIteratorModifier(categories: categories))
  }
}
