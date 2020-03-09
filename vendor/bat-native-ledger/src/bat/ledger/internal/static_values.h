/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_STATIC_VALUES_H_
#define BRAVELEDGER_STATIC_VALUES_H_

#include <cstdint>
#include <string>
#include <vector>

#define LEDGER_STAGING_SERVER               \
"https://ledger-staging.mercury.basicattentiontoken.org"
#define LEDGER_PRODUCTION_SERVER            \
"https://ledger.mercury.basicattentiontoken.org"
#define LEDGER_DEVELOPMENT_SERVER            \
"https://ledger.rewards.brave.software"

#define BALANCE_STAGING_SERVER              \
"https://balance-staging.mercury.basicattentiontoken.org"
#define BALANCE_PRODUCTION_SERVER           \
"https://balance.mercury.basicattentiontoken.org"
#define BALANCE_DEVELOPMENT_SERVER            \
"https://balance.rewards.brave.software"

#define PUBLISHER_STAGING_SERVER            \
"https://publishers-staging.basicattentiontoken.org"
#define PUBLISHER_PRODUCTION_SERVER         \
"https://publishers.basicattentiontoken.org"
#define PUBLISHER_DEVELOPMENT_SERVER         \
"https://creators.brave.software"

#define PUBLISHER_DISTRO_STAGING_SERVER     \
"https://publishers-staging-distro.basicattentiontoken.org"
#define PUBLISHER_DISTRO_PRODUCTION_SERVER  \
"https://publishers-distro.basicattentiontoken.org"
#define PUBLISHER_DISTRO_DEVELOPMENT_SERVER  \
"https://creators-distro.brave.software"

#define PROMOTION_STAGING_SERVER              \
"https://grant.rewards.bravesoftware.com"
#define PROMOTION_PRODUCTION_SERVER           \
"https://grant.rewards.brave.com"
#define PROMOTION_DEVELOPMENT_SERVER            \
"https://grant.rewards.brave.software"

#define PREFIX_V1                       "/v1"
#define PREFIX_V2                       "/v2"
#define PREFIX_V3                       "/v3"
#define PREFIX_V4                       "/v4"
#define PREFIX_V5                       "/v5"
#define REGISTER_PERSONA                "/registrar/persona"
#define WALLET_PROPERTIES               "/wallet/"
#define WALLET_PROPERTIES_END           "/balance"
#define RECOVER_WALLET_PUBLIC_KEY       "/wallet?publicKey="
#define GET_PROMOTION_CAPTCHA           "/captchas/"
#define GET_PUBLISHERS_LIST             "/api/v3/public/channels"
#define GET_PROMOTION_ATTESTATION       "/attestations/"

#define REGISTRARVK_FIELDNAME           "registrarVK"
#define VERIFICATION_FIELDNAME          "verification"
#define SURVEYOR_ID                     "surveyorId"
#define SURVEYOR_IDS                    "surveyorIds"

#define LEDGER_CURRENCY                 "BAT"

#define SIGNATURE_ALGORITHM             "ed25519"

#define LEDGER_STATE_FILENAME           "6e16793f-52e1-41fb-b6a2-24b99b47e8f8"
#define LEDGER_PUBLISHER_STATE_FILENAME "ee1e6705-bc4f-4aba-b03c-57cc8cb2ae4d"
#define PUBLISHERS_DB_NAME              "d2c799cd-f37f-4230-9a04-ca23ba5be240"
#define MEDIA_CACHE_DB_NAME             "9956db89-9105-420b-9ad4-49348bc536af"

// TODO(samartnik): remove as soon as we move on new safetynet check API
#define SAFETYNET_ERROR_MESSAGE         \
"Error profile and integrity must both be true"
#define SAFETYNET_ERROR_CODE            422

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
#define WALLET_PASSPHRASE_DELIM         " "
#define DICTIONARY_DELIMITER            ","
#define NICEWARE_BYTES_WRITTEN          32

#define SEED_LENGTH                     32
#define SALT_LENGTH                     64

#define INVALID_LEGACY_WALLET           -1

#define TWITCH_MINIMUM_SECONDS          10
#define TWITCH_MAXIMUM_SECONDS_CHUNK    120

#define VOTE_BATCH_SIZE                 10

namespace braveledger_ledger {

static const uint8_t g_hkdfSalt[] = {
    126, 244, 99, 158, 51, 68, 253, 80, 133, 183, 51, 180, 77,
    62, 74, 252, 62, 106, 96, 125, 241, 110, 134, 87, 190, 208,
    158, 84, 125, 69, 246, 207, 162, 247, 107, 172, 37, 34, 53,
    246, 105, 20, 215, 5, 248, 154, 179, 191, 46, 17, 6, 72, 210,
    91, 10, 169, 145, 248, 22, 147, 117, 24, 105, 12};

static const double _d = 1.0 / (30.0 * 1000.0);

static const uint64_t _default_min_page_time_before_logging_a_visit = 8;

static const uint64_t _milliseconds_day = 24 * 60 * 60 * 1000;

static const uint64_t _milliseconds_hour = 60 * 60 * 1000;

static const uint64_t _milliseconds_minute = 60 * 1000;

static const uint64_t _milliseconds_second = 1000;

// 30 days in seconds
static const uint64_t _reconcile_default_interval = 30 * 24 * 60 * 60;

// 1 day in seconds
static const uint64_t _promotion_load_interval = 24 * 60 * 60;

// pending contribution expiration in seconds (90 days)
static const uint64_t _pending_contribution_expiration = 90 * 24 * 60 * 60;

static const std::vector<std::string> _add_funds_limited_countries = {
  "JP"
};

}  // namespace braveledger_ledger

#endif  // BRAVELEDGER_STATIC_VALUES_H_
