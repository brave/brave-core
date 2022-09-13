/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_ADBLOCK_REGIONAL_CATALOG_ENTRY_PRIVATE_H_
#define BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_ADBLOCK_REGIONAL_CATALOG_ENTRY_PRIVATE_H_

#import <Foundation/Foundation.h>
#include "brave/ios/browser/api/brave_shields/adblock_regional_catalog_entry.h"

namespace brave_shields {
class RegionalCatalogEntry;
}  // namespace brave_shields

NS_ASSUME_NONNULL_BEGIN

@interface AdblockRegionalCatalogEntry (Private)
- (instancetype)initWithFilterList:(brave_shields::RegionalCatalogEntry)entry;
- (brave_shields::RegionalCatalogEntry)entry;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_ADBLOCK_REGIONAL_CATALOG_ENTRY_PRIVATE_H_
