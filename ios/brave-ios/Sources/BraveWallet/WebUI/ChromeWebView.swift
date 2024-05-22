// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import SwiftUI

struct ChromeWebView: UIViewControllerRepresentable {

  var title: String
  let urlString: String

  public func makeUIViewController(
    context: UIViewControllerRepresentableContext<ChromeWebView>
  ) -> ChromeWebViewController {
    return ChromeWebViewController(privateBrowsing: false).then {
      $0.title = title
      $0.loadURL(urlString)
      if #available(iOS 16.4, *) {
        $0.webView.isInspectable = true
      }
    }
  }

  public func updateUIViewController(
    _ uiViewController: ChromeWebViewController,
    context: UIViewControllerRepresentableContext<ChromeWebView>
  ) {
    uiViewController.title = title
  }
}
