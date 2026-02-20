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
      // CIP-30 API methods
      case getNetworkId
      case getBalance
      case getUsedAddresses
      case getUnusedAddresses
      case getChangeAddress
      case getRewardAddresses
      case getUtxos
      case getCollateral
      case signTx
      case signData
      case submitTx
    }
    var method: Method
    var args: String?

    private enum CodingKeys: String, CodingKey {
      case method
      case args
    }
  }

  // Argument structures for parsing JSON args
  private struct GetUtxosArgs: Decodable {
    var amount: String?
    var paginate: Paginate?

    struct Paginate: Decodable {
      var page: Int32
      var limit: Int32
    }
  }

  private struct GetCollateralArgs: Decodable {
    var amount: String?
  }

  private struct SignTxArgs: Decodable {
    var tx: String
    var partialSign: Bool
  }

  private struct SignDataArgs: Decodable {
    var address: String
    var payload: String
  }

  private struct SubmitTxArgs: Decodable {
    var tx: String
  }

  func tab(
    _ tab: some TabState,
    receivedScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    Logger.module.info("🦴 Cardano: Received script message")
    if !verifyMessage(message: message) {
      Logger.module.error("🦴 Cardano: Security token verification FAILED")
      assertionFailure("Missing required security token.")
      return
    }

    if tab.walletCardanoProvider == nil {
      Logger.module.error("🦴 Cardano: walletCardanoProvider is NIL - provider not created!")
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
      Logger.module.error("🦴 Cardano: Failed to decode message or validate frame")
      return
    }
    Logger.module.info("🦴 Cardano: Message decoded - method: \(body.method.rawValue)")
    if message.webView?.url?.isLocal == false,
      message.webView?.hasOnlySecureContent == false
    {  // prevent communication in mixed-content scenarios
      Logger.module.error("🦴 Cardano: Failed cardano provider communication security test")
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
        } else if api != nil {
          // Return success - API object is stored in tab
          replyHandler("{}", nil)
        }
      case .isEnabled:
        await isEnabled(tab: tab, replyHandler: replyHandler)
      // CIP-30 API methods - require API to be enabled first
      case .getNetworkId:
        await callApiMethod(tab: tab, replyHandler: replyHandler) { api in
          await api.networkId()
        }
      case .getBalance:
        break;
//        await callApiMethod(tab: tab, replyHandler: replyHandler) { api in
//          await api.balance()
//        }
      case .getUsedAddresses:
        break;
//        await callApiMethod(tab: tab, replyHandler: replyHandler) { api in
//          await api.usedAddresses()
//        }
      case .getUnusedAddresses:
        break;
//        await callApiMethod(tab: tab, replyHandler: replyHandler) { api in
//          await api.unusedAddresses()
//        }
      case .getChangeAddress:
        break;
//        await callApiMethod(tab: tab, replyHandler: replyHandler) { api in
//          await api.changeAddress()
//        }
      case .getRewardAddresses:
        break;
//        await callApiMethod(tab: tab, replyHandler: replyHandler) { api in
//          await api.rewardAddresses()
//        }
      case .getUtxos:
        break;
        // TODO: getUtxos
//        guard let argsString = body.args,
//          let argsData = argsString.data(using: .utf8),
//          let args = try? JSONDecoder().decode(GetUtxosArgs.self, from: argsData)
//        else {
//          Logger.module.error("🦴 Cardano: Failed to parse getUtxos arguments")
//          replyHandler(nil, buildErrorJson(errorMessage: "Invalid arguments for getUtxos"))
//          return
//        }
//        await callApiMethod(tab: tab, replyHandler: replyHandler) { api in
//          await api.utxos(amount: args.amount, paginate: args.paginate.map { paginate in
//            BraveWallet.CardanoPaginate(page: paginate.page, limit: paginate.limit)
//          })
//        }
      case .getCollateral:
        break;
//        guard let argsString = body.args,
//          let argsData = argsString.data(using: .utf8),
//          let args = try? JSONDecoder().decode(GetCollateralArgs.self, from: argsData)
//        else {
//          Logger.module.error("🦴 Cardano: Failed to parse getCollateral arguments")
//          replyHandler(nil, buildErrorJson(errorMessage: "Invalid arguments for getCollateral"))
//          return
//        }
//        await callApiMethod(tab: tab, replyHandler: replyHandler) { api in
//          await api.collateral(amount: args.amount ?? "")
//        }
      case .signTx:
        break;
//        guard let argsString = body.args,
//          let argsData = argsString.data(using: .utf8),
//          let args = try? JSONDecoder().decode(SignTxArgs.self, from: argsData)
//        else {
//          Logger.module.error("🦴 Cardano: Failed to parse signTx arguments")
//          replyHandler(nil, buildErrorJson(errorMessage: "Invalid arguments for signTx"))
//          return
//        }
//        await callApiMethod(tab: tab, replyHandler: replyHandler) { api in
//          await api.signTx(tx: args.tx, isPartialSign: args.partialSign)
//        }
      case .signData:
        break;
//        guard let argsString = body.args,
//          let argsData = argsString.data(using: .utf8),
//          let args = try? JSONDecoder().decode(SignDataArgs.self, from: argsData)
//        else {
//          Logger.module.error("🦴 Cardano: Failed to parse signData arguments")
//          replyHandler(nil, buildErrorJson(errorMessage: "Invalid arguments for signData"))
//          return
//        }
//        await callApiMethod(tab: tab, replyHandler: replyHandler) { api in
//          await api.signData(address: args.address, payload: args.payload)
//        }
      case .submitTx:
        break;
//        guard let argsString = body.args,
//          let argsData = argsString.data(using: .utf8),
//          let args = try? JSONDecoder().decode(SubmitTxArgs.self, from: argsData)
//        else {
//          Logger.module.error("🦴 Cardano: Failed to parse submitTx arguments")
//          replyHandler(nil, buildErrorJson(errorMessage: "Invalid arguments for submitTx"))
//          return
//        }
//        await callApiMethod(tab: tab, replyHandler: replyHandler) { api in
//          await api.submitTx(signedTx: args.tx)
//        }
      }
    }
  }

  /// Enables the Cardano provider and returns the API instance.
  @MainActor func enable(tab: some TabState) async -> (BraveWalletCardanoApi?, String?) {
    guard let provider = tab.walletCardanoProvider else {
      Logger.module.error("🦴 Cardano: Provider is nil in enable()")
      return (nil, buildErrorJson(errorMessage: Strings.Wallet.internalErrorMessage))
    }

    Logger.module.info("🦴 Cardano: Calling provider.enable()...")

    return await withCheckedContinuation { continuation in
      provider.enable { api, error in
        if let error = error {
          Logger.module.error("🦴 Cardano: Enable failed - \(error.errorMessage)")
          continuation.resume(returning: (nil, self.buildErrorJson(errorMessage: error.errorMessage)))
          return
        }

        if let api = api {
          Logger.module.info("🦴 Cardano: Enable succeeded! Got API object")
          // Store API for future use
          tab.walletCardanoApi = api
          continuation.resume(returning: (api, nil))
        } else {
          Logger.module.error("🦴 Cardano: Enable returned nil API with no error")
          continuation.resume(returning: (nil, self.buildErrorJson(errorMessage: "No API returned")))
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

  /// Helper method to call API methods with error handling
  @MainActor private func callApiMethod<T>(
    tab: some TabState,
    replyHandler: @escaping (Any?, String?) -> Void,
    method: @escaping (BraveWalletCardanoApi) async -> (T?, BraveWallet.CardanoProviderErrorBundle?)
  ) async {
    guard let api = tab.walletCardanoApi else {
      Logger.module.error("🦴 Cardano: API is nil - must call enable() first")
      replyHandler(nil, buildErrorJson(errorMessage: "Not enabled - call enable() first"))
      return
    }

    let (result, error) = await method(api)

    if let error = error {
      Logger.module.error("🦴 Cardano: API call failed - \(error.errorMessage)")
      replyHandler(nil, buildErrorJson(errorMessage: error.errorMessage))
      return
    }

    if let result = result {
      // Convert result to JSON string for JavaScript
      if let resultString = result as? String {
        replyHandler(resultString, nil)
      } else if let resultArray = result as? [String] {
        // Convert array to JSON
        if let jsonData = try? JSONSerialization.data(withJSONObject: resultArray),
           let jsonString = String(data: jsonData, encoding: .utf8) {
          replyHandler(jsonString, nil)
        } else {
          replyHandler(nil, buildErrorJson(errorMessage: "Failed to serialize result"))
        }
      } else if let resultInt = result as? Int32 {
        replyHandler(resultInt, nil)
      } else {
        replyHandler(nil, buildErrorJson(errorMessage: "Unknown result type"))
      }
    } else {
      replyHandler(nil, buildErrorJson(errorMessage: "No result returned"))
    }
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
