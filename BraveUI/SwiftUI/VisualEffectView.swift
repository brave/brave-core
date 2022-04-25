// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI

public struct VisualEffectView: UIViewRepresentable {
  var effect: UIVisualEffect?

  public init(effect: UIVisualEffect) {
    self.effect = effect
  }

  public func makeUIView(context: UIViewRepresentableContext<Self>) -> UIVisualEffectView {
    UIVisualEffectView()
  }

  public func updateUIView(_ uiView: UIVisualEffectView, context: UIViewRepresentableContext<Self>) {
    uiView.effect = effect
  }
}
