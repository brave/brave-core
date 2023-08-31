// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_V2_KNOBS_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_V2_KNOBS_H_

namespace brave_news::knobs {
namespace switches {

extern const char kBraveNewsMinBlockCards[];
extern const char kBraveNewsMaxBlockCards[];

extern const char kBraveNewsPopRecencyHalfLife[];
extern const char kBraveNewsPopRecencyFallback[];

extern const char kBraveNewsInlineDiscoveryRatio[];

extern const char kBraveNewsSourceSubscribedMin[];
extern const char kBraveNewsSourceSubscribedBoost[];
extern const char kBraveNewsChannelSubscribedBoost[];

extern const char kBraveNewsSourceVisitsMin[];

}  // namespace knobs::switches

int GetMinBlockCards();
int GetMaxBlockCards();

double GetPopRecencyHalfLife();
double GetPopRecencyFallback();

double GetInlineDiscoveryRatio();

double GetSourceSubscribedMin();
double GetSourceSubscribedBoost();
double GetChannelSubscribedBoost();

double GetSourceVisitsMin();

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_V2_KNOBS_H_
