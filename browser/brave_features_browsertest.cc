/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/feature_list.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/autofill/core/common/autofill_features.h"
#include "components/unified_consent/feature.h"
#include "content/public/common/content_features.h"

using BraveFeaturesBrowserTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveFeaturesBrowserTest, AutoFillServerCommunication) {
  EXPECT_FALSE(base::FeatureList::IsEnabled(
      autofill::features::kAutofillServerCommunication));
  EXPECT_FALSE(
      base::FeatureList::IsEnabled(features::kAudioServiceOutOfProcess));
}

IN_PROC_BROWSER_TEST_F(BraveFeaturesBrowserTest, UnifiedConsent) {
  EXPECT_FALSE(base::FeatureList::IsEnabled(unified_consent::kUnifiedConsent));
}
