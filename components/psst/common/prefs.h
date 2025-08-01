// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_COMMON_PREFS_H_
#define BRAVE_COMPONENTS_PSST_COMMON_PREFS_H_

#include <optional>
#include <string>

#include "base/values.h"
#include "brave/components/psst/common/psst_common.h"

class PrefService;

namespace psst::prefs {

std::optional<ConsentStatus> GetConsentStatus(const std::string& name,
                                              const std::string& user_id,
                                              const PrefService& prefs);

std::optional<int> GetScriptVersion(const std::string& name,
                                    const std::string& user_id,
                                    const PrefService& prefs);

std::optional<base::Value::List> GetUrlsToSkip(const std::string& name,
                                               const std::string& user_id,
                                               const PrefService& prefs);

void SetPsstSettings(const std::string& name,
                     const std::string& user_id,
                     std::optional<ConsentStatus> consent_status,
                     std::optional<int> script_version,
                     std::optional<base::Value::List> urls_to_skip,
                     PrefService& prefs);
}  // namespace psst::prefs

#endif  // BRAVE_COMPONENTS_PSST_COMMON_PREFS_H_
