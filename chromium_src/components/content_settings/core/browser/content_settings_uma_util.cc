/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/content_settings/core/browser/content_settings_uma_util.h"

// Leave a gap between Chromium values and our values in the kHistogramValue
// array so that we don't have to renumber when new content settings types are
// added upstream.
namespace {

// Do not change the value arbitrarily. This is used to validate we have a gap
// between Chromium's and Brave's histograms. This value must be less than 1000
// as upstream performs a sanity check that the total number of buckets isn't
// unreasonably large.
constexpr int kBraveValuesStart = 900;

constexpr int brave_value(int incr) {
  return kBraveValuesStart + incr;
}

}  // namespace

static_assert(static_cast<int>(ContentSettingsType::kMaxValue) <
                  kBraveValuesStart,
              "There must a gap between the histograms used by Chromium, and "
              "the ones used by Brave.");

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
  {ContentSettingsType::BRAVE_ETHEREUM, brave_value(9)},                  \
  {ContentSettingsType::BRAVE_SOLANA, brave_value(10)},                   \
  {ContentSettingsType::BRAVE_GOOGLE_SIGN_IN, brave_value(11)},           \
  {ContentSettingsType::BRAVE_HTTPS_UPGRADE, brave_value(12)},            \
  {ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE, brave_value(13)},      \
  {ContentSettingsType::BRAVE_LOCALHOST_ACCESS, brave_value(14)},         \
  {ContentSettingsType::BRAVE_OPEN_AI_CHAT, brave_value(15)},             \
  /* Begin webcompat items */                                                \
  {ContentSettingsType::BRAVE_WEBCOMPAT_NONE, brave_value(50)}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_AUDIO, brave_value(51)}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_CANVAS, brave_value(52)}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_DEVICE_MEMORY, brave_value(53)}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_EVENT_SOURCE_POOL, brave_value(54)}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_FONT, brave_value(55)}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_HARDWARE_CONCURRENCY, brave_value(56)}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_KEYBOARD, brave_value(57)}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_LANGUAGE, brave_value(58)}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_MEDIA_DEVICES, brave_value(59)}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_PLUGINS, brave_value(60)}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_SCREEN, brave_value(61)}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_SPEECH_SYNTHESIS, brave_value(62)}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_USB_DEVICE_SERIAL_NUMBER, brave_value(63)}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_USER_AGENT, brave_value(64)}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_WEBGL, brave_value(65)}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_WEBGL2, brave_value(66)}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_WEB_SOCKETS_POOL, brave_value(67)}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_ALL, brave_value(68)}, \
  {ContentSettingsType::BRAVE_SHIELDS_METADATA, brave_value(69)},
// clang-format on

#define kDefaultProvider       \
  kRemoteListProvider:         \
  return "RemoteListProvider"; \
  case ProviderType::kDefaultProvider

#include "src/components/content_settings/core/browser/content_settings_uma_util.cc"

#undef BRAVE_HISTOGRAM_VALUE_LIST
#undef kDefaultProvider
