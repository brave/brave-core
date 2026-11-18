// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Combine
import Foundation
import XCTest

@testable import BraveWallet

@MainActor class DecentralizedDNSHelperTests: XCTestCase {

  /// ENS content hash which resolves to
  /// `ipfs://bafybeihqoo7bq7uoaybzpfwegks33vw2h5adyl4t7joz3pofkr6h7yhdxq`
  private static let mockIPFSContentHash: [NSNumber] = {
    let hex = "e30101701220f073be187e8e06039796c432a5bdd6da3f403c2f93fa5d9dbdc5547c7fe0e3bc"
    var bytes: [NSNumber] = []
    var index = hex.startIndex
    while index < hex.endIndex {
      let next = hex.index(index, offsetBy: 2)
      bytes.append(NSNumber(value: UInt8(hex[index..<next], radix: 16)!))
      index = next
    }
    return bytes
  }()

  /// Test `.init(rpcService:isPrivateMode:)` will return nil when `isPrivateMode` is true.
  func testInitFailsInPrivateMode() {
    XCTAssertNil(
      DecentralizedDNSHelper(
        rpcService: MockJsonRpcService(),
        isPrivateMode: true
      )
    )
  }

  /// Test `lookup(domain:)` provided an `.eth` domain will return `.loadInterstitial(.ethereum)` when ENS Resolve Method is `.ask`.
  func testLookupWithENSDomainAsk() async {
    let domain = "braveexample.eth"

    let rpcService = MockJsonRpcService()
    rpcService._ensResolveMethod = { $0(.ask) }
    rpcService._ensGetContentHash = { _, completion in
      completion([], false, .success, "")
    }

    guard
      let sut = DecentralizedDNSHelper(
        rpcService: rpcService,
        isPrivateMode: false
      )
    else {
      XCTFail("Unexpected test setup")
      return
    }

    let result = await sut.lookup(domain: domain)
    guard case .loadInterstitial(let service) = result else {
      XCTFail("Expected to load interstitial")
      return
    }
    XCTAssertEqual(service, .ethereum)
  }

  /// Test `lookup(domain:)` provided an `.eth` domain will return `.load(URL)` when ENS Resolve Method is `.enabled`.
  func testLookupWithENSDomainEnabled() async {
    let domain = "braveexample.eth"
    let resolvedURL = URL(
      string: "ipfs://bafybeihqoo7bq7uoaybzpfwegks33vw2h5adyl4t7joz3pofkr6h7yhdxq"
    )!

    let rpcService = MockJsonRpcService()
    rpcService._ensResolveMethod = { $0(.enabled) }
    rpcService._ensGetContentHash = { _, completion in
      let contentHash = Self.mockIPFSContentHash
      completion(contentHash, false, .success, "")
    }

    guard
      let sut = DecentralizedDNSHelper(
        rpcService: rpcService,
        isPrivateMode: false
      )
    else {
      XCTFail("Unexpected test setup")
      return
    }

    let result = await sut.lookup(domain: domain)
    guard case .load(let url) = result else {
      XCTFail("Expected to load url")
      return
    }
    XCTAssertEqual(url, resolvedURL)
  }

  /// Test `lookup(domain:)` provided an `.eth` domain will return `.none` when ENS Resolve Method is `.disabled`.
  func testLookupWithENSDomainDisabled() async {
    let domain = "braveexample.eth"

    let rpcService = MockJsonRpcService()
    rpcService._ensResolveMethod = { $0(.disabled) }
    rpcService._ensGetContentHash = { _, completion in
      completion([], false, .success, "")
    }

    guard
      let sut = DecentralizedDNSHelper(
        rpcService: rpcService,
        isPrivateMode: false
      )
    else {
      XCTFail("Unexpected test setup")
      return
    }

    let result = await sut.lookup(domain: domain)
    XCTAssertEqual(result, .none)
  }

  /// Test `lookup(domain:)` provided an `.eth` domain will return `.loadInterstitial(.ethereumOffchain)` when ENS Resolve Method is `.enabled` & ENS Offchain Resolve Method is `.ask`.
  func testLookupWithENSOffchainDomainAsk() async {
    let domain = "braveoffchainexample.eth"

    let rpcService = MockJsonRpcService()
    rpcService._ensResolveMethod = { $0(.enabled) }
    rpcService._ensOffchainLookupResolveMethod = { $0(.ask) }
    rpcService._ensGetContentHash = { _, completion in
      completion([], true, .success, "")
    }

    guard
      let sut = DecentralizedDNSHelper(
        rpcService: rpcService,
        isPrivateMode: false
      )
    else {
      XCTFail("Unexpected test setup")
      return
    }

    let result = await sut.lookup(domain: domain)
    guard case .loadInterstitial(let service) = result else {
      XCTFail("Expected to load interstitial")
      return
    }
    XCTAssertEqual(service, .ethereumOffchain)

  }

  /// Test `lookup(domain:)` provided an `.eth` domain will return `.load(URL)` when ENS Resolve Method is `.enabled` & ENS Offchain Resolve Method is `.enabled`.
  func testLookupWithENSOffchainDomainEnabled() async {
    let domain = "braveoffchainexample.eth"
    let resolvedURL = URL(
      string: "ipfs://bafybeihqoo7bq7uoaybzpfwegks33vw2h5adyl4t7joz3pofkr6h7yhdxq"
    )!

    let rpcService = MockJsonRpcService()
    rpcService._ensResolveMethod = { $0(.enabled) }
    rpcService._ensOffchainLookupResolveMethod = { $0(.enabled) }
    rpcService._ensGetContentHash = { _, completion in
      let contentHash = Self.mockIPFSContentHash
      completion(contentHash, false, .success, "")
    }

    guard
      let sut = DecentralizedDNSHelper(
        rpcService: rpcService,
        isPrivateMode: false
      )
    else {
      XCTFail("Unexpected test setup")
      return
    }

    let result = await sut.lookup(domain: domain)
    guard case .load(let url) = result else {
      XCTFail("Expected to load url")
      return
    }
    XCTAssertEqual(url, resolvedURL)
  }

  /// Test `lookup(domain:)` provided an `.eth` domain will return `.none` when ENS Resolve Method is `.enabled` & ENS Offchain Resolve Method is `.disabled`.
  func testLookupWithENSOffchainDomainDisabled() async {
    let domain = "braveoffchainexample.eth"

    let rpcService = MockJsonRpcService()
    rpcService._ensResolveMethod = { $0(.enabled) }
    rpcService._ensOffchainLookupResolveMethod = { $0(.disabled) }
    rpcService._ensGetContentHash = { _, completion in
      completion([], false, .internalError, "")
    }

    guard
      let sut = DecentralizedDNSHelper(
        rpcService: rpcService,
        isPrivateMode: false
      )
    else {
      XCTFail("Unexpected test setup")
      return
    }

    let result = await sut.lookup(domain: domain)
    XCTAssertEqual(result, .none)
  }

  /// Test `lookup(domain:)` provided a `.sol` domain  will return `.loadInterstitial(.solana)` when SNS Resolve Method is `.ask`.
  func testLookupWithSNSDomainAsk() async {
    let domain = "braveexample.sol"

    let rpcService = MockJsonRpcService()
    rpcService._snsResolveMethod = { $0(.ask) }
    rpcService._snsResolveHost = { _, completion in
      completion(nil)
    }

    guard
      let sut = DecentralizedDNSHelper(
        rpcService: rpcService,
        isPrivateMode: false
      )
    else {
      XCTFail("Unexpected test setup")
      return
    }

    let result = await sut.lookup(domain: domain)
    guard case .loadInterstitial(let service) = result else {
      XCTFail("Expected to load interstitial")
      return
    }
    XCTAssertEqual(service, .solana)
  }

  /// Test `lookup(domain:)` provided a `.sol` domain  will return `.loadURL(URL)` when SNS Resolve Method is `.enabled`.
  func testLookupWithSNSDomainEnabled() async {
    let domain = "braveexample.sol"
    let resolvedURL = URL(string: "https://brave.com")!

    let rpcService = MockJsonRpcService()
    rpcService._snsResolveMethod = { $0(.enabled) }
    rpcService._snsResolveHost = { _, completion in
      completion(resolvedURL)
    }

    guard
      let sut = DecentralizedDNSHelper(
        rpcService: rpcService,
        isPrivateMode: false
      )
    else {
      XCTFail("Unexpected test setup")
      return
    }

    let result = await sut.lookup(domain: domain)
    guard case .load(let url) = result else {
      XCTFail("Expected to load url")
      return
    }
    XCTAssertEqual(url, resolvedURL)
  }

  /// Test `lookup(domain:)` provided a `.sol` domain  will return `.none` when SNS Resolve Method is `.disabled`.
  func testLookupWithSNSDomainDisabled() async {
    let domain = "braveexample.sol"

    let rpcService = MockJsonRpcService()
    rpcService._snsResolveMethod = { $0(.disabled) }
    rpcService._snsResolveHost = { _, completion in
      completion(nil)
    }

    guard
      let sut = DecentralizedDNSHelper(
        rpcService: rpcService,
        isPrivateMode: false
      )
    else {
      XCTFail("Unexpected test setup")
      return
    }

    let result = await sut.lookup(domain: domain)
    XCTAssertEqual(result, .none)
  }

  /// Test `lookup(domain:)` provided a `.crypto` domain  will return `.loadInterstitial(.solana)` when Unstoppable Domains Resolve Method is `.ask`.
  func testLookupWithUnstoppableDomainsDomainAsk() async {
    let domain = "braveexample.crypto"

    let rpcService = MockJsonRpcService()
    rpcService._unstoppableDomainsResolveMethod = { $0(.ask) }
    rpcService._unstoppableDomainsResolveDns = { _, completion in
      completion(nil, .internalError, "")
    }

    guard
      let sut = DecentralizedDNSHelper(
        rpcService: rpcService,
        isPrivateMode: false
      )
    else {
      XCTFail("Unexpected test setup")
      return
    }

    let result = await sut.lookup(domain: domain)
    guard case .loadInterstitial(let service) = result else {
      XCTFail("Expected to load interstitial")
      return
    }
    XCTAssertEqual(service, .unstoppable)
  }

  /// Test `lookup(domain:)` provided a `.crypto` domain  will return `.loadURL(URL)` when Unstoppable Domains Resolve Method is `.enabled`.
  func testLookupWithUnstoppableDomainsDomainEnabled() async {
    let domain = "braveexample.crypto"
    let resolvedURL = URL(string: "https://brave.com")!

    let rpcService = MockJsonRpcService()
    rpcService._unstoppableDomainsResolveMethod = { $0(.enabled) }
    rpcService._unstoppableDomainsResolveDns = { _, completion in
      completion(resolvedURL, .success, "")
    }

    guard
      let sut = DecentralizedDNSHelper(
        rpcService: rpcService,
        isPrivateMode: false
      )
    else {
      XCTFail("Unexpected test setup")
      return
    }

    let result = await sut.lookup(domain: domain)
    guard case .load(let url) = result else {
      XCTFail("Expected to load url")
      return
    }
    XCTAssertEqual(url, resolvedURL)
  }

  /// Test `lookup(domain:)` provided a `.crypto` domain  will return `.none` when Unstoppable Domains Resolve Method is `.disabled`.
  func testLookupWithUnstoppableDomainsDomainDisabled() async {
    let domain = "braveexample.crypto"

    let rpcService = MockJsonRpcService()
    rpcService._unstoppableDomainsResolveMethod = { $0(.disabled) }
    rpcService._unstoppableDomainsResolveDns = { _, completion in
      completion(nil, .internalError, "")
    }

    guard
      let sut = DecentralizedDNSHelper(
        rpcService: rpcService,
        isPrivateMode: false
      )
    else {
      XCTFail("Unexpected test setup")
      return
    }

    let result = await sut.lookup(domain: domain)
    XCTAssertEqual(result, .none)
  }

  func testIsSupported() {
    let ensDomain = "ethereum.eth"
    XCTAssertTrue(DecentralizedDNSHelper.isSupported(domain: ensDomain))
    let nonSupportedENSDomain = "ethereumeth"
    XCTAssertFalse(DecentralizedDNSHelper.isSupported(domain: nonSupportedENSDomain))

    let snsDomain = "braveexample.sol"
    XCTAssertTrue(DecentralizedDNSHelper.isSupported(domain: snsDomain))
    let nonSupportedSNSDomain = "braveexamplesol"
    XCTAssertFalse(DecentralizedDNSHelper.isSupported(domain: nonSupportedSNSDomain))

    let unstoppableDomain = "braveexample"
    XCTAssertFalse(DecentralizedDNSHelper.isSupported(domain: unstoppableDomain))
    for tld in WalletConstants.supportedENSExtensions {
      let domainWithTld = unstoppableDomain + tld
      XCTAssertTrue(DecentralizedDNSHelper.isSupported(domain: domainWithTld))
    }
    let nonSupportedUnstoppableDomain = "braveexamplecrypto"
    XCTAssertFalse(DecentralizedDNSHelper.isSupported(domain: nonSupportedUnstoppableDomain))
  }
}
