/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_VARIATIONS_MODEL_BRAVE_VARIATIONS_SEED_FETCHER_H_
#define BRAVE_IOS_BROWSER_VARIATIONS_MODEL_BRAVE_VARIATIONS_SEED_FETCHER_H_

#include <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface BraveVariationsSeedFetcher : NSObject
- (instancetype)init;
- (void)fetchSeedWithCompletion:(void (^)(bool success))completion;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_VARIATIONS_MODEL_BRAVE_VARIATIONS_SEED_FETCHER_H_
