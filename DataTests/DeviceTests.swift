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
        var device: Device?
        
        backgroundSaveAndWaitForExpectation {
            device = Device.currentDevice()
        }
        let newDevice = Device.currentDevice()
        
        XCTAssertEqual(try! DataController.mainThreadContext.fetch(fetchRequest).count, 1)
        
        XCTAssertEqual(device, newDevice)
        
    }

    func testDeleteAll() {
        backgroundSaveAndWaitForExpectation {
            _ = Device.currentDevice()
        }
        
        backgroundSaveAndWaitForExpectation {
            Device.deleteAll { }
        }
        
        XCTAssertEqual(try! DataController.mainThreadContext.fetch(fetchRequest).count, 0)
       
    }

    // MARK: Syncable
    
    func testAddWithSave() {
        let context = DataController.workerThreadContext
        backgroundSaveAndWaitForExpectation {
            let result = Device.add(rootObject: nil, save: true, sendToSync: false, context: context) as? Device
            XCTAssertNotNil(result)
        }
        
        XCTAssertEqual(try! DataController.mainThreadContext.fetch(fetchRequest).count, 1)
    }
    
    func testUpdate() {
        let newName = "newName"
        let newDeviceId = [1, 2, 3]
        
        let root = SyncDevice()
        root.name = newName
        root.deviceId = newDeviceId
        
        let device = Device.currentDevice()
        
        XCTAssertNotEqual(device?.name, newName)
        XCTAssertNotEqual(device?.deviceId, newDeviceId)
        
        // No CD save
        device?.update(syncRecord: root)
        
        XCTAssertEqual(device?.name, newName)
        XCTAssertEqual(device?.deviceId, newDeviceId)
    }
}
