/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_HISTOGRAM_VALUE_LIST                              \
    {ContentSettingsType::BRAVE_ADS, 74},                       \
    {ContentSettingsType::BRAVE_COSMETIC_FILTERING, 75},        \
    {ContentSettingsType::BRAVE_TRACKERS, 76},                  \
    {ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES, 77}, \
    {ContentSettingsType::BRAVE_JAVASCRIPT, 78},                \
    {ContentSettingsType::BRAVE_FINGERPRINTING_V2, 79},         \
    {ContentSettingsType::BRAVE_SHIELDS, 80},                   \
    {ContentSettingsType::BRAVE_REFERRERS, 81},                 \
    {ContentSettingsType::BRAVE_COOKIES, 82},                   \
    {ContentSettingsType::BRAVE_FACEBOOK_EMBEDS, 83},           \
    {ContentSettingsType::BRAVE_TWITTER_EMBEDS, 84},            \
    {ContentSettingsType::BRAVE_LINKEDIN_EMBEDS, 85},

#define BRAVE_IS_RENDERER_CONTENT_SETTING \
  content_type == ContentSettingsType::AUTOPLAY ||

#include "../../../../../../components/content_settings/core/common/content_settings.cc"

#undef BRAVE_HISTOGRAM_VALUE_LIST
#undef BRAVE_IS_RENDERER_CONTENT_SETTING
