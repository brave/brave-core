// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WebKit
import Shared
import BraveShared
import BraveCore
import BraveVPN
import os.log

class BraveSkusScriptHandler: TabContentScript {
  typealias ReplyHandler = (Any?, String?) -> Void
  
  private let braveSkusManager: BraveSkusManager
  
  required init?(tab: Tab) {
    guard let manager = BraveSkusManager(isPrivateMode: tab.isPrivate) else {
      return nil
    }
    
    self.braveSkusManager = manager
  }
    
  static let scriptName = "BraveSkusScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "\(scriptName)_\(messageUUID)"
  static let scriptSandbox: WKContentWorld = .page
  static let userScript: WKUserScript? = {
    guard var script = loadUserScript(named: scriptName) else {
      return nil
    }
    return WKUserScript(source: secureScript(handlerName: messageHandlerName,
                                             securityToken: scriptId,
                                             script: script),
                        injectionTime: .atDocumentStart,
                        forMainFrameOnly: true,
                        in: scriptSandbox)
  }()
  
  private enum Method: Int {
    case refreshOrder = 1
    case fetchOrderCredentials = 2
    case prepareCredentialsPresentation = 3
    case credentialsSummary = 4
  }
  
  func userContentController(_ userContentController: WKUserContentController,
                             didReceiveScriptMessage message: WKScriptMessage,
                             replyHandler: @escaping (Any?, String?) -> Void) {
    if !verifyMessage(message: message) {
      assertionFailure("Missing required security token.")
      return
    }
    
    let allowedHosts = DomainUserScript.braveSkus.associatedDomains
    
    guard let requestHost = message.frameInfo.request.url?.host,
          allowedHosts.contains(requestHost),
          message.frameInfo.isMainFrame else {
      Logger.module.error("Brave skus request called from disallowed host")
      return
    }
    
    guard let response = message.body as? [String: Any],
          let methodId = response["method_id"] as? Int,
          let data = response["data"] as? [String: Any] else {
      Logger.module.error("Failed to retrieve method id")
      return
    }
    
    switch methodId {
    case Method.refreshOrder.rawValue:
      if let orderId = data["orderId"] as? String {
        
        braveSkusManager.refreshOrder(for: orderId, domain: requestHost) { result in
          replyHandler(result, nil)
        }
      }
    case Method.fetchOrderCredentials.rawValue:
      if let orderId = data["orderId"] as? String {
        braveSkusManager.fetchOrderCredentials(for: orderId, domain: requestHost) { result in
          replyHandler(result, nil)
        }
      }
    case Method.prepareCredentialsPresentation.rawValue:
      assertionFailure("The website should never call the credentialsPresentation.")
    case Method.credentialsSummary.rawValue:
      if let domain = data["domain"] as? String {
        braveSkusManager.credentialSummary(for: domain) { result in
          replyHandler(result, nil)
        }
      }
    default:
      assertionFailure("Failure, the website called unhandled method with id: \(methodId)")
    }
  }
}
