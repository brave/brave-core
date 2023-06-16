// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import CoreData
import XCTest
@testable import Data
import TestHelpers
import BraveCore

class WalletUserAssetGroupTests: CoreDataTestCase {
  let fetchRequest = NSFetchRequest<WalletUserAssetGroup>(entityName:"WalletUserAssetGroup")
  
  private func entity(for context: NSManagedObjectContext) -> NSEntityDescription {
    return NSEntityDescription.entity(forEntityName: "WalletUserAssetGroup", in: context)!
  }
  
  // MARK: - Get group
  
  func testGetGroup() {
    let group = createAndWait(groupId: "60.0x1")
    let getGroup = WalletUserAssetGroup.getGroup(groupId: "60.0x1", context: nil)
    XCTAssertNotNil(getGroup)
    XCTAssertEqual(getGroup!.groupId, "60.0x1")
  }
  
  func testGetAllGroups() {
    createAndWait(groupId: "60.0x1")
    createAndWait(groupId: "60.0x2")
    createAndWait(groupId: "60.0x3")
    let allGroups = WalletUserAssetGroup.getAllGroups()
    XCTAssertNotNil(allGroups)
    XCTAssertEqual(allGroups!.count, 3)
  }
  
  // MARK: - Deleting
  
  func testRemoveAssetGroup() {
    createAndWait(groupId: "60.0x1")
    XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 1)
    backgroundSaveAndWaitForExpectation {
      WalletUserAssetGroup.removeGroup("60.0x1")
    }
    XCTAssertEqual(try! DataController.viewContext.count(for: self.fetchRequest), 0)
  }
  
  func testRemoveAllAssetGroups() {
    createAndWait(groupId: "60.0x1")
    createAndWait(groupId: "60.0x2")
    createAndWait(groupId: "60.0x3")
    XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 3)
    backgroundSaveAndWaitForExpectation {
      WalletUserAssetGroup.removeAllGroup()
    }
    XCTAssertEqual(try! DataController.viewContext.count(for: self.fetchRequest), 0)
  }
  
  // MARK: - Utility
  
  @discardableResult
  private func createAndWait(groupId: String) -> WalletUserAssetGroup {
    backgroundSaveAndWaitForExpectation {
      DataController.perform(context: .new(inMemory: false), save: true) { context in
        let group = WalletUserAssetGroup(context: context, groupId: groupId)
      }
    }
    let userAssetGroup = try! DataController.viewContext.fetch(fetchRequest).first!
    return userAssetGroup
  }
}
