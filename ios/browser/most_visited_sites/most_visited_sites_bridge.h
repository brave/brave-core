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
@protocol MostVisitedSitesBridgeObserver

/// The set of tiles changed. Called with the initial set shortly after the
/// observer is attached. Mirrors
/// `ntp_tiles::MostVisitedSites::Observer::OnURLsAvailable`.
- (void)mostVisitedSitesDidUpdateTiles:(NSArray<NTPTileBridge*>*)tiles;

/// A favicon became available for the tile at `url`. Mirrors
/// `ntp_tiles::MostVisitedSites::Observer::OnIconMadeAvailable`.
- (void)mostVisitedSitesDidUpdateFaviconForURL:(NSURL*)url;

@end

/// The list of most visited sites, ranked by frecency.
/// Wraps `ntp_tiles::MostVisitedSites`.
NS_SWIFT_NAME(MostVisitedSites)
@protocol MostVisitedSitesBridge

/// The most recently delivered tiles. Empty until the first delivery.
@property(readonly) NSArray<NTPTileBridge*>* tiles;

/// Receives tile and favicon updates.
@property(nonatomic, weak, nullable) id<MostVisitedSitesBridgeObserver>
    observer;

/// Requests an asynchronous refresh. The observer is notified only if the set
/// of tiles changed. Mirrors `ntp_tiles::MostVisitedSites::Refresh`.
- (void)refresh;

/// Hide or restore a URL from a blocked list.
/// Mirrors `ntp_tiles::MostVisitedSites::AddOrRemoveBlockedUrl`.
- (void)setBlocked:(BOOL)blocked forURL:(NSURL*)url;

/// Restores every URL hidden from a blocked list. Mirrors
/// `ntp_tiles::MostVisitedSites::ClearBlockedUrls`.
- (void)clearBlockedURLs;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_MOST_VISITED_SITES_MOST_VISITED_SITES_BRIDGE_H_
