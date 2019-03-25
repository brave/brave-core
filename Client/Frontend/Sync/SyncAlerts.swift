// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared

struct SyncAlerts {
    static var noConnection: UIAlertController {
        let alert = UIAlertController(title: Strings.SyncNoConnectionTitle,
                                      message: Strings.SyncNoConnectionBody, preferredStyle: .alert)
        
        let okAction = UIAlertAction(title: Strings.OKString, style: .default)
        alert.addAction(okAction)
        
        return alert
    }
    
    static var initializationError: UIAlertController {
        let title = Strings.SyncInitErrorTitle
        let message = Strings.SyncInitErrorMessage
        let alert = UIAlertController(title: title, message: message, preferredStyle: .alert)
        alert.addAction(UIAlertAction(title: Strings.OKString, style: .default, handler: nil))
        
        return alert
    }
}
