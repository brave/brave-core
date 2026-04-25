// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

extension View {
  /// Backports the `containerCornerOffset` modifier for iOS versions prior to 26.0.
  ///
  /// This modifier offsets the container's corner radius on the specified edges, allowing
  /// views to adjust their corner rounding behavior within container contexts.
  ///
  /// - Parameters:
  ///   - edges: The set of edges where the corner offset should be applied.
  ///   - sizeToFit: When `true`, the view's size is adjusted to fit the offset corners.
  ///                Defaults to `false`.
  /// - Returns: The modified view with container corner offset applied (on iOS 26.0+),
  ///            or the unmodified view (on earlier iOS versions).
  ///
  /// - Note: On iOS 26.0 and later, this function calls the native `containerCornerOffset`
  ///         modifier. On earlier iOS versions, this function returns the view unchanged.
  @available(iOS, obsoleted: 26.0)
  @_disfavoredOverload
  @ViewBuilder public func containerCornerOffset(
    _ edges: Edge.Set,
    sizeToFit: Bool = false
  ) -> some View {
    if #available(iOS 26.0, *) {
      self
        .containerCornerOffset(edges, sizeToFit: sizeToFit)
    } else {
      self
    }
  }
}
