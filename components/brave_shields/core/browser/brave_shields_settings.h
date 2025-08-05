// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_BRAVE_SHIELDS_SETTINGS_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_BRAVE_SHIELDS_SETTINGS_H_

#include "brave/components/brave_shields/core/common/brave_shields_panel.mojom.h"
#include "brave/components/brave_shields/core/common/shields_settings.mojom.h"
#include "components/content_settings/core/browser/cookie_settings.h"

class GURL;
class HostContentSettingsMap;
class PrefService;

namespace brave_shields {

void SetIsBraveShieldsEnabled(HostContentSettingsMap* map,
                              bool enable,
                              const GURL& url,
                              PrefService* local_state = nullptr);
bool GetIsBraveShieldsEnabled(HostContentSettingsMap* map, const GURL& url);

void SetDefaultAdBlockMode(HostContentSettingsMap* map,
                           mojom::AdBlockMode mode,
                           PrefService* local_state = nullptr,
                           PrefService* profile_state = nullptr);
mojom::AdBlockMode GetDefaultAdBlockMode(HostContentSettingsMap* map);

void SetAdBlockMode(HostContentSettingsMap* map,
                    mojom::AdBlockMode mode,
                    const GURL& url,
                    PrefService* local_state = nullptr,
                    PrefService* profile_state = nullptr);
mojom::AdBlockMode GetAdBlockMode(HostContentSettingsMap* map, const GURL& url);

void SetDefaultFingerprintMode(HostContentSettingsMap* map,
                               mojom::FingerprintMode mode,
                               PrefService* local_state = nullptr,
                               PrefService* profile_state = nullptr);
mojom::FingerprintMode GetDefaultFingerprintMode(HostContentSettingsMap* map);

void SetFingerprintMode(HostContentSettingsMap* map,
                        mojom::FingerprintMode mode,
                        const GURL& url,
                        PrefService* local_state = nullptr,
                        PrefService* profile_state = nullptr);
mojom::FingerprintMode GetFingerprintMode(HostContentSettingsMap* map,
                                          const GURL& url);

void SetIsNoScriptEnabledByDefault(HostContentSettingsMap* map,
                                   bool is_enabled,
                                   PrefService* local_state = nullptr);
bool GetNoScriptEnabledByDefault(HostContentSettingsMap* map);

void SetIsNoScriptEnabled(HostContentSettingsMap* map,
                          bool is_enabled,
                          const GURL& url,
                          PrefService* local_state = nullptr);
bool GetNoScriptEnabled(HostContentSettingsMap* map, const GURL& url);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_BRAVE_SHIELDS_SETTINGS_H_
