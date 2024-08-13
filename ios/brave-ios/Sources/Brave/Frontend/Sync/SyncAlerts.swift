// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import UIKit

struct SyncAlerts {
  static var noConnection: UIAlertController {
    let alert = UIAlertController(
      title: Strings.Sync.noConnectionTitle,
      message: Strings.Sync.noConnectionBody,
      preferredStyle: .alert
    )

    let okAction = UIAlertAction(title: Strings.OKString, style: .default)
    alert.addAction(okAction)
    return alert
  }

  static var initializationError: UIAlertController {
    let title = Strings.Sync.initErrorTitle
    let message = Strings.Sync.initErrorMessage
    let alert = UIAlertController(title: title, message: message, preferredStyle: .alert)
    alert.addAction(UIAlertAction(title: Strings.OKString, style: .default, handler: nil))

    return alert
  }
}
