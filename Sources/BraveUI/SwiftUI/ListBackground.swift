// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

extension View {
  /// Sets the background color of a List when using iOS 16 which is no longer backed by `UITableView`, thus
  /// not respecting `UIAppearance` overrides
  @available(iOS, introduced: 14.0, deprecated: 16.0, message: "Use `scrollContentBackground` and `background` directly")
  public func listBackgroundColor(_ color: Color) -> some View {
    if #available(iOS 16.0, *) {
      return self.scrollContentBackground(.hidden).background(color)
    } else {
      return self
    }
  }
}
