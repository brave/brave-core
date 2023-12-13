/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_CONTENT_SETTINGS_PREF_PROVIDER_H_
#define BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_CONTENT_SETTINGS_PREF_PROVIDER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "base/thread_annotations.h"
#include "components/content_settings/core/browser/content_settings_observer.h"
#include "components/content_settings/core/browser/content_settings_origin_identifier_value_map.h"
#include "components/content_settings/core/browser/content_settings_pref_provider.h"
#include "components/prefs/pref_change_registrar.h"

namespace content_settings {

// With this subclass, shields configuration is persisted across sessions.
class BravePrefProvider : public PrefProvider,
                          public Observer {
 public:
  BravePrefProvider(PrefService* prefs,
                    bool off_the_record,
                    bool store_last_modified,
                    bool restore_session);
  BravePrefProvider(const BravePrefProvider&) = delete;
  BravePrefProvider& operator=(const BravePrefProvider&) = delete;
  ~BravePrefProvider() override;

  static void CopyPluginSettingsForMigration(PrefService* prefs);
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  // content_settings::PrefProvider overrides:
  void ShutdownOnUIThread() override;
  bool SetWebsiteSetting(const ContentSettingsPattern& primary_pattern,
                         const ContentSettingsPattern& secondary_pattern,
                         ContentSettingsType content_type,
                         base::Value&& value,
                         const ContentSettingConstraints& constraints,
                         const PartitionKey& partition_key =
                             PartitionKey::WipGetDefault()) override;
  std::unique_ptr<RuleIterator> GetRuleIterator(
      ContentSettingsType content_type,
      bool incognito,
      const PartitionKey& partition_key) const override;

  // calls superclass directly
  bool SetWebsiteSettingForTest(const ContentSettingsPattern& primary_pattern,
                                const ContentSettingsPattern& secondary_pattern,
                                ContentSettingsType content_type,
                                base::Value&& value,
                                const ContentSettingConstraints& constraints);

 private:
  friend class BravePrefProviderTest;
  FRIEND_TEST_ALL_PREFIXES(BravePrefProviderTest, TestShieldsSettingsMigration);
  FRIEND_TEST_ALL_PREFIXES(BravePrefProviderTest,
                           TestShieldsSettingsMigrationV2toV4);
  FRIEND_TEST_ALL_PREFIXES(BravePrefProviderTest,
                           TestShieldsSettingsMigrationVersion);
  FRIEND_TEST_ALL_PREFIXES(BravePrefProviderTest,
                           TestShieldsSettingsMigrationFromResourceIDs);
  FRIEND_TEST_ALL_PREFIXES(BravePrefProviderTest,
                           TestShieldsSettingsMigrationFromUnknownSettings);
  FRIEND_TEST_ALL_PREFIXES(BravePrefProviderTest, EnsureNoWildcardEntries);
  FRIEND_TEST_ALL_PREFIXES(BravePrefProviderTest, MigrateFPShieldsSettings);
  void MigrateShieldsSettings(bool incognito);
  void EnsureNoWildcardEntries(ContentSettingsType content_type);
  void MigrateShieldsSettingsFromResourceIds();
  void MigrateShieldsSettingsFromResourceIdsForOneType(
      const std::string& preference_path,
      const std::string& patterns_string,
      const base::Time& expiration,
      const base::Time& last_modified,
      SessionModel session_model,
      int setting);
  void MigrateShieldsSettingsV1ToV2();
  void MigrateShieldsSettingsV1ToV2ForOneType(ContentSettingsType content_type);
  void MigrateShieldsSettingsV2ToV3();
  void MigrateShieldsSettingsV3ToV4(int start_version);
  void MigrateFingerprintingSettings();
  void MigrateFingerprintingSetingsToOriginScoped();
  void UpdateCookieRules(ContentSettingsType content_type, bool incognito);
  void OnCookieSettingsChanged(ContentSettingsType content_type);
  void NotifyChanges(const std::vector<std::unique_ptr<OwnedRule>>& rules,
                     bool incognito);
  bool SetWebsiteSettingInternal(
      const ContentSettingsPattern& primary_pattern,
      const ContentSettingsPattern& secondary_pattern,
      ContentSettingsType content_type,
      base::Value&& value,
      const ContentSettingConstraints& constraints,
      const PartitionKey& partition_key = PartitionKey::WipGetDefault());

  // content_settings::Observer overrides:
  void OnContentSettingChanged(const ContentSettingsPattern& primary_pattern,
                               const ContentSettingsPattern& secondary_pattern,
                               ContentSettingsType content_type) override;
  void OnCookiePrefsChanged(const std::string& pref);

  std::map<bool /* is_incognito */, OriginIdentifierValueMap> cookie_rules_;
  std::map<bool /* is_incognito */, std::vector<std::unique_ptr<OwnedRule>>>
      brave_cookie_rules_;
  std::map<bool /* is_incognito */, std::vector<std::unique_ptr<OwnedRule>>>
      brave_shield_down_rules_;

  bool initialized_;
  bool store_last_modified_;

  PrefChangeRegistrar pref_change_registrar_;

  base::WeakPtrFactory<BravePrefProvider> weak_factory_;
};

}  //  namespace content_settings

#endif  // BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_CONTENT_SETTINGS_PREF_PROVIDER_H_
