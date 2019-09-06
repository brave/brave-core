/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared
import BraveShared
import Data

class RoundInterfaceButton: UIButton {
    override func layoutSubviews() {
        super.layoutSubviews()
        layer.cornerRadius = bounds.height / 2.0
    }
}

class SyncViewController: UIViewController {

    override func loadView() {
        view = SyncView()
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        NotificationCenter.default.addObserver(self, selector: #selector(didLeaveSyncGroup), name: Sync.Notifications.didLeaveSyncGroup, object: nil)
    }
    
    deinit {
        NotificationCenter.default.removeObserver(self)
    }
    
    /// Perform a block of code only if user has a network connection, shows an error alert otherwise.
    /// Most of sync initialization methods require an internet connection.
    func doIfConnected(code: () -> Void) {
        if !DeviceInfo.hasConnectivity() {
            present(SyncAlerts.noConnection, animated: true)
            return
        }
        
        code()
    }
    
    @objc func didLeaveSyncGroup() {
        DispatchQueue.main.async { [weak self] in
            self?.navigationController?.popToRootViewController(animated: true)
        }
    }
    
    // This is used for `appearance()` usage, so can target sync background views
    class SyncView: UIView {}
}
