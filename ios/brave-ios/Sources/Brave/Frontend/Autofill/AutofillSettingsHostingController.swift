// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI
import UIKit

final class AutofillSettingsHostingController<Content: View>: UIHostingController<Content> {
  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    // UINavigationController hides the toolbar by default.
    navigationController?.setNavigationBarHidden(true, animated: animated)
  }

  override func viewWillDisappear(_ animated: Bool) {
    super.viewWillDisappear(animated)
    if isMovingFromParent {
      // Restore the navigation bar state
      navigationController?.setNavigationBarHidden(false, animated: animated)
    }
  }
}
