/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_EMBEDDER_SUPPORT_USER_AGENT_UTILS_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_EMBEDDER_SUPPORT_USER_AGENT_UTILS_H_

#define GetUserAgentMetadata GetUserAgentMetadata_ChromiumImpl
#include "src/components/embedder_support/user_agent_utils.h"  // IWYU pragma: export
#undef GetUserAgentMetadata

namespace embedder_support {

blink::UserAgentMetadata GetUserAgentMetadata(bool only_low_entropy_ch = false);

blink::UserAgentMetadata GetUserAgentMetadata(const PrefService* pref_service,
                                              bool only_low_entropy_ch = false);

}  // namespace embedder_support

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_EMBEDDER_SUPPORT_USER_AGENT_UTILS_H_
