// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/brave_shields_utils_service_impl.h"

#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "components/prefs/pref_service.h"
#include "net/base/features.h"
#include "url/gurl.h"

namespace brave_shields {

BraveShieldsUtilsServiceImpl::BraveShieldsUtilsServiceImpl(HostContentSettingsMap* map,
                             PrefService* local_state,
                             PrefService* profile_state)
    : map_(map), local_state_(local_state), profile_state_(profile_state) {}

BraveShieldsUtilsServiceImpl::~BraveShieldsUtilsServiceImpl() = default;

void BraveShieldsUtilsServiceImpl::SetIsBraveShieldsEnabled(bool is_enabled, const GURL& url) {
  brave_shields::SetBraveShieldsEnabled(map_, is_enabled, url, local_state_);
}

void BraveShieldsUtilsServiceImpl::GetIsBraveShieldsEnabled(const GURL& url, GetIsBraveShieldsEnabledCallback callback) {
  bool is_enabled = brave_shields::GetBraveShieldsEnabled(map_, url);
  std::move(callback).Run(is_enabled);
}

void BraveShieldsUtilsServiceImpl::SetAdBlockMode(mojom::AdBlockMode mode, const GURL& url) {
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

  brave_shields::SetAdControlType(map_, control_type_ad, url, local_state_);

  brave_shields::SetCosmeticFilteringControlType(
      map_, control_type_cosmetic, url, local_state_, profile_state_);
}

void BraveShieldsUtilsServiceImpl::GetAdBlockMode(const GURL& url,
                                  GetAdBlockModeCallback callback) {
  ControlType control_type_ad = brave_shields::GetAdControlType(map_, url);

  ControlType control_type_cosmetic =
      brave_shields::GetCosmeticFilteringControlType(map_, url);

  if (control_type_ad == ControlType::ALLOW) {
    std::move(callback).Run(mojom::AdBlockMode::ALLOW);
    return;
  }

  if (control_type_cosmetic == ControlType::BLOCK) {
    std::move(callback).Run(mojom::AdBlockMode::AGGRESSIVE);
  } else {
    std::move(callback).Run(mojom::AdBlockMode::STANDARD);
  }
}

void BraveShieldsUtilsServiceImpl::SetIsBlockScriptsEnabled(bool is_enabled, const GURL& url) {
  ControlType control_type =
      is_enabled ? ControlType::BLOCK : ControlType::ALLOW;
  brave_shields::SetNoScriptControlType(map_, control_type, url, local_state_);
}

void BraveShieldsUtilsServiceImpl::GetIsBlockScriptsEnabled(const GURL& url,
                              GetIsBlockScriptsEnabledCallback callback) {
  ControlType control_type = brave_shields::GetNoScriptControlType(map_, url);

  if (control_type == ControlType::ALLOW) {
    std::move(callback).Run(false);
    return;
  }

  std::move(callback).Run(true);
}

void BraveShieldsUtilsServiceImpl::SetFingerprintMode(mojom::FingerprintMode mode, const GURL& url) {
  ControlType control_type;

  if (mode == mojom::FingerprintMode::ALLOW_MODE) {
    control_type = ControlType::ALLOW;
  } else if (mode == mojom::FingerprintMode::STRICT_MODE) {
    control_type = ControlType::BLOCK;
  } else {
    control_type = ControlType::DEFAULT;  // STANDARD_MODE
  }

  brave_shields::SetFingerprintingControlType(map_, control_type, url,
                                              local_state_, profile_state_);
}

void BraveShieldsUtilsServiceImpl::GetFingerprintMode(const GURL& url,
                                GetFingerprintModeCallback callback) {
  ControlType control_type =
      brave_shields::GetFingerprintingControlType(map_, url);

  if (control_type == ControlType::ALLOW) {
    std::move(callback).Run(mojom::FingerprintMode::ALLOW_MODE);
  } else if (control_type == ControlType::BLOCK) {
    std::move(callback).Run(mojom::FingerprintMode::STRICT_MODE);
  } else {
    std::move(callback).Run(mojom::FingerprintMode::STANDARD_MODE);
  }
}

}  // namespace brave_shields
