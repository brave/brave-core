/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_OPENTABS_BRAVE_TABGENERATOR_API_PRIVATE_H_
#define BRAVE_IOS_BROWSER_API_OPENTABS_BRAVE_TABGENERATOR_API_PRIVATE_H_

#import <Foundation/Foundation.h>

#include "brave/ios/browser/api/opentabs/brave_tabgenerator_api.h"

NS_ASSUME_NONNULL_BEGIN

class Browser;

@interface BraveTabGeneratorAPI (Private)
- (instancetype)initWithBrowser:(Browser*)browser;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_OPENTABS_BRAVE_TABGENERATOR_API_PRIVATE_H_
