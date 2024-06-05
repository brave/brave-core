// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import AVKit
import Foundation
import SwiftUI

/// Displays controls for picking playback routes such as AirPlay
@available(iOS 16.0, *)
struct RoutePickerView: View {
  @State private var underlyingRoutePickerControl: UIControl?

  var body: some View {
    Button {
      underlyingRoutePickerControl?.sendActions(for: .touchUpInside)
    } label: {
      Label("AirPlay", braveSystemImage: "leo.airplay.video")
    }
    .labelStyle(.iconOnly)
    .buttonStyle(.playbackControl)
    .tint(Color(braveSystemName: .textSecondary))
    .background {
      Representable(control: $underlyingRoutePickerControl)
        .opacity(0)
        .accessibilityHidden(true)
    }
  }

  struct Representable: UIViewRepresentable {
    // FIXME: WeakBox?
    @Binding var control: UIControl?

    func makeUIView(context: Context) -> AVRoutePickerView {
      let view = AVRoutePickerView()
      view.prioritizesVideoDevices = true
      view.tintColor = .clear
      view.activeTintColor = .clear
      view.backgroundColor = .clear
      return view
    }

    func updateUIView(_ uiView: AVRoutePickerView, context: Context) {
      DispatchQueue.main.async {
        // Dispatch off main to avoid updating state during body computation
        control = uiView.subviews.compactMap({ $0 as? UIControl }).first
      }
    }
  }
}
