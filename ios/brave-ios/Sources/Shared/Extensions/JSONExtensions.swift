// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SwiftyJSON

extension JSON {
  // SwiftyJSON pretty prints the string value by default. Since all of our
  // existing code required the string to not be pretty printed, this helper
  // can be used as a shorthand for non-pretty printed strings.
  public func stringValue() -> String? {
    return self.rawString(.utf8, options: [])
  }
}
