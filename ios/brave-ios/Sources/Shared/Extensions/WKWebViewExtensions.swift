// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import JavaScriptCore
import WebKit

extension WKWebViewConfiguration {
  public func enablePageTopColorSampling() {
    let selector = Selector("_setSa\("mpledPageTopColorMaxDiff")erence:")
    if responds(to: selector) {
      perform(selector, with: 5.0 as Double)
    }
  }
}
