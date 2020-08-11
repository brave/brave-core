/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_STATIC_VALUES_H_
#define BRAVELEDGER_STATIC_VALUES_H_

#include <string>
#include <vector>

#define PREFIX_V1                       "/v1"
#define PREFIX_V2                       "/v2"
#define PREFIX_V3                       "/v3"
#define PREFIX_V4                       "/v4"
#define PREFIX_V5                       "/v5"

#define SIGNATURE_ALGORITHM             "ed25519"

#define YOUTUBE_MEDIA_TYPE              "youtube"
#define TWITCH_MEDIA_TYPE               "twitch"
#define TWITTER_MEDIA_TYPE              "twitter"
#define REDDIT_MEDIA_TYPE               "reddit"
#define VIMEO_MEDIA_TYPE                "vimeo"
#define GITHUB_MEDIA_TYPE               "github"
#define YOUTUBE_PROVIDER_URL            "https://www.youtube.com/oembed"
#define TWITCH_PROVIDER_URL             "https://api.twitch.tv/v5/oembed"
#define VIMEO_PROVIDER_URL              "https://vimeo.com/api/oembed.json"
#define YOUTUBE_TLD                     "youtube.com"
#define TWITCH_TLD                      "twitch.tv"
#define TWITTER_TLD                     "twitter.com"
#define REDDIT_TLD                      "reddit.com"
#define VIMEO_TLD                       "vimeo.com"
#define GITHUB_TLD                      "github.com"
#define TWITCH_VOD_URL                  "https://www.twitch.tv/videos/"
#define REDDIT_USER_URL                 "https://www.reddit.com/user/"
#define OLD_REDDIT_DOMAIN               "old.reddit.com"
#define MEDIA_DELIMITER                 "_"

#define TWITCH_MINIMUM_SECONDS          10
#define TWITCH_MAXIMUM_SECONDS_CHUNK    120

namespace braveledger_ledger {

// 30 days in seconds
static const uint64_t _reconcile_default_interval = 30 * 24 * 60 * 60;

// 1 day in seconds
static const uint64_t _promotion_load_interval = 24 * 60 * 60;

// pending contribution expiration in seconds (90 days)
static const uint64_t _pending_contribution_expiration = 90 * 24 * 60 * 60;

static const std::vector<std::string> _add_funds_limited_countries = {
  "JP"
};

static const double _vote_price = 0.25;

}  // namespace braveledger_ledger

#endif  // BRAVELEDGER_STATIC_VALUES_H_
