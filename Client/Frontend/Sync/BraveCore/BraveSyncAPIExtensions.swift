// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import BraveShared
import Shared

extension BraveSyncAPI {

  public static let seedByteLength = 32

  var isInSyncGroup: Bool {
    return Preferences.Chromium.syncEnabled.value
  }

  @discardableResult
  func joinSyncGroup(codeWords: String, syncProfileService: BraveSyncProfileServiceIOS) -> Bool {
    if self.setSyncCode(codeWords) {
      Preferences.Chromium.syncEnabled.value = true
      enableSyncTypes(syncProfileService: syncProfileService)

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

  func enableSyncTypes(syncProfileService: BraveSyncProfileServiceIOS) {
    syncProfileService.userSelectedTypes = []

    if Preferences.Chromium.syncBookmarksEnabled.value {
      syncProfileService.userSelectedTypes.update(with: .BOOKMARKS)
    }

    if Preferences.Chromium.syncHistoryEnabled.value {
      syncProfileService.userSelectedTypes.update(with: .HISTORY)
    }

    if Preferences.Chromium.syncPasswordsEnabled.value {
      syncProfileService.userSelectedTypes.update(with: .PASSWORDS)
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
    let deviceStateListener = BraveSyncDeviceListener(
      observer,
      onRemoved: { [weak self] observer in
        self?.deviceObservers.remove(observer)
      })
    deviceStateListener.observer = createSyncDeviceObserver(observer)

    deviceObservers.add(deviceStateListener)
    return deviceStateListener
  }

  public func removeAllObservers() {
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

    fileprivate init(
      _ onDeviceInfoChanged: @escaping () -> Void,
      onRemoved: @escaping (BraveSyncDeviceListener) -> Void
    ) {
      self.onRemoved = onRemoved
      super.init()
    }

    deinit {
      self.onRemoved(self)
    }
  }
}

extension BraveSyncAPI.QrCodeDataValidationResult {
  var errorDescription: String {
    switch self {
    case .valid:
      return ""
    case .notWellFormed:
      return Strings.invalidSyncCodeDescription
    case .versionDeprecated:
      return Strings.syncDeprecatedVersionError
    case .expired:
      return Strings.syncExpiredError
    case .validForTooLong:
      return Strings.syncValidForTooLongError
    default:
      assertionFailure("Invalid Error Description")
      return Strings.invalidSyncCodeDescription
    }
  }
}

extension BraveSyncAPI.WordsValidationStatus {
  var errorDescription: String {
    switch self {
    case .valid:
      return ""
    case .notValidPureWords:
      return Strings.invalidSyncCodeDescription
    case .versionDeprecated:
      return Strings.syncDeprecatedVersionError
    case .expired:
      return Strings.syncExpiredError
    case .validForTooLong:
      return Strings.syncValidForTooLongError
    case .wrongWordsNumber:
      return Strings.notEnoughWordsDescription
    default:
      assertionFailure("Invalid Error Description")
      return Strings.invalidSyncCodeDescription
    }
  }
}
