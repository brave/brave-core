// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import CoreData
import Foundation
import TestHelpers
import XCTest

@testable import Data

class WalletUserAssetBalanceTests: CoreDataTestCase {

  let asset = BraveWallet.BlockchainToken(
    contractAddress: "0x123",
    name: "mockAsset",
    logo: "",
    isCompressed: false,
    isErc20: true,
    isErc721: false,
    isErc1155: false,
    splTokenProgram: .unsupported,
    isNft: false,
    isSpam: false,
    symbol: "MA",
    decimals: 18,
    visible: true,
    tokenId: "asset",
    coingeckoId: "",
    chainId: "0x1",
    coin: .eth,
    isShielded: false
  )
  let asset2 = BraveWallet.BlockchainToken(
    contractAddress: "0x123",
    name: "mockAsset2",
    logo: "",
    isCompressed: false,
    isErc20: false,
    isErc721: false,
    isErc1155: false,
    splTokenProgram: .unknown,
    isNft: false,
    isSpam: false,
    symbol: "MA2",
    decimals: 16,
    visible: false,
    tokenId: "asset2",
    coingeckoId: "",
    chainId: "0x67",
    coin: .sol,
    isShielded: false
  )
  let account1 = "0x35DCec532e809A3dAa04ED3Fd949495f7BAc9090"
  let account2 = "0x84D4937cd23753FB71310438b7C2e4Ade45b3896"
  let account3 = "Be6iLdEVKj4bqSMKseMs8qdzBWudLFkebsEVGRmgabcd"
  let mockSolNetwork = BraveWallet.NetworkInfo(
    chainId: "0x67",
    chainName: "Solana Test Network",
    blockExplorerUrls: [],
    iconUrls: [],
    activeRpcEndpointIndex: 0,
    rpcEndpoints: [],
    symbol: "SOL",
    symbolName: "SOL",
    decimals: 9,
    coin: .sol,
    supportedKeyrings: []
  )

  let fetchRequest = NSFetchRequest<WalletUserAssetBalance>(
    entityName: String(describing: WalletUserAssetBalance.self)
  )

  // MARK: - Get Balance

  func testGetBalance() {
    createAndWait(asset: asset, balance: "0.1234", account: account1)
    createAndWait(asset: asset, balance: "0.8766", account: account2)
    createAndWait(asset: asset2, balance: "5.6789", account: account3)

    DataController.viewContext.refreshAllObjects()
    let assetBalance = WalletUserAssetBalance.getBalances(for: asset, account: account1)
    let asset2Balance = WalletUserAssetBalance.getBalances(for: asset2, account: account3)
    let assetBalances = WalletUserAssetBalance.getBalances(for: asset)
    let allAssetBalance = assetBalances?.reduce(
      0.0,
      { partialResult, assetBalance in
        partialResult + (Double(assetBalance.balance) ?? 0.0)
      }
    )
    XCTAssertEqual(assetBalance?.count, 1)
    XCTAssertEqual(assetBalance!.first!.balance, "0.1234")
    XCTAssertEqual(assetBalance!.first!.accountAddress, account1)
    XCTAssertEqual(asset2Balance?.count, 1)
    XCTAssertEqual(asset2Balance!.first!.balance, "5.6789")
    XCTAssertEqual(asset2Balance!.first!.accountAddress, account3)
    XCTAssertEqual(assetBalances?.count, 2)
    XCTAssertEqual(allAssetBalance, 1)  // 0.1234 + 0.8766 == 1
  }

  // MARK: - Delete

  func testRemoveAllAssetBalance() {
    createAndWait(asset: asset, balance: "0.1234", account: account1)
    createAndWait(asset: asset, balance: "0.8766", account: account2)
    XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 2)

    backgroundSaveAndWaitForExpectation {
      Task {
        await WalletUserAssetBalance.removeBalances(
          for: asset
        )
      }
    }

    XCTAssertEqual(try! DataController.viewContext.count(for: self.fetchRequest), 0)
  }

  func testRemoveAssetBalanceByAccount() {
    createAndWait(asset: asset, balance: "0.1234", account: account1)
    createAndWait(asset: asset, balance: "0.8766", account: account2)
    XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 2)

    backgroundSaveAndWaitForExpectation {
      Task {
        await WalletUserAssetBalance.removeBalances(for: asset, account: account1)
      }
    }

    XCTAssertEqual(try! DataController.viewContext.count(for: self.fetchRequest), 1)
  }

  func testRemoveAssetBalanceByNetwork() {
    createAndWait(asset: asset, balance: "0.1234", account: account1)
    createAndWait(asset: asset, balance: "0.8766", account: account2)
    createAndWait(asset: asset2, balance: "5.6789", account: account3)
    XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 3)

    backgroundSaveAndWaitForExpectation {
      Task {
        await WalletUserAssetBalance.removeBalances(for: mockSolNetwork)
      }
    }

    XCTAssertEqual(try! DataController.viewContext.count(for: self.fetchRequest), 2)
  }

  // MARK: - Update
  func testUpdateAssetBalance() {
    createAndWait(asset: asset, balance: "0.1234", account: account1)
    createAndWait(asset: asset, balance: "0.8766", account: account2)
    createAndWait(asset: asset2, balance: "5.6789", account: account3)
    XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 3)

    backgroundSaveAndWaitForExpectation {
      Task {
        await WalletUserAssetBalance.updateBalance(for: asset, balance: "9.8765", account: account1)
      }
    }

    DataController.viewContext.refreshAllObjects()
    let assetBalance1 = WalletUserAssetBalance.getBalances(for: asset, account: account1)
    XCTAssertEqual(assetBalance1?.count, 1)
    XCTAssertEqual(assetBalance1!.first!.balance, "9.8765")
    let assetBalance2 = WalletUserAssetBalance.getBalances(for: asset, account: account2)
    XCTAssertEqual(assetBalance2?.count, 1)
    XCTAssertEqual(assetBalance2!.first!.balance, "0.8766")
    let assetBalance3 = WalletUserAssetBalance.getBalances(for: asset2, account: account3)
    XCTAssertEqual(assetBalance3?.count, 1)
    XCTAssertEqual(assetBalance3!.first!.balance, "5.6789")
  }

  // MARK: - Utility

  @discardableResult
  private func createAndWait(
    asset: BraveWallet.BlockchainToken,
    balance: String,
    account: String
  ) -> WalletUserAssetBalance {
    backgroundSaveAndWaitForExpectation {
      Task {
        await WalletUserAssetBalance.updateBalance(for: asset, balance: balance, account: account)
      }
    }
    let request = try! DataController.viewContext.fetch(fetchRequest)
    return request.first!
  }
}
