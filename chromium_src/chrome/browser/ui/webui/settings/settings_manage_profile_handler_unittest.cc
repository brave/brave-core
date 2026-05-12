/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// TI-040: include upstream tests so overrides keep passing upstream coverage,
// then add Brave-specific assertions.
#include "base/functional/callback_helpers.h"
#include "content/public/test/test_utils.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_unittest_util.h"

#include <chrome/browser/ui/webui/settings/settings_manage_profile_handler_unittest.cc>

namespace settings {

// Selecting the GAIA avatar path must deactivate a saved Brave custom avatar
// (macro `BRAVE_HANDLE_SET_PROFILE_ICON_TO_GAIA_AVATAR_END` in the chromium_src
// wrapper for `settings_manage_profile_handler.cc`).
TEST_F(ManageProfileHandlerTest,
       SelectingGaiaAvatarDeactivatesBraveCustomAvatar) {
  gfx::Image custom(gfx::test::CreateImage(32, 32, SK_ColorGREEN));
  entry()->SetBraveCustomAvatar(custom, base::DoNothing());
  content::RunAllTasksUntilIdle();

  ASSERT_TRUE(entry()->HasBraveCustomAvatar());
  ASSERT_TRUE(entry()->IsUsingBraveCustomAvatar());

  // Exercise the real message path (handler method is private; only
  // `HandleSetProfileIconToGaiaAvatar` is friended to the upstream-named test).
  web_ui()->HandleReceivedMessage("setProfileIconToGaiaAvatar",
                                  base::ListValue());

  EXPECT_TRUE(entry()->HasBraveCustomAvatar());
  EXPECT_FALSE(entry()->IsUsingBraveCustomAvatar());
}

}  // namespace settings
