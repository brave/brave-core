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
    case maxSize
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
      case getUsedAddresses
      case getUnusedAddresses
      case getChangeAddress
      case getRewardAddresses
      case getUtxos
      case getBalance
      case signTx
      case signData
      case submitTx
      case getCollateral
    }

    var method: Method
    var args: String

    private enum CodingKeys: String, CodingKey {
      case method
      case args
    }
  }

  // MARK: - Argument Structures

  /// Arguments for getUtxos. Both fields are optional per CIP-30 spec.
  /// Uses `try?` for decoding since missing arguments should default to nil.
  private struct GetUtxosArgs: Decodable {
    struct Paginate: Decodable {
      var page: Int32
      var limit: Int32
    }

    var amount: String?
    var paginate: Paginate?
  }

  /// Arguments for signData. Both fields are required.
  /// Uses `guard` for decoding to ensure validation before calling API.
  private struct SignDataArgs: Decodable {
    var addr: String
    var payload: String
  }

  /// Arguments for signTx. Both fields are required.
  /// Uses `guard` for decoding to ensure validation before calling API.
  private struct SignTxArgs: Decodable {
    var tx: String
    var partialSign: Bool
  }

  /// Arguments for getCollateral. The amount field is required.
  /// Uses `guard` for decoding to ensure validation before calling API.
  private struct CollateralArgs: Decodable {
    var amount: String
  }

  /// Arguments for submitTx. The tx field is required.
  /// Uses `guard` for decoding to ensure validation before calling API.
  private struct SubmitTxArgs: Decodable {
    var tx: String
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
      // CIP-30 API methods - require API to be enabled first
      case .getNetworkId:
        await callApiMethod(tab: tab, replyHandler: replyHandler) { api in
          await api.networkId()
        }
      case .getUsedAddresses:
        await callApiMethod(tab: tab, replyHandler: replyHandler) { api in
          await api.usedAddresses()
        }
      case .getUnusedAddresses:
        await callApiMethod(tab: tab, replyHandler: replyHandler) { api in
          await api.unusedAddresses()
        }
      case .getChangeAddress:
        await callApiMethod(tab: tab, replyHandler: replyHandler) { api in
          await api.changeAddress()
        }
      case .getRewardAddresses:
        await callApiMethod(tab: tab, replyHandler: replyHandler) { api in
          await api.rewardAddresses()
        }
      case .getUtxos:
        let argsData = body.args.data(using: .utf8) ?? Data()
        let args = try? JSONDecoder().decode(GetUtxosArgs.self, from: argsData)
        await callApiMethod(tab: tab, replyHandler: replyHandler) { api in
          let amount: String? = args?.amount
          let paginate: BraveWallet.CardanoProviderPagination? =
            args?.paginate.map { paginate in
              BraveWallet.CardanoProviderPagination(
                page: paginate.page,
                limit: paginate.limit
              )
            }
          return await api.utxos(amount: amount, paginate: paginate)
        }
      case .getBalance:
        await callApiMethod(tab: tab, replyHandler: replyHandler) { api in
          await api.balance()
        }
      case .signTx:
        let argsData = body.args.data(using: .utf8) ?? Data()
        guard let args = try? JSONDecoder().decode(SignTxArgs.self, from: argsData) else {
          replyHandler(
            nil,
            buildErrorJson(
              code: Int32(BraveWallet.CardanoProviderError.internalError.rawValue),
              errorMessage: "Invalid arguments for signTx"
            )
          )
          return
        }
        await callApiMethod(tab: tab, replyHandler: replyHandler) { api in
          await api.signTx(txCbor: args.tx, partialSign: args.partialSign)
        }
      case .signData:
        let argsData = body.args.data(using: .utf8) ?? Data()
        guard let args = try? JSONDecoder().decode(SignDataArgs.self, from: argsData) else {
          replyHandler(
            nil,
            buildErrorJson(
              code: Int32(BraveWallet.CardanoProviderError.internalError.rawValue),
              errorMessage: "Invalid arguments for signData"
            )
          )
          return
        }

        await callApiMethod(tab: tab, replyHandler: replyHandler) { api in
          await api.signData(address: args.addr, payloadHex: args.payload)
        }
      case .submitTx:
        let argsData = body.args.data(using: .utf8) ?? Data()
        guard let args = try? JSONDecoder().decode(SubmitTxArgs.self, from: argsData) else {
          replyHandler(
            nil,
            buildErrorJson(
              code: Int32(BraveWallet.CardanoProviderError.internalError.rawValue),
              errorMessage: "Invalid arguments for submitTx"
            )
          )
          return
        }
        await callApiMethod(tab: tab, replyHandler: replyHandler) { api in
          await api.submitTx(signedTxCbor: args.tx)
        }
      case .getCollateral:
        let argsData = body.args.data(using: .utf8) ?? Data()
        guard let args = try? JSONDecoder().decode(CollateralArgs.self, from: argsData) else {
          replyHandler(
            nil,
            buildErrorJson(
              code: Int32(BraveWallet.CardanoProviderError.internalError.rawValue),
              errorMessage: "Invalid arguments for getCollateral"
            )
          )
          return
        }
        await callApiMethod(tab: tab, replyHandler: replyHandler) { api in
          await api.collateral(amount: args.amount)
        }
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
          code: Int32(BraveWallet.CardanoProviderError.internalError.rawValue),
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
              nil,
              self.buildErrorJson(
                code: error.code,
                errorMessage: error.errorMessage
              )
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
            returning: (
              nil,
              self.buildErrorJson(
                code: Int32(BraveWallet.CardanoProviderError.unknown.rawValue),
                errorMessage: "No API returned"
              )
            )
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

  /// Builds an error JSON response following CIP-30 error format.
  ///
  /// Error handling follows three cases per CIP-30 specification:
  /// 1. Regular errors: Returns `{code: number, info: string}`
  /// 2. Pagination errors: Returns `{code: number, info: string, maxSize: number}`
  /// 3. Local validation errors: Uses hardcoded error codes when no backend error exists
  ///
  /// - Parameters:
  ///   - code: Error code from CardanoProviderErrorBundle or hardcoded CardanoProviderError enum
  ///   - errorMessage: Human-readable error message
  ///   - paginationError: Optional pagination error payload containing maxSize when page limit exceeded
  /// - Returns: JSON string representation of the error object
  private func buildErrorJson(
    code: Int32,
    errorMessage: String,
    paginationError: BraveWallet.CardanoProviderPaginationErrorPayload? = nil
  ) -> String? {
    var fields: [String: Any] = [
      Keys.code.rawValue: code,
      Keys.info.rawValue: errorMessage,
    ]
    if let paginationError {
      fields[Keys.maxSize.rawValue] = paginationError.payload
    }
    return JSONSerialization.jsObject(withNative: fields)
  }

  /// Helper method to call CIP-30 API methods with unified error handling.
  ///
  /// This method handles all API calls after enable() succeeds and provides:
  /// 1. API availability check (must call enable() first)
  /// 2. Error handling with proper error codes from CardanoProviderErrorBundle
  /// 3. Special handling for BaseValue dictionary results (signData method)
  /// 4. Pagination error support when page limits are exceeded
  ///
  /// - Parameters:
  ///   - tab: Current tab containing the CardanoApi instance
  ///   - replyHandler: WebKit reply handler for sending response back to JavaScript
  ///   - method: Async closure that calls the actual API method
  @MainActor private func callApiMethod<T>(
    tab: some TabState,
    replyHandler: @escaping (Any?, String?) -> Void,
    method: @escaping (BraveWalletCardanoApi) async -> (T?, BraveWallet.CardanoProviderErrorBundle?)
  ) async {
    guard let api = tab.walletCardanoApi else {
      Logger.module.error("Cardano: API is nil - must call enable() first")
      replyHandler(
        nil,
        buildErrorJson(
          code: Int32(BraveWallet.CardanoProviderError.internalError.rawValue),
          errorMessage: "Not enabled - call enable() first"
        )
      )
      return
    }

    let (result, error) = await method(api)

    if let error = error {
      Logger.module.error("Cardano: API call failed - \(error.errorMessage)")
      replyHandler(
        nil,
        buildErrorJson(
          code: error.code,
          errorMessage: error.errorMessage,
          paginationError: error.paginationErrorPayload
        )
      )
      return
    }

    if let result = result {
      // Check if result is a BaseValue dictionary (from signData)
      if let dictResult = result as? [String: BaseValue] {
        // Convert BaseValue dictionary to String dictionary
        // BaseValue objects from Mojo cannot be directly serialized, so we extract
        // the stringValue from each BaseValue and create a plain String dictionary
        var stringDicResult: [String: String] = [:]
        for (key, value) in dictResult {
          if let stringValue = value.stringValue {
            stringDicResult[key] = stringValue
          }
        }
        replyHandler(stringDicResult, nil)
      } else {
        // For all other types (String, [String], Int32), pass directly
        // WebKit automatically converts these Swift types to JavaScript equivalents
        replyHandler(result, nil)
      }
    } else {
      // API returned nil result with no error - this shouldn't happen
      replyHandler(
        nil,
        buildErrorJson(
          code: Int32(BraveWallet.CardanoProviderError.unknown.rawValue),
          errorMessage: "No result returned"
        )
      )
    }
  }
}
