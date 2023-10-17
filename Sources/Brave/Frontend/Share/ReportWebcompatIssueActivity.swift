// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Strings
import UIKit
import SwiftUI

class ReportWebCompatibilityIssueActivity: UIActivity, MenuActivity {
  private let callback: () -> Void

  init(callback: @escaping () -> Void) {
    self.callback = callback
  }

  override var activityTitle: String? {
    Strings.Shields.reportABrokenSite
  }
  
  private var imageName: String {
    "leo.warning.triangle-outline"
  }

  override var activityImage: UIImage? {
    UIImage(braveSystemNamed: imageName)?.applyingSymbolConfiguration(.init(scale: .large))
  }
  
  var menuImage: Image {
    Image(braveSystemName: imageName)
  }

  override func perform() {
    callback()
    activityDidFinish(true)
  }

  override func canPerform(withActivityItems activityItems: [Any]) -> Bool {
    return true
  }
}
