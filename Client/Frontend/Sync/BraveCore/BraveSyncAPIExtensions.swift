// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import BraveShared

extension BraveSyncAPI {
    
    public static let seedByteLength = 32
    
    var isInSyncGroup: Bool {
        return Preferences.Chromium.syncEnabled.value
    }
    
    @discardableResult
    func joinSyncGroup(codeWords: String) -> Bool {
        if self.setSyncCode(codeWords) {
            Preferences.Chromium.syncEnabled.value = true
            enableSyncTypes()
            
            return true
        }
        return false
    }
    
    func removeDeviceFromSyncGroup(deviceGuid: String) {
        deleteDevice(deviceGuid)
    }
    
    func leaveSyncGroup() {
        // Remove all observers before leaving the sync chain
        removeAllObservers()
        
        resetSync()
        Preferences.Chromium.syncEnabled.value = false
    }
    
    func enableSyncTypes() {
        // TODO: Handle fetching syncProfileService from using AppDelegate IOS-4170
        guard let syncProfileService =
                (UIApplication.shared.delegate as? AppDelegate)?.braveCore.syncProfileService else {
            return
        }

        syncProfileService.userSelectedTypes = []
        
        if Preferences.Chromium.syncBookmarksEnabled.value {
            syncProfileService.userSelectedTypes.update(with: .BOOKMARKS)
        }
        
        if Preferences.Chromium.syncHistoryEnabled.value {
            syncProfileService.userSelectedTypes.update(with: .HISTORY)
        }
    }
    
    func addServiceStateObserver(_ observer: @escaping () -> Void) -> AnyObject {
        let serviceStateListener = BraveSyncServiceListener(onRemoved: { [weak self] observer in
            self?.serviceObservers.remove(observer)
        })
        serviceStateListener.observer = createSyncServiceObserver(observer)

        serviceObservers.add(serviceStateListener)
        return serviceStateListener
    }
    
    func addDeviceStateObserver(_ observer: @escaping () -> Void) -> AnyObject {
        let deviceStateListener = BraveSyncDeviceListener(observer, onRemoved: { [weak self] observer in
            self?.deviceObservers.remove(observer)
        })
        deviceStateListener.observer = createSyncDeviceObserver(observer)
        
        deviceObservers.add(deviceStateListener)
        return deviceStateListener
    }
    
    func removeAllObservers() {
        serviceObservers.objectEnumerator().forEach({
            ($0 as? BraveSyncServiceListener)?.observer = nil
        })
        
        deviceObservers.objectEnumerator().forEach({
            ($0 as? BraveSyncDeviceListener)?.observer = nil
        })
        
        serviceObservers.removeAllObjects()
        deviceObservers.removeAllObjects()
    }
    
    private struct AssociatedObjectKeys {
        static var serviceObservers: Int = 0
        static var deviceObservers: Int = 1
    }
    
    private var serviceObservers: NSHashTable<BraveSyncServiceListener> {
        if let observers = objc_getAssociatedObject(self, &AssociatedObjectKeys.serviceObservers) as? NSHashTable<BraveSyncServiceListener> {
            return observers
        }
        
        let defaultValue = NSHashTable<BraveSyncServiceListener>.weakObjects()
        objc_setAssociatedObject(self, &AssociatedObjectKeys.serviceObservers, defaultValue, .OBJC_ASSOCIATION_RETAIN_NONATOMIC)
        
        return defaultValue
    }
    
    private var deviceObservers: NSHashTable<BraveSyncDeviceListener> {
        if let observers = objc_getAssociatedObject(self, &AssociatedObjectKeys.deviceObservers) as? NSHashTable<BraveSyncDeviceListener> {
            return observers
        }
        
        let defaultValue = NSHashTable<BraveSyncDeviceListener>.weakObjects()
        objc_setAssociatedObject(self, &AssociatedObjectKeys.deviceObservers, defaultValue, .OBJC_ASSOCIATION_RETAIN_NONATOMIC)
        
        return defaultValue
    }

}

extension BraveSyncAPI {
    private class BraveSyncServiceListener: NSObject {
        
        // MARK: Internal
        
        var observer: Any?
        private var onRemoved: (BraveSyncServiceListener) -> Void
        
        // MARK: Lifecycle
        
        fileprivate init(onRemoved: @escaping (BraveSyncServiceListener) -> Void) {
            self.onRemoved = onRemoved
            super.init()
        }
        
        deinit {
            self.onRemoved(self)
        }
    }

    private class BraveSyncDeviceListener: NSObject {
        
        // MARK: Internal
        
        var observer: Any?
        private var onRemoved: (BraveSyncDeviceListener) -> Void
        
        // MARK: Lifecycle
        
        fileprivate init(_ onDeviceInfoChanged: @escaping () -> Void,
                         onRemoved: @escaping (BraveSyncDeviceListener) -> Void) {
            self.onRemoved = onRemoved
            super.init()
        }
        
        deinit {
            self.onRemoved(self)
        }
    }
}

extension BraveSyncAPI {
    func getQRCodeImageV2(_ size: CGSize) -> UIImage? {
        let hexCode = hexSeed(fromSyncCode: getSyncCode())
        if hexCode.isEmpty {
            return nil
        }

        // Typically QR Codes use isoLatin1, but it doesn't matter here
        // as we're not encoding any special characters
        guard let syncCodeData = BraveSyncQRCodeModel(syncHexCode: hexCode).jsonData,
              !syncCodeData.isEmpty else {
            return nil
        }
        
        guard let filter = CIFilter(name: "CIQRCodeGenerator") else {
            return nil
        }

        filter.do {
            $0.setValue(syncCodeData, forKey: "inputMessage")
            $0.setValue("H", forKey: "inputCorrectionLevel")
        }
        
        if let image = filter.outputImage,
            image.extent.size.width > 0.0,
            image.extent.size.height > 0.0 {
            let scaleX = size.width / image.extent.size.width
            let scaleY = size.height / image.extent.size.height
            let transform = CGAffineTransform(scaleX: scaleX, y: scaleY)
            
            return UIImage(ciImage: image.transformed(by: transform),
                           scale: UIScreen.main.scale,
                           orientation: .up)
        }
        
        return nil
    }
}
