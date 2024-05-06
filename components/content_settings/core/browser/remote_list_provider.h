/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_REMOTE_LIST_PROVIDER_H_
#define BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_REMOTE_LIST_PROVIDER_H_

#include <memory>

#include "components/content_settings/core/browser/content_settings_observable_provider.h"

namespace content_settings {

// RemoteListProvider provides webcompat exceptions rules from the remote list
// to the HostContentSettingsMap. This allows remote rules to be overridden
// by user-defined rules.
class RemoteListProvider : public ObservableProvider {
 public:
  RemoteListProvider();
  ~RemoteListProvider() override = default;

  RemoteListProvider(const RemoteListProvider&) = delete;
  RemoteListProvider& operator=(const RemoteListProvider&) = delete;

  std::unique_ptr<RuleIterator> GetRuleIterator(
      ContentSettingsType content_type,
      bool off_the_record,
      const PartitionKey& partition_key) const override;

  std::unique_ptr<Rule> GetRule(
      const GURL& primary_url,
      const GURL& secondary_url,
      ContentSettingsType content_type,
      bool off_the_record,
      const PartitionKey& partition_key) const override;

  bool SetWebsiteSetting(const ContentSettingsPattern& primary_pattern,
                         const ContentSettingsPattern& secondary_pattern,
                         ContentSettingsType content_type,
                         base::Value&& value,
                         const ContentSettingConstraints& constraints,
                         const PartitionKey& partition_key) override;

  void ClearAllContentSettingsRules(ContentSettingsType content_type,
                                    const PartitionKey& partition_key) override;

  void ShutdownOnUIThread() override;
};

}  // namespace content_settings

#endif  // BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_REMOTE_LIST_PROVIDER_H_
