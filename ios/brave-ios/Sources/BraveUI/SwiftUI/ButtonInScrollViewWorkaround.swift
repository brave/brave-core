// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

extension View {
  /// Applies a workaround to fix Button's action activating when you attempt to drag a sheet with
  /// presentation detents (or even just to dismiss the sheet).
  ///
  /// Apply this modifier to the outer container holding the Button's or the ScrollView itself
  ///
  /// FB18927179
  @ViewBuilder public func enableButtonScrollViewDragWorkaround() -> some View {
    // This is currently a bug as of 18.0 and tested to still be broken up to iOS 26.0
    if #available(iOS 18, *) {
      self.simultaneousGesture(TapGesture())
    } else {
      self
    }
  }
}
