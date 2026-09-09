/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DIALOG_PRESENTING_H_
#define BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DIALOG_PRESENTING_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

// Implemented by the view controller hosting the Brave Account rows, so that
// the WebUI can ask for the authentication flows to be presented over them.
// `BraveAccountUIIOS` finds it by walking up from the WebState's view.
OBJC_EXPORT
@protocol BraveAccountDialogPresenting <NSObject>
- (void)presentBraveAccountDialogForURL:(NSURL*)url
    NS_SWIFT_NAME(presentBraveAccountDialog(for:));
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DIALOG_PRESENTING_H_
