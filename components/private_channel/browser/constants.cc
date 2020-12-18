/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/private_channel/browser/constants.h"

const int kMaxPrivateChannelServerResponseSizeBytes = 1024 * 1024;
const char kPrivateChannelVersion[] =
    "30ee124d76368c52339c8d965d3a07c1db66a555";

extern const char PRIVATE_CHANNEL_STAGING_SERVER[] =
    "https://repsys.rewards.brave.software";
extern const char PRIVATE_CHANNEL_PRODUCTION_SERVER[] =
    "https://repsys.rewards.brave.com";
extern const char PRIVATE_CHANNEL_DEVELOPMENT_SERVER[] = "http://0.0.0.0:80";

extern const char PRIVATE_CHANNEL_API_VERSION[] = "/private-channel/v1";
extern const char PRIVATE_CHANNEL_META_ENDPOINT[] = "/meta";
extern const char PRIVATE_CHANNEL_FIRST_ROUND_ENDPOINT[] = "/attestation/start";
extern const char PRIVATE_CHANNEL_SECOND_ROUND_ENDPOINT[] =
    "/attestation/result";
