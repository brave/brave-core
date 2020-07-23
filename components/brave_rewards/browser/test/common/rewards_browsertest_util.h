/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_COMMON_REWARDS_BROWSERTEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_COMMON_REWARDS_BROWSERTEST_UTIL_H_

#include <string>

#include "base/files/file_path.h"
#include "bat/ledger/internal/request/request_util.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "chrome/browser/ui/browser.h"
#include "net/test/embedded_test_server/default_handlers.h"
#include "url/gurl.h"

namespace rewards_browsertest_util {

enum class ContributionType { OneTimeTip, MonthlyTip };

void GetTestDataDir(base::FilePath* test_data_dir);

double IsRewardsEnabled(Browser* browser, const bool private_window = false);

GURL GetRewardsUrl();

GURL GetRewardsInternalsUrl();

GURL GetNewTabUrl();

void EnableRewardsViaCode(
    Browser* browser,
    brave_rewards::RewardsServiceImpl* rewards_service);

GURL GetUrl(
    net::EmbeddedTestServer* https_server,
    const std::string& publisher_key,
    const std::string& path = "");

void ActivateTabAtIndex(Browser* browser, const int index);

std::string BalanceDoubleToString(double amount);

std::string GetUpholdExternalAddress();

void NavigateToPublisherPage(
    Browser* browser,
    net::EmbeddedTestServer* https_server,
    const std::string& publisher_key,
    const std::string& path = "");

}  // namespace rewards_browsertest_util

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_COMMON_REWARDS_BROWSERTEST_UTIL_H_
