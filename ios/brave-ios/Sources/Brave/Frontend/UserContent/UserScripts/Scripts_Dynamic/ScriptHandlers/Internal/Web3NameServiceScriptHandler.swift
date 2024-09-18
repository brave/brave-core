// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveWallet
import Foundation
import Shared
import WebKit
import os.log

protocol Web3NameServiceScriptHandlerDelegate: AnyObject {
  func web3NameServiceDecisionHandler(_ proceed: Bool, web3Service: Web3Service, originalURL: URL)
}

class Web3NameServiceScriptHandler: TabContentScript {

  // The `rawValue`s MUST match `Web3Domain.html`
  enum ParamKey: String {
    case buttonType = "button_type"
    case serviceId = "service_id"
  }
  enum ParamValue: String {
    case proceed
    case disable
  }

  weak var delegate: Web3NameServiceScriptHandlerDelegate?
  var originalURL: URL?

  static let scriptName = "Web3NameServiceScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "\(scriptName)_\(messageUUID)"
  static let scriptSandbox: WKContentWorld = .page
  static let userScript: WKUserScript? = nil

  func tab(
    _ tab: Tab,
    receivedScriptMessage message: WKScriptMessage,
    replyHandler: (Any?, String?) -> Void
  ) {
    defer { replyHandler(nil, nil) }

    guard let params = message.body as? [String: String], let originalURL = originalURL else {
      return
    }

    if params[ParamKey.buttonType.rawValue] == ParamValue.disable.rawValue,
      let serviceId = params[ParamKey.serviceId.rawValue],
      let service = Web3Service(rawValue: serviceId)
    {
      delegate?.web3NameServiceDecisionHandler(
        false,
        web3Service: service,
        originalURL: originalURL
      )
    } else if params[ParamKey.buttonType.rawValue] == ParamValue.proceed.rawValue,
      let serviceId = params[ParamKey.serviceId.rawValue],
      let service = Web3Service(rawValue: serviceId)
    {
      delegate?.web3NameServiceDecisionHandler(true, web3Service: service, originalURL: originalURL)
    } else {
      assertionFailure("Invalid message: \(message.body)")
    }
  }
}
