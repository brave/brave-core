// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

extension Notification.Name {
  public static let dataLoginDidChange = Notification.Name("DataLoginDidChange")

  public static let privateDataClearedHistory = Notification.Name("PrivateDataClearedHistory")

  // Fired when the user finishes navigating to a page and the location has changed
  public static let onLocationChange = Notification.Name("OnLocationChange")
  public static let didRestoreSession = Notification.Name("DidRestoreSession")

  public static let databaseWasRecreated = Notification.Name("DatabaseWasRecreated")

  public static let dynamicFontChanged = Notification.Name("DynamicFontChanged")

  public static let reachabilityStatusChanged = Notification.Name("ReachabilityStatusChanged")

  public static let contentBlockerTabSetupRequired = Notification.Name(
    "ContentBlockerTabSetupRequired"
  )

  public static let fileDidDownload = Notification.Name("FileDidDownload")

  public static let thumbnailEditOn = Notification.Name("ThumbnailEditOn")
  public static let thumbnailEditOff = Notification.Name("ThumbnailEditOff")

  // MARK: - Rewards
  public static let adsOrRewardsToggledInSettings = Notification.Name(
    "adsOrRewardsToggledInSettings"
  )
}
