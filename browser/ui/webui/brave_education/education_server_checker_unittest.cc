/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_education/education_server_checker.h"

#include <memory>
#include <optional>

#include "base/test/test_future.h"
#include "brave/components/brave_education/education_urls.h"
#include "brave/components/brave_education/features.h"
#include "components/language/core/browser/language_prefs.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_education {

class EducationServerCheckerTest : public testing::Test {
 protected:
  void SetUp() override {
    language::LanguagePrefs::RegisterProfilePrefs(prefs_.registry());
  }

  void AddSuccessResponse(EducationPageType page_type) {
    auto url = GetEducationPageServerURL(page_type);
    test_url_loader_factory_.AddResponse(url.spec(), "success");
  }

  void AddErrorResponse(EducationPageType page_type) {
    auto url = GetEducationPageServerURL(page_type);
    test_url_loader_factory_.AddResponse(url.spec(), "error",
                                         net::HTTP_NOT_FOUND);
  }

  std::unique_ptr<EducationServerChecker> CreateChecker() {
    return std::make_unique<EducationServerChecker>(
        prefs_, test_url_loader_factory_.GetSafeWeakWrapper());
  }

  content::BrowserTaskEnvironment task_environment_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
};

TEST_F(EducationServerCheckerTest, SuccessResponse) {
  AddSuccessResponse(EducationPageType::kGettingStarted);

  base::test::TestFuture<bool> future;
  auto checker = CreateChecker();
  checker->IsServerPageAvailable(EducationPageType::kGettingStarted,
                                 future.GetCallback());
  ASSERT_TRUE(future.Wait());
  ASSERT_TRUE(future.Get());
}

TEST_F(EducationServerCheckerTest, BadResponse) {
  AddErrorResponse(EducationPageType::kGettingStarted);

  base::test::TestFuture<bool> future;
  auto checker = CreateChecker();
  checker->IsServerPageAvailable(EducationPageType::kGettingStarted,
                                 future.GetCallback());
  ASSERT_TRUE(future.Wait());
  ASSERT_FALSE(future.Get());
}

}  // namespace brave_education
