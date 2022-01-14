/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import WebKit

private let log = Logger.browserLogger

class PrintHelper: TabContentScript {
    private weak var browserController: BrowserViewController?
    private weak var tab: Tab?
    private var isPresentingController = false
    private var printCounter = 0
    private var isBlocking = false
    private var currentDomain: String?

    class func name() -> String {
        return "PrintHelper"
    }

    required init(browserController: BrowserViewController, tab: Tab) {
        self.browserController = browserController
        self.tab = tab
    }

    func scriptMessageHandlerName() -> String? {
        return "printHandler"
    }

    func userContentController(_ userContentController: WKUserContentController, didReceiveScriptMessage message: WKScriptMessage) {
        guard let body = message.body as? [String: AnyObject] else {
            return
        }
        
        if UserScriptManager.isMessageHandlerTokenMissing(in: body) {
            log.debug("Missing required security token.")
            return
        }
        
        if let tab = tab, let webView = tab.webView, let url = webView.url {
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
                printController.present(animated: true, completionHandler: { _, _, _ in
                    self?.isPresentingController = false
                })
            }
            
            printCounter += 1
            
            if printCounter > 1 {
                // Show confirm alert here.
                let suppressSheet = UIAlertController(title: nil, message: Strings.suppressAlertsActionMessage, preferredStyle: .actionSheet)
                suppressSheet.addAction(UIAlertAction(title: Strings.suppressAlertsActionTitle, style: .destructive, handler: { [weak self] _ in
                    self?.isBlocking = true
                }))
                
                suppressSheet.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: { _ in
                    showPrintSheet()
                }))
                if UIDevice.current.userInterfaceIdiom == .pad, let popoverController = suppressSheet.popoverPresentationController {
                    popoverController.sourceView = webView
                    popoverController.sourceRect = CGRect(x: webView.bounds.midX, y: webView.bounds.midY, width: 0, height: 0)
                    popoverController.permittedArrowDirections = []
                }

                browserController?.present(suppressSheet, animated: true)
            } else {
                showPrintSheet()
            }
        }
    }
}
