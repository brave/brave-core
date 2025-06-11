/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_METRIC_CONFIG_H_
#define BRAVE_COMPONENTS_P3A_METRIC_CONFIG_H_

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "base/json/json_value_converter.h"
#include "brave/components/p3a/metric_log_type.h"

namespace p3a {

enum class MetricAttribute {
  // Default attributes
  kAnswerIndex,
  kVersion,
  kYoi,
  kChannel,
  kPlatform,
  kCountryCode,
  kWoi,
  // Alternative attributes
  kGeneralPlatform,
  kRegion,
  kSubregion,
  kRef,
  kDateOfInstall,
  kWeekOfActivation,
  kDateOfActivation,
  kMaxValue = kDateOfActivation,
};

inline constexpr MetricAttribute kDefaultMetricAttributes[] = {
    MetricAttribute::kAnswerIndex, MetricAttribute::kVersion,
    MetricAttribute::kYoi,         MetricAttribute::kChannel,
    MetricAttribute::kPlatform,    MetricAttribute::kCountryCode,
    MetricAttribute::kWoi,
};

using MetricAttributes = std::array<std::optional<MetricAttribute>, 8>;
using MetricAttributesToAppend = std::array<std::optional<MetricAttribute>, 2>;

struct MetricConfig {
  // Once the metric value has been sent, the value will be removed from the log
  // store
  bool ephemeral = false;
  // Should only be sent via Constellation
  bool constellation_only = false;
  // Should only be sent via Nebula
  bool nebula = false;
  // Avoid reporting "other" for countries not included in the allowlist
  // and rely on STAR to provide k-anonymity
  bool disable_country_strip = false;
  // Ordered attributes to be included with the metric
  std::optional<MetricAttributes> attributes;
  // Ordered attributes to be appended to the list of default attributes
  MetricAttributesToAppend append_attributes;
  // If true, the activation date will be recorded for this metric.
  // Only the first report of the metric will set the activation date
  // accordingly.
  bool record_activation_date = false;
  // If provided, the activation date recorded from another metric
  // will be reported.
  std::optional<std::string_view> activation_metric_name;

  // If specified in a remote configuration, the cadence of the metric will be
  // overridden.
  std::optional<MetricLogType> cadence;
};

// This struct is used to store the remote configuration for a metric.
// The remote configuration is provided by the component updater.
struct RemoteMetricConfig {
  RemoteMetricConfig();
  ~RemoteMetricConfig();

  RemoteMetricConfig(const RemoteMetricConfig&) = delete;
  RemoteMetricConfig& operator=(const RemoteMetricConfig&) = delete;

  std::optional<bool> ephemeral;
  std::optional<bool> constellation_only;
  std::optional<bool> nebula;
  std::optional<bool> disable_country_strip;
  std::optional<MetricAttributes> attributes;
  std::optional<MetricAttributesToAppend> append_attributes;
  std::optional<bool> record_activation_date;
  std::optional<std::string> activation_metric_name;
  std::optional<MetricLogType> cadence;
  std::unique_ptr<base::Value::Dict> definition;

  static void RegisterJSONConverter(
      base::JSONValueConverter<RemoteMetricConfig>* converter);
};

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_METRIC_CONFIG_H_
