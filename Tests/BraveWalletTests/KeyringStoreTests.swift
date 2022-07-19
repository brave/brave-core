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
  
  override func setUp() {
    super.setUp()
    WalletDebugFlags.isSolanaEnabled = true
  }
  
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
    keyringService._setSelectedAccount = { $2(true) }
    
    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._addObserver = { _ in }
    rpcService._chainId = { $1(currentChainId) }
    rpcService._network = { $1(currentNetwork) }
    
    rpcService._setNetwork = { _, _, completion in
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
}
