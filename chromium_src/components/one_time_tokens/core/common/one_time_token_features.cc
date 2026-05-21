/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_override.h"

#include <components/one_time_tokens/core/common/one_time_token_features.cc>

namespace one_time_tokens::features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kGmailOtpRetrievalService, base::FEATURE_ENABLED_BY_DEFAULT},
}});

}  // namespace one_time_tokens::features
