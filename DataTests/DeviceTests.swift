// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import CoreData
@testable import Data

class DeviceTests: CoreDataTestCase {
    
    let fetchRequest = NSFetchRequest<Device>(entityName: String(describing: Device.self))
    
    private func entity(for context: NSManagedObjectContext) -> NSEntityDescription {
        return NSEntityDescription.entity(forEntityName: String(describing: Device.self), in: context)!
    }
    
    func testCurrentDevice() {
        
        // Adding not current device to verify nothing is returned in Device.currentDevice()
        backgroundSaveAndWaitForExpectation {
            Device.add(name: "Brave")
        }
        let device = Device.currentDevice()
        
        XCTAssertEqual(try! DataController.viewContext.fetch(fetchRequest).count, 1)
        
        XCTAssertNil(device)
        
        backgroundSaveAndWaitForExpectation {
            Device.add(name: "Second device", isCurrent: true)
        }
        
        let newCurrentDevice = Device.currentDevice()
        XCTAssertNotNil(newCurrentDevice)
    }

    func testDeleteAll() {
        backgroundSaveAndWaitForExpectation {
            Device.add(name: "Brave")
        }
        
        backgroundSaveAndWaitForExpectation {
            Device.deleteAll()
        }
        
        XCTAssertEqual(try! DataController.viewContext.fetch(fetchRequest).count, 0)
       
    }

    // MARK: Syncable
    
    func testAddWithSave() {
        backgroundSaveAndWaitForExpectation {
            DataController.perform { context in
                Device.createResolvedRecord(rootObject: nil, save: true, context: .existing(context))
            }
        }
        
        XCTAssertEqual(try! DataController.viewContext.fetch(fetchRequest).count, 1)
    }
    
    func testUniqueSyncUUID() {
        XCTAssertEqual(try! DataController.viewContext.fetch(fetchRequest).count, 0)
        backgroundSaveAndWaitForExpectation {
            DataController.perform { context in
                Device.createResolvedRecord(rootObject: nil, save: true, context: .existing(context))
            }
        }
        
        // get the device on main thread
        let device = try! DataController.viewContext.fetch(fetchRequest).first
        
        XCTAssertEqual(try! DataController.viewContext.fetch(fetchRequest).count, 1)
        let root = SyncDevice()
        root.objectId = device?.syncUUID
        
        backgroundSaveAndWaitForExpectation {
            DataController.perform { context in
                Device.createResolvedRecord(rootObject: root, save: true, context: .existing(context))
            }
        }
        
        XCTAssertEqual(try! DataController.viewContext.fetch(fetchRequest).count, 1)
    }
    
    func testUpdate() {
        let newName = "newName"
        let newDeviceId = [1, 2, 3]
        
        let root = SyncDevice()
        root.name = newName
        root.deviceId = newDeviceId
        
        backgroundSaveAndWaitForExpectation {
            Device.add(name: "Brave", isCurrent: true)
        }
        
        let device = Device.currentDevice()
        
        XCTAssertNotEqual(device?.name, newName)
        XCTAssertNotEqual(device?.deviceId, newDeviceId)
        
        // No CD save
        device?.updateResolvedRecord(root)
        
        XCTAssertEqual(device?.name, newName)
        XCTAssertEqual(device?.deviceId, newDeviceId)
    }
}
