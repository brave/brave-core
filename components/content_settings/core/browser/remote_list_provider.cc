/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/remote_list_provider.h"

#include <memory>
#include <vector>

#include "brave/components/webcompat/content/browser/webcompat_exceptions_service.h"
#include "components/content_settings/core/browser/content_settings_rule.h"

namespace content_settings {

namespace {

class RemoteListIterator : public RuleIterator {
 public:
  explicit RemoteListIterator(
      const std::vector<ContentSettingsPattern>& pattern_vector)
      : pattern_vector_(pattern_vector), count_(pattern_vector.size()) {}
  ~RemoteListIterator() override {}
  bool HasNext() const override { return pattern_index_ < count_; }
  std::unique_ptr<Rule> Next() override {
    auto pattern = pattern_vector_.at(pattern_index_);
    ++pattern_index_;
    return std::make_unique<Rule>(pattern, ContentSettingsPattern::Wildcard(),
                                  base::Value(CONTENT_SETTING_ALLOW),
                                  RuleMetaData());
  }

 private:
  int pattern_index_ = 0;
  const std::vector<ContentSettingsPattern> pattern_vector_;
  int count_;
};

}  // namespace

RemoteListProvider::RemoteListProvider() {}

std::unique_ptr<RuleIterator> RemoteListProvider::GetRuleIterator(
    ContentSettingsType content_type,
    bool off_the_record,
    const PartitionKey& partition_key) const {
  auto* svc = webcompat::WebcompatExceptionsService::GetInstance();
  if (!svc) {
    return nullptr;
  }
  const auto& pattern_vector = svc->GetPatterns(content_type);
  return std::make_unique<RemoteListIterator>(pattern_vector);
}

std::unique_ptr<Rule> RemoteListProvider::GetRule(
    const GURL& primary_url,
    const GURL& secondary_url,
    ContentSettingsType content_type,
    bool off_the_record,
    const PartitionKey& partition_key) const {
  auto* svc = webcompat::WebcompatExceptionsService::GetInstance();
  if (!svc) {
    return nullptr;
  }
  const auto& pattern_vector = svc->GetPatterns(content_type);
  for (auto pattern : pattern_vector) {
    if (pattern.Matches(primary_url)) {
      return std::make_unique<Rule>(pattern, ContentSettingsPattern::Wildcard(),
                                    base::Value(CONTENT_SETTING_ALLOW),
                                    RuleMetaData());
    }
  }
  return nullptr;
}

bool RemoteListProvider::SetWebsiteSetting(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type,
    base::Value&& value,
    const ContentSettingConstraints& constraints,
    const PartitionKey& partition_key) {
  // RemoteListProvider is read-only.
  return false;
}

void RemoteListProvider::ClearAllContentSettingsRules(
    ContentSettingsType content_type,
    const PartitionKey& partition_key) {
  // RemoteListProvider is read-only.
}

void RemoteListProvider::ShutdownOnUIThread() {
  DCHECK(CalledOnValidThread());
  RemoveAllObservers();
}

}  // namespace content_settings
