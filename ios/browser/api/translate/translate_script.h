/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_TRANSLATE_TRANSLATE_SCRIPT_H_
#define BRAVE_IOS_BROWSER_API_TRANSLATE_TRANSLATE_SCRIPT_H_

#include <Foundation/Foundation.h>

OBJC_EXPORT
@interface TranslateScript : NSObject
@property(nonatomic, class, readonly) NSString* script;
@end

#endif  // BRAVE_IOS_BROWSER_API_TRANSLATE_TRANSLATE_SCRIPT_H_
