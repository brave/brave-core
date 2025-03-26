// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import Strings

/// Houses a CWVWebView to handle loading WebUI pages in a limited scope
class ChromeWebUIController: UIViewController, BraveWebViewUIDelegate {
  private let configuration: CWVWebViewConfiguration

  init(braveCore: BraveCoreMain, isPrivateBrowsing: Bool) {
    configuration =
      isPrivateBrowsing
      ? braveCore.nonPersistentWebViewConfiguration : braveCore.defaultWebViewConfiguration
    super.init(nibName: nil, bundle: nil)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  var webView: BraveWebView {
    view as! BraveWebView
  }

  override func loadView() {
    // CWVWebView always needs a non-zero size
    view = BraveWebView(frame: .init(width: 1, height: 1), configuration: configuration)
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    webView.uiDelegate = self
  }

  // MARK: - CWVUIDelegate

  func webView(
    _ webView: CWVWebView,
    runJavaScriptAlertPanelWithMessage message: String,
    pageURL url: URL,
    completionHandler: @escaping () -> Void
  ) {
    let alertController = UIAlertController(title: nil, message: message, preferredStyle: .alert)
    alertController.addAction(
      .init(
        title: Strings.OKString,
        style: .default,
        handler: { _ in
          completionHandler()
        }
      )
    )
    present(alertController, animated: true)
  }

  func webView(
    _ webView: CWVWebView,
    runJavaScriptConfirmPanelWithMessage message: String,
    pageURL url: URL,
    completionHandler: @escaping (Bool) -> Void
  ) {
    let alertController = UIAlertController(title: nil, message: message, preferredStyle: .alert)
    alertController.addAction(
      .init(
        title: Strings.OKString,
        style: .default,
        handler: { _ in
          completionHandler(true)
        }
      )
    )
    alertController.addAction(
      .init(
        title: Strings.CancelString,
        style: .cancel,
        handler: { _ in
          completionHandler(false)
        }
      )
    )
    present(alertController, animated: true)
  }

  func webView(
    _ webView: CWVWebView,
    runJavaScriptTextInputPanelWithPrompt prompt: String,
    defaultText: String,
    pageURL url: URL,
    completionHandler: @escaping (String?) -> Void
  ) {
    let alertController = UIAlertController(title: nil, message: prompt, preferredStyle: .alert)
    alertController.addTextField(configurationHandler: { textField in
      textField.placeholder = defaultText
    })
    alertController.addAction(
      .init(
        title: Strings.OKString,
        style: .default,
        handler: { [weak alertController] _ in
          guard let textField = alertController?.textFields?.first else { return }
          completionHandler(textField.text)
        }
      )
    )
    alertController.addAction(
      .init(
        title: Strings.CancelString,
        style: .cancel,
        handler: { _ in
          completionHandler(nil)
        }
      )
    )
    present(alertController, animated: true)
  }
}
