/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PSST_BROWSER_CORE_BRAVE_PSST_UTILS_H_
#define BRAVE_COMPONENTS_PSST_BROWSER_CORE_BRAVE_PSST_UTILS_H_

#include "brave/components/psst/common/psst_metadata_schema.h"
#include "brave/components/psst/common/psst_ui_common.mojom-forward.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "url/origin.h"

namespace psst {

// Saves the PSST metadata for the (origin, user_id) pair with the given
// details.
void SetPsstMetadata(HostContentSettingsMap* map,
                     const url::Origin& origin,
                     ConsentStatus consent_status,
                     int script_version,
                     std::string_view user_id,
                     base::Value::List urls_to_skip);

// Returns the PSST permission info for the (origin, user_id) pair if exists
std::optional<PsstMetadata> GetPsstMetadata(HostContentSettingsMap* map,
                                            const url::Origin& origin,
                                            std::string_view user_id);

void SetPsstMetadata(HostContentSettingsMap* map,
                     const url::Origin& origin,
                     PsstMetadata psst_metadata);
}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CORE_BRAVE_PSST_UTILS_H_
