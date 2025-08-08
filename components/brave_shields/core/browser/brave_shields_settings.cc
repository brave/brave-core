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

BraveShieldsSettings::BraveShieldsSettings(
    HostContentSettingsMap* host_content_settings_map,
    PrefService* local_state,
    PrefService* profile_state)
    : host_content_settings_map_(host_content_settings_map),
      local_state_(local_state),
      profile_state_(profile_state) {
  CHECK(host_content_settings_map_);
}

BraveShieldsSettings::~BraveShieldsSettings() = default;

void BraveShieldsSettings::SetBraveShieldsEnabled(bool is_enabled,
                                                  const GURL& url) {
  brave_shields::SetBraveShieldsEnabled(host_content_settings_map_, is_enabled,
                                        url, local_state_);
}

bool BraveShieldsSettings::GetBraveShieldsEnabled(const GURL& url) {
  return brave_shields::GetBraveShieldsEnabled(host_content_settings_map_, url);
}

void BraveShieldsSettings::SetDefaultAdBlockMode(mojom::AdBlockMode mode) {
  SetAdBlockMode(mode, GURL());
}

mojom::AdBlockMode BraveShieldsSettings::GetDefaultAdBlockMode() {
  return GetAdBlockMode(GURL());
}

void BraveShieldsSettings::SetAdBlockMode(mojom::AdBlockMode mode,
                                          const GURL& url) {
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

  brave_shields::SetAdControlType(host_content_settings_map_, control_type_ad,
                                  url, local_state_);

  brave_shields::SetCosmeticFilteringControlType(host_content_settings_map_,
                                                 control_type_cosmetic, url,
                                                 local_state_, profile_state_);
}

mojom::AdBlockMode BraveShieldsSettings::GetAdBlockMode(const GURL& url) {
  ControlType control_type_ad =
      brave_shields::GetAdControlType(host_content_settings_map_, url);

  ControlType control_type_cosmetic =
      brave_shields::GetCosmeticFilteringControlType(host_content_settings_map_,
                                                     url);

  if (control_type_ad == ControlType::ALLOW) {
    return mojom::AdBlockMode::ALLOW;
  }

  if (control_type_cosmetic == ControlType::BLOCK) {
    return mojom::AdBlockMode::AGGRESSIVE;
  } else {
    return mojom::AdBlockMode::STANDARD;
  }
}

void BraveShieldsSettings::SetDefaultFingerprintMode(
    mojom::FingerprintMode mode) {
  SetFingerprintMode(mode, GURL());
}

mojom::FingerprintMode BraveShieldsSettings::GetDefaultFingerprintMode() {
  return GetFingerprintMode(GURL());
}

void BraveShieldsSettings::SetFingerprintMode(mojom::FingerprintMode mode,
                                              const GURL& url) {
#if BUILDFLAG(IS_IOS)
  /// Strict FingerprintMode is not supported on iOS
  CHECK(mode != mojom::FingerprintMode::STRICT_MODE);
#endif

  ControlType control_type;

  if (mode == mojom::FingerprintMode::ALLOW_MODE) {
    control_type = ControlType::ALLOW;
  } else if (mode == mojom::FingerprintMode::STRICT_MODE) {
    control_type = ControlType::BLOCK;
  } else {
    control_type = ControlType::DEFAULT;  // STANDARD_MODE
  }

  brave_shields::SetFingerprintingControlType(host_content_settings_map_,
                                              control_type, url, local_state_,
                                              profile_state_);
}

mojom::FingerprintMode BraveShieldsSettings::GetFingerprintMode(
    const GURL& url) {
  ControlType control_type = brave_shields::GetFingerprintingControlType(
      host_content_settings_map_, url);

  if (control_type == ControlType::ALLOW) {
    return mojom::FingerprintMode::ALLOW_MODE;
  } else if (control_type == ControlType::BLOCK) {
#if BUILDFLAG(IS_IOS)
    /// Strict FingerprintMode is not supported on iOS.
    /// In case of sync'd setting, return standard mode.
    return mojom::FingerprintMode::STANDARD_MODE;
#else
    return mojom::FingerprintMode::STRICT_MODE;
#endif
  } else {
    return mojom::FingerprintMode::STANDARD_MODE;
  }
}

void BraveShieldsSettings::SetIsNoScriptEnabledByDefault(bool is_enabled) {
  SetIsNoScriptEnabled(is_enabled, GURL());
}

bool BraveShieldsSettings::GetNoScriptEnabledByDefault() {
  return GetNoScriptEnabled(GURL());
}

void BraveShieldsSettings::SetIsNoScriptEnabled(bool is_enabled,
                                                const GURL& url) {
  ControlType control_type =
      is_enabled ? ControlType::BLOCK : ControlType::ALLOW;
  brave_shields::SetNoScriptControlType(host_content_settings_map_,
                                        control_type, url, local_state_);
}

bool BraveShieldsSettings::GetNoScriptEnabled(const GURL& url) {
  ControlType control_type =
      brave_shields::GetNoScriptControlType(host_content_settings_map_, url);

  if (control_type == ControlType::ALLOW) {
    return false;
  }

  return true;
}

}  // namespace brave_shields
