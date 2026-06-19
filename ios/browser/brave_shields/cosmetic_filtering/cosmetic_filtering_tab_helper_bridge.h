// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_SHIELDS_COSMETIC_FILTERING_COSMETIC_FILTERING_TAB_HELPER_BRIDGE_H_
#define BRAVE_IOS_BROWSER_BRAVE_SHIELDS_COSMETIC_FILTERING_COSMETIC_FILTERING_TAB_HELPER_BRIDGE_H_

#ifdef __cplusplus
#include "brave/ios/browser/brave_shields/cosmetic_filtering/cosmetic_filtering_args.h"
#else
#include "cosmetic_filtering_args.h"
#endif

NS_ASSUME_NONNULL_BEGIN

@protocol CosmeticFilteringTabHelperBridge

@required

/**
 * Gets the arguments needed for `cosmetic_filtering.ts`
 *
 * @param url The URL of the site needing cosmetic filtering.
 * @param completion A block invoked with the CosmeticFilteringArgs model for
 * the site
 */
- (void)cosmeticFilteringArgsFor:(NSURL*)url
                      completion:(void (^)(CosmeticFilteringArgs*))completion
    NS_SWIFT_NAME(cosmeticFilteringArgs(for:completion:));

/**
 * Given a set of classes and ids, returns the standard & aggressive selectors
 * to hide
 *
 * @param url The URL of the site needing cosmetic filtering.
 * @param ids The ids found on the site.
 * @param classes The classes found on the site.
 * @param completion A block invoked with the standard & aggressive selectors to
 * hide
 */
- (void)selectorsToHideFor:(NSURL*)frameURL
                       ids:(NSSet<NSString*>*)ids
                   classes:(NSSet<NSString*>*)classes
                completion:
                    (void (^)(NSSet<NSString*>* standardSelectors,
                              NSSet<NSString*>* aggressiveSelectors))completion
    NS_SWIFT_NAME(selectorsToHide(for:ids:classes:completion:));

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_BRAVE_SHIELDS_COSMETIC_FILTERING_COSMETIC_FILTERING_TAB_HELPER_BRIDGE_H_
