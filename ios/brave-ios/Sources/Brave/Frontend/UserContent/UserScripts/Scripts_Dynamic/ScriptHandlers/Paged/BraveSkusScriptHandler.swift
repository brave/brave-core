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
  private weak var tab: Tab?
  private let braveSkusManager = BraveSkusManager(isPrivateMode: false)!
  
  required init?(tab: Tab) {
    self.tab = tab
  }
    
  static let scriptName = "BraveSkusScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "\(scriptName)_\(messageUUID)"
  static let scriptSandbox: WKContentWorld = .page
  static let userScript: WKUserScript? = {
    guard var script = loadUserScript(named: scriptName) else {
      return nil
    }
    
    return WKUserScript(source: secureScript(handlerNamesMap: ["$<message_handler>": messageHandlerName]
                                                 .merging(Method.map, uniquingKeysWith: { $1 }),
                                             securityToken: scriptId,
                                             script: script),
                        injectionTime: .atDocumentStart,
                        forMainFrameOnly: true,
                        in: scriptSandbox)
  }()
  
  func userContentController(_ userContentController: WKUserContentController,
                             didReceiveScriptMessage message: WKScriptMessage,
                             replyHandler: @escaping (Any?, String?) -> Void) {
    if !verifyMessage(message: message) {
      assertionFailure("Missing required security token.")
      return
    }
    
    // Validate that we can handle this message
    guard let requestHost = message.frameInfo.request.url?.host,
          message.frameInfo.isMainFrame,
          DomainUserScript.braveSkus.associatedDomains.contains(requestHost) else {
      Logger.module.error("Brave skus request called from disallowed host")
      return
    }
    
    guard let response = message.body as? [String: Any] else {
      Logger.module.error("Brave skus request with no message")
      return
    }
    
    guard let requestedMethod = response["method_id"] as? Int,
          let method = Method(rawValue: requestedMethod)  else {
      Logger.module.error("Brave skus request with invalid method-id")
      return
    }
    
    do {
      switch method {
      case .refreshOrder:
        let order = try OrderMessage.from(message: message)
        braveSkusManager.refreshOrder(for: order.orderId, domain: requestHost) { result in
          replyHandler(result, nil)
        }
        
      case .fetchOrderCredentials:
        let order = try OrderMessage.from(message: message)
        braveSkusManager.fetchOrderCredentials(for: order.orderId, domain: requestHost) { result in
          replyHandler(result, nil)
        }
        
      case .prepareCredentialsPresentation:
        Logger.module.error("Error - Website calling prepareCredentialsPresentation when it shouldn't")
        replyHandler(nil, nil)
        return
        
      case .credentialsSummary:
        let summary = try CredentialSummaryMessage.from(message: message)
        braveSkusManager.credentialSummary(for: summary.domain) { result in
          replyHandler(result, nil)
        }
      }
    } catch {
      Logger.module.error("Error Deserializing Skus Message: \(error)")
      replyHandler(nil, nil)
      return
    }
  }
}

extension BraveSkusScriptHandler {
  private enum Method: Int, CaseIterable {
    case refreshOrder = 1
    case fetchOrderCredentials = 2
    case prepareCredentialsPresentation = 3
    case credentialsSummary = 4
    
    static var map: [String: String] {
      var jsDict = [String: String]()
      allCases.forEach({
        jsDict.updateValue("\($0.rawValue)", forKey: "$<\($0)>")
      })
      return jsDict
    }
  }
  
  private struct OrderMessage: SkusWebMessage {
    let orderId: String
  }
  
  private struct CredentialsMessage: SkusWebMessage {
    let domain: String
    let path: String
  }
  
  private struct CredentialSummaryMessage: SkusWebMessage {
    let domain: String
  }
}

private enum SkusWebMessageError: Error {
  case invalidFormat
}

private protocol SkusWebMessage: Codable {
  static func from(message: WKScriptMessage) throws -> Self
}

extension SkusWebMessage {
  static func from(message: WKScriptMessage) throws -> Self {
    guard let body = message.body as? [String: Any] else {
      throw SkusWebMessageError.invalidFormat
    }
    
    guard let messageData = body["data"] else {
      throw SkusWebMessageError.invalidFormat
    }
    
    let data = try JSONSerialization.data(withJSONObject: messageData)
    return try JSONDecoder().decode(Self.self, from: data)
  }
}
