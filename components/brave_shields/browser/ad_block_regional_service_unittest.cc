/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_regional_service.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(AdBlockRegionalServiceTest, SupportedLocales) {
  std::vector<std::string> locales({ "fr", "fR", "fr-FR", "fr-ca" });
    std::for_each(locales.begin(), locales.end(),
      [this](const std::string& locale){
    EXPECT_TRUE(brave_shields::AdBlockRegionalService::IsSupportedLocale(locale));
  });
}
