/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/content_settings/core/common/content_settings.h"

// Leave a gap between Chromium values and our values in the kHistogramValue
// array so that we don't have to renumber when new content settings types are
// added upstream.
namespace {

// Do not change the value arbitrarily. This variable is only used for the
// DCHECK in ContentSettingTypeToHistogramValue function below.
constexpr int kBraveValuesStart = 1000;

constexpr int brave_value(int incr) {
  return kBraveValuesStart + incr;
}

}  // namespace

// clang-format off
#define BRAVE_HISTOGRAM_VALUE_LIST                                        \
  {ContentSettingsType::BRAVE_ADS, brave_value(0)},                       \
  {ContentSettingsType::BRAVE_COSMETIC_FILTERING, brave_value(1)},        \
  {ContentSettingsType::BRAVE_TRACKERS, brave_value(2)},                  \
  {ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES, brave_value(3)}, \
  {ContentSettingsType::BRAVE_FINGERPRINTING_V2, brave_value(4)},         \
  {ContentSettingsType::BRAVE_SHIELDS, brave_value(5)},                   \
  {ContentSettingsType::BRAVE_REFERRERS, brave_value(6)},                 \
  {ContentSettingsType::BRAVE_COOKIES, brave_value(7)},                   \
  {ContentSettingsType::BRAVE_SPEEDREADER, brave_value(8)},               \
  {ContentSettingsType::BRAVE_ETHEREUM, brave_value(9)},
// clang-format on

#define ContentSettingTypeToHistogramValue \
  ContentSettingTypeToHistogramValue_ChromiumImpl

#define RendererContentSettingRules RendererContentSettingRules_ChromiumImpl

#include "src/components/content_settings/core/common/content_settings.cc"

#undef RendererContentSettingRules
#undef ContentSettingTypeToHistogramValue
#undef BRAVE_HISTOGRAM_VALUE_LIST

int ContentSettingTypeToHistogramValue(ContentSettingsType content_setting,
                                       size_t* num_values) {
  DCHECK(static_cast<int>(ContentSettingsType::NUM_TYPES) < kBraveValuesStart);
  return ContentSettingTypeToHistogramValue_ChromiumImpl(content_setting,
                                                         num_values);
}

RendererContentSettingRules::RendererContentSettingRules() = default;
RendererContentSettingRules::~RendererContentSettingRules() = default;

// static
bool RendererContentSettingRules::IsRendererContentSetting(
    ContentSettingsType content_type) {
  return RendererContentSettingRules_ChromiumImpl::IsRendererContentSetting(
             content_type) ||
         content_type == ContentSettingsType::AUTOPLAY;
}

namespace content_settings {
namespace {

bool IsExplicitSetting(const ContentSettingsPattern& primary_pattern,
                       const ContentSettingsPattern& secondary_pattern) {
  return !primary_pattern.MatchesAllHosts() ||
         !secondary_pattern.MatchesAllHosts();
}

}  // namespace

bool IsExplicitSetting(const ContentSettingPatternSource& setting) {
  return IsExplicitSetting(setting.primary_pattern, setting.secondary_pattern);
}

bool IsExplicitSetting(const SettingInfo& setting) {
  return IsExplicitSetting(setting.primary_pattern, setting.secondary_pattern);
}

}  // namespace content_settings
