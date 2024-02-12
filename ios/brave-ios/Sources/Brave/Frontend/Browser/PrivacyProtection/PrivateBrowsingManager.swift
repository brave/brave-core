// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Data
import Combine

public final class PrivateBrowsingManager: ObservableObject {
  
  public init() {}

  @Published public var isPrivateBrowsing = false {
    didSet {
      if oldValue != isPrivateBrowsing {
        if !isPrivateBrowsing {
          Domain.clearInMemoryDomains()
        }
      }
    }
  }
  
  var browserColors: any BrowserColors {
    isPrivateBrowsing ? .privateMode : .standard
  }
}
