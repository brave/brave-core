// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import AVKit
import Foundation
import Strings
import SwiftUI

/// Displays controls for picking playback routes such as AirPlay
struct RoutePickerView: View {
  @State private var underlyingRoutePickerControl: UIControl?

  var body: some View {
    Button {
      underlyingRoutePickerControl?.sendActions(for: .touchUpInside)
    } label: {
      Label(Strings.Playlist.accessibilityAirPlay, braveSystemImage: "leo.airplay.video")
    }
    .labelStyle(.iconOnly)
    .buttonStyle(.playbackControl)
    .backgroundStyle(Color(braveSystemName: .containerHighlight))
    .tint(Color(braveSystemName: .textSecondary))
    .background {
      Representable(control: $underlyingRoutePickerControl)
        .opacity(0)
        .accessibilityHidden(true)
    }
  }

  struct Representable: UIViewRepresentable {
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
