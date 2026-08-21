// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/most_visited_sites/most_visited_sites_bridge_impl.h"

#include <utility>

#include "base/strings/sys_string_conversions.h"
#include "components/ntp_tiles/most_visited_sites.h"
#include "components/ntp_tiles/ntp_tile.h"
#include "ios/chrome/browser/ntp_tiles/model/most_visited_sites_observer_bridge.h"
#include "net/base/apple/url_conversions.h"
#include "url/gurl.h"

@implementation NTPTileBridge

- (instancetype)initWithURL:(NSURL*)url title:(NSString*)title {
  if ((self = [super init])) {
    _url = url;
    _title = title;
  }
  return self;
}

@end

@interface MostVisitedSitesSubscriptionImpl
    : NSObject <MostVisitedSitesObserving, MostVisitedSitesSubscription>

- (instancetype)initWithObserver:(id<MostVisitedSitesObserverBridge>)observer
                mostVisitedSites:(ntp_tiles::MostVisitedSites*)mostVisitedSites
                     maxNumSites:(NSUInteger)maxNumSites;

@end

@implementation MostVisitedSitesSubscriptionImpl {
  __weak id<MostVisitedSitesObserverBridge> _observer;
  raw_ptr<ntp_tiles::MostVisitedSites> _mostVisitedSites;
  std::unique_ptr<ntp_tiles::MostVisitedSitesObserverBridge> _observerBridge;
}

- (instancetype)initWithObserver:(id<MostVisitedSitesObserverBridge>)observer
                mostVisitedSites:(ntp_tiles::MostVisitedSites*)mostVisitedSites
                     maxNumSites:(NSUInteger)maxNumSites {
  if ((self = [super init])) {
    _observer = observer;
    _mostVisitedSites = mostVisitedSites;
    _observerBridge =
        std::make_unique<ntp_tiles::MostVisitedSitesObserverBridge>(
            self, mostVisitedSites);
    _mostVisitedSites->AddMostVisitedURLsObserver(_observerBridge.get(),
                                                  maxNumSites);
  }
  return self;
}

- (void)dealloc {
  [self unsubscribe];
}

- (void)unsubscribe {
  if (_observerBridge) {
    _mostVisitedSites->RemoveMostVisitedURLsObserver(_observerBridge.get());
    _observerBridge.reset();
  }
}

#pragma mark - MostVisitedSitesObserving

- (void)mostVisitedSites:(ntp_tiles::MostVisitedSites*)mostVisitedSites
          didUpdateTiles:(const ntp_tiles::NTPTilesVector&)tiles {
  NSMutableArray<NTPTileBridge*>* bridgedTiles =
      [NSMutableArray arrayWithCapacity:tiles.size()];
  for (const ntp_tiles::NTPTile& tile : tiles) {
    NSURL* url = net::NSURLWithGURL(tile.url);
    if (!url) {
      continue;
    }
    [bridgedTiles
        addObject:[[NTPTileBridge alloc]
                      initWithURL:url
                            title:base::SysUTF16ToNSString(tile.title)]];
  }
  [_observer mostVisitedSitesDidUpdateTiles:[bridgedTiles copy]];
}

- (void)mostVisitedSites:(ntp_tiles::MostVisitedSites*)mostVisi
    didUpdateFaviconForURL:(const GURL&)siteURL {
  NSURL* url = net::NSURLWithGURL(siteURL);
  if (!url) {
    return;
  }
  [_observer mostVisitedSitesDidUpdateFaviconForURL:url];
}

@end

@implementation MostVisitedSitesBridgeImpl {
  std::unique_ptr<ntp_tiles::MostVisitedSites> _mostVisitedSites;
}

- (instancetype)initWithMostVisitedSites:
    (std::unique_ptr<ntp_tiles::MostVisitedSites>)mostVisitedSites {
  if ((self = [super init])) {
    _mostVisitedSites = std::move(mostVisitedSites);
    _mostVisitedSites->EnableTileTypes(
        ntp_tiles::MostVisitedSites::EnableTileTypesOptions().with_top_sites(
            true));
  }
  return self;
}

- (id<MostVisitedSitesSubscription>)
    startTopSitesOnlyWithObserver:(id<MostVisitedSitesObserverBridge>)observer
                      maxNumSites:(NSUInteger)maxNumSites {
  return [[MostVisitedSitesSubscriptionImpl alloc]
      initWithObserver:observer
      mostVisitedSites:_mostVisitedSites.get()
           maxNumSites:maxNumSites];
}

- (void)refresh {
  _mostVisitedSites->Refresh();
}

- (void)setBlocked:(BOOL)blocked forURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  if (!gurl.is_valid()) {
    return;
  }
  _mostVisitedSites->AddOrRemoveBlockedUrl(gurl, blocked);
}

- (void)clearBlockedURLs {
  _mostVisitedSites->ClearBlockedUrls();
}

@end
