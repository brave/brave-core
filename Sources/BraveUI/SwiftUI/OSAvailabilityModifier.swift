/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI

extension View {
  /// Applies conditional modifiers based on the OS availability (15+, 14.5+, etc.)
  ///
  /// - warning: Only make conditional modifiers apply based on availability within the `transform` closure.
  ///            SwiftUI if/else affect identity and liftime of a ``View`` and conditionally applying based
  ///            on other things may have unintended side-effects.
  public func osAvailabilityModifiers<Content: View>(
    @ViewBuilder _ transform: @escaping (Self) -> Content
  ) -> some View {
    transform(self)
  }
}
