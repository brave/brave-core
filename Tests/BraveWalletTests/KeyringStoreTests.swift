// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import Combine
import BraveCore
@testable import BraveWallet

class KeyringStoreTests: XCTestCase {
  
  private var cancellables: Set<AnyCancellable> = .init()
  
  private func setupServices() -> (BraveWallet.TestKeyringService, BraveWallet.TestJsonRpcService, BraveWallet.TestBraveWalletService) {
    let currentNetwork: BraveWallet.NetworkInfo = .mockMainnet
    let currentChainId = currentNetwork.chainId
    let currentSelectedCoin: BraveWallet.CoinType = .eth
    let currentSelectedAccount: BraveWallet.AccountInfo = .mockEthAccount
    
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._keyringInfo = { keyringId, completion in
      switch keyringId {
      case BraveWallet.DefaultKeyringId:
        completion(.mockDefaultKeyringInfo)
      case BraveWallet.SolanaKeyringId:
        completion(.mockSolanaKeyringInfo)
      case BraveWallet.FilecoinKeyringId:
        completion(.mockFilecoinKeyringInfo)
      default:
        completion(.init())
      }
    }
    keyringService._addObserver = { _ in }
    keyringService._isLocked = { $0(false) }
    keyringService._selectedAccount = { $1(currentSelectedAccount.address) }
    keyringService._setSelectedAccount = { $3(true) }
    
    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._addObserver = { _ in }
    rpcService._chainIdForOrigin = { $2(currentChainId) }
    rpcService._network = { $2(currentNetwork) }
    
    rpcService._setNetwork = { _, _, _, completion in
      completion(true)
    }
    
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._addObserver = { _ in }
    walletService._selectedCoin = { $0(currentSelectedCoin) }
    
    return (keyringService, rpcService, walletService)
  }
  
  func testAllKeyrings() {
    let (keyringService, rpcService, walletService) = setupServices()
    
    let store = KeyringStore(
      keyringService: keyringService,
      walletService: walletService,
      rpcService: rpcService
    )
    
    let expectedKeyrings = [BraveWallet.KeyringInfo.mockDefaultKeyringInfo, BraveWallet.KeyringInfo.mockSolanaKeyringInfo]
    
    let allTokensExpectation = expectation(description: "allKeyrings")
    store.$allKeyrings
      .dropFirst()
      .sink { allKeyrings in
        defer { allTokensExpectation.fulfill() }
        XCTAssertEqual(allKeyrings.count, 2)
        for keyring in allKeyrings {
          XCTAssertTrue(expectedKeyrings.contains(where: { $0.id == keyring.id }))
        }
      }
      .store(in: &cancellables)
    
    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }
  
  func testInitialSelectedAccount() {
    let (keyringService, rpcService, walletService) = setupServices()
    
    let store = KeyringStore(
      keyringService: keyringService,
      walletService: walletService,
      rpcService: rpcService
    )
    
    let initalSelectedAccountExpectation = expectation(description: "initalSelectedAccount")
    store.$selectedAccount
      .dropFirst()
      .collect(1)
      .sink { receivedValues in
        defer { initalSelectedAccountExpectation.fulfill() }
        guard let account = receivedValues.last else {
          XCTFail("account not updated")
          return
        }
        XCTAssertEqual(account.address, BraveWallet.AccountInfo.mockEthAccount.address)
      }
      .store(in: &cancellables)
    
    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }
  
  @MainActor func testIsStrongPassword() async {
    let (keyringService, rpcService, walletService) = setupServices()
    let store = KeyringStore(
      keyringService: keyringService,
      walletService: walletService,
      rpcService: rpcService
    )
    
    let invalidPassword1 = ""
    var isStrongPassword = await store.isStrongPassword(invalidPassword1)
    XCTAssertFalse(isStrongPassword)
    
    let invalidPassword2 = "1234"
    isStrongPassword = await store.isStrongPassword(invalidPassword2)
    XCTAssertFalse(isStrongPassword)
    
    let validPassword = "12345678"
    isStrongPassword = await store.isStrongPassword(validPassword)
    XCTAssertTrue(isStrongPassword)
    
    let uuid = UUID().uuidString
    // first 30 characters of uuid
    let validPassword2 = String(uuid[uuid.startIndex..<uuid.index(uuid.startIndex, offsetBy: 30)])
    isStrongPassword = await store.isStrongPassword(validPassword2)
    XCTAssertTrue(isStrongPassword)
    
    let strongPassword = "LDKH66BJbLsHQPEAK@4_zak*"
    isStrongPassword = await store.isStrongPassword(strongPassword)
    XCTAssertTrue(isStrongPassword)
  }
}
