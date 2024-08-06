// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import CoreData
import Foundation
import TestHelpers
import XCTest

@testable import Data

class WalletUserAssetTests: CoreDataTestCase {

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
    decimals: 16,
    visible: true,
    tokenId: "",
    coingeckoId: "",
    chainId: "0x1",
    coin: .eth
  )
  let asset2 = BraveWallet.BlockchainToken(
    contractAddress: "0x123",
    name: "mockAsset2",
    logo: "",
    isCompressed: false,
    isErc20: true,
    isErc721: false,
    isErc1155: false,
    splTokenProgram: .unsupported,
    isNft: false,
    isSpam: false,
    symbol: "MA",
    decimals: 16,
    visible: false,
    tokenId: "",
    coingeckoId: "",
    chainId: "0x1",
    coin: .eth
  )
  let asset3 = BraveWallet.BlockchainToken(
    contractAddress: "0x123",
    name: "mockAsset3",
    logo: "",
    isCompressed: false,
    isErc20: true,
    isErc721: false,
    isErc1155: false,
    splTokenProgram: .unsupported,
    isNft: false,
    isSpam: false,
    symbol: "MA",
    decimals: 16,
    visible: false,
    tokenId: "",
    coingeckoId: "",
    chainId: "0x1",
    coin: .eth
  )

  let fetchRequest = NSFetchRequest<WalletUserAsset>(entityName: "WalletUserAsset")

  private func entity(for context: NSManagedObjectContext) -> NSEntityDescription {
    return NSEntityDescription.entity(forEntityName: "WalletUserAsset", in: context)!
  }

  // MARK: - Adding

  func testAddWalletUserAsset() {
    let userAsset = createAndWait(asset: asset)
    XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 1)

    XCTAssertEqual(userAsset.contractAddress, asset.contractAddress)
  }

  // MARK: - Update Visibility and spam status

  func testUpdateAsset() {

    let userAsset = createAndWait(asset: asset)

    XCTAssertTrue(userAsset.visible)
    XCTAssertFalse(userAsset.isSpam)
    XCTAssertFalse(userAsset.isDeletedByUser)

    backgroundSaveAndWaitForExpectation {
      Task {
        await WalletUserAsset.updateUserAsset(
          for: asset,
          visible: false,
          isSpam: true,
          isDeletedByUser: true
        )
      }
    }

    DataController.viewContext.refreshAllObjects()
    // Make sure only one record was added to DB
    XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 1)

    XCTAssertFalse(userAsset.visible)
    XCTAssertTrue(userAsset.isSpam)
    XCTAssertTrue(userAsset.isDeletedByUser)
  }

  func testGetAllVisibleAssets() {
    createAndWait(asset: asset)
    createAndWait(asset: asset2)
    createAndWait(asset: asset3)

    backgroundSaveAndWaitForExpectation {
      Task {
        await WalletUserAsset.updateUserAsset(
          for: asset2,
          visible: false,
          isSpam: false,
          isDeletedByUser: false
        )
      }
    }

    DataController.viewContext.refreshAllObjects()
    let allAssets = WalletUserAsset.getAllUserAssets(visible: true)
    XCTAssertNotNil(allAssets)
    XCTAssertEqual(allAssets!.count, 2)
  }

  func testGetAllVisibleAssetsAfterDeletion() {
    createAndWait(asset: asset)
    createAndWait(asset: asset2)

    backgroundSaveAndWaitForExpectation {
      Task {
        await WalletUserAsset.updateUserAsset(
          for: asset2,
          visible: false,
          isSpam: false,
          isDeletedByUser: true
        )
      }
    }

    DataController.viewContext.refreshAllObjects()
    let allDeletedAssets = WalletUserAsset.getAllUserDeletedUserAssets()
    XCTAssertNotNil(allDeletedAssets)
    XCTAssertEqual(allDeletedAssets!.count, 1)
  }

  // MARK: - Deleting

  func testRemoveAsset() {
    createAndWait(asset: asset)
    XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 1)
    backgroundSaveAndWaitForExpectation {
      Task {
        await WalletUserAsset.removeUserAsset(token: asset)
      }
    }
    XCTAssertEqual(try! DataController.viewContext.count(for: self.fetchRequest), 0)
  }

  // MARK: - Utility

  @discardableResult
  private func createAndWait(
    asset: BraveWallet.BlockchainToken
  ) -> WalletUserAsset {
    backgroundSaveAndWaitForExpectation {
      Task {
        await WalletUserAsset.addUserAsset(token: asset)
      }
    }
    let userAsset = try! DataController.viewContext.fetch(fetchRequest).first!
    return userAsset
  }
}
