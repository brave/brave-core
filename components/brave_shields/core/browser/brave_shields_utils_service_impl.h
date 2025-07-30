// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_BRAVE_SHIELDS_SERVICE_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_BRAVE_SHIELDS_SERVICE_IMPL_H_

#include "brave/components/brave_shields/core/common/brave_shields_panel.mojom.h"

class GURL;
class HostContentSettingsMap;
class PrefService;

namespace brave_shields {

class BraveShieldsUtilsServiceImpl : public mojom::BraveShieldsUtilsService {
 public:
  BraveShieldsUtilsServiceImpl(HostContentSettingsMap* map,
                               PrefService* local_state,
                               PrefService* profile_state);
  ~BraveShieldsUtilsServiceImpl() override;

  void SetIsBraveShieldsEnabled(bool is_enabled, const GURL& url) override;
  void GetIsBraveShieldsEnabled(
      const GURL& url,
      GetIsBraveShieldsEnabledCallback callback) override;

  void SetAdBlockMode(mojom::AdBlockMode mode, const GURL& url) override;
  void GetAdBlockMode(const GURL& url,
                      GetAdBlockModeCallback callback) override;

  void SetIsBlockScriptsEnabled(bool is_enabled, const GURL& url) override;
  void GetIsBlockScriptsEnabled(
      const GURL& url,
      GetIsBlockScriptsEnabledCallback callback) override;

  void SetFingerprintMode(mojom::FingerprintMode mode, const GURL& url) override;
  void GetFingerprintMode(
      const GURL& url,
      GetFingerprintModeCallback callback) override;

 private:
  const raw_ptr<HostContentSettingsMap> map_ = nullptr;
  const raw_ptr<PrefService> local_state_ = nullptr;
  const raw_ptr<PrefService> profile_state_ = nullptr;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_BRAVE_SHIELDS_SERVICE_IMPL_H_
