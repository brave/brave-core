/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_METRIC_CONFIG_H_
#define BRAVE_COMPONENTS_P3A_METRIC_CONFIG_H_

#include <array>
#include <optional>

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
  kMaxValue = kRef,
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
  bool ephemeral;
  // Should only be sent via Constellation
  bool constellation_only;
  // Should only be sent via Nebula
  bool nebula;
  // Avoid reporting "other" for countries not included in the allowlist
  // and rely on STAR to provide k-anonymity
  bool disable_country_strip;
  // Ordered attributes to be included with the metric
  std::optional<MetricAttributes> attributes;
  // Ordered attributes to be appended to the list of default attributes
  MetricAttributesToAppend append_attributes;
};

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_METRIC_CONFIG_H_
