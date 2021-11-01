/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/configuration_info_log.h"

#include "base/containers/fixed_flat_map.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"

#define INFO_LOG_LEVEL INFO

namespace ads {

namespace {

constexpr auto kConfigurationInfoMessages =
    base::MakeFixedFlatMap<ConfigurationInfoEvent, base::StringPiece>(
        {{ConfigurationInfoEvent::kBrowserActivated, "Browser ACTIVATED."},
         {ConfigurationInfoEvent::kBrowserInactivated, "Browser INACTIVATED."},
         {ConfigurationInfoEvent::kBrowserForegrounded,
          "Browser FOREGROUNDED."},
         {ConfigurationInfoEvent::kBrowserBackgrounded,
          "Browser BACKGROUNDED."},
         {ConfigurationInfoEvent::kFullScreen,
          "Window switched to FULLSCREEN state."},
         {ConfigurationInfoEvent::kWindowed,
          "Window switched to WINDOWED state."},
         {ConfigurationInfoEvent::kMinimized,
          "Window switched to MINIMIZED state."}});

std::string ToString(bool val) {
  return val ? "true" : "false";
}

std::string RotationToString(DisplayInfo::Rotation rotation) {
  switch (rotation) {
    case DisplayInfo::Rotation::Rotation_0:
      return "0 degrees";
    case DisplayInfo::Rotation::Rotation_90:
      return "90 degrees";
    case DisplayInfo::Rotation::Rotation_180:
      return "180 degrees";
    case DisplayInfo::Rotation::Rotation_270:
      return "270 degrees";
  }
}

}  // namespace

void WriteConfigurationInfoLog(ConfigurationInfoEvent event) {
  WriteConfigurationInfoLog(kConfigurationInfoMessages.at(event));
}

void WriteConfigurationInfoLog(base::StringPiece log) {
  VLOG(3) << "ADS_LOG: " << log;
}

void WriteConfigurationInfoLog(
    const std::vector<BrowserWindowInfo>& windows_info) {
  for (const BrowserWindowInfo& info : windows_info) {
    ads::WriteConfigurationInfoLog("Browser window: " + info.name);
    ads::WriteConfigurationInfoLog(" - fullcreen " +
                                   ToString(info.is_fullscreen));
    ads::WriteConfigurationInfoLog(" - maximized " +
                                   ToString(info.is_maximized));
    ads::WriteConfigurationInfoLog(" - minimized " +
                                   ToString(info.is_minimized));
  }
}

void WriteConfigurationInfoLog(const FocusAssistStatus& status) {
  ads::WriteConfigurationInfoLog("Focus assist status:");
  ads::WriteConfigurationInfoLog(" - enabled: " + ToString(status.enabled));
  ads::WriteConfigurationInfoLog(" - reason: " + status.reason);
}

void WriteConfigurationInfoLog(const NativeNotificationsStatus& status) {
  ads::WriteConfigurationInfoLog("Native notifications status:");
  ads::WriteConfigurationInfoLog(" - enabled: " + ToString(status.enabled));
  ads::WriteConfigurationInfoLog(" - reason: " + status.reason);
}

void WriteConfigurationInfoLog(
    const std::vector<WindowParams>& windows_params) {
  for (const WindowParams& params : windows_params) {
    ads::WriteConfigurationInfoLog("OS window: ");
    ads::WriteConfigurationInfoLog(" - title: " + params.title);
  }
}

void WriteConfigurationInfoLog(const std::vector<DisplayInfo>& displays_info) {
  ads::WriteConfigurationInfoLog("Displays info: ");
  for (size_t i = 0; i < displays_info.size(); ++i) {
    ads::WriteConfigurationInfoLog("Display " + base::NumberToString(i) + ":");
    ads::WriteConfigurationInfoLog(" - rotaion: " +
                                   RotationToString(displays_info[i].rotation));
  }
}

}  // namespace ads
