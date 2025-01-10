// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Shared
import UIKit
import XCTest

@testable import Brave
@testable import BraveWallet

@MainActor class SolanaProviderScriptHandlerTests: XCTestCase {

  /// Test `connect()`, given no parameters, the error flow will return a tuple `(result: Any?, error: String?)`
  /// with the error populated as the given error dictionary
  func testConnectFailure() async {
    let provider: BraveWallet.TestSolanaProvider = .init()
    provider._connect = { params, completion in
      completion(.internalError, Strings.Wallet.internalErrorMessage, "")
    }
    let tab = Tab(configuration: .init())
    let solProviderHelper = SolanaProviderScriptHandler()
    tab.walletSolProvider = provider

    let (publicKey, error) = await solProviderHelper.connect(tab: tab, args: nil)
    XCTAssertNil(publicKey)
    guard let error = error,  // dictionary's are unordered, need to check each value
      let errorDict = MojoBase.Value(jsonString: error)?.dictionaryValue,
      let code = errorDict["code"]?.intValue, let message = errorDict["message"]?.stringValue
    else {
      XCTFail("Unexpected result to connect request")
      return
    }
    XCTAssertEqual(code, Int32(BraveWallet.SolanaProviderError.internalError.rawValue))
    XCTAssertEqual(message, Strings.Wallet.internalErrorMessage)
  }

  /// Test `connect()`, given no parameters, the success flow will return a tuple `(result: Any?, error: String?)`
  /// with the result as type `String` (base58 encoded public key)
  func testConnectSuccess() async {
    let provider: BraveWallet.TestSolanaProvider = .init()
    provider._connect = { [kTestPublicKey] params, completion in
      completion(.success, "", kTestPublicKey)
    }
    let tab = Tab(configuration: .init())
    let solProviderHelper = SolanaProviderScriptHandler()
    tab.walletSolProvider = provider

    let (publicKey, error) = await solProviderHelper.connect(tab: tab, args: nil)
    XCTAssertNil(error)
    guard let publicKey = publicKey as? String else {
      XCTFail("Unexpected result for publickey")
      return
    }
    XCTAssertEqual(publicKey, kTestPublicKey)
  }

  /// Test `connect()`, given json string `{onlyIfTrusted: Bool}`, the success flow will return a tuple `(result: Any?, error: String?)`
  /// with the result as type `String` (base58 encoded public key)
  func testConnectOnlyIfTrustedSuccess() async {
    let argDict: [String: MojoBase.Value] = ["onlyIfTrusted": MojoBase.Value(boolValue: true)]
    guard let args = MojoBase.Value(dictionaryValue: argDict).jsonString else {
      XCTFail("Unexpected test setup")
      return
    }
    let provider: BraveWallet.TestSolanaProvider = .init()
    provider._connect = { [kTestPublicKey] params, completion in
      XCTAssertNotNil(params)  // onlyIfTrusted provided as an arg, should be non-nil
      completion(.success, "", kTestPublicKey)
    }
    let tab = Tab(configuration: .init())
    let solProviderHelper = SolanaProviderScriptHandler()
    tab.walletSolProvider = provider

    let (publicKey, error) = await solProviderHelper.connect(tab: tab, args: args)
    XCTAssertNil(error)
    guard let publicKey = publicKey as? String else {
      XCTFail("Unexpected result for publickey")
      return
    }
    XCTAssertEqual(publicKey, kTestPublicKey)
  }

  /// Test `signAndSendTransaction()`, given a json string `{serializedMessage: [Uint8], signatures: [Buffer]}`,
  /// the error flow will return a tuple `(Any?, String?)` with the error populated as the given error dictionary
  func testSignAndSendTransactionFailure() async {
    let argDict: [String: MojoBase.Value] = [
      "serializedMessage": MojoBase.Value(
        listValue: kSerializedMessage.map(MojoBase.Value.init(intValue:))
      ),
      "signatures": MojoBase.Value(listValue: []),
    ]
    guard let args = MojoBase.Value(dictionaryValue: argDict).jsonString else {
      XCTFail("Unexpected test setup")
      return
    }

    let provider: BraveWallet.TestSolanaProvider = .init()
    provider._signAndSendTransaction = { signTransactionParam, sendOptions, completion in
      completion(.internalError, Strings.Wallet.internalErrorMessage, [:])
    }
    let tab = Tab(configuration: .init())
    let solProviderHelper = SolanaProviderScriptHandler()
    tab.walletSolProvider = provider

    let (result, error) = await solProviderHelper.signAndSendTransaction(tab: tab, args: args)
    XCTAssertNil(result)
    guard let error = error,  // dictionary's are unordered, need to check each value
      let errorDict = MojoBase.Value(jsonString: error)?.dictionaryValue,
      let code = errorDict["code"]?.intValue, let message = errorDict["message"]?.stringValue
    else {
      XCTFail("Unexpected result to signAndSendTransaction request")
      return
    }
    XCTAssertEqual(code, Int32(BraveWallet.SolanaProviderError.internalError.rawValue))
    XCTAssertEqual(message, Strings.Wallet.internalErrorMessage)
  }

  /// Test `signAndSendTransaction()`, given a json string `{serializedMessage: [Uint8], signatures: [Buffer]}`, the success flow will
  /// return a tuple `(result: Any?, error: String?)` with the result as type `[String: String]` with the base 58 encoded public key and signature
  func testSignAndSendTransactionSuccess() async {
    let argDict: [String: MojoBase.Value] = [
      "serializedMessage": MojoBase.Value(
        listValue: kSerializedMessage.map(MojoBase.Value.init(intValue:))
      ),
      "signatures": MojoBase.Value(listValue: []),
    ]
    guard let args = MojoBase.Value(dictionaryValue: argDict).jsonString else {
      XCTFail("Unexpected test setup")
      return
    }

    let provider: BraveWallet.TestSolanaProvider = .init()
    provider._signAndSendTransaction = {
      [kTestPublicKey, kTestSignature] signTransactionParam, sendOptions, completion in
      completion(
        .success,
        "",
        [
          "publicKey": MojoBase.Value(stringValue: kTestPublicKey),
          "signature": MojoBase.Value(stringValue: kTestSignature),
        ]
      )
    }
    let tab = Tab(configuration: .init())
    let solProviderHelper = SolanaProviderScriptHandler()
    tab.walletSolProvider = provider

    let (result, error) = await solProviderHelper.signAndSendTransaction(tab: tab, args: args)
    XCTAssertNil(error)
    guard let result = result as? [String: String] else {
      XCTFail("Unexpected result to signAndSendTransaction request")
      return
    }
    XCTAssertEqual(result["publicKey"], kTestPublicKey)
    XCTAssertEqual(result["signature"], kTestSignature)
  }

  /// Test `signAndSendTransaction()`, given a json string `{serializedMessage: [Uint8], signatures: [Buffer], sendOptions: [:]}`, the success
  /// flow will return a tuple `(result: Any?, error: String?)` with the result as type `[String: String]` with the base 58 encoded public key and signature
  func testSignAndSendTransactionWithSendOptionsSuccess() async {
    let argDict: [String: MojoBase.Value] = [
      "serializedMessage": MojoBase.Value(
        listValue: kSerializedMessage.map(MojoBase.Value.init(intValue:))
      ),
      "signatures": MojoBase.Value(listValue: []),
      "sendOptions": MojoBase.Value(dictionaryValue: [:]),
    ]
    guard let args = MojoBase.Value(dictionaryValue: argDict).jsonString else {
      XCTFail("Unexpected test setup")
      return
    }

    let provider: BraveWallet.TestSolanaProvider = .init()
    provider._signAndSendTransaction = {
      [kTestPublicKey, kTestSignature] signTransactionParam, sendOptions, completion in
      XCTAssertNotNil(sendOptions)  // sendOptions provided as an arg, should be non-nil
      completion(
        .success,
        "",
        [
          "publicKey": MojoBase.Value(stringValue: kTestPublicKey),
          "signature": MojoBase.Value(stringValue: kTestSignature),
        ]
      )
    }
    let tab = Tab(configuration: .init())
    let solProviderHelper = SolanaProviderScriptHandler()
    tab.walletSolProvider = provider

    let (result, error) = await solProviderHelper.signAndSendTransaction(tab: tab, args: args)
    XCTAssertNil(error)
    guard let result = result as? [String: String] else {
      XCTFail("Unexpected result to signAndSendTransaction request")
      return
    }
    XCTAssertEqual(result["publicKey"], kTestPublicKey)
    XCTAssertEqual(result["signature"], kTestSignature)
  }

  /// Test `signMessage()`, given a json string `{[[UInt8]]}`, the error flow will return
  /// a tuple `(Any?, String?)` with the error populated as the given error dictionary
  func testSignMessageFailure() async {
    let argArray: [MojoBase.Value] = [
      MojoBase.Value(listValue: kMessageToSign.map { MojoBase.Value.init(intValue: $0) })
    ]
    guard let args = MojoBase.Value(listValue: argArray).jsonString else {
      XCTFail("Unexpected test setup")
      return
    }

    let provider: BraveWallet.TestSolanaProvider = .init()
    provider._signMessage = { _, _, completion in
      completion(.internalError, Strings.Wallet.internalErrorMessage, [:])
    }
    let tab = Tab(configuration: .init())
    let solProviderHelper = SolanaProviderScriptHandler()
    tab.walletSolProvider = provider

    let (result, error) = await solProviderHelper.signMessage(tab: tab, args: args)
    XCTAssertNil(result)
    guard let error = error,  // dictionary's are unordered, need to check each value
      let errorDict = MojoBase.Value(jsonString: error)?.dictionaryValue,
      let code = errorDict["code"]?.intValue, let message = errorDict["message"]?.stringValue
    else {
      XCTFail("Unexpected result to signMessage request")
      return
    }
    XCTAssertEqual(code, Int32(BraveWallet.SolanaProviderError.internalError.rawValue))
    XCTAssertEqual(message, Strings.Wallet.internalErrorMessage)
  }

  /// Test `signMessage()`, given a json string `{[[UInt8]]}`, the success flow will return a tuple `(result: Any?, error: String?)`
  /// with the result as js object with the base 58 encoded public key and signature as decoded base 58 (array of UInt8).
  func testSignMessageSuccess() async {
    let argArray: [MojoBase.Value] = [
      MojoBase.Value(listValue: kMessageToSign.map { MojoBase.Value.init(intValue: $0) })
    ]
    guard let args = MojoBase.Value(listValue: argArray).jsonString else {
      XCTFail("Unexpected test setup")
      return
    }

    let provider: BraveWallet.TestSolanaProvider = .init()
    provider._signMessage = {
      [kTestPublicKey, kTestSignature] blobMsg, displayEncoding, completion in
      completion(
        .success,
        "",
        [
          "publicKey": MojoBase.Value(stringValue: kTestPublicKey),
          "signature": MojoBase.Value(stringValue: kTestSignature),
        ]
      )
    }
    let tab = Tab(configuration: .init())
    let solProviderHelper = SolanaProviderScriptHandler()
    tab.walletSolProvider = provider

    let (result, error) = await solProviderHelper.signMessage(tab: tab, args: args)
    XCTAssertNil(error)
    guard let result = MojoBase.Value(jsonString: (result as? String) ?? "")?.dictionaryValue else {
      XCTFail("Unexpected result to signMessage request")
      return
    }
    XCTAssertEqual(result["publicKey"]?.stringValue, kTestPublicKey)
    let signature: [UInt8] =
      result["signature"]?.listValue?.map { $0.intValue }.map(UInt8.init) ?? []
    let expectedSignature: [UInt8] =
      (NSData(base58Encoded: kTestSignature) as? Data)?.getBytes() ?? []
    XCTAssertFalse(signature.isEmpty)
    XCTAssertEqual(signature, expectedSignature)
  }

  /// Test `signMessage()`, given a json string `{[[UInt8], String]}`, the success flow will return a tuple `(result: Any?, error: String?)`
  /// with the result as type `[String: Any]` with the base 58 encoded public key and signature as decoded base 58 (array of UInt8).
  func testSignMessageWithDisplayEncodingSuccess() async {
    let argArray: [MojoBase.Value] = [
      MojoBase.Value(listValue: kMessageToSign.map { MojoBase.Value.init(intValue: $0) }),
      MojoBase.Value(stringValue: "hex"),
    ]
    guard let args = MojoBase.Value(listValue: argArray).jsonString else {
      XCTFail("Unexpected test setup")
      return
    }

    let provider: BraveWallet.TestSolanaProvider = .init()
    provider._signMessage = {
      [kTestPublicKey, kTestSignature] blobMsg, displayEncoding, completion in
      // displayEncoding provided as optional arg, should be non-nil
      XCTAssertNotNil(displayEncoding)
      completion(
        .success,
        "",
        [
          "publicKey": MojoBase.Value(stringValue: kTestPublicKey),
          "signature": MojoBase.Value(stringValue: kTestSignature),
        ]
      )
    }
    let tab = Tab(configuration: .init())
    let solProviderHelper = SolanaProviderScriptHandler()
    tab.walletSolProvider = provider

    let (result, error) = await solProviderHelper.signMessage(tab: tab, args: args)
    XCTAssertNil(error)
    guard let result = MojoBase.Value(jsonString: (result as? String) ?? "")?.dictionaryValue else {
      XCTFail("Unexpected result to signMessage request")
      return
    }
    XCTAssertEqual(result["publicKey"]?.stringValue, kTestPublicKey)
    let signature: [UInt8] =
      result["signature"]?.listValue?.map { $0.intValue }.map(UInt8.init) ?? []
    let expectedSignature: [UInt8] =
      (NSData(base58Encoded: kTestSignature) as? Data)?.getBytes() ?? []
    XCTAssertFalse(signature.isEmpty)
    XCTAssertEqual(signature, expectedSignature)
  }

  /// Test `signTransaction()`, given a json string `{serializedMessage: Buffer, signatures: {publicKey: String, signature: Buffer}}`,
  /// the error flow will return a tuple `(Any?, String?)` with the error populated as the given error dictionary
  func testSignTransactionFailure() async {
    let signature = MojoBase.Value(dictionaryValue: [
      "publicKey": MojoBase.Value(stringValue: kTestPublicKey)
    ])
    let serializedMessage = MojoBase.Value(
      listValue: kSerializedMessage.map { MojoBase.Value(intValue: $0) }
    )
    let transaction = MojoBase.Value(dictionaryValue: [
      "serializedMessage": serializedMessage,
      "signatures": MojoBase.Value(listValue: [signature]),
    ])
    guard let args = transaction.jsonString else {
      XCTFail("Unexpected test setup")
      return
    }

    let provider: BraveWallet.TestSolanaProvider = .init()
    provider._signTransaction = { _, completion in
      completion(.internalError, Strings.Wallet.internalErrorMessage, [], .legacy)
    }
    let tab = Tab(configuration: .init())
    let solProviderHelper = SolanaProviderScriptHandler()
    tab.walletSolProvider = provider

    let (result, error) = await solProviderHelper.signTransaction(tab: tab, args: args)
    XCTAssertNil(result)
    guard let error = error,  // dictionary's are unordered, need to check each value
      let errorDict = MojoBase.Value(jsonString: error)?.dictionaryValue,
      let code = errorDict["code"]?.intValue, let message = errorDict["message"]?.stringValue
    else {
      XCTFail("Unexpected result to signTransaction request")
      return
    }
    XCTAssertEqual(code, Int32(BraveWallet.SolanaProviderError.internalError.rawValue))
    XCTAssertEqual(message, Strings.Wallet.internalErrorMessage)
  }

  /// Test `signTransaction()`, given a json string `{serializedMessage: Buffer, signatures: {publicKey: String, signature: Buffer}}`,
  /// the success flow will return a tuple `(result: Any?, error: String?)` with the result as type `String` containing a json array of UInt8
  func testSignTransactionSuccessLegacyTx() async {
    let signature = MojoBase.Value(dictionaryValue: [
      "publicKey": MojoBase.Value(stringValue: kTestPublicKey)
    ])
    let serializedMessage = MojoBase.Value(
      listValue: kSerializedMessage.map { MojoBase.Value(intValue: $0) }
    )
    let transaction = MojoBase.Value(dictionaryValue: [
      "serializedMessage": serializedMessage,
      "signatures": MojoBase.Value(listValue: [signature]),
    ])
    guard let args = transaction.jsonString else {
      XCTFail("Unexpected test setup")
      return
    }

    let provider: BraveWallet.TestSolanaProvider = .init()
    provider._signTransaction = { [kSerializedTx] _, completion in
      completion(.success, "", kSerializedTx.map(NSNumber.init(value:)), .legacy)
    }
    let tab = Tab(configuration: .init())
    let solProviderHelper = SolanaProviderScriptHandler()
    tab.walletSolProvider = provider

    let (result, error) = await solProviderHelper.signTransaction(tab: tab, args: args)
    XCTAssertNil(error)
    guard let result = result as? [String: Any] else {
      XCTFail("Unexpected result to signTransaction request")
      return
    }
    let serializedTx =
      (result["serializedTx"] as? [NSNumber])?.compactMap({ Int32($0.intValue) }) ?? []
    XCTAssertEqual(serializedTx, kSerializedTx)
    let version = result["version"] as? Int ?? -1
    XCTAssertEqual(version, BraveWallet.SolanaMessageVersion.legacy.rawValue)
  }

  /// Test `signTransaction()`, given a json string `{serializedMessage: Buffer, signatures: {publicKey: String, signature: Buffer}}`,
  /// the success flow will return a tuple `(result: Any?, error: String?)` with the result as type `String` containing a json array of UInt8
  func testSignTransactionSuccessVersionedTxV0() async {
    let signature = MojoBase.Value(dictionaryValue: [
      "publicKey": MojoBase.Value(stringValue: kTestPublicKey)
    ])
    let serializedMessage = MojoBase.Value(
      listValue: kSerializedMessage.map { MojoBase.Value(intValue: $0) }
    )
    let transaction = MojoBase.Value(dictionaryValue: [
      "serializedMessage": serializedMessage,
      "signatures": MojoBase.Value(listValue: [signature]),
    ])
    guard let args = transaction.jsonString else {
      XCTFail("Unexpected test setup")
      return
    }

    let provider: BraveWallet.TestSolanaProvider = .init()
    provider._signTransaction = { [kSerializedTx] _, completion in
      completion(.success, "", kSerializedTx.map(NSNumber.init(value:)), .V0)
    }
    let tab = Tab(configuration: .init())
    let solProviderHelper = SolanaProviderScriptHandler()
    tab.walletSolProvider = provider

    let (result, error) = await solProviderHelper.signTransaction(tab: tab, args: args)
    XCTAssertNil(error)
    guard let result = result as? [String: Any] else {
      XCTFail("Unexpected result to signTransaction request")
      return
    }
    let serializedTx =
      (result["serializedTx"] as? [NSNumber])?.compactMap({ Int32($0.intValue) }) ?? []
    XCTAssertEqual(serializedTx, kSerializedTx)
    let version = result["version"] as? Int ?? -1
    XCTAssertEqual(version, BraveWallet.SolanaMessageVersion.V0.rawValue)
  }

  /// Test `signAllTransactions()`, given a json string `[{serializedMessage: Buffer, signatures: {publicKey: String, signature: Buffer}}]`,
  /// the error flow will return a tuple `(Any?, String?)` with the error populated as the given error dictionary
  func testSignAllTransactionsFailure() async {
    let signature = MojoBase.Value(dictionaryValue: [
      "publicKey": MojoBase.Value(stringValue: kTestPublicKey)
    ])
    let serializedMessage = MojoBase.Value(
      listValue: kSerializedMessage.map { MojoBase.Value(intValue: $0) }
    )
    let transaction = MojoBase.Value(dictionaryValue: [
      "serializedMessage": serializedMessage,
      "signatures": MojoBase.Value(listValue: [signature]),
    ])
    guard let args = MojoBase.Value(listValue: [transaction]).jsonString else {
      XCTFail("Unexpected test setup")
      return
    }

    let provider: BraveWallet.TestSolanaProvider = .init()
    provider._signAllTransactions = { _, completion in
      completion(.internalError, Strings.Wallet.internalErrorMessage, [], [])
    }
    let tab = Tab(configuration: .init())
    let solProviderHelper = SolanaProviderScriptHandler()
    tab.walletSolProvider = provider

    let (result, error) = await solProviderHelper.signAllTransactions(tab: tab, args: args)
    XCTAssertNil(result)
    guard let error = error,  // dictionary's are unordered, need to check each value
      let errorDict = MojoBase.Value(jsonString: error)?.dictionaryValue,
      let code = errorDict["code"]?.intValue, let message = errorDict["message"]?.stringValue
    else {
      XCTFail("Unexpected result to signAllTransactions request")
      return
    }
    XCTAssertEqual(code, Int32(BraveWallet.SolanaProviderError.internalError.rawValue))
    XCTAssertEqual(message, Strings.Wallet.internalErrorMessage)
  }

  /// Test `signAllTransactions()`, given a json string `[{serializedMessage: Buffer, signatures: {publicKey: String, signature: Buffer}}]`,
  /// the success flow will return a tuple `(result: Any?, error: String?)` with the result as type `String` containing a json 2-dimensional array of Uint8.
  func testSignAllTransactionsSuccessLegacyTx() async {
    let signature = MojoBase.Value(dictionaryValue: [
      "publicKey": MojoBase.Value(stringValue: kTestPublicKey)
    ])
    let serializedMessage = MojoBase.Value(
      listValue: kSerializedMessage.map { MojoBase.Value(intValue: $0) }
    )
    let transaction = MojoBase.Value(dictionaryValue: [
      "serializedMessage": serializedMessage,
      "signatures": MojoBase.Value(listValue: [signature]),
    ])
    guard let args = MojoBase.Value(listValue: [transaction]).jsonString else {
      XCTFail("Unexpected test setup")
      return
    }

    let provider: BraveWallet.TestSolanaProvider = .init()
    provider._signAllTransactions = { [kSerializedTx] _, completion in
      completion(
        .success,
        "",
        [kSerializedTx.map(NSNumber.init(value:))],
        [NSNumber(value: BraveWallet.SolanaMessageVersion.legacy.rawValue)]
      )
    }
    let tab = Tab(configuration: .init())
    let solProviderHelper = SolanaProviderScriptHandler()
    tab.walletSolProvider = provider

    let (result, error) = await solProviderHelper.signAllTransactions(tab: tab, args: args)
    XCTAssertNil(error)
    guard let result = result as? [[String: Any]] else {
      XCTFail("Unexpected result to signAllTransactions request")
      return
    }

    let serializedTxs = result.compactMap {
      ($0["serializedTx"] as? [NSNumber])?.compactMap({ Int32($0.intValue) })
    }
    XCTAssertEqual(serializedTxs, [kSerializedTx])
    let versions = result.compactMap { ($0["version"] as? Int) }
    XCTAssertEqual(versions, [BraveWallet.SolanaMessageVersion.legacy.rawValue])
  }

  /// Test `signAllTransactions()`, given a json string `[{serializedMessage: Buffer, signatures: {publicKey: String, signature: Buffer}}]`,
  /// the success flow will return a tuple `(result: Any?, error: String?)` with the result as type `String` containing a json 2-dimensional array of Uint8.
  func testSignAllTransactionsSuccessVersionedTxV0() async {
    let signature = MojoBase.Value(dictionaryValue: [
      "publicKey": MojoBase.Value(stringValue: kTestPublicKey)
    ])
    let serializedMessage = MojoBase.Value(
      listValue: kSerializedMessage.map { MojoBase.Value(intValue: $0) }
    )
    let transaction = MojoBase.Value(dictionaryValue: [
      "serializedMessage": serializedMessage,
      "signatures": MojoBase.Value(listValue: [signature]),
    ])
    guard let args = MojoBase.Value(listValue: [transaction]).jsonString else {
      XCTFail("Unexpected test setup")
      return
    }

    let provider: BraveWallet.TestSolanaProvider = .init()
    provider._signAllTransactions = { [kSerializedTx] _, completion in
      completion(
        .success,
        "",
        [kSerializedTx.map(NSNumber.init(value:))],
        [NSNumber(value: BraveWallet.SolanaMessageVersion.V0.rawValue)]
      )
    }
    let tab = Tab(configuration: .init())
    let solProviderHelper = SolanaProviderScriptHandler()
    tab.walletSolProvider = provider

    let (result, error) = await solProviderHelper.signAllTransactions(tab: tab, args: args)
    XCTAssertNil(error)
    guard let result = result as? [[String: Any]] else {
      XCTFail("Unexpected result to signAllTransactions request")
      return
    }

    let serializedTxs = result.compactMap {
      ($0["serializedTx"] as? [NSNumber])?.compactMap({ Int32($0.intValue) })
    }
    XCTAssertEqual(serializedTxs, [kSerializedTx])
    let versions = result.compactMap { ($0["version"] as? Int) }
    XCTAssertEqual(versions, [BraveWallet.SolanaMessageVersion.V0.rawValue])
  }

  private let kTestPublicKey = "test_base58_public_key"
  private let kTestSignature =
    "As4N6cok5f7nhXp56Hdw8dWZpUnY8zjYKzBqK45CexE1qNPCqt6Y2gnZduGgqASDD1c6QULBRy"
  private let kSerializedMessage: [Int32] = [
    1, 0, 1, 2, 161, 51, 89, 91, 115, 210, 217, 212, 76, 159, 171,
    200, 40, 150, 157, 70, 197, 71, 24, 44, 209, 108, 143, 4, 58, 251,
    215, 62, 201, 172, 159, 197, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 65, 223, 160, 84, 229, 200, 41,
    124, 255, 227, 200, 207, 94, 13, 128, 218, 71, 139, 178, 169, 91, 44,
    201, 177, 125, 166, 36, 96, 136, 125, 3, 136, 1, 1, 2, 0, 0,
    12, 2, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 0,
  ]

  private let kMessageToSign: [Int32] = [
    84, 111, 32, 97, 118, 111, 105, 100, 32, 100, 105, 103, 105, 116, 97,
    108, 32, 100, 111, 103, 110, 97, 112, 112, 101, 114, 115, 44, 32, 115,
    105, 103, 110, 32, 98, 101, 108, 111, 119, 32, 116, 111, 32, 97, 117,
    116, 104, 101, 110, 116, 105, 99, 97, 116, 101, 32, 119, 105, 116, 104,
    32, 67, 114, 121, 112, 116, 111, 67, 111, 114, 103, 105, 115, 46,
  ]

  private let kSerializedTx: [Int32] = [
    1, 224, 52, 14, 175, 211, 221, 245, 217, 123, 232, 68, 232, 120, 145,
    131, 154, 133, 31, 130, 73, 190, 13, 227, 109, 83, 152, 160, 202, 226,
    134, 138, 141, 135, 187, 72, 153, 173, 159, 205, 222, 253, 26, 44, 34,
    18, 250, 176, 21, 84, 7, 142, 247, 65, 218, 40, 117, 145, 118, 52,
    75, 183, 98, 232, 10, 1, 0, 1, 2, 161, 51, 89, 91, 115, 210,
    217, 212, 76, 159, 171, 200, 40, 150, 157, 70, 197, 71, 24, 44, 209,
    108, 143, 4, 58, 251, 215, 62, 201, 172, 159, 197, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 65, 223,
    160, 84, 229, 200, 41, 124, 255, 227, 200, 207, 94, 13, 128, 218, 71,
    139, 178, 169, 91, 44, 201, 177, 125, 166, 36, 96, 136, 125, 3, 136,
    1, 1, 2, 0, 0, 12, 2, 0, 0, 0, 100, 0, 0, 0, 0,
    0, 0, 0,
  ]
}
