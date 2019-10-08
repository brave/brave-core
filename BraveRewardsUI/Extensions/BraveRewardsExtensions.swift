// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveRewards

extension LogLevel {
  /// The common prefix to place before the log (uppercased)
  public var logPrefix: String {
    switch self {
    case .logDebug: return "DEBUG"
    case .logError: return "ERROR"
    case .logInfo: return "INFO"
    case .logRequest: return "REQUEST"
    case .logWarning: return "WARNING"
    case .logResponse: return "RESPONSE"
    @unknown default:
      assertionFailure()
      return "DEBUG"
    }
  }
}
