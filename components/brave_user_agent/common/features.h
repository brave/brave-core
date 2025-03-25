// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_USER_AGENT_COMMON_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_USER_AGENT_COMMON_FEATURES_H_

#include "base/component_export.h"
#include "base/feature_list.h"

namespace brave_user_agent {
namespace features {

COMPONENT_EXPORT(BRAVE_USER_AGENT_COMMON)
BASE_DECLARE_FEATURE(kUseBraveUserAgent);

bool IsUseBraveUserAgentEnabled();

}  // namespace features
}  // namespace brave_user_agent

#endif  // BRAVE_COMPONENTS_BRAVE_USER_AGENT_COMMON_FEATURES_H_
