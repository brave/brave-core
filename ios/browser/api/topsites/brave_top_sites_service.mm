// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/strings/sys_string_conversions.h"
#import "brave/ios/browser/api/topsites/brave_top_sites_service_internal.h"
#include "components/ntp_tiles/constants.h"
#include "components/ntp_tiles/most_visited_sites.h"
#include "components/ntp_tiles/ntp_tile.h"
#include "ios/chrome/browser/ntp_tiles/model/ios_most_visited_sites_factory.h"
#include "ios/chrome/browser/ntp_tiles/model/most_visited_sites_observer_bridge.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "net/base/apple/url_conversions.h"

@interface BraveTopSite ()
@property(nonatomic, copy) NSURL* url;
@property(nonatomic, copy) NSString* title;
@end

@implementation BraveTopSite

- (instancetype)initWithURL:(NSURL*)url title:(NSString*)title {
  if ((self = [super init])) {
    self.url = url;
    self.title = title;
  }
  return self;
}

@end

@interface BraveTopSitesService () <MostVisitedSitesObserving> {
  std::unique_ptr<ntp_tiles::MostVisitedSites> _mostVisitedSites;
  std::unique_ptr<ntp_tiles::MostVisitedSitesObserverBridge> _observerBridge;
}
@end

@implementation BraveTopSitesService

- (instancetype)initWithProfile:(ProfileIOS*)profile {
  if ((self = [super init])) {
    _tiles = @[];
    _mostVisitedSites = IOSMostVisitedSitesFactory::NewForBrowserState(profile);
    _observerBridge =
        std::make_unique<ntp_tiles::MostVisitedSitesObserverBridge>(
            self, _mostVisitedSites.get());
    _mostVisitedSites->AddMostVisitedURLsObserver(
        _observerBridge.get(), ntp_tiles::kMaxNumMostVisited);
  }
  return self;
}

- (void)dealloc {
  _mostVisitedSites->RemoveMostVisitedURLsObserver(_observerBridge.get());
}

- (void)addOrRemoveBlockedUrl:(NSURL*)url add:(BOOL)add {
  _mostVisitedSites->AddOrRemoveBlockedUrl(net::GURLWithNSURL(url), add);
}

#pragma mark - MostVisitedSitesObserving

- (void)mostVisitedSites:(ntp_tiles::MostVisitedSites*)sites
          didUpdateTiles:(const ntp_tiles::NTPTilesVector&)tiles {
  NSMutableArray<BraveTopSite*>* result =
      [NSMutableArray arrayWithCapacity:tiles.size()];
  for (const ntp_tiles::NTPTile& tile : tiles) {
    [result addObject:[[BraveTopSite alloc]
                          initWithURL:net::NSURLWithGURL(tile.url)
                                title:base::SysUTF16ToNSString(tile.title)]];
  }
  self.tiles = [result copy];
}

- (void)mostVisitedSites:(ntp_tiles::MostVisitedSites*)sites
    didUpdateFaviconForURL:(const GURL&)siteURL {
  // Re-fire KVO so that swift side can reload favicons
  self.tiles = _tiles;
}

@end
