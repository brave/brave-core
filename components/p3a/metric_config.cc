/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/metric_config.h"

#include "base/containers/fixed_flat_map.h"

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
        {"woi", MetricAttribute::kWoi},
        {"general_platform", MetricAttribute::kGeneralPlatform},
        {"region", MetricAttribute::kRegion},
        {"subregion", MetricAttribute::kSubregion},
        {"ref", MetricAttribute::kRef},
        {"dtoi", MetricAttribute::kDateOfInstall},
        {"dtoa", MetricAttribute::kDateOfActivation},
        {"woa", MetricAttribute::kWeekOfActivation},
    });

bool GetMetricAttribute(const base::Value* value,
                        std::optional<MetricAttribute>* field) {
  if (!value || !value->is_string()) {
    return false;
  }

  const auto& str = value->GetString();
  auto it = kMetricAttributeMap.find(str);
  if (it == kMetricAttributeMap.end()) {
    return false;
  }
  *field = it->second;
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

bool GetMetricAttributes(const base::Value* value,
                         std::optional<MetricAttributes>* field) {
  if (!value || !value->is_list()) {
    return false;
  }

  const auto& list = value->GetList();

  MetricAttributes attributes;

  for (size_t i = 0; i < list.size() && i < attributes.size(); i++) {
    if (!GetMetricAttribute(&list[i], &attributes[i])) {
      return false;
    }
  }

  *field = attributes;
  return true;
}

bool GetAppendAttributes(const base::Value* value,
                         std::optional<MetricAttributesToAppend>* field) {
  if (!value || !value->is_list()) {
    return false;
  }

  const auto& list = value->GetList();

  MetricAttributesToAppend attributes;

  for (size_t i = 0; i < list.size() && i < attributes.size(); i++) {
    if (!GetMetricAttribute(&list[i], &attributes[i])) {
      return false;
    }
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

bool GetUniquePtrDict(const base::Value* value,
                      std::unique_ptr<base::Value::Dict>* field) {
  if (!value || !value->is_dict()) {
    return false;
  }
  *field = std::make_unique<base::Value::Dict>(value->GetDict().Clone());
  return true;
}

}  // namespace

RemoteMetricConfig::RemoteMetricConfig() = default;
RemoteMetricConfig::~RemoteMetricConfig() = default;

void RemoteMetricConfig::RegisterJSONConverter(
    base::JSONValueConverter<RemoteMetricConfig>* converter) {
  converter->RegisterCustomValueField(
      "ephemeral", &RemoteMetricConfig::ephemeral, &GetOptionalBool);
  converter->RegisterCustomValueField("constellation_only",
                                      &RemoteMetricConfig::constellation_only,
                                      &GetOptionalBool);
  converter->RegisterCustomValueField("nebula", &RemoteMetricConfig::nebula,
                                      &GetOptionalBool);
  converter->RegisterCustomValueField(
      "disable_country_strip", &RemoteMetricConfig::disable_country_strip,
      &GetOptionalBool);
  converter->RegisterCustomValueField(
      "attributes", &RemoteMetricConfig::attributes, &GetMetricAttributes);
  converter->RegisterCustomValueField("append_attributes",
                                      &RemoteMetricConfig::append_attributes,
                                      &GetAppendAttributes);
  converter->RegisterCustomValueField(
      "record_activation_date", &RemoteMetricConfig::record_activation_date,
      &GetOptionalBool);
  converter->RegisterCustomValueField(
      "activation_metric_name", &RemoteMetricConfig::activation_metric_name,
      &GetOptionalString);
  converter->RegisterCustomValueField("cadence", &RemoteMetricConfig::cadence,
                                      &GetMetricLogType);
  converter->RegisterCustomValueField(
      "definition", &RemoteMetricConfig::definition, &GetUniquePtrDict);
}

}  // namespace p3a
