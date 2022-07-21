/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_ADBLOCK_FILTER_LIST_PRIVATE_H_
#define BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_ADBLOCK_FILTER_LIST_PRIVATE_H_

#import <Foundation/Foundation.h>
#include "brave/ios/browser/api/brave_shields/adblock_filter_list.h"

namespace adblock {
class FilterList;
}  // namespace adblock

NS_ASSUME_NONNULL_BEGIN

@interface AdblockFilterList (Private)
- (instancetype)initWithFilterList:(adblock::FilterList)filterList;
- (adblock::FilterList)filterList;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_ADBLOCK_FILTER_LIST_PRIVATE_H_
