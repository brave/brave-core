/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/feature_list.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/password_manager/core/common/password_manager_features.h"

using BraveFeaturesBrowserTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveFeaturesBrowserTest, AutoFillPasswordDefault) {
  EXPECT_TRUE(
    base::FeatureList::IsEnabled(password_manager::features::kFillOnAccountSelect));
}
