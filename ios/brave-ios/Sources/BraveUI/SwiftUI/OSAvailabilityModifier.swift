// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

extension View {
  /// Applies conditional modifiers based on the OS availability (15+, 14.5+, etc.)
  ///
  /// - warning: Only make conditional modifiers apply based on availability within the `transform` closure.
  ///            SwiftUI if/else affect identity and liftime of a ``View`` and conditionally applying based
  ///            on other things may have unintended side-effects.
  public func osAvailabilityModifiers<Content: View>(
    @ViewBuilder _ transform: @escaping (Self) -> Content
  ) -> some View {
    transform(self)
  }

  /// Applies container corner offset modifier in an OS-availability-safe manner.
  ///
  /// Conditionally applies the `containerCornerOffset` modifier based on iOS version.
  /// On iOS 26.0 and later, the modifier is applied; on earlier versions, the view is returned unchanged.
  ///
  /// - Parameters:
  ///   - edges: The edges where the corner offset should be applied. Defaults to `.leading`.
  ///   - sizeToFit: Whether the container should size to fit its content. Defaults to `false`.
  /// - Returns: A view with the container corner offset applied (on iOS 26.0+) or the original view (on earlier versions).
  public func osAvailableContainerCornerOffset(
    _ edges: Edge.Set,
    sizeToFit: Bool = false
  ) -> some View {
    modifier(OSAvailableContainerCornerOffset(edges, sizeToFit: sizeToFit))
  }
}

/// View modifier that conditionally applies container corner offset based on iOS version availability.
///
/// This modifier applies the `containerCornerOffset` modifier on iOS 26.0 and later,
/// and returns the view unchanged on earlier versions. This allows using newer SwiftUI APIs
/// while maintaining compatibility with older iOS versions.
struct OSAvailableContainerCornerOffset: ViewModifier {
  /// The edges where the corner offset should be applied.
  var edges: Edge.Set

  /// Whether the container should size to fit its content.
  var sizeToFit: Bool

  /// Creates a container corner offset modifier.
  ///
  /// - Parameters:
  ///   - edges: The edges where the corner offset should be applied. Defaults to `.leading`.
  ///   - sizeToFit: Whether the container should size to fit its content. Defaults to `false`.
  init(_ edges: Edge.Set, sizeToFit: Bool = false) {
    self.edges = edges
    self.sizeToFit = sizeToFit
  }

  /// Applies the container corner offset modifier if available, otherwise returns the content unchanged.
  ///
  /// - Parameter content: The view content to modify.
  /// - Returns: The modified view with container corner offset applied (on iOS 26.0+) or the original view (on earlier versions).
  func body(content: Content) -> some View {
    if #available(iOS 26.0, *) {
      content
        .containerCornerOffset(edges, sizeToFit: sizeToFit)
    } else {
      content
    }
  }
}
