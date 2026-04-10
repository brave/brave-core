/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/metric_config.h"

#include "base/containers/fixed_flat_map.h"
#include "base/containers/map_util.h"

namespace p3a {

namespace {

constexpr auto kMetricAttributeMap =
    base::MakeFixedFlatMap<std::string_view, MetricAttribute>({
        {"answer_index", MetricAttribute::kAnswerIndex},
        {"version", MetricAttribute::kVersion},
        {"yoi", MetricAttribute::kYoi},
        {"channel", MetricAttribute::kChannel},
        {"platform", MetricAttribute::kPlatform},
        {"country_code", MetricAttribute::kCountryCode},
        {"locale_country_code", MetricAttribute::kLocaleCountryCode},
        {"woi", MetricAttribute::kWoi},
        {"general_platform", MetricAttribute::kGeneralPlatform},
        {"region", MetricAttribute::kRegion},
        {"subregion", MetricAttribute::kSubregion},
        {"ref", MetricAttribute::kRef},
        {"dtoi", MetricAttribute::kDateOfInstall},
        {"dtoa", MetricAttribute::kDateOfActivation},
        {"woa", MetricAttribute::kWeekOfActivation},
        {"is_browser_default", MetricAttribute::kIsBrowserDefault},
        {"custom_attribute", MetricAttribute::kCustomAttribute},
    });

bool GetMetricAttribute(const base::Value* value,
                        std::optional<MetricAttribute>* field) {
  if (!value || !value->is_string()) {
    return false;
  }

  const auto& str = value->GetString();
  const auto* attribute = base::FindOrNull(kMetricAttributeMap, str);
  if (!attribute) {
    return false;
  }
  *field = *attribute;
  return true;
}

bool GetMetricLogType(const base::Value* value,
                      std::optional<MetricLogType>* field) {
  if (!value || !value->is_string()) {
    return false;
  }

  auto log_type = StringToMetricLogType(value->GetString());
  if (!log_type) {
    return false;
  }
  *field = log_type.value();
  return true;
}

template <size_t N>
bool GetMetricAttributes(
    const base::Value* value,
    std::optional<std::array<std::optional<MetricAttribute>, N>>* field) {
  if (!value || !value->is_list()) {
    return false;
  }

  std::array<std::optional<MetricAttribute>, N> attributes;
  size_t attr_idx = 0;
  for (const auto& item : value->GetList()) {
    if (attr_idx >= N) {
      break;
    }
    if (!GetMetricAttribute(&item, &attributes[attr_idx])) {
      continue;
    }
    attr_idx++;
  }

  *field = attributes;
  return true;
}

bool GetOptionalString(const base::Value* value,
                       std::optional<std::string>* field) {
  if (!value || !value->is_string()) {
    return false;
  }

  *field = value->GetString();
  return true;
}

bool GetOptionalBool(const base::Value* value, std::optional<bool>* field) {
  if (!value || !value->is_bool()) {
    return false;
  }
  *field = value->GetBool();
  return true;
}

bool GetCustomAttributes(const base::Value* value,
                         std::optional<RemoteCustomAttributes>* field) {
  if (!value || !value->is_list()) {
    return false;
  }

  const auto& list = value->GetList();

  RemoteCustomAttributes attributes;

  for (size_t i = 0; i < list.size() && i < attributes.size(); i++) {
    if (!list[i].is_string()) {
      return false;
    }
    attributes[i] = list[i].GetString();
  }

  *field = attributes;
  return true;
}

}  // namespace

RemoteMetricConfig::RemoteMetricConfig() = default;
RemoteMetricConfig::~RemoteMetricConfig() = default;

RemoteMetricConfig::RemoteMetricConfig(const RemoteMetricConfig&) = default;
RemoteMetricConfig& RemoteMetricConfig::operator=(const RemoteMetricConfig&) =
    default;

void RemoteMetricConfig::RegisterJSONConverter(
    base::JSONValueConverter<RemoteMetricConfig>* converter) {
  converter->RegisterCustomValueField(
      "ephemeral", &RemoteMetricConfig::ephemeral, &GetOptionalBool);
  converter->RegisterCustomValueField("nebula", &RemoteMetricConfig::nebula,
                                      &GetOptionalBool);
  converter->RegisterCustomValueField(
      "disable_country_strip", &RemoteMetricConfig::disable_country_strip,
      &GetOptionalBool);
  converter->RegisterCustomValueField(
      "attributes", &RemoteMetricConfig::attributes,
      &GetMetricAttributes<std::tuple_size_v<MetricAttributes>>);
  converter->RegisterCustomValueField(
      "append_attributes", &RemoteMetricConfig::append_attributes,
      &GetMetricAttributes<std::tuple_size_v<MetricAttributesToAppend>>);
  converter->RegisterCustomValueField(
      "record_activation_date", &RemoteMetricConfig::record_activation_date,
      &GetOptionalBool);
  converter->RegisterCustomValueField(
      "activation_metric_name", &RemoteMetricConfig::activation_metric_name,
      &GetOptionalString);
  converter->RegisterCustomValueField("cadence", &RemoteMetricConfig::cadence,
                                      &GetMetricLogType);
  converter->RegisterCustomValueField("custom_attributes",
                                      &RemoteMetricConfig::custom_attributes,
                                      &GetCustomAttributes);
}

}  // namespace p3a
