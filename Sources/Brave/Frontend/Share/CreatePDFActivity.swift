// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import WebKit
import os.log

/// An activity that will create a PDF of a given web page
class CreatePDFActivity: UIActivity {
  private let callback: () -> Void

  init(callback: @escaping () -> Void) {
    self.callback = callback
    super.init()
  }

  override var activityTitle: String? {
    Strings.createPDF
  }

  override var activityImage: UIImage? {
    UIImage(systemName: "doc", withConfiguration: UIImage.SymbolConfiguration(scale: .large))
  }

  override func perform() {
    callback()
    activityDidFinish(true)
  }

  override func canPerform(withActivityItems activityItems: [Any]) -> Bool {
    return true
  }
}
