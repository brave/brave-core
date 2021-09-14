/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_HISTORY_BRAVE_PASSWORD_API_PRIVATE_H_
#define BRAVE_IOS_BROWSER_API_HISTORY_BRAVE_PASSWORD_API_PRIVATE_H_

#import <Foundation/Foundation.h>

#include "brave/ios/browser/api/password/brave_password_api.h"
#include "base/memory/ref_counted.h"

NS_ASSUME_NONNULL_BEGIN

namespace password_manager {
class PasswordStore;
}

@interface BravePasswordAPI (Private)
- (instancetype)initWithBrowserState(scoped_refptr<password_manager::PasswordStore>)passwordStore;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_HISTORY_BRAVE_PASSWORD_API_PRIVATE_H_
