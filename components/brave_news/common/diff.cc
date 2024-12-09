// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/common/diff.h"

#include "base/values.h"

namespace brave_news::mojom {

namespace {
template <typename T>
base::Value Diff(const std::map<std::string, T>& old_value,
                 const std::map<std::string, T>& new_value) {
  base::Value::Dict result;
  for (const auto& [key, value] : new_value) {
    auto it = old_value.find(key);
    if (it == old_value.end()) {
      result.Set(key, Diff(it->second, value));
    }
  }

  // Check for removed items - we set them to null so the client knows to remove
  // them.
  for (const auto& [key, value] : old_value) {
    if (new_value.find(key) == new_value.end()) {
      result.Set(key, base::Value(base::Value::Type::NONE));
    }
  }
  return base::Value(std::move(result));
}

base::Value Diff(const std::vector<std::string>& old_value,
                 const std::vector<std::string>& new_value) {
  base::Value::Dict result;
  for (size_t i = 0; i < new_value.size(); i++) {
    if (i >= old_value.size()) {
      result.Set(std::to_string(i), new_value[i]);
    } else if (old_value[i] != new_value[i]) {
      result.Set(std::to_string(i), new_value[i]);
    }
  }


  for (size_t i = new_value.size(); i < old_value.size(); i++) {
    result.Set(std::to_string(i), base::Value(base::Value::Type::NONE));
  }
  return base::Value(std::move(result));
}

}  // namespace

base::Value Diff(const StatePtr& old_value, const StatePtr& new_value) {
  auto result = base::Value::Dict();

  auto configuration_diff =
      Diff(old_value->configuration, new_value->configuration);
  if (!configuration_diff.is_none()) {
    result.Set("configuration", std::move(configuration_diff));
  }

  auto channels_diff = Diff(old_value->channels, new_value->channels);
  if (!channels_diff.is_none()) {
    result.Set("channels", std::move(channels_diff));
  }

  auto publishers_diff = Diff(old_value->publishers, new_value->publishers);
  if (!publishers_diff.is_none()) {
    result.Set("publishers", std::move(publishers_diff));
  }

  auto suggested_publisher_ids_diff = Diff(old_value->suggested_publisher_ids,
                                           new_value->suggested_publisher_ids);
  if (!suggested_publisher_ids_diff.is_none()) {
    result.Set("suggested_publisher_ids",
               std::move(suggested_publisher_ids_diff));
  }

  return base::Value(std::move(result));
}

base::Value Diff(const ConfigurationPtr& old_value,
                 const ConfigurationPtr& new_value) {
  auto result = base::Value::Dict();
  if (old_value->is_opted_in != new_value->is_opted_in) {
    result.Set("is_opted_in", new_value->is_opted_in);
  }
  if (old_value->show_on_ntp != new_value->show_on_ntp) {
    result.Set("show_on_ntp", new_value->show_on_ntp);
  }
  if (old_value->open_articles_in_new_tab !=
      new_value->open_articles_in_new_tab) {
    result.Set("open_articles_in_new_tab", new_value->open_articles_in_new_tab);
  }
  return base::Value(std::move(result));
}

base::Value Diff(const ChannelPtr& old_value, const ChannelPtr& new_value) {
  auto result = base::Value::Dict();
  if (old_value->id != new_value->id) {
    result.Set("id", new_value->id);
  }
  if (old_value->name != new_value->name) {
    result.Set("name", new_value->name);
  }
  return base::Value(std::move(result));
}

base::Value Diff(const PublisherPtr& old_value, const PublisherPtr& new_value) {
  auto result = base::Value::Dict();
  if (old_value->publisher_id != new_value->publisher_id) {
    result.Set("publisher_id", new_value->publisher_id);
  }
  if (old_value->publisher_name != new_value->publisher_name) {
    result.Set("publisher_name", new_value->publisher_name);
  }

  if (old_value->user_enabled != new_value->user_enabled) {
    result.Set("user_enabled", new_value->user_enabled);
  }
  return base::Value(std::move(result));
}

}  // namespace brave_news::mojom
