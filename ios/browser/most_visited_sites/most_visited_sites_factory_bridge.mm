// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/most_visited_sites/most_visited_sites_factory_bridge.h"

#include <memory>
#include <utility>

#include "base/apple/foundation_util.h"
#include "brave/ios/browser/api/profile/profile_bridge.h"
#include "brave/ios/browser/api/profile/profile_bridge_impl.h"
#include "brave/ios/browser/most_visited_sites/most_visited_sites_bridge_impl.h"
#include "components/ntp_tiles/most_visited_sites.h"
#include "ios/chrome/browser/ntp_tiles/model/ios_most_visited_sites_factory.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/thread/web_thread.h"

@implementation MostVisitedSitesFactoryBridge

+ (id<MostVisitedSitesBridge>)mostVisitedSitesForProfile:
    (id<ProfileBridge>)profileBridge {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  ProfileBridgeImpl* holder =
      base::apple::ObjCCastStrict<ProfileBridgeImpl>(profileBridge);
  ProfileIOS* profile = holder.profile;
  DCHECK(!profile->IsOffTheRecord());
  std::unique_ptr<ntp_tiles::MostVisitedSites> mostVisitedSites =
      IOSMostVisitedSitesFactory::NewForBrowserState(profile);
  return [[MostVisitedSitesBridgeImpl alloc]
      initWithMostVisitedSites:std::move(mostVisitedSites)];
}

@end
