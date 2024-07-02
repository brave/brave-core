// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import SwiftUI

struct ChromeWebView: UIViewControllerRepresentable {

  let urlString: String
  @Binding var title: String
  let openURL: ((URL) -> Void)?

  public func makeUIViewController(
    context: UIViewControllerRepresentableContext<ChromeWebView>
  ) -> ChromeWebViewController {
    return ChromeWebViewController(privateBrowsing: false).then {
      $0.title = title
      $0.loadURL(urlString)
      $0.view.backgroundColor = UIColor(braveSystemName: .containerBackground)
      $0.webView.backgroundColor = UIColor(braveSystemName: .containerBackground)
      $0.webView.scrollView.backgroundColor = UIColor(braveSystemName: .containerBackground)
      // WKWebView flashes white screen on load regardless of background colour assignments
      $0.webView.isOpaque = false
      if #available(iOS 16.4, *) {
        $0.webView.isInspectable = true
      }
      context.coordinator.webView = $0.webView
      $0.delegate = context.coordinator
    }
  }

  public func updateUIViewController(
    _ uiViewController: ChromeWebViewController,
    context: UIViewControllerRepresentableContext<ChromeWebView>
  ) {
    uiViewController.title = title
  }

  func makeCoordinator() -> Coordinator {
    Coordinator(self, openURL: openURL)
  }

  class Coordinator: NSObject, ChromeWebViewControllerUIDelegate {

    var parent: ChromeWebView
    var webView: WKWebView? {
      didSet {
        guard let webView else { return }
        webView.addObserver(
          self,
          forKeyPath: "title",
          options: .new,
          context: nil
        )
      }
    }
    let openURL: ((URL) -> Void)?

    init(
      _ parent: ChromeWebView,
      openURL: ((URL) -> Void)?
    ) {
      self.parent = parent
      self.openURL = openURL
    }

    override func observeValue(
      forKeyPath keyPath: String?,
      of object: Any?,
      change: [NSKeyValueChangeKey: Any]?,
      context: UnsafeMutableRawPointer?
    ) {
      if keyPath == "title", let title = webView?.title {
        parent.title = title
      }
    }

    func chromeWebViewController(
      _ chromeWebViewController: ChromeWebViewController,
      openNewWindowFor newWindowURL: URL,
      openerURL: URL,
      initiatedByUser: Bool
    ) {
      openURL?(newWindowURL)
    }
  }
}
