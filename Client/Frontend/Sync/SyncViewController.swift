/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared
import BraveShared

struct SyncUX {
    static let backgroundColor = UIColor(rgb: 0xF8F8F8)
}

class RoundInterfaceButton: UIButton {
    override func layoutSubviews() {
        super.layoutSubviews()
        layer.cornerRadius = bounds.height / 2.0
    }
}

class SyncViewController: UIViewController {

    override func viewDidLoad() {
        super.viewDidLoad()

        view.backgroundColor = SyncUX.backgroundColor
    }
    
    func showNoConnectionAlert() {
        let alert = UIAlertController(title: Strings.SyncNoConnectionTitle,
                                      message: Strings.SyncNoConnectionBody, preferredStyle: .alert)
        
        let okAction = UIAlertAction(title: Strings.OKString, style: .default)
        alert.addAction(okAction)
        
        present(alert, animated: true)
    }
    
    /// Perform a block of code only if user has a network connection, shows an error alert otherwise.
    /// Most of sync initialization methods require an internet connection.
    func doIfConnected(code: () -> Void) {
        if !DeviceInfo.hasConnectivity() {
            showNoConnectionAlert()
            return
        }
        
        code()
    }
}
