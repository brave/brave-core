// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/brave_shields_settings.h"

#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/brave_shields/core/common/brave_shield_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

namespace brave_shields {

void SetIsBraveShieldsEnabled(HostContentSettingsMap* map,
                              bool is_enabled,
                              const GURL& url,
                              PrefService* local_state) {
  brave_shields::SetBraveShieldsEnabled(map, is_enabled, url, local_state);
}

bool GetIsBraveShieldsEnabled(HostContentSettingsMap* map, const GURL& url) {
  return brave_shields::GetBraveShieldsEnabled(map, url);
}

void SetDefaultAdBlockMode(HostContentSettingsMap* map,
                           mojom::AdBlockMode mode,
                           PrefService* local_state,
                           PrefService* profile_state) {
  SetAdBlockMode(map, mode, GURL(), local_state, profile_state);
}

mojom::AdBlockMode GetDefaultAdBlockMode(HostContentSettingsMap* map) {
  return GetAdBlockMode(map, GURL());
}

void SetAdBlockMode(HostContentSettingsMap* map,
                    mojom::AdBlockMode mode,
                    const GURL& url,
                    PrefService* local_state,
                    PrefService* profile_state) {
  ControlType control_type_ad;
  ControlType control_type_cosmetic;

  if (mode == mojom::AdBlockMode::ALLOW) {
    control_type_ad = ControlType::ALLOW;
  } else {
    control_type_ad = ControlType::BLOCK;
  }

  if (mode == mojom::AdBlockMode::AGGRESSIVE) {
    control_type_cosmetic = ControlType::BLOCK;  // aggressive
  } else if (mode == mojom::AdBlockMode::STANDARD) {
    control_type_cosmetic = ControlType::BLOCK_THIRD_PARTY;  // standard
  } else {
    control_type_cosmetic = ControlType::ALLOW;  // allow
  }

  brave_shields::SetAdControlType(map, control_type_ad, url, local_state);

  brave_shields::SetCosmeticFilteringControlType(
      map, control_type_cosmetic, url, local_state, profile_state);
}

mojom::AdBlockMode GetAdBlockMode(HostContentSettingsMap* map,
                                  const GURL& url) {
  ControlType control_type_ad = brave_shields::GetAdControlType(map, url);

  ControlType control_type_cosmetic =
      brave_shields::GetCosmeticFilteringControlType(map, url);

  if (control_type_ad == ControlType::ALLOW) {
    return mojom::AdBlockMode::ALLOW;
  }

  if (control_type_cosmetic == ControlType::BLOCK) {
    return mojom::AdBlockMode::AGGRESSIVE;
  } else {
    return mojom::AdBlockMode::STANDARD;
  }
}

void SetDefaultFingerprintMode(HostContentSettingsMap* map,
                               mojom::FingerprintMode mode,
                               PrefService* local_state,
                               PrefService* profile_state) {
  SetFingerprintMode(map, mode, GURL(), local_state, profile_state);
}

mojom::FingerprintMode GetDefaultFingerprintMode(HostContentSettingsMap* map) {
  return GetFingerprintMode(map, GURL());
}

void SetFingerprintMode(HostContentSettingsMap* map,
                        mojom::FingerprintMode mode,
                        const GURL& url,
                        PrefService* local_state,
                        PrefService* profile_state) {
#if !BUILDFLAG(IS_IOS)
  /// Strict FingerprintMode is not supported on iOS
  DCHECK(mode != mojom::FingerprintMode::STRICT_MODE);
#endif

  ControlType control_type;

  if (mode == mojom::FingerprintMode::ALLOW_MODE) {
    control_type = ControlType::ALLOW;
  } else if (mode == mojom::FingerprintMode::STRICT_MODE) {
    control_type = ControlType::BLOCK;
  } else {
    control_type = ControlType::DEFAULT;  // STANDARD_MODE
  }

  brave_shields::SetFingerprintingControlType(map, control_type, url,
                                              local_state, profile_state);
}

mojom::FingerprintMode GetFingerprintMode(HostContentSettingsMap* map,
                                          const GURL& url) {
  ControlType control_type =
      brave_shields::GetFingerprintingControlType(map, url);

  if (control_type == ControlType::ALLOW) {
    return mojom::FingerprintMode::ALLOW_MODE;
  } else if (control_type == ControlType::BLOCK) {
    return mojom::FingerprintMode::STRICT_MODE;
  } else {
    return mojom::FingerprintMode::STANDARD_MODE;
  }
}

void SetIsNoScriptEnabledByDefault(HostContentSettingsMap* map,
                                   bool is_enabled,
                                   PrefService* local_state) {
  SetIsNoScriptEnabled(map, is_enabled, GURL(), local_state);
}

bool GetNoScriptEnabledByDefault(HostContentSettingsMap* map) {
  return GetNoScriptEnabled(map, GURL());
}

void SetIsNoScriptEnabled(HostContentSettingsMap* map,
                          bool is_enabled,
                          const GURL& url,
                          PrefService* local_state) {
  ControlType control_type =
      is_enabled ? ControlType::BLOCK : ControlType::ALLOW;
  brave_shields::SetNoScriptControlType(map, control_type, url, local_state);
}

bool GetNoScriptEnabled(HostContentSettingsMap* map, const GURL& url) {
  ControlType control_type = brave_shields::GetNoScriptControlType(map, url);

  if (control_type == ControlType::ALLOW) {
    return false;
  }

  return true;
}

}  // namespace brave_shields
