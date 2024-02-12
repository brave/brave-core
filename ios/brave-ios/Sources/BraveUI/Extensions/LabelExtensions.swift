// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

extension Label where Title == Text, Icon == Image {
  public init<S>(_ title: S, uiImage: UIImage) where S: StringProtocol {
    self.init {
      Text(title)
    } icon: {
      Image(uiImage: uiImage)
    }
  }
}
