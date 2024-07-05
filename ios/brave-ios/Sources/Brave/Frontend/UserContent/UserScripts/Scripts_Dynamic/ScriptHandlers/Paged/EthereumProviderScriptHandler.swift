// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Foundation
import WebKit
import os.log

class EthereumProviderScriptHandler: TabContentScript {
  private static let supportedSingleArgMethods = [
    "net_listening", "net_peerCount",
    "net_version", "eth_chainId",
    "eth_syncing", "eth_coinbase",
    "eth_mining", "eth_hashrate",
    "eth_accounts", "eth_newBlockFilter",
    "eth_newPendingTransactionFilter",
  ]

  private weak var tab: Tab?

  init(tab: Tab) {
    self.tab = tab
  }

  static let scriptName = "WalletEthereumProviderScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "\(scriptName)_\(messageUUID)"
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
      forMainFrameOnly: true,
      in: scriptSandbox
    )
  }()

  private struct MessageBody: Decodable {
    enum Method: String, Decodable {
      case request
      case isConnected
      case enable
      case send
      case sendAsync
      case isUnlocked
    }

    var method: Method
    var args: String

    private enum CodingKeys: String, CodingKey {
      case method
      case args
    }
  }

  func userContentController(
    _ userContentController: WKUserContentController,
    didReceive message: WKScriptMessage
  ) async -> (Any?, String?) {
    if !verifyMessage(message: message) {
      assertionFailure("Missing required security token.")
      return (nil, nil)
    }

    guard let tab = tab,
      !tab.isPrivate,
      let provider = tab.walletEthProvider,
      // Fail if there is no last committed URL yet
      !message.frameInfo.securityOrigin.host.isEmpty,
      message.frameInfo.isMainFrame,  // Fail the request came from 3p origin
      JSONSerialization.isValidJSONObject(message.body),
      let messageData = try? JSONSerialization.data(withJSONObject: message.body, options: []),
      let body = try? JSONDecoder().decode(MessageBody.self, from: messageData)
    else {
      Logger.module.error("Failed to handle ethereum provider communication")
      return (nil, nil)
    }

    if message.webView?.url?.isLocal == false,
      message.webView?.hasOnlySecureContent == false
    {  // prevent communication in mixed-content scenarios
      Logger.module.error("Failed ethereum provider communication security test")
      return (nil, nil)
    }

    // The web page has communicated with `window.ethereum`, so we should show the wallet icon
    tab.isWalletIconVisible = true

    switch body.method {
    case .request:
      guard let requestPayload = MojoBase.Value(jsonString: body.args) else {
        return (nil, "Invalid args")
      }
      return await handleResponse(provider.request(input: requestPayload))
    case .isConnected:
      return (nil, nil)
    case .enable:
      return await handleResponse(provider.enable())
    case .sendAsync:
      guard let requestPayload = MojoBase.Value(jsonString: body.args) else {
        return (nil, "Invalid args")
      }
      return await handleResponse(provider.sendAsync(input: requestPayload))
    case .send:
      struct SendPayload {
        var method: String
        var params: MojoBase.Value?
        init?(payload: String) {
          guard let jsonValue = MojoBase.Value(jsonString: payload)?.dictionaryValue,
            let method = jsonValue["method"]?.stringValue
          else { return nil }
          self.method = method
          self.params = jsonValue["params"]  // can be undefined in JS
        }
      }
      guard let sendPayload = SendPayload(payload: body.args) else {
        return (nil, "Invalid args")
      }

      if sendPayload.method.isEmpty {
        if let params = sendPayload.params, params.tag != .null {
          return await handleResponse(provider.sendAsync(input: params))
        }

        // Empty method with no params is not valid
        return (nil, "Invalid args")
      }

      if !Self.supportedSingleArgMethods.contains(sendPayload.method),
        sendPayload.params == nil || sendPayload.params?.tag == .null
      {
        // If its not a single arg supported method and there are no parameters then its not a valid
        // call
        return (nil, "Invalid args")
      }

      return await handleResponse(
        provider.send(
          method: sendPayload.method,
          params: sendPayload.params ?? .init(listValue: [])
        )
      )
    case .isUnlocked:
      let isLocked = await provider.isLocked()
      return (!isLocked, nil)
    }
  }

  @MainActor
  private func handleResponse(
    _ response: (
      id: MojoBase.Value, formedResponse: MojoBase.Value,
      reject: Bool,
      firstAllowedAccount: String, updateJSProperties: Bool
    )
  ) async -> (Any?, String?) {
    if response.updateJSProperties {
      await tab?.updateEthereumProperties()
    }

    if response.reject {
      return (nil, response.formedResponse.jsonString)
    }

    return (response.formedResponse.jsonObject, nil)
  }
}
