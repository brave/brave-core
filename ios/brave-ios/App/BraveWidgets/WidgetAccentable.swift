// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

extension Image {
  func widgetAccentedRenderingModeFullColor() -> some View {
    if #available(iOS 18.0, *) {
      return self.widgetAccentedRenderingMode(.fullColor)
    } else {
      return self
    }
  }
}
