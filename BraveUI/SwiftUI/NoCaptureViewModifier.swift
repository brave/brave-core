// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

private struct NoCaptureViewModifier: ViewModifier {
  @State private var isCaptured: Bool = false

  func body(content: Content) -> some View {
    content
      .opacity(isCaptured ? 0 : 1)
      .onAppear {
        isCaptured = UIScreen.main.isCaptured
      }
      .onReceive(
        NotificationCenter.default.publisher(for: UIScreen.capturedDidChangeNotification, object: nil)
      ) { notification in
        isCaptured = (notification.object as? UIScreen)?.isCaptured ?? UIScreen.main.isCaptured
      }
  }
}

extension View {
  /// Redacts the given view if the system is actively recording, mirroring, or using AirPlay to stream the
  /// contents of the screen.
  public func noCapture() -> some View {
    modifier(NoCaptureViewModifier())
  }
}
