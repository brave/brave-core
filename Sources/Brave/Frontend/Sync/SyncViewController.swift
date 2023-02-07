/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared
import BraveShared
import Data

class RoundInterfaceView: UIView {
  override func layoutSubviews() {
    super.layoutSubviews()
    layer.cornerRadius = min(bounds.height, bounds.width) / 2.0
  }
}

class SyncViewController: UIViewController {

  override func viewDidLoad() {
    super.viewDidLoad()
    
    view.backgroundColor = .secondaryBraveBackground
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
}
