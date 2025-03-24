// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import WebKit

extension WKWebView {
  var sampledPageTopColor: UIColor? {
    let selector = Selector("_sampl\("edPageTopC")olor")
    if responds(to: selector), let result = perform(selector) {
      return result.takeUnretainedValue() as? UIColor
    }
    return nil
  }

  var dataForDisplayedPDF: Data? {
    guard responds(to: Selector(("_dataForDisplayedPDF"))) else {
      return nil
    }
    return perform(Selector(("_dataForDisplayedPDF"))).takeUnretainedValue() as? Data
  }

  var viewScale: CGFloat? {
    get {
      value(forKey: "viewScale") as? CGFloat
    }
    set {
      setValue(newValue ?? 1, forKey: "viewScale")
    }
  }
}

extension WKWebViewConfiguration {
  func enablePageTopColorSampling() {
    let selector = Selector("_setSa\("mpledPageTopColorMaxDiff")erence:")
    if responds(to: selector) {
      perform(selector, with: 5.0 as Double)
    }
  }
}

extension WKBackForwardList {
  func clear() {
    let selector = Selector("_cl\("ea")r")
    if responds(to: selector) {
      perform(selector, on: .main, with: nil, waitUntilDone: true)
    }
  }
}
