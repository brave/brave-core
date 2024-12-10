// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import Preferences
import Shared
import os.log

public struct BraveSyncDevice: Codable {
  let chromeVersion: String
  let hasSharingInfo: Bool
  let id: String
  let guid: String
  let isCurrentDevice: Bool
  let supportsSelfDelete: Bool
  let lastUpdatedTimestamp: TimeInterval
  let name: String?
  let os: String
  let sendTabToSelfReceivingEnabled: Bool
  let type: String
}

extension BraveSyncAPI {

  public static let seedByteLength = 32

  var isInSyncGroup: Bool {
    return Preferences.Chromium.syncEnabled.value
  }

  /// Property that determines if the local sync chain should be resetted
  var shouldLeaveSyncGroup: Bool {
    guard isInSyncGroup else {
      return false
    }

    return (!isSyncFeatureActive && !isInitialSyncFeatureSetupComplete)
      || isSyncAccountDeletedNoticePending
  }

  var isSendTabToSelfVisible: Bool {
    guard let json = getDeviceListJSON(), let data = json.data(using: .utf8) else {
      return false
    }

    do {
      let devices = try JSONDecoder().decode([BraveSyncDevice].self, from: data)
      return devices.count > 1
    } catch {
      Logger.module.error(
        "Error occurred while parsing device information: \(error.localizedDescription)"
      )
      return false
    }
  }

  @discardableResult
  func joinSyncGroup(codeWords: String, syncProfileService: BraveSyncProfileServiceIOS) -> Bool {
    if setSyncCode(codeWords) {
      // Enable default sync type Bookmarks when joining a chain
      Preferences.Chromium.syncBookmarksEnabled.value = true
      enableSyncTypes(syncProfileService: syncProfileService)
      requestSync()
      setSetupComplete()
      Preferences.Chromium.syncEnabled.value = true

      return true
    }
    return false
  }

  func removeDeviceFromSyncGroup(deviceGuid: String) {
    deleteDevice(deviceGuid)
  }

  /// Method for leaving sync chain
  /// Removing Observers, clearing local preferences and calling reset chain on brave-core side
  /// - Parameter preservingObservers: Parameter that decides if observers should be preserved or removed
  func leaveSyncGroup(preservingObservers: Bool = false) {
    if !preservingObservers {
      // Remove all observers before leaving the sync chain
      removeAllObservers()
    }

    resetSyncChain()
    Preferences.Chromium.syncEnabled.value = false
  }

  func resetSyncChain() {
    Preferences.Chromium.syncHistoryEnabled.value = false
    Preferences.Chromium.syncPasswordsEnabled.value = false
    Preferences.Chromium.syncOpenTabsEnabled.value = false

    resetSync()
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

    if Preferences.Chromium.syncOpenTabsEnabled.value {
      syncProfileService.userSelectedTypes.update(with: .TABS)
    }

    if Preferences.Chromium.syncAutofillEnabled.value {
      syncProfileService.userSelectedTypes.update(with: .AUTOFILL)
    }
  }

  /// Method to add observer for SyncService for onStateChanged and onSyncShutdown
  /// OnStateChanged can be called in various situations like successful initialization - services unavaiable
  /// sync shutdown - sync errors - sync chain deleted
  /// - Parameters:
  ///   - onStateChanged: Callback for sync service state changes
  ///   - onServiceShutdown: Callback for sync service shutdown
  /// - Returns: Listener for service
  func addServiceStateObserver(
    _ onStateChanged: @escaping () -> Void,
    onServiceShutdown: @escaping () -> Void = {}
  ) -> AnyObject {
    let serviceStateListener = BraveSyncServiceListener(onRemoved: { [weak self] observer in
      self?.serviceObservers.remove(observer)
    })
    serviceStateListener.observer = createSyncServiceObserver(
      onStateChanged,
      onSyncServiceShutdown: onServiceShutdown
    )

    serviceObservers.add(serviceStateListener)
    return serviceStateListener
  }

  func addDeviceStateObserver(_ observer: @escaping () -> Void) -> AnyObject {
    let deviceStateListener = BraveSyncDeviceListener(
      observer,
      onRemoved: { [weak self] observer in
        self?.deviceObservers.remove(observer)
      }
    )
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
    if let observers = objc_getAssociatedObject(self, &AssociatedObjectKeys.serviceObservers)
      as? NSHashTable<BraveSyncServiceListener>
    {
      return observers
    }

    let defaultValue = NSHashTable<BraveSyncServiceListener>.weakObjects()
    objc_setAssociatedObject(
      self,
      &AssociatedObjectKeys.serviceObservers,
      defaultValue,
      .OBJC_ASSOCIATION_RETAIN_NONATOMIC
    )

    return defaultValue
  }

  private var deviceObservers: NSHashTable<BraveSyncDeviceListener> {
    if let observers = objc_getAssociatedObject(self, &AssociatedObjectKeys.deviceObservers)
      as? NSHashTable<BraveSyncDeviceListener>
    {
      return observers
    }

    let defaultValue = NSHashTable<BraveSyncDeviceListener>.weakObjects()
    objc_setAssociatedObject(
      self,
      &AssociatedObjectKeys.deviceObservers,
      defaultValue,
      .OBJC_ASSOCIATION_RETAIN_NONATOMIC
    )

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
      return Strings.Sync.invalidSyncCodeDescription
    case .versionDeprecated:
      return Strings.Sync.deprecatedVersionError
    case .expired:
      return Strings.Sync.expiredError
    case .validForTooLong:
      return Strings.Sync.validForTooLongError
    default:
      assertionFailure("Invalid Error Description")
      return Strings.Sync.invalidSyncCodeDescription
    }
  }
}

extension BraveSyncAPI.WordsValidationStatus {
  var errorDescription: String {
    switch self {
    case .valid:
      return ""
    case .notValidPureWords:
      return Strings.Sync.invalidSyncCodeDescription
    case .versionDeprecated:
      return Strings.Sync.deprecatedVersionError
    case .expired:
      return Strings.Sync.expiredError
    case .validForTooLong:
      return Strings.Sync.validForTooLongError
    case .wrongWordsNumber:
      return Strings.Sync.notEnoughWordsDescription
    default:
      assertionFailure("Invalid Error Description")
      return Strings.Sync.invalidSyncCodeDescription
    }
  }
}
