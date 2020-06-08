/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

extension Notification.Name {
    public static let dataLoginDidChange = Notification.Name("DataLoginDidChange")

    public static let privateDataClearedHistory = Notification.Name("PrivateDataClearedHistory")
    public static let privateDataClearedDownloadedFiles = Notification.Name("PrivateDataClearedDownloadedFiles")

    // Fired when the user finishes navigating to a page and the location has changed
    public static let onLocationChange = Notification.Name("OnLocationChange")
    public static let didRestoreSession = Notification.Name("DidRestoreSession")

    // MARK: Notification UserInfo Keys
    public static let userInfoKeyHasSyncableAccount = Notification.Name("UserInfoKeyHasSyncableAccount")
  
    // Fired when the login synchronizer has finished applying remote changes
    public static let dataRemoteLoginChangesWereApplied = Notification.Name("DataRemoteLoginChangesWereApplied")

    // Fired when a the page metadata extraction script has completed and is being passed back to the native client
    public static let onPageMetadataFetched = Notification.Name("OnPageMetadataFetched")

    // Leaving these here in case we want to use these for our own sync implementation
    public static let profileDidStartSyncing = Notification.Name("ProfileDidStartSyncing")
    public static let profileDidFinishSyncing = Notification.Name("ProfileDidFinishSyncing")

    public static let databaseWasRecreated = Notification.Name("DatabaseWasRecreated")

    public static let passcodeDidChange = Notification.Name("PasscodeDidChange")

    public static let passcodeDidCreate = Notification.Name("PasscodeDidCreate")

    public static let passcodeDidRemove = Notification.Name("PasscodeDidRemove")

    public static let dynamicFontChanged = Notification.Name("DynamicFontChanged")

    public static let userInitiatedSyncManually = Notification.Name("UserInitiatedSyncManually")

    public static let bookmarkBufferValidated = Notification.Name("BookmarkBufferValidated")

    public static let faviconDidLoad = Notification.Name("FaviconDidLoad")

    public static let reachabilityStatusChanged = Notification.Name("ReachabilityStatusChanged")

    public static let contentBlockerTabSetupRequired = Notification.Name("ContentBlockerTabSetupRequired")

    public static let homePanelPrefsChanged = Notification.Name("HomePanelPrefsChanged")

    public static let fileDidDownload = Notification.Name("FileDidDownload")
    
    public static let thumbnailEditOn = Notification.Name("ThumbnailEditOn")
    public static let thumbnailEditOff = Notification.Name("ThumbnailEditOff")
    
    public static let privacyModeChanged = Notification.Name("PrivacyModeChanged")
    
    // MARK: - Rewards
    public static let adsOrRewardsToggledInSettings = Notification.Name("adsOrRewardsToggledInSettings")
}
