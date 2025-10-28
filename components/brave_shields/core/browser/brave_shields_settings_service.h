// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_BRAVE_SHIELDS_SETTINGS_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_BRAVE_SHIELDS_SETTINGS_SERVICE_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "brave/components/brave_shields/core/common/brave_shields_panel.mojom.h"
#include "brave/components/brave_shields/core/common/shields_settings.mojom.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/keyed_service/core/keyed_service.h"

class GURL;
class HostContentSettingsMap;
class PrefService;

namespace brave_shields {

class BraveShieldsSettingsService : public KeyedService {
 public:
  explicit BraveShieldsSettingsService(
      HostContentSettingsMap& host_content_settings_map,
      PrefService* local_state = nullptr,
      PrefService* profile_state = nullptr);
  ~BraveShieldsSettingsService() override;

  void SetBraveShieldsEnabled(bool enable, const GURL& url);
  bool GetBraveShieldsEnabled(const GURL& url);

  void SetDefaultAdBlockMode(mojom::AdBlockMode mode);
  mojom::AdBlockMode GetDefaultAdBlockMode();

  void SetAdBlockMode(mojom::AdBlockMode mode, const GURL& url);
  mojom::AdBlockMode GetAdBlockMode(const GURL& url);

  void SetDefaultFingerprintMode(mojom::FingerprintMode mode);
  mojom::FingerprintMode GetDefaultFingerprintMode();

  void SetFingerprintMode(mojom::FingerprintMode mode, const GURL& url);
  mojom::FingerprintMode GetFingerprintMode(const GURL& url);

  void SetNoScriptEnabledByDefault(bool is_enabled);
  bool IsNoScriptEnabledByDefault();

  void SetNoScriptEnabled(bool is_enabled, const GURL& url);
  bool IsNoScriptEnabled(const GURL& url);

  void SetDefaultAutoShredMode(mojom::AutoShredMode mode);
  mojom::AutoShredMode GetDefaultAutoShredMode();

  void SetAutoShredMode(mojom::AutoShredMode mode, const GURL& url);
  mojom::AutoShredMode GetAutoShredMode(const GURL& url);

  void SetDefaultAutoShredMode(mojom::AutoShredMode mode);
  mojom::AutoShredMode GetDefaultAutoShredMode();

  void SetAutoShredMode(mojom::AutoShredMode mode, const GURL& url);
  mojom::AutoShredMode GetAutoShredMode(const GURL& url);

  void SetDefaultAutoShredMode(mojom::AutoShredMode mode);
  mojom::AutoShredMode GetDefaultAutoShredMode();

  void SetAutoShredMode(mojom::AutoShredMode mode, const GURL& url);
  mojom::AutoShredMode GetAutoShredMode(const GURL& url);

  void SetDefaultAutoShredMode(mojom::AutoShredMode mode);
  mojom::AutoShredMode GetDefaultAutoShredMode();

  void SetAutoShredMode(mojom::AutoShredMode mode, const GURL& url);
  mojom::AutoShredMode GetAutoShredMode(const GURL& url);

  void SetDefaultAutoShredMode(mojom::AutoShredMode mode);
  mojom::AutoShredMode GetDefaultAutoShredMode();

  void SetAutoShredMode(mojom::AutoShredMode mode, const GURL& url);
  mojom::AutoShredMode GetAutoShredMode(const GURL& url);

  void SetDefaultAutoShredMode(mojom::AutoShredMode mode);
  mojom::AutoShredMode GetDefaultAutoShredMode();

  void SetAutoShredMode(mojom::AutoShredMode mode, const GURL& url);
  mojom::AutoShredMode GetAutoShredMode(const GURL& url);

  void SetDefaultAutoShredMode(mojom::AutoShredMode mode);
  mojom::AutoShredMode GetDefaultAutoShredMode();

  void SetAutoShredMode(mojom::AutoShredMode mode, const GURL& url);
  mojom::AutoShredMode GetAutoShredMode(const GURL& url);

  void SetDefaultAutoShredMode(mojom::AutoShredMode mode);
  mojom::AutoShredMode GetDefaultAutoShredMode();

  void SetAutoShredMode(mojom::AutoShredMode mode, const GURL& url);
  mojom::AutoShredMode GetAutoShredMode(const GURL& url);

  bool IsJsBlockingEnforced(const GURL& url);
  mojom::ContentSettingsOverriddenDataPtr GetJsContentSettingOverriddenData(
      const GURL& url);

 private:
  const raw_ref<HostContentSettingsMap>
      host_content_settings_map_;       // NOT OWNED
  raw_ptr<PrefService> local_state_;    // NOT OWNED
  raw_ptr<PrefService> profile_prefs_;  // NOT OWNED
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_BRAVE_SHIELDS_SETTINGS_SERVICE_H_
