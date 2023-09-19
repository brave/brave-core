// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WidgetKit
import SwiftUI

struct WidgetBackgroundViewModifier<BackgroundContent: View>: ViewModifier {
  var backgroundView: BackgroundContent
  
  func body(content: Content) -> some View {
#if swift(>=5.9)
    if #available(iOS 17.0, *) {
      content.containerBackground(for: .widget) {
        backgroundView
      }
    } else {
      content.background(backgroundView)
    }
#else
    content.background(backgroundView)
#endif
  }
}

extension View {
  /// Adds a `containerBackground` on iOS 17
  func widgetBackground<Content: View>(@ViewBuilder _ content: () -> Content) -> some View {
    modifier(WidgetBackgroundViewModifier(backgroundView: content()))
  }
}
