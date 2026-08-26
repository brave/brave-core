// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_MOST_VISITED_SITES_MOST_VISITED_SITES_BRIDGE_IMPL_H_
#define BRAVE_IOS_BROWSER_MOST_VISITED_SITES_MOST_VISITED_SITES_BRIDGE_IMPL_H_

#import <Foundation/Foundation.h>

#include <memory>

#include "brave/ios/browser/most_visited_sites/most_visited_sites_bridge.h"

namespace ntp_tiles {
class MostVisitedSites;
}  // namespace ntp_tiles

NS_ASSUME_NONNULL_BEGIN

@interface MostVisitedSitesBridgeImpl : NSObject <MostVisitedSitesBridge>

- (instancetype)init NS_UNAVAILABLE;

- (instancetype)initWithMostVisitedSites:
    (std::unique_ptr<ntp_tiles::MostVisitedSites>)mostVisitedSites
    NS_DESIGNATED_INITIALIZER;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_MOST_VISITED_SITES_MOST_VISITED_SITES_BRIDGE_IMPL_H_
