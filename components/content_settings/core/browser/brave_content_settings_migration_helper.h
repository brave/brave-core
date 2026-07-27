/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_CONTENT_SETTINGS_MIGRATION_HELPER_H_
#define BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_CONTENT_SETTINGS_MIGRATION_HELPER_H_

#include <memory>
#include <vector>

#include "components/content_settings/core/common/content_settings_types.h"

namespace content_settings {

class PrefProvider;
struct Rule;

namespace brave_content_settings_migration {

// Returns JAVASCRIPT rules that were authored by Shields and should be migrated
// to BRAVE_JAVASCRIPT. User-authored Chromium Site Settings exceptions remain
// on JAVASCRIPT.
std::vector<std::unique_ptr<Rule>> GetShieldsAuthoredJavascriptRules(
    PrefProvider& provider);

}  // namespace brave_content_settings_migration

}  // namespace content_settings

#endif  // BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_CONTENT_SETTINGS_MIGRATION_HELPER_H_
