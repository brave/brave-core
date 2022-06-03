// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

private struct AlertOnScreenshotViewModifier: ViewModifier {
  var alert: () -> Alert
  @State private var isPresentingAlert: Bool = false

  func body(content: Content) -> some View {
    content
      .alert(isPresented: $isPresentingAlert, content: alert)
      .onReceive(
        NotificationCenter.default.publisher(
          for: UIApplication.userDidTakeScreenshotNotification, object: nil
        )
      ) { _ in
        isPresentingAlert = true
      }
  }
}

extension View {
  /// Displays an alert on the current view if the user takes a screenshot of their device
  public func alertOnScreenshot(_ alert: @escaping () -> Alert) -> some View {
    modifier(AlertOnScreenshotViewModifier(alert: alert))
  }
}
