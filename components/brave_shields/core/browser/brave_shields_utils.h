// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_BRAVE_SHIELDS_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_BRAVE_SHIELDS_UTILS_H_

#include "base/containers/span.h"
#include "brave/components/brave_shields/core/common/brave_shields_panel.mojom-data-view.h"
#include "brave/components/brave_shields/core/common/brave_shields_settings_values.h"
#include "brave/components/brave_shields/core/common/shields_settings.mojom.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_types.h"

class GURL;
class HostContentSettingsMap;
class PrefService;

namespace brave_shields {

ContentSettingsPattern GetPatternFromURL(const GURL& url);

bool IsBraveShieldsManaged(PrefService* prefs,
                           HostContentSettingsMap* map,
                           GURL url);

bool IsShowStrictFingerprintingModeEnabled();

// Enables a webcompat exception for a specific URL.
void SetWebcompatEnabled(HostContentSettingsMap* map,
                         ContentSettingsType webcompat_settings_type,
                         bool enabled,
                         const GURL& url,
                         PrefService* local_state);

bool IsWebcompatEnabled(HostContentSettingsMap* map,
                        ContentSettingsType webcompat_settings_type,
                        const GURL& url);

bool IsDeveloperModeEnabled(PrefService* profile_state);

void SetAllowElementBlockerInPrivateModeEnabled(PrefService* local_state,
                                                bool value);
bool GetAllowElementBlockerInPrivateModeEnabled(PrefService* local_state);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_BRAVE_SHIELDS_UTILS_H_
