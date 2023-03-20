/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_TARGETING_USER_MODEL_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_TARGETING_USER_MODEL_BUILDER_H_

#include "base/functional/callback.h"
#include "brave/components/brave_ads/core/internal/processors/contextual/text_embedding/text_embedding_html_event_info.h"

namespace brave_ads::targeting {

struct UserModelInfo;

using BuildUserModelCallback =
    base::OnceCallback<void(const targeting::UserModelInfo& user_model)>;

void BuildUserModel(BuildUserModelCallback callback);

}  // namespace brave_ads::targeting

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_TARGETING_USER_MODEL_BUILDER_H_
