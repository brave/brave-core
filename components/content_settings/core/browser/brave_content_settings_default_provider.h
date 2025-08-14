/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_CONTENT_SETTINGS_DEFAULT_PROVIDER_H_
#define BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_CONTENT_SETTINGS_DEFAULT_PROVIDER_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/synchronization/lock.h"
#include "components/content_settings/core/browser/content_settings_default_provider.h"
#include "components/content_settings/core/browser/content_settings_rule.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/prefs/pref_change_registrar.h"

class GURL;

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace content_settings {

class PartitionKey;

class BraveDefaultProvider : public DefaultProvider {
 public:
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  BraveDefaultProvider(PrefService* prefs,
                       bool off_the_record,
                       bool should_record_metrics);
  BraveDefaultProvider(const BraveDefaultProvider&) = delete;
  BraveDefaultProvider& operator=(const BraveDefaultProvider&) = delete;
  ~BraveDefaultProvider() override;

  std::unique_ptr<Rule> GetRule(
      const GURL& primary_url,
      const GURL& secondary_url,
      ContentSettingsType content_type,
      bool off_the_record,
      const PartitionKey& partition_key) const override;

 private:
  void OnAdBlockOnlyModeChanged();

  std::unique_ptr<Rule> MaybeAdjustRuleForAdBlockOnlyMode(
      std::unique_ptr<Rule> rule,
      const GURL& primary_url,
      ContentSettingsType content_type) const;
  std::unique_ptr<Rule> MaybeSetRuleValueForAdBlockOnlyMode(
      std::unique_ptr<Rule> rule,
      const GURL& primary_url,
      ContentSetting content_setting) const;

  raw_ptr<PrefService> prefs_;

  PrefChangeRegistrar pref_change_registrar_;

  mutable base::Lock lock_;
  bool ad_block_only_mode_enabled_ GUARDED_BY(lock_) = false;

  base::WeakPtrFactory<BraveDefaultProvider> weak_factory_{this};
};

}  //  namespace content_settings

#endif  // BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_CONTENT_SETTINGS_DEFAULT_PROVIDER_H_
