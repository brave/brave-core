// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_V2_KNOBS_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_V2_KNOBS_H_

namespace brave_news::knobs {
namespace switches {

// An integer indicating the minimum number of cards (following the hero) in a
// block.
extern const char kBraveNewsMinBlockCards[];

// An integer indicating the maximum number of cards (following the hero) in a
// block.
extern const char kBraveNewsMaxBlockCards[];

// A double. Every N hours the popRecency will halve. I.e, if this was 24, then
// every day the popularity score will be halved.
extern const char kBraveNewsPopScoreHalfLife[];

// A double which is used as the fallback |pop_score| value for articles we
// don't have a |pop_score| for, such as articles from a direct feed, or just
// articles that Brave Search doesn't have enough information about.
extern const char kBraveNewsPopScoreFallback[];

// The ratio at which inline cards present discovery options (i.e. a source the
// user has not visited before).
// For example, this is 1:3 by default, so 0.25
extern const char kBraveNewsInlineDiscoveryRatio[];

// The minimum subscription weight for sources the user is not subscribed to,
// and is not subscribed to a channel containing. Note: If the user has
// explicitly stated they don't want to see a source, the source will have a
// zero weight here. This allows sources the user is not subscribed to to show
// up in the feed (i.e. for discover cards). It should be a small, but non-zero
// value.
extern const char kBraveNewsSourceSubscribedMin[];

// The boost which is applied to sources that the user has explicitly followed.
// This also applies to direct feeds.
extern const char kBraveNewsSourceSubscribedBoost[];

// The boost which is applied to sources where the user is following a channel
// containing the source. Ideally this is a smaller value than the subscribed
// boost, as that provides a stronger signal of interest.
extern const char kBraveNewsChannelSubscribedBoost[];

// The minimum visit weighting to apply to sources (i.e. unvisited sources).
// This value is used so unvisited sources still show up in the feed. Source
// visits are calculated as the normalized visit count (i.e. 0 - 1) + this
// offset.
extern const char kBraveNewsSourceVisitsMin[];

}  // namespace switches

int GetMinBlockCards();
int GetMaxBlockCards();

double GetPopRecencyHalfLife();
double GetPopScoreFallback();

double GetInlineDiscoveryRatio();

double GetSourceSubscribedMin();
double GetSourceSubscribedBoost();
double GetChannelSubscribedBoost();

double GetSourceVisitsMin();

}  // namespace brave_news::knobs

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_V2_KNOBS_H_
