/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef BRAVELEDGER_STATIC_VALUES_H_
#define BRAVELEDGER_STATIC_VALUES_H_

#include <cstdint>
#include <string>

#define LEDGER_STAGING_SERVER           "https://ledger-staging.mercury.basicattentiontoken.org"
#define LEDGER_PRODUCTION_SERVER        "https://ledger.mercury.basicattentiontoken.org"
#define BALANCE_STAGING_SERVER          "https://balance-staging.mercury.basicattentiontoken.org"
#define BALANCE_PRODUCTION_SERVER       "https://balance.mercury.basicattentiontoken.org"

#define PREFIX_V2                       "/v2"
#define PREFIX_V3                       "/v3"
#define REGISTER_PERSONA                "/registrar/persona"
#define REGISTER_VIEWING                "/registrar/viewing"
#define WALLET_PROPERTIES               "/wallet/"
#define WALLET_PROPERTIES_END           "/balance"
#define RECONCILE_CONTRIBUTION          "/surveyor/contribution/current/"
#define SURVEYOR_VOTING                 "/surveyor/voting/"
#define SURVEYOR_BATCH_VOTING           "/batch/surveyor/voting"
#define UPDATE_RULES_V1                 "/v1/publisher/ruleset?consequential=true"
#define UPDATE_RULES_V2                 "/v2/publisher/ruleset?limit=512&excludedOnly=false"
#define RECOVER_WALLET_PUBLIC_KEY       "/v2/wallet?publicKey="
#define RECOVER_WALLET                  "/v2/wallet/"
#define GET_SET_PROMOTION                "/v1/grants"
#define GET_PROMOTION_CAPTCHA            "/v1/captchas/"

#define REGISTRARVK_FIELDNAME           "registrarVK"
#define VERIFICATION_FIELDNAME          "verification"
#define SURVEYOR_ID                     "surveyorId"
#define SURVEYOR_IDS                    "surveyorIds"

#define CURRENCY                        "BAT"

#define SIGNATURE_ALGORITHM             "ed25519"

#define AD_FREE_SETTINGS                "adFree"

#define LEDGER_STATE_FILENAME           "6e16793f-52e1-41fb-b6a2-24b99b47e8f8"
#define LEDGER_PUBLISHER_STATE_FILENAME "ee1e6705-bc4f-4aba-b03c-57cc8cb2ae4d"
#define PUBLISHERS_DB_NAME              "d2c799cd-f37f-4230-9a04-ca23ba5be240"
#define MEDIA_CACHE_DB_NAME             "9956db89-9105-420b-9ad4-49348bc536af"

#define YOUTUBE_MEDIA_TYPE              "youtube"
#define TWITCH_MEDIA_TYPE                "twitch"
#define YOUTUBE_PROVIDER_NAME            "YouTube"
#define YOUTUBE_PROVIDER_URL            "https://www.youtube.com/oembed"

#define SEED_LENGTH                     32
#define SALT_LENGTH                     64

#define TWITCH_MINIMUM_SECONDS          10
#define TWITCH_MAXIMUM_SECONDS_CHUNK    120

#define VOTE_BATCH_SIZE                 10

namespace braveledger_ledger {

static const bool g_isProduction = false;
static const uint8_t g_hkdfSalt[] = {126, 244, 99, 158, 51, 68, 253, 80, 133, 183, 51, 180, 77,
  62, 74, 252, 62, 106, 96, 125, 241, 110, 134, 87, 190, 208,
  158, 84, 125, 69, 246, 207, 162, 247, 107, 172, 37, 34, 53,
  246, 105, 20, 215, 5, 248, 154, 179, 191, 46, 17, 6, 72, 210,
  91, 10, 169, 145, 248, 22, 147, 117, 24, 105, 12};

static const double _d = 1.0 / (30.0 * 1000.0);
static const unsigned int _default_min_pubslisher_duration = 8000;  // In milliseconds

static const uint64_t _milliseconds_day = 24 * 60 * 60 * 1000;
static const uint64_t _milliseconds_hour = 60 * 60 * 1000;
static const uint64_t _milliseconds_minute = 60 * 1000;
static const uint64_t _milliseconds_second = 1000;

static const unsigned int _twitch_events_array_size = 8;
// Important: set _twitch_events_array_size as a correct array size when you modify items in _twitch_events
static const std::string _twitch_events[] = {"buffer-empty", "buffer-refill", "video_end",
  "minute-watched", "video_pause", "player_click_vod_seek", "video-play", "video_error"};

}  // namespace braveledger_ledger

#endif  // BRAVELEDGER_STATIC_VALUES_H_
