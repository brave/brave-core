/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SEARCH_ENGINES_NTP_SEARCH_ENGINE_UTILS_H_
#define BRAVE_COMPONENTS_SEARCH_ENGINES_NTP_SEARCH_ENGINE_UTILS_H_

#include <string_view>

class TemplateURL;
class TemplateURLService;

namespace ntp_search {

bool IsEligibleNtpSearchEngine(const TemplateURL& engine,
                               const TemplateURLService& service);

// Resolves persisted widget IDs, rejecting engines no longer eligible for use.
TemplateURL* FindNtpSearchEngine(std::string_view id,
                                 TemplateURLService& service);

}  // namespace ntp_search

#endif  // BRAVE_COMPONENTS_SEARCH_ENGINES_NTP_SEARCH_ENGINE_UTILS_H_
