// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "src/chrome/browser/web_applications/web_app_helpers_unittest.cc"

namespace web_app {

TEST(WebAppHelpers, Brave_IsValidWebAppUrl) {
  EXPECT_TRUE(IsValidWebAppUrl(GURL("chrome://leo-ai")));
}

}  // namespace web_app
