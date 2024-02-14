// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

/// A class for scaling font based on size category and accessibility
/// The font will scale depending on size of your view and accessibility settings
struct ScaledFont: ViewModifier {
  @Environment(\.sizeCategory)
  var sizeCategory

  var size: CGFloat
  var weight: Font.Weight
  var design: Font.Design

  func body(content: Content) -> some View {
    let scaledSize = UIFontMetrics.default.scaledValue(for: size)
    return content.font(.system(size: scaledSize, weight: weight, design: design))
  }
}

/// Extension so font scaling can be used on `View` as a modifier.
extension View {
  func scaledFont(size: CGFloat, weight: Font.Weight = .regular, design: Font.Design = .default) -> some View {
    return self.modifier(ScaledFont(size: size, weight: weight, design: design))
  }
}
