// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import WebKit
import os.log

class PrintScriptHandler: TabContentScript {
  private weak var browserController: BrowserViewController?
  private var isPresentingController = false
  private var printCounter = 0
  private var isBlocking = false
  private var currentDomain: String?

  required init(browserController: BrowserViewController) {
    self.browserController = browserController
  }

  static let scriptName = "PrintScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "printScriptHandler"
  static let scriptSandbox: WKContentWorld = .page
  static let userScript: WKUserScript? = {
    guard var script = loadUserScript(named: scriptName) else {
      return nil
    }
    return WKUserScript(
      source: secureScript(
        handlerName: messageHandlerName,
        securityToken: scriptId,
        script: script
      ),
      injectionTime: .atDocumentStart,
      forMainFrameOnly: false,
      in: scriptSandbox
    )
  }()

  func tab(
    _ tab: Tab,
    receivedScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    defer { replyHandler(nil, nil) }

    if !verifyMessage(message: message, securityToken: UserScriptManager.securityToken) {
      assertionFailure("Missing required security token.")
      return
    }

    if let webView = tab.webView, let url = webView.lastCommittedURL {
      // If the main-frame's URL has changed
      if let domain = url.baseDomain, domain != currentDomain, message.frameInfo.isMainFrame {
        isBlocking = false
        printCounter = 0
      }

      currentDomain = url.baseDomain

      if isPresentingController || isBlocking {
        return
      }

      let showPrintSheet = { [weak self] in
        self?.isPresentingController = true

        let printController = UIPrintInteractionController.shared
        printController.printFormatter = webView.viewPrintFormatter()
        printController.present(
          animated: true,
          completionHandler: { _, _, _ in
            self?.isPresentingController = false
          }
        )
      }

      printCounter += 1

      if printCounter > 1 {
        // Show confirm alert here.
        let suppressAlertStyle: UIAlertController.Style = UIDevice.isIpad ? .alert : .actionSheet

        let suppressSheet = UIAlertController(
          title: nil,
          message:
            Strings.suppressAlertsActionMessage,
          preferredStyle: suppressAlertStyle
        )

        suppressSheet.addAction(
          UIAlertAction(
            title: Strings.suppressAlertsActionTitle,
            style: .destructive,
            handler: { [weak self] _ in
              self?.isBlocking = true
            }
          )
        )

        suppressSheet.addAction(
          UIAlertAction(
            title: Strings.cancelButtonTitle,
            style: .cancel,
            handler: { _ in
              showPrintSheet()
            }
          )
        )

        if UIDevice.current.userInterfaceIdiom == .pad,
          let popoverController = suppressSheet.popoverPresentationController
        {
          popoverController.sourceView = webView
          popoverController.sourceRect = CGRect(
            x: webView.bounds.midX,
            y: webView.bounds.midY,
            width: 0,
            height: 0
          )
          popoverController.permittedArrowDirections = []
        }

        browserController?.present(suppressSheet, animated: true)
      } else {
        showPrintSheet()
      }
    }
  }
}
