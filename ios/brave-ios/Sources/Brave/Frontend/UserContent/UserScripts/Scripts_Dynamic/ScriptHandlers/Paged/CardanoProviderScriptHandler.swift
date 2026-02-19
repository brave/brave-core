// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Foundation
import Strings
import Web
import WebKit
import os.log

class CardanoProviderScriptHandler: TabContentScript {

  fileprivate enum Keys: String {
    case code
    case message
  }

  static let scriptName = "WalletCardanoProviderScript"
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
      case enable
      case isEnabled
    }
    var method: Method
    var args: String?

    private enum CodingKeys: String, CodingKey {
      case method
      case args
    }
  }

  func tab(
    _ tab: some TabState,
    receivedScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    if !verifyMessage(message: message) {
      assertionFailure("Missing required security token.")
      return
    }

    guard !tab.isPrivate,
      let provider = tab.walletCardanoProvider,
      // Fail if there is no last committed URL yet
      !message.frameInfo.securityOrigin.host.isEmpty,
      message.frameInfo.isMainFrame,  // Fail if the request came from 3p origin
      JSONSerialization.isValidJSONObject(message.body),
      let messageData = try? JSONSerialization.data(withJSONObject: message.body, options: []),
      let body = try? JSONDecoder().decode(MessageBody.self, from: messageData)
    else {
      Logger.module.error("Failed to handle cardano provider communication")
      return
    }

    if message.webView?.url?.isLocal == false,
      message.webView?.hasOnlySecureContent == false
    {  // prevent communication in mixed-content scenarios
      Logger.module.error("Failed cardano provider communication security test")
      return
    }

    // The web page has communicated with `window.cardano.brave`, so we should show the wallet icon
    tab.isWalletIconVisible = true

    Task { @MainActor in
      switch body.method {
      case .enable:
        let (api, error) = await enable(tab: tab)
        if let error = error {
          replyHandler(nil, error)
        } else if let api = api {
          // TODO: Return success with API connection info to JavaScript
          // For now, just return empty success
          replyHandler("{}", nil)
        }
      case .isEnabled:
        await isEnabled(tab: tab, replyHandler: replyHandler)
      }
    }
  }

  /// Enables the Cardano provider and returns the API instance.
  /// TODO: This requires C++ interface change to return pending_remote<CardanoApi>
  @MainActor func enable(tab: some TabState) async -> (BraveWalletCardanoApi?, String?) {
    guard let provider = tab.walletCardanoProvider else {
      return (nil, buildErrorJson(errorMessage: Strings.Wallet.internalErrorMessage))
    }

    // TODO: Once C++ interface is changed to:
    // Enable() => (pending_remote<CardanoApi>? api, CardanoProviderErrorBundle? error)
    //
    // This will be implemented as:
    // let (api, error) = await provider.enable()
    // if let error = error {
    //   return (nil, buildErrorJson(errorMessage: error.errorMessage))
    // }
    // tab.walletCardanoApi = api  // Store for future use
    // return (api, nil)

    Logger.module.error("CardanoProvider.enable() is blocked - waiting for C++ interface change")
    return (nil, buildErrorJson(errorMessage: "Cardano dApp support is not yet available"))
  }

  /// Checks if the Cardano provider is enabled
  @MainActor func isEnabled(
    tab: some TabState,
    replyHandler: @escaping (Any?, String?) -> Void
  ) async {
    guard let provider = tab.walletCardanoProvider else {
      replyHandler(false, nil)
      return
    }

    let enabled = await provider.isEnabled()
    replyHandler(enabled, nil)
  }

  private func buildErrorJson(errorMessage: String) -> String? {
    JSONSerialization.jsObject(
      withNative: [
        Keys.code.rawValue: -1,
        Keys.message.rawValue: errorMessage,
      ] as [String: Any]
    )
  }
}
