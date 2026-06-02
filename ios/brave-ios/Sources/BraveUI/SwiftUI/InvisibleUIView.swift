// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

/// An invisible `UIViewRepresentable` that bridges between UIKit and SwiftUI.
/// This can be used as a background or overlay for SwiftUI View as a representation so that
/// UIKit land can reference
public struct InvisibleUIView: UIViewRepresentable {

  public init() {}

  public let uiView = UIView()

  public func makeUIView(context: Context) -> UIView {
    uiView.backgroundColor = .clear
    return uiView
  }

  public func updateUIView(_ uiView: UIView, context: Context) {
  }
}
