// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import XCTest

@testable import BraveWallet

class WalletArrayExtensionTests: XCTestCase {
  func testOddLengthInput() {
    let inputHexData = "0xff573"
    let bytes = [NSNumber](hexString: inputHexData)
    let result: [NSNumber] = .init([15, 245, 115])
    XCTAssertNotNil(bytes)
    XCTAssertEqual(bytes, result)
  }

  func testEvenLengthInput() {
    let inputHexData = "0xff5733"
    let bytes = [NSNumber](hexString: inputHexData)
    let result: [NSNumber] = .init([255, 87, 51])
    XCTAssertNotNil(bytes)
    XCTAssertEqual(bytes, result)
  }

  func testNoHexPrefixInput() {
    let inputHexData = "ff5733"
    let bytes = [NSNumber](hexString: inputHexData)
    let result: [NSNumber] = .init([255, 87, 51])
    XCTAssertNotNil(bytes)
    XCTAssertEqual(bytes, result)
  }

  func testInvalidHexInput() {
    let inputHexData = "0x0csadgasg"
    let bytes = [NSNumber](hexString: inputHexData)
    XCTAssertNil(bytes)
  }

  func testEmptyHexInput() {
    let inputHexData = "0x"
    let bytes = [NSNumber](hexString: inputHexData)
    let result: [NSNumber] = .init()
    XCTAssertNotNil(bytes)
    XCTAssertEqual(bytes, result)
  }

  func testAssetViewModelArraySort() {
    let account1: BraveWallet.AccountInfo = .mockEthAccount
    let account2: BraveWallet.AccountInfo = .mockSolAccount
    let account3: BraveWallet.AccountInfo = .mockFilAccount
    let account4: BraveWallet.AccountInfo = .mockBtcAccount
    let testAccount1: BraveWallet.AccountInfo = .mockFilTestnetAccount
    let testAccount2: BraveWallet.AccountInfo = .mockBtcTestnetAccount

    // ETH value $1000
    var viewModel1: AssetViewModel = .init(
      groupType: .none,
      token: .previewToken,
      network: .mockMainnet,
      price: "1000",
      history: [],
      balanceForAccounts: [account1.address: 1]
    )
    // USDC value $500
    var viewModel2: AssetViewModel = .init(
      groupType: .none,
      token: .mockUSDCToken,
      network: .mockMainnet,
      price: "500",
      history: [],
      balanceForAccounts: [account1.address: 1]
    )
    // SOL value $500
    var viewModel3: AssetViewModel = .init(
      groupType: .none,
      token: .mockSolToken,
      network: .mockSolana,
      price: "500",
      history: [],
      balanceForAccounts: [account2.address: 1]
    )
    // FIL value $100 on mainnet
    var viewModel4: AssetViewModel = .init(
      groupType: .none,
      token: .mockFilToken,
      network: .mockFilecoinMainnet,
      price: "100",
      history: [],
      balanceForAccounts: [account3.address: 1]
    )
    // BTC value $20000 on mainnet
    var viewModel5: AssetViewModel = .init(
      groupType: .none,
      token: .mockBTCToken,
      network: .mockBitcoinMainnet,
      price: "20000",
      history: [],
      balanceForAccounts: [account4.address: 1]
    )
    // FIL value $100 on testnet
    var viewModel6: AssetViewModel = .init(
      groupType: .none,
      token: .mockFilToken,
      network: .mockFilecoinTestnet,
      price: "50",
      history: [],
      balanceForAccounts: [testAccount1.address: 2]
    )
    // BTC value $40000 on testnet
    var viewModel7: AssetViewModel = .init(
      groupType: .none,
      token: .mockBTCToken,
      network: .mockBitcoinTestnet,
      price: "20000",
      history: [],
      balanceForAccounts: [testAccount2.address: 2]
    )
    let array: [AssetViewModel] = [
      viewModel1, viewModel2,
      viewModel3, viewModel4,
      viewModel5, viewModel6,
      viewModel7,
    ]
    let sortedArray = array.sorted {
      AssetViewModel.sorted(lhs: $0, rhs: $1)
    }
    XCTAssertEqual(sortedArray[safe: 0], viewModel7)
    XCTAssertEqual(sortedArray[safe: 1], viewModel5)
    XCTAssertEqual(sortedArray[safe: 2], viewModel1)
    XCTAssertEqual(sortedArray[safe: 3], viewModel2)
    XCTAssertEqual(sortedArray[safe: 4], viewModel3)
    XCTAssertEqual(sortedArray[safe: 5], viewModel4)
    XCTAssertEqual(sortedArray[safe: 6], viewModel6)
  }

  func testAssetGroupViewModelArraySort() {
    // no sorting for group type .none

    // sort group type .account
    let account1: BraveWallet.AccountInfo = .mockEthAccount
    let account2: BraveWallet.AccountInfo = .mockSolAccount
    let account3: BraveWallet.AccountInfo = .mockFilAccount
    let account4: BraveWallet.AccountInfo = .mockBtcAccount
    let testAccount1: BraveWallet.AccountInfo = .mockFilTestnetAccount
    let testAccount2: BraveWallet.AccountInfo = .mockBtcTestnetAccount
    // ETH value $1000
    var viewModel1: AssetViewModel = .init(
      groupType: .account(account1),
      token: .previewToken,
      network: .mockMainnet,
      price: "1000",
      history: [],
      balanceForAccounts: [account1.address: 1]
    )
    // USDC value $500
    var viewModel2: AssetViewModel = .init(
      groupType: .account(account1),
      token: .mockUSDCToken,
      network: .mockMainnet,
      price: "500",
      history: [],
      balanceForAccounts: [account1.address: 1]
    )
    // SOL value $100
    var viewModel3: AssetViewModel = .init(
      groupType: .account(account2),
      token: .mockSolToken,
      network: .mockSolana,
      price: "100",
      history: [],
      balanceForAccounts: [account2.address: 1]
    )
    // FIL value $100 on mainnet
    var viewModel4: AssetViewModel = .init(
      groupType: .account(account3),
      token: .mockFilToken,
      network: .mockFilecoinMainnet,
      price: "100",
      history: [],
      balanceForAccounts: [account3.address: 1]
    )
    // BTC value $20000 on mainnet
    var viewModel5: AssetViewModel = .init(
      groupType: .account(account4),
      token: .mockBTCToken,
      network: .mockBitcoinMainnet,
      price: "20000",
      history: [],
      balanceForAccounts: [account4.address: 1]
    )
    // FIL value $100 on testnet
    var viewModel6: AssetViewModel = .init(
      groupType: .account(testAccount1),
      token: .mockFilToken,
      network: .mockFilecoinTestnet,
      price: "50",
      history: [],
      balanceForAccounts: [testAccount1.address: 2]
    )
    // BTC value $100 on testnet
    var viewModel7: AssetViewModel = .init(
      groupType: .account(testAccount2),
      token: .mockBTCToken,
      network: .mockBitcoinTestnet,
      price: "20000",
      history: [],
      balanceForAccounts: [testAccount2.address: 0.005]
    )
    var group1: AssetGroupViewModel = .init(
      groupType: .account(account1),
      assets: [viewModel1, viewModel2]
    )
    var group2: AssetGroupViewModel = .init(
      groupType: .account(account2),
      assets: [viewModel3]
    )
    var group3: AssetGroupViewModel = .init(
      groupType: .account(account3),
      assets: [viewModel4]
    )
    var group4: AssetGroupViewModel = .init(
      groupType: .account(account4),
      assets: [viewModel5]
    )
    var group5: AssetGroupViewModel = .init(
      groupType: .account(testAccount1),
      assets: [viewModel6]
    )
    var group6: AssetGroupViewModel = .init(
      groupType: .account(testAccount2),
      assets: [viewModel7]
    )

    var array: [AssetGroupViewModel] = [
      group1, group2,
      group3, group4,
      group5, group6,
    ]
    var sortedArray = array.sorted {
      AssetGroupViewModel.sorted(lhs: $0, rhs: $1)
    }
    XCTAssertEqual(sortedArray[safe: 0], group4)
    XCTAssertEqual(sortedArray[safe: 1], group1)
    XCTAssertEqual(sortedArray[safe: 2], group2)
    XCTAssertEqual(sortedArray[safe: 3], group6)
    XCTAssertEqual(sortedArray[safe: 4], group3)
    XCTAssertEqual(sortedArray[safe: 5], group5)

    // ETH value $1000
    viewModel1 = .init(
      groupType: .network(.mockMainnet),
      token: .previewToken,
      network: .mockMainnet,
      price: "1000",
      history: [],
      balanceForAccounts: [account1.address: 1]
    )
    // USDC value $500
    viewModel2 = .init(
      groupType: .network(.mockMainnet),
      token: .mockUSDCToken,
      network: .mockMainnet,
      price: "500",
      history: [],
      balanceForAccounts: [account1.address: 1]
    )
    // SOL value $100
    viewModel3 = .init(
      groupType: .network(.mockSolana),
      token: .mockSolToken,
      network: .mockSolana,
      price: "100",
      history: [],
      balanceForAccounts: [account2.address: 1]
    )
    // FIL value $100 on mainnet
    viewModel4 = .init(
      groupType: .network(.mockFilecoinMainnet),
      token: .mockFilToken,
      network: .mockFilecoinMainnet,
      price: "100",
      history: [],
      balanceForAccounts: [account3.address: 1]
    )
    // BTC value $20000 on mainnet
    viewModel5 = .init(
      groupType: .network(.mockBitcoinMainnet),
      token: .mockBTCToken,
      network: .mockBitcoinMainnet,
      price: "20000",
      history: [],
      balanceForAccounts: [account4.address: 1]
    )
    // FIL value $100 on testnet
    viewModel6 = .init(
      groupType: .network(.mockFilecoinTestnet),
      token: .mockFilToken,
      network: .mockFilecoinTestnet,
      price: "50",
      history: [],
      balanceForAccounts: [testAccount1.address: 2]
    )
    // BTC value $100 on testnet
    viewModel7 = .init(
      groupType: .network(.mockBitcoinTestnet),
      token: .mockBTCToken,
      network: .mockBitcoinTestnet,
      price: "20000",
      history: [],
      balanceForAccounts: [testAccount2.address: 0.005]
    )
    group1 = .init(
      groupType: .network(.mockMainnet),
      assets: [viewModel1, viewModel2]
    )
    group2 = .init(
      groupType: .network(.mockSolana),
      assets: [viewModel3]
    )
    group3 = .init(
      groupType: .network(.mockFilecoinMainnet),
      assets: [viewModel4]
    )
    group4 = .init(
      groupType: .network(.mockBitcoinMainnet),
      assets: [viewModel5]
    )
    group5 = .init(
      groupType: .network(.mockFilecoinTestnet),
      assets: [viewModel6]
    )
    group6 = .init(
      groupType: .network(.mockBitcoinTestnet),
      assets: [viewModel7]
    )

    array = [
      group1, group2,
      group3, group4,
      group5, group6,
    ]
    sortedArray = array.sorted {
      AssetGroupViewModel.sorted(lhs: $0, rhs: $1)
    }
    XCTAssertEqual(sortedArray[safe: 0], group4)
    XCTAssertEqual(sortedArray[safe: 1], group1)
    XCTAssertEqual(sortedArray[safe: 2], group2)
    XCTAssertEqual(sortedArray[safe: 3], group3)
    XCTAssertEqual(sortedArray[safe: 4], group6)
    XCTAssertEqual(sortedArray[safe: 5], group5)
  }
}
