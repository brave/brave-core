// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_SHIELDS_SCRIPTLETS_SCRIPTLETS_TAB_HELPER_BRIDGE_H_
#define BRAVE_IOS_BROWSER_BRAVE_SHIELDS_SCRIPTLETS_SCRIPTLETS_TAB_HELPER_BRIDGE_H_

NS_ASSUME_NONNULL_BEGIN

@protocol ScriptletsTabHelperBridge

@required

/**
 * Requests the scriptlets to inject for a given frame.
 *
 * @param frameURL The URL of the frame requesting the scriptlets.
 * @param completion A block invoked with the scriptlet JavaScript strings to
 * inject. The array is empty if there are no scriptlets for the frame.
 */
- (void)requestScriptletsForFrameURL:(NSURL*)frameURL
                          completion:(void (^)(NSArray<NSString*>* scriptlets))
                                         completion
    NS_SWIFT_NAME(requestScriptlets(frameURL:completion:));

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_BRAVE_SHIELDS_SCRIPTLETS_SCRIPTLETS_TAB_HELPER_BRIDGE_H_
