// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import Strings
import Web
import WebKit
import os.log

class CardanoProviderScriptHandler: TabContentScript {

  fileprivate enum Keys: String {
    case code
    case info
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
    var args: String

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
      // Fail if there is no last committed URL yet
      !message.frameInfo.securityOrigin.host.isEmpty,
      message.frameInfo.isMainFrame,  // Fail the request came from 3p origin
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

    // The web page has communicated with `window.cardano`, so we should show the wallet icon
    tab.isWalletIconVisible = true

    Task { @MainActor in
      switch body.method {
      case .enable:
        let (api, error) = await enable(tab: tab)
        if let error = error {
          replyHandler(nil, error)
        } else if api != nil {
          // Return success - API object is stored in tab
          replyHandler("{}", nil)
        }
      case .isEnabled:
        await isEnabled(tab: tab, replyHandler: replyHandler)
      }
    }
  }

  /// Enables the Cardano provider and returns the API instance.
  @MainActor func enable(tab: some TabState) async -> (BraveWalletCardanoApi?, String?) {
    guard let provider = tab.walletCardanoProvider else {
      Logger.module.error("Cardano: Provider is nil in enable()")
      return (
        nil,
        buildErrorJson(
          status: .internalError,
          errorMessage: Strings.Wallet.internalErrorMessage
        )
      )
    }

    Logger.module.info("Cardano: Calling provider.enable()...")

    return await withCheckedContinuation { continuation in
      provider.enable { api, error in
        if let error = error {
          Logger.module.error("Cardano: Enable failed - \(error.errorMessage)")
          continuation.resume(
            returning: (
              nil, self.buildErrorJson(status: .internalError, errorMessage: error.errorMessage)
            )
          )
          return
        }

        if let api = api {
          Logger.module.info("Cardano: Enable succeeded! Got API object")
          // Store API for future use
          tab.walletCardanoApi = api
          continuation.resume(returning: (api, nil))
        } else {
          Logger.module.error("Cardano: Enable returned nil API with no error")
          continuation.resume(
            returning: (nil, self.buildErrorJson(status: .unknown, errorMessage: "No API returned"))
          )
        }
      }
    }
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

  private func buildErrorJson(
    status: BraveWallet.CardanoProviderError,
    errorMessage: String
  ) -> String? {
    JSONSerialization.jsObject(
      withNative: [
        Keys.code.rawValue: status.rawValue,
        Keys.info.rawValue: errorMessage,
      ] as [String: Any]
    )
  }
}
