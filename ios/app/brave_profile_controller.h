// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_APP_BRAVE_PROFILE_CONTROLLER_H_
#define BRAVE_IOS_APP_BRAVE_PROFILE_CONTROLLER_H_

#import <Foundation/Foundation.h>

@class BraveBookmarksAPI, BraveHistoryAPI, BravePasswordAPI, BraveOpenTabsAPI,
    BraveSendTabAPI, BraveSyncAPI, BraveSyncProfileServiceIOS, DeAmpPrefs,
    BraveTabGeneratorAPI, BraveWalletAPI, BraveStats, AIChat,
    DefaultHostContentSettings, CWVWebViewConfiguration, WebImageDownloader,
    NTPBackgroundImagesService, BraveWebViewConfiguration;
@protocol AIChatDelegate
, IpfsAPI, ProfileBridge;

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface BraveProfileController : NSObject
@property(readonly) id<ProfileBridge> profile;
@property(readonly) BraveBookmarksAPI* bookmarksAPI;
@property(readonly) BraveHistoryAPI* historyAPI;
@property(readonly) BravePasswordAPI* passwordAPI;
@property(readonly) BraveOpenTabsAPI* openTabsAPI;
@property(readonly) BraveSendTabAPI* sendTabAPI;
@property(readonly) BraveSyncAPI* syncAPI;
@property(readonly) BraveSyncProfileServiceIOS* syncProfileService;
@property(readonly) BraveTabGeneratorAPI* tabGeneratorAPI;
@property(readonly) BraveWalletAPI* braveWalletAPI;
@property(readonly) BraveStats* braveStats;
@property(readonly) DeAmpPrefs* deAmpPrefs;
@property(readonly) id<IpfsAPI> ipfsAPI;
- (AIChat*)aiChatAPIWithDelegate:(id<AIChatDelegate>)delegate;
/// The default content settings for regular browsing windows
@property(readonly) DefaultHostContentSettings* defaultHostContentSettings;
@property(readonly) NTPBackgroundImagesService* backgroundImagesService;
@property(readonly) WebImageDownloader* webImageDownloader;

@property(readonly) BraveWebViewConfiguration* defaultWebViewConfiguration;
@property(readonly)
    BraveWebViewConfiguration* nonPersistentWebViewConfiguration;
- (void)notifyLastPrivateTabClosed;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_APP_BRAVE_PROFILE_CONTROLLER_H_
