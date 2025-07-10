// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Data
import Foundation
import Web

extension TabDataValues {
  private struct BraveShieldsTabHelperKey: TabDataKey {
    static var defaultValue: BraveShieldsTabHelper?
  }
  public var braveShieldsHelper: BraveShieldsTabHelper? {
    get { self[BraveShieldsTabHelperKey.self] }
    set { self[BraveShieldsTabHelperKey.self] = newValue }
  }
}

public class BraveShieldsTabHelper {
  private weak var tab: (any TabState)?

  public init(tab: some TabState) {
    self.tab = tab
  }
}
