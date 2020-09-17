  /* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PRIVATE_CHANNEL_BROWSER_STATIC_VALUES_H_
#define BRAVE_COMPONENTS_PRIVATE_CHANNEL_BROWSER_STATIC_VALUES_H_

#include <stdint.h>
#include <string>

const int kMaxPrivateChannelServerResponseSizeBytes = 1024 * 1024;

#define PRIVATE_CHANNEL_STAGING_SERVER     \
"https://repsys.rewards.brave.software"
#define PRIVATE_CHANNEL_PRODUCTION_SERVER   \
"https://repsys.rewards.brave.com"
#define PRIVATE_CHANNEL_DEVELOPMENT_SERVER   \
"https://repsys.rewards.brave.software"

#define PRIVATE_CHANNEL_API_VERSION "/private-channel/v1"
#define PRIVATE_CHANNEL_META_ENDPOINT "/meta"
#define PRIVATE_CHANNEL_FIRST_ROUND_ENDPOINT "/attestation/start"
#define PRIVATE_CHANNEL_SECOND_ROUND_ENDPOINT "/attestation/result"

#endif  // BRAVE_COMPONENTS_PRIVATE_CHANNEL_BROWSER_STATIC_VALUES_H_
