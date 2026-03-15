// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_WEB_LOGINS_LOGINS_TAB_HELPER_BRIDGE_H_
#define BRAVE_IOS_BROWSER_WEB_LOGINS_LOGINS_TAB_HELPER_BRIDGE_H_

#include <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol LoginsTabHelperBridge
@required
// Called when a form is submitted with credentials. `credentialsJSON` is a
// JSON string encoding a dictionary with the same keys as the "submit"
// message data in LoginsScriptHandler (type, hostname, username,
// usernameField, password, passwordField, formSubmitURL).
- (void)handleFormSubmit:(NSString*)credentialsJSON;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_WEB_LOGINS_LOGINS_TAB_HELPER_BRIDGE_H_
