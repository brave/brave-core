// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

/// A representable for displaying a `UIActivityIndicatorView` on iOS 13
///
/// On iOS 14+ you may use `ProgressView` with the `CircularProgressStyle`
public struct ActivityIndicatorView: UIViewRepresentable {
  /// The style
  public var style: UIActivityIndicatorView.Style
  /// Whether or not the indicator is animating
  public var isAnimating: Bool

  public init(style: UIActivityIndicatorView.Style = .medium, isAnimating: Bool) {
    self.style = style
    self.isAnimating = isAnimating
  }

  public func makeUIView(context: Context) -> UIActivityIndicatorView {
    UIActivityIndicatorView(style: style)
  }

  public func updateUIView(_ uiView: UIActivityIndicatorView, context: Context) {
    uiView.style = style
    if isAnimating {
      uiView.startAnimating()
    } else {
      uiView.stopAnimating()
    }
  }
}
