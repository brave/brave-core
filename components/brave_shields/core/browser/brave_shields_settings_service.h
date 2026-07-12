// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_BRAVE_SHIELDS_SETTINGS_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_BRAVE_SHIELDS_SETTINGS_SERVICE_H_

#include "base/containers/span.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/token.h"
#include "brave/components/brave_shields/core/common/brave_shields_panel.mojom.h"
#include "brave/components/brave_shields/core/common/farbling_prng.h"
#include "brave/components/brave_shields/core/common/shields_settings.mojom.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/keyed_service/core/keyed_service.h"

class GURL;
class HostContentSettingsMap;
class PrefService;

namespace user_prefs {
class PrefRegistrySyncable;
}

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

  bool MakePseudoRandomGeneratorForURL(
      const GURL& url,
      base::span<const uint8_t> additional_entropy,
      FarblingPRNG* prng);

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  // Returns the underlying farbling token stored for |url|. For containers
  // which runs in an isolated storage an |additional_entropy| is applied on top
  // to the token. Lastly, an additional |profile_level_farbling_entropy| maybe
  // applied on top again, if brave_shields::features::kBraveFarblingTokenReset
  // is enabled.
  base::Token GetFarblingToken(const GURL& url,
                               base::span<const uint8_t> additional_entropy);

  // Test only method. This is only respected if |g_stable_farbling_tokens_seed|
  // is NOT set. If it's set then the |profile_level_farbling_entropy| shall be
  // ignored. To not ignore it, you must allowlist a list of test tokens via
  // ScopedAllowlistedProfileTokensForTesting.
  void set_profile_level_farbling_entropy_for_testing(
      const base::Token profile_level_farbling_entropy) {
    CHECK_IS_TEST();
    profile_level_farbling_entropy_ = profile_level_farbling_entropy;
  }

 private:
  const raw_ref<HostContentSettingsMap>
      host_content_settings_map_;       // NOT OWNED
  raw_ptr<PrefService> local_state_;    // NOT OWNED
  raw_ptr<PrefService> profile_prefs_;  // NOT OWNED

  // This token is generated when the service is created and stays stable until
  // the service is destoryed. It allows to show different farbled values for a
  // site across browser restarts.
  base::Token profile_level_farbling_entropy_;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_BRAVE_SHIELDS_SETTINGS_SERVICE_H_
