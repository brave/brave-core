// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WebKit
import BraveCore
import struct Shared.Logger

private let log = Logger.browserLogger

class EthereumProviderHelper: TabContentScript {
  private static let supportedSingleArgMethods = [
    "net_listening", "net_peerCount",
    "net_version", "eth_chainId",
    "eth_syncing", "eth_coinbase",
    "eth_mining", "eth_hashrate",
    "eth_accounts", "eth_newBlockFilter",
    "eth_newPendingTransactionFilter"
  ]
  
  private weak var tab: Tab?
  
  init(tab: Tab) {
    self.tab = tab
  }
  
  static func name() -> String {
    return "walletEthereumProvider"
  }
  
  func scriptMessageHandlerName() -> String? {
    return "walletEthereumProvider_\(UserScriptManager.messageHandlerTokenString)"
  }
  
  static func shouldInjectWalletProvider(_ completion: @escaping (Bool) -> Void) {
    BraveWallet.KeyringServiceFactory.get(privateMode: false)?
      .keyringInfo(BraveWallet.DefaultKeyringId, completion: { keyring in
        completion(keyring.isKeyringCreated)
      })
  }
  
  private struct MessageBody: Decodable {
    enum Method: String, Decodable {
      case request
      case isConnected
      case enable
      case send
      case sendAsync
      case isUnlocked
    }
    var securityToken: String
    var method: Method
    var args: String
    
    private enum CodingKeys: String, CodingKey {
      case securityToken = "securitytoken"
      case method
      case args
    }
  }
  
  func userContentController(
    _ userContentController: WKUserContentController,
    didReceiveScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    guard let tab = tab,
          !tab.isPrivate,
          let provider = tab.walletEthProvider,
          !message.frameInfo.securityOrigin.host.isEmpty, // Fail if there is no last committed URL yet
          message.frameInfo.isMainFrame, // Fail the request came from 3p origin
          JSONSerialization.isValidJSONObject(message.body),
          let messageData = try? JSONSerialization.data(withJSONObject: message.body, options: []),
          let body = try? JSONDecoder().decode(MessageBody.self, from: messageData),
          body.securityToken == UserScriptManager.securityTokenString
    else {
      log.error("Failed to handle ethereum provider communication")
      return
    }
    
    if message.webView?.url?.isLocal == false,
        message.webView?.hasOnlySecureContent == false { // prevent communication in mixed-content scenarios
      log.error("Failed ethereum provider communication security test")
      return
    }
    
    // The web page has communicated with `window.ethereum`, so we should show the wallet icon
    tab.isWalletIconVisible = true
    
    func handleResponse(
      id: MojoBase.Value,
      formedResponse: MojoBase.Value,
      reject: Bool,
      firstAllowedAccount: String,
      updateJSProperties: Bool
    ) {
      if reject {
        replyHandler(nil, formedResponse.jsonString)
      } else {
        replyHandler(formedResponse.jsonObject, nil)
      }
      if updateJSProperties {
        tab.updateEthereumProperties()
      }
    }
    
    switch body.method {
    case .request:
      guard let requestPayload = MojoBase.Value(jsonString: body.args) else {
        replyHandler(nil, "Invalid args")
        return
      }
      provider.request(requestPayload, completion: handleResponse)
    case .isConnected:
      replyHandler(nil, nil)
    case .enable:
      provider.enable(handleResponse)
    case .sendAsync:
      guard let requestPayload = MojoBase.Value(jsonString: body.args) else {
        replyHandler(nil, "Invalid args")
        return
      }
      provider.request(requestPayload, completion: handleResponse)
    case .send:
      struct SendPayload {
        var method: String
        var params: MojoBase.Value?
        init?(payload: String) {
          guard let jsonValue = MojoBase.Value(jsonString: payload)?.dictionaryValue,
                let method = jsonValue["method"]?.stringValue
          else { return nil }
          self.method = method
          self.params = jsonValue["params"] // can be undefined in JS
        }
      }
      guard let sendPayload = SendPayload(payload: body.args) else {
        replyHandler(nil, "Invalid args")
        return
      }
      
      if sendPayload.method.isEmpty {
        if let params = sendPayload.params, params.tag != .null {
          // Same as sendAsync
          provider.request(params, completion: handleResponse)
        } else {
          // Empty method with no params is not valid
          replyHandler(nil, "Invalid args")
        }
        return
      }
      
      if !Self.supportedSingleArgMethods.contains(sendPayload.method),
         (sendPayload.params == nil || sendPayload.params?.tag == .null) {
        // If its not a single arg supported method and there are no parameters then its not a valid
        // call
        replyHandler(nil, "Invalid args")
        return
      }
      
      provider.send(
        sendPayload.method,
        params: sendPayload.params ?? .init(listValue: []),
        completion: handleResponse
      )
    case .isUnlocked:
      provider.isLocked { isLocked in
        replyHandler(!isLocked, nil)
      }
    }
  }
}
