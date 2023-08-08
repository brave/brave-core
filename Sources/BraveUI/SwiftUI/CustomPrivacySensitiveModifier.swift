// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI

/// Custom modifier to behave similar to `.privacySensitive()`
/// The `.privacySensitive()` doesn't work in sheets because the
/// `redactedReasons` doesn't get passed through the environment in sheets.
private struct CustomPrivacySensitiveModifier: ViewModifier {
  var isDisclosed: Bool
  
  @State private var isBackgrounded: Bool = false
  @State private var isCaptured: Bool = false
  
  func body(content: Content) -> some View {
    Group {
      if isBackgrounded || isCaptured || !isDisclosed {
        content
          .opacity(0)
          .overlay(alignment: .center) {
            RoundedRectangle(cornerRadius: 2)
              .fill(Color.black.opacity(0.1)).frame(width: 64).padding(.vertical, 2.5)
            }
      } else {
        content
      }
    }
    .onAppear {
      isCaptured = UIScreen.main.isCaptured
    }
    .onReceive(
      NotificationCenter.default.publisher(for: UIScreen.capturedDidChangeNotification, object: nil)
    ) { notification in
      isCaptured = (notification.object as? UIScreen)?.isCaptured ?? UIScreen.main.isCaptured
    }
    .onReceive(
      NotificationCenter.default.publisher(for: UIApplication.willResignActiveNotification)
    ) { _ in
      isBackgrounded = true
    }
    .onReceive(
      NotificationCenter.default.publisher(for: UIApplication.didBecomeActiveNotification)
    ) { _ in
      isBackgrounded = false
    }
  }
}

extension View {
  /// Redacts the given view if the app is backgrounded, or the system is actively
  /// recording, mirroring, or using AirPlay to stream the contents of the screen.
  public func customPrivacySensitive(isDisclosed: Bool = true) -> some View {
    modifier(CustomPrivacySensitiveModifier(isDisclosed: isDisclosed))
  }
}
