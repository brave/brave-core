// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI

extension View {
  /// Helper for `hidden()` modifier that accepts a boolean determining if we should hide the view or not.
  @ViewBuilder public func hidden(isHidden: Bool) -> some View {
    if isHidden {
      self.hidden()
    } else {
      self
    }
  }
}
