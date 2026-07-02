// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_BRAVE_SHIELDS_SETTINGS_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_BRAVE_SHIELDS_SETTINGS_SERVICE_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "brave/components/brave_shields/core/common/brave_shields_panel.mojom.h"
#include "brave/components/brave_shields/core/common/brave_shields_settings_values.h"
#include "brave/components/brave_shields/core/common/shields_settings.mojom.h"
#include "brave/components/https_upgrade_exceptions/browser/https_upgrade_exceptions_service.h"
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

  void SetCosmeticFilteringControlType(ControlType type, const GURL& url);
  ControlType GetCosmeticFilteringControlType(const GURL& url);
  bool IsFirstPartyCosmeticFilteringEnabled(const GURL& url);

  void SetNoScriptControlType(ControlType type, const GURL& url);
  ControlType GetNoScriptControlType(const GURL& url);

  void SetFingerprintingControlType(ControlType type, const GURL& url);
  ControlType GetFingerprintingControlType(const GURL& url);

  bool IsReduceLanguageEnabledForProfile(PrefService* pref_service);
  bool ShouldDoReduceLanguage(const GURL& url);

  void SetHttpsUpgradeControlType(ControlType type, const GURL& url);
  ControlType GetHttpsUpgradeControlType(const GURL& url);
  bool ShouldUpgradeToHttps(
      const GURL& url,
      https_upgrade_exceptions::HttpsUpgradeExceptionsService*
          https_upgrade_exceptions_service);
  bool ShouldForceHttps(const GURL& url);

  mojom::FarblingLevel GetFarblingLevel(const GURL& primary_url);
  base::Token GetFarblingToken(const GURL& url,
                               base::span<const uint8_t> additional_entropy);

  void SetAdControlType(ControlType type, const GURL& url);
  ControlType GetAdControlType(const GURL& url);

  DomainBlockingType GetDomainBlockingType(const GURL& url);

  void SetDefaultAdBlockMode(mojom::AdBlockMode mode);
  mojom::AdBlockMode GetDefaultAdBlockMode();

  void SetAdBlockMode(mojom::AdBlockMode mode, const GURL& url);
  mojom::AdBlockMode GetAdBlockMode(const GURL& url);

  void SetDefaultFingerprintMode(mojom::FingerprintMode mode);
  mojom::FingerprintMode GetDefaultFingerprintMode();

  void SetFingerprintMode(mojom::FingerprintMode mode, const GURL& url);
  mojom::FingerprintMode GetFingerprintMode(const GURL& url);

  void SetCookieControlType(ControlType type, const GURL& url);
  ControlType GetCookieControlType(
      content_settings::CookieSettings* cookie_settings,
      const GURL& url);

  void SetNoScriptEnabledByDefault(bool is_enabled);
  bool IsNoScriptEnabledByDefault();

  void SetNoScriptEnabled(bool is_enabled, const GURL& url);
  bool IsNoScriptEnabled(const GURL& url);

#if !BUILDFLAG(IS_IOS)
  bool GetForgetFirstPartyStorageEnabled(const GURL& url);
  void SetForgetFirstPartyStorageEnabled(bool is_enabled, const GURL& url);
#endif

  void SetDefaultAutoShredMode(mojom::AutoShredMode mode);
  mojom::AutoShredMode GetDefaultAutoShredMode();

  void SetAutoShredMode(mojom::AutoShredMode mode, const GURL& url);
  mojom::AutoShredMode GetAutoShredMode(const GURL& url);

  bool IsJsBlockingEnforced(const GURL& url);
  mojom::ContentSettingsOverriddenDataPtr GetJsContentSettingOverriddenData(
      const GURL& url);

  bool IsShieldsDisabledOnAnyHostMatchingDomainOf(const GURL& url) const;

  void SetShredBrowsingHistory(bool value);
  bool IsShredBrowsingHistoryEnabled();

 private:
  const raw_ref<HostContentSettingsMap>
      host_content_settings_map_;       // NOT OWNED
  raw_ptr<PrefService> local_state_;    // NOT OWNED
  raw_ptr<PrefService> profile_prefs_;  // NOT OWNED
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_BRAVE_SHIELDS_SETTINGS_SERVICE_H_
