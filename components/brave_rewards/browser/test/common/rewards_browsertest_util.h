/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_COMMON_REWARDS_BROWSERTEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_COMMON_REWARDS_BROWSERTEST_UTIL_H_

#include <optional>
#include <string>

#include "base/files/file_path.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

class Browser;
class Profile;

namespace brave_rewards::test_util {

void GetTestDataDir(base::FilePath* test_data_dir);

GURL GetRewardsUrl();

GURL GetRewardsInternalsUrl();

GURL GetNewTabUrl();

void StartProcess(RewardsServiceImpl* rewards_service);

void StartProcessWithConnectedUser(Profile* profile);

GURL GetUrl(
    net::EmbeddedTestServer* https_server,
    const std::string& publisher_key,
    const std::string& path = "");

void ActivateTabAtIndex(Browser* browser, const int index);

std::string BalanceDoubleToString(double amount);

std::string GetUpholdExternalAddress();

std::string GetGeminiExternalAddress();

void NavigateToPublisherPage(
    Browser* browser,
    net::EmbeddedTestServer* https_server,
    const std::string& publisher_key,
    const std::string& path = "");

void NavigateToPublisherAndWaitForUpdate(Browser* browser,
                                         net::EmbeddedTestServer* https_server,
                                         const std::string& publisher_key);

void WaitForEngineStop(RewardsServiceImpl* rewards_service);

void WaitForAutoContributeVisitTime();

void CreateRewardsWallet(RewardsServiceImpl* rewards_service,
                         const std::string& country = "US");

void SetOnboardingBypassed(Browser* browser, bool bypassed = true);

std::optional<std::string> EncryptPrefString(const std::string& value);

}  // namespace brave_rewards::test_util

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_COMMON_REWARDS_BROWSERTEST_UTIL_H_
