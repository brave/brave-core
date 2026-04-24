// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/constants/webui_url_constants.h"

#include <components/webapps/browser/web_app_url_config_unittest.cc>

namespace webapps {

#if BUILDFLAG(ENABLE_AI_CHAT)
TEST(WebAppUrlConfigTest, Brave_IsUrlEligibleForWebApp) {
  EXPECT_TRUE(IsUrlEligibleForWebApp(GURL("chrome://leo-ai")));
}
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

}  // namespace webapps
