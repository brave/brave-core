// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/most_visited_sites/most_visited_sites_bridge_impl.h"

#include <map>
#include <utility>

#include "base/strings/sys_string_conversions.h"
#include "components/ntp_tiles/most_visited_sites.h"
#include "components/ntp_tiles/ntp_tile.h"
#include "components/ntp_tiles/section_type.h"
#include "net/base/apple/url_conversions.h"
#include "url/gurl.h"

@implementation NTPTileBridge

- (instancetype)initWithURL:(NSURL*)url title:(NSString*)title {
  if ((self = [super init])) {
    _url = [url copy];
    _title = [title copy];
  }
  return self;
}

@end

namespace {

// Forwards `ntp_tiles::MostVisitedSites::Observer` callbacks directly to a
// `MostVisitedSitesObserverBridge`.
class MostVisitedSitesObserverImpl
    : public ntp_tiles::MostVisitedSites::Observer {
 public:
  explicit MostVisitedSitesObserverImpl(
      id<MostVisitedSitesObserverBridge> observer)
      : observer_(observer) {}

  void OnURLsAvailable(
      bool is_user_triggered,
      const std::map<ntp_tiles::SectionType, ntp_tiles::NTPTilesVector>&
          sections) override {
    const ntp_tiles::NTPTilesVector& tiles =
        sections.at(ntp_tiles::SectionType::PERSONALIZED);
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
    [observer_ mostVisitedSitesDidUpdateTiles:[bridgedTiles copy]];
  }

  void OnIconMadeAvailable(const GURL& site_url) override {
    [observer_
        mostVisitedSitesDidUpdateFaviconForURL:net::NSURLWithGURL(site_url)];
  }

 private:
  __weak id<MostVisitedSitesObserverBridge> observer_;
};

}  // namespace

@interface MostVisitedSitesObservationImpl
    : NSObject <MostVisitedSitesObservation>

- (instancetype)initWithObserverBridge:
    (std::unique_ptr<MostVisitedSitesObserverImpl>)observerBridge;

@end

@implementation MostVisitedSitesObservationImpl {
  std::unique_ptr<MostVisitedSitesObserverImpl> _observerBridge;
}

- (instancetype)initWithObserverBridge:
    (std::unique_ptr<MostVisitedSitesObserverImpl>)observerBridge {
  if ((self = [super init])) {
    _observerBridge = std::move(observerBridge);
  }
  return self;
}

- (void)dealloc {
  [self invalidate];
}

- (void)invalidate {
  _observerBridge.reset();
}

@end

@implementation MostVisitedSitesBridgeImpl {
  std::unique_ptr<ntp_tiles::MostVisitedSites> _mostVisitedSites;
}

- (instancetype)initWithMostVisitedSites:
    (std::unique_ptr<ntp_tiles::MostVisitedSites>)mostVisitedSites {
  if ((self = [super init])) {
    _mostVisitedSites = std::move(mostVisitedSites);
  }
  return self;
}

- (id<MostVisitedSitesObservation>)
    addMostVisitedURLsObserver:(id<MostVisitedSitesObserverBridge>)observer
                   maxNumSites:(NSUInteger)maxNumSites {
  auto observerBridge =
      std::make_unique<MostVisitedSitesObserverImpl>(observer);
  _mostVisitedSites->AddMostVisitedURLsObserver(observerBridge.get(),
                                                maxNumSites);
  return [[MostVisitedSitesObservationImpl alloc]
      initWithObserverBridge:std::move(observerBridge)];
}

- (void)enableTopSitesOnlyTileTypes {
  _mostVisitedSites->EnableTileTypes(
      ntp_tiles::MostVisitedSites::EnableTileTypesOptions().with_top_sites(
          true));
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
