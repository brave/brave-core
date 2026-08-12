// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_MOST_VISITED_SITES_MOST_VISITED_SITES_BRIDGE_H_
#define BRAVE_IOS_BROWSER_MOST_VISITED_SITES_MOST_VISITED_SITES_BRIDGE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// A site shown in the most visited tiles. Wraps `ntp_tiles::NTPTile`.
OBJC_EXPORT
NS_SWIFT_NAME(NTPTile)
@interface NTPTileBridge : NSObject

@property(readonly) NSURL* url;
@property(readonly) NSString* title;

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithURL:(NSURL*)url
                      title:(NSString*)title NS_DESIGNATED_INITIALIZER;

@end

/// Observes the tiles vended by a `MostVisitedSitesBridge`.
NS_SWIFT_NAME(MostVisitedSitesObserver)
@protocol MostVisitedSitesObserverBridge

/// The set of tiles changed. Called with the initial set shortly after the
/// observer is attached.
- (void)mostVisitedSitesDidUpdateTiles:(NSArray<NTPTileBridge*>*)tiles;

/// A favicon became available for the tile at `url`.
- (void)mostVisitedSitesDidUpdateFaviconForURL:(NSURL*)url;

@end

/// The list of most visited sites, ranked by frecency.
/// Wraps `ntp_tiles::MostVisitedSites`.
NS_SWIFT_NAME(MostVisitedSites)
@protocol MostVisitedSitesBridge

/// Starts producing tiles and delivering them to `observer`.
///
/// Only history-backed tiles are surfaced (`ntp_tiles::TileSource::TOP_SITES`,
/// ranked by frecency).
///
/// `observer` is held weakly and receives the initial set of tiles
/// May only be called once;
/// `maxNumSites` is the max number of the returning tiles.
- (void)startTopSitesOnlyWithObserver:
            (id<MostVisitedSitesObserverBridge>)observer
                          maxNumSites:(NSUInteger)maxNumSites
    NS_SWIFT_NAME(startTopSitesOnly(observer:maxNumSites:));

/// Requests an asynchronous refresh. The observer is notified only if the set
/// of tiles changed.
- (void)refresh;

/// Hide or restore a URL from a blocked list.
- (void)setBlocked:(BOOL)blocked forURL:(NSURL*)url;

/// Restores every URL hidden from a blocked list.
- (void)clearBlockedURLs;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_MOST_VISITED_SITES_MOST_VISITED_SITES_BRIDGE_H_
