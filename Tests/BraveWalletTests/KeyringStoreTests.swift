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
    let currentSelectedAccount: BraveWallet.AccountInfo = .mockEthAccount
    
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._keyringInfo = { keyringId, completion in
      switch keyringId {
      case BraveWallet.KeyringId.default:
        completion(.mockDefaultKeyringInfo)
      case BraveWallet.KeyringId.solana:
        completion(.mockSolanaKeyringInfo)
      case BraveWallet.KeyringId.filecoin:
        completion(.mockFilecoinKeyringInfo)
      case BraveWallet.KeyringId.filecoinTestnet:
        completion(.mockFilecoinTestnetKeyringInfo)
      default:
        completion(.init())
      }
    }
    keyringService._addObserver = { _ in }
    keyringService._isLocked = { $0(false) }
    keyringService._allAccounts = { completion in
      completion(.init(
        accounts: [currentSelectedAccount],
        selectedAccount: currentSelectedAccount,
        ethDappSelectedAccount: [currentSelectedAccount].first(where: { $0.coin == .eth }),
        solDappSelectedAccount: [currentSelectedAccount].first(where: { $0.coin == .sol })
      ))
    }
    keyringService._setSelectedAccount = { $1(true) }
    
    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._addObserver = { _ in }
    rpcService._chainIdForOrigin = { $2(currentChainId) }
    rpcService._network = { $2(currentNetwork) }
    
    rpcService._setNetwork = { _, _, _, completion in
      completion(true)
    }
    
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._addObserver = { _ in }
    
    return (keyringService, rpcService, walletService)
  }
  
  func testAllKeyrings() {
    let (keyringService, rpcService, walletService) = setupServices()
    
    let store = KeyringStore(
      keyringService: keyringService,
      walletService: walletService,
      rpcService: rpcService
    )
    
    let expectedKeyrings = [BraveWallet.KeyringInfo.mockDefaultKeyringInfo, BraveWallet.KeyringInfo.mockSolanaKeyringInfo, BraveWallet.KeyringInfo.mockFilecoinKeyringInfo, BraveWallet.KeyringInfo.mockFilecoinTestnetKeyringInfo]
    
    let allTokensExpectation = expectation(description: "allKeyrings")
    store.$allKeyrings
      .dropFirst()
      .sink { allKeyrings in
        defer { allTokensExpectation.fulfill() }
        XCTAssertEqual(allKeyrings.count, 4)
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
    
    let emptyPassword1 = ""
    var passwordStatus = await store.validatePassword(emptyPassword1)
    XCTAssertEqual(passwordStatus, .none)
    
    let invalidPassword2 = "1234"
    passwordStatus = await store.validatePassword(invalidPassword2)
    XCTAssertEqual(passwordStatus, .invalid)
    
    let validWeakPassword = "12345678"
    passwordStatus = await store.validatePassword(validWeakPassword)
    XCTAssertEqual(passwordStatus, .weak)
    
    let uuid = UUID().uuidString
    // first 30 characters of uuid
    let validMediumPassword = String(uuid[uuid.startIndex..<uuid.index(uuid.startIndex, offsetBy: 15)])
    passwordStatus = await store.validatePassword(validMediumPassword)
    XCTAssertEqual(passwordStatus, .medium)
    
    let strongPassword = "LDKH66BJbLsHQPEAK@4_zak*"
    passwordStatus = await store.validatePassword(strongPassword)
    XCTAssertEqual(passwordStatus, .strong)
  }
}
