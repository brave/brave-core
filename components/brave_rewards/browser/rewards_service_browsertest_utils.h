/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef REWARDS_SERVICE_BROWSERTEST_UTILS_H_
#define REWARDS_SERVICE_BROWSERTEST_UTILS_H_

#include <string>
#include "build/build_config.h"


namespace content {
class WebContents;
} // namespace content

namespace rewards_service_browsertest_utils {

void WaitForElementToAppear(content::WebContents*,
    const std::string& selector);

void WaitForElementToEqual(content::WebContents*,
    const std::string& selector, const std::string& expectedValue);

void WaitForElementToContain(content::WebContents*,
    const std::string& selector, const std::string& substring);

void WaitForElementThenClick(content::WebContents*,
    const std::string& selector);

} // namespace rewards_service_browsertest_utils

#endif // REWARDS_SERVICE_BROWSERTEST_UTILS_H_
