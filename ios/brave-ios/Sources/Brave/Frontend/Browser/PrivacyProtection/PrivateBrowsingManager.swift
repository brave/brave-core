// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Combine
import Data
import Foundation

enum TabBrowsingMode: Int {
  case regular, `private`, tor
}

public final class PrivateBrowsingManager: ObservableObject {

  public init() {}

  @Published public var isTorBrowsing = false

  @Published public var isPrivateBrowsing = false {
    didSet {
      if oldValue != isPrivateBrowsing {
        if !isPrivateBrowsing {
          isTorBrowsing = false
          Domain.clearInMemoryDomains()
        }
      }
    }
  }

  var browserColors: any BrowserColors {
    isPrivateBrowsing ? .privateMode : .standard
  }
}
