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

  enum CardanoError: Error {
    case providerError(BraveWallet.CardanoProviderErrorBundle)
    case internalError(String)
    case unknown
  }

  /// Converts CardanoError to JSON error string
  private func errorToJson(_ error: CardanoError) -> String? {
    switch error {
    case .providerError(let bundle):
      return buildErrorJson(
        code: bundle.code,
        errorMessage: bundle.errorMessage,
        paginationError: bundle.paginationErrorPayload
      )
    case .internalError(let message):
      return buildErrorJson(
        code: Int32(BraveWallet.CardanoProviderError.internalError.rawValue),
        errorMessage: message
      )
    case .unknown:
      return buildErrorJson(
        code: Int32(BraveWallet.CardanoProviderError.unknown.rawValue),
        errorMessage: Strings.Wallet.unknownError
      )
    }
  }

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

  typealias ApiResult = (data: Any?, error: BraveWallet.CardanoProviderErrorBundle?)

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
        do {
          _ = try await enable(tab: tab)
          // Return success - API object is stored in tab
          replyHandler("{}", nil)
        } catch let error as CardanoError {
          replyHandler(nil, errorToJson(error))
        } catch {
          replyHandler(nil, errorToJson(.unknown))
        }
        return

      case .isEnabled:
        let enabled = await isEnabled(tab: tab)
        replyHandler(enabled, nil)
        return

      default:
        break  // Fall through to API methods
      }

      // CIP-30 API methods - require API to be enabled first
      guard let api = tab.walletCardanoApi else {
        Logger.module.error("Cardano: API is nil - must call enable() first")
        replyHandler(
          nil,
          errorToJson(.internalError(Strings.Wallet.internalErrorMessage))
        )
        return
      }

      var cardanoApiResult: ApiResult
      switch body.method {
      case .getNetworkId:
        cardanoApiResult = await api.networkId()
      case .getUsedAddresses:
        cardanoApiResult = await api.usedAddresses()
      case .getUnusedAddresses:
        cardanoApiResult = await api.unusedAddresses()
      case .getChangeAddress:
        cardanoApiResult = await api.changeAddress()
      case .getRewardAddresses:
        cardanoApiResult = await api.rewardAddresses()
      case .getUtxos:
        let argsData = Data(body.args.utf8)
        let args = try? JSONDecoder().decode(GetUtxosArgs.self, from: argsData)
        let amount: String? = args?.amount
        let paginate: BraveWallet.CardanoProviderPagination? =
          args?.paginate.map { paginate in
            BraveWallet.CardanoProviderPagination(
              page: paginate.page,
              limit: paginate.limit
            )
          }
        cardanoApiResult = await api.utxos(amount: amount, paginate: paginate)
      case .getBalance:
        cardanoApiResult = await api.balance()
      case .signTx:
        let argsData = Data(body.args.utf8)
        guard let args = try? JSONDecoder().decode(SignTxArgs.self, from: argsData) else {
          replyHandler(
            nil,
            errorToJson(.internalError("Invalid arguments for signTx"))
          )
          return
        }
        cardanoApiResult = await api.signTx(txCbor: args.tx, partialSign: args.partialSign)
      case .signData:
        let argsData = Data(body.args.utf8)
        guard let args = try? JSONDecoder().decode(SignDataArgs.self, from: argsData) else {
          replyHandler(
            nil,
            errorToJson(.internalError("Invalid arguments for signData"))
          )
          return
        }
        cardanoApiResult = await api.signData(address: args.addr, payloadHex: args.payload)
      case .submitTx:
        let argsData = Data(body.args.utf8)
        guard let args = try? JSONDecoder().decode(SubmitTxArgs.self, from: argsData) else {
          replyHandler(
            nil,
            errorToJson(.internalError("Invalid arguments for submitTx"))
          )
          return
        }
        cardanoApiResult = await api.submitTx(signedTxCbor: args.tx)
      case .getCollateral:
        let argsData = Data(body.args.utf8)
        guard let args = try? JSONDecoder().decode(CollateralArgs.self, from: argsData) else {
          replyHandler(
            nil,
            errorToJson(.internalError("Invalid arguments for getCollateral"))
          )
          return
        }
        cardanoApiResult = await api.collateral(amount: args.amount)
      case .enable, .isEnabled:
        // Already handled above
        fatalError("Should not reach here")
      }

      handleApiResult(
        result: cardanoApiResult,
        replyHandler: replyHandler
      )
    }
  }

  private func handleApiResult(
    result: ApiResult,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    if let error = result.error {
      Logger.module.error("Cardano: API call failed - \(error.errorMessage)")
      replyHandler(nil, errorToJson(.providerError(error)))
      return
    }

    if let result = result.data {
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
      Logger.module.error("Cardano: API call returned nil with no error")
      replyHandler(nil, errorToJson(.unknown))
    }
  }

  /// Enables the Cardano provider and returns the API instance.
  @MainActor func enable(tab: some TabState) async throws -> BraveWalletCardanoApi {
    guard let provider = tab.walletCardanoProvider else {
      Logger.module.error("Cardano: Provider is nil in enable()")
      throw CardanoError.internalError(Strings.Wallet.internalErrorMessage)
    }

    Logger.module.info("Cardano: Calling provider.enable()...")

    return try await withTaskCancellationHandler {
      try await withCheckedThrowingContinuation { continuation in
        provider.enable { api, error in
          if let error = error {
            Logger.module.error("Cardano: Enable failed - \(error.errorMessage)")
            continuation.resume(throwing: CardanoError.providerError(error))
            return
          }

          if let api = api {
            Logger.module.info("Cardano: Enable succeeded! Got API object")
            tab.walletCardanoApi = api
            continuation.resume(returning: api)
          } else {
            Logger.module.error("Cardano: Enable returned nil API with no error")
            continuation.resume(throwing: CardanoError.unknown)
          }
        }
      }
    } onCancel: {
      // provider has no cancel method
      // user has to re-connect wallet in order
      // to make the site to call enable() again.
      Logger.module.info("Cardano: Enable task cancelled")
    }
  }

  /// Checks if the Cardano provider is enabled
  @MainActor func isEnabled(tab: some TabState) async -> Bool {
    guard let provider = tab.walletCardanoProvider else { return false }
    return await provider.isEnabled()
  }
}
