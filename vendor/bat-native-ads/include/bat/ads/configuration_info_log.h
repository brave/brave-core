/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_CONFIGURATION_INFO_LOG_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_CONFIGURATION_INFO_LOG_H_

#include <string>
#include <vector>

#include "base/strings/string_piece.h"

namespace ads {

struct DisplayInfo {
  enum class Rotation {
    Rotation_0,
    Rotation_90,
    Rotation_180,
    Rotation_270
  };

  Rotation rotation;
};

struct BrowserWindowInfo {
  std::string name;
  bool is_fullscreen;
  bool is_maximized;
  bool is_minimized;
};

struct FocusAssistStatus {
  bool enabled;
  std::string reason;
};

struct NativeNotificationsStatus {
  bool enabled;
  std::string reason;
};

struct WindowParams {
  std::string title;
};

enum class ConfigurationInfoEvent {
  kBrowserActivated,
  kBrowserInactivated,
  kBrowserForegrounded,
  kBrowserBackgrounded,
  kFocusAssistOn,
  kFocusAssistOff,
  kAllWindowsTimer,
  kFullScreen,
  kWindowed,
  kMinimized
};

void WriteConfigurationInfoLog(ConfigurationInfoEvent event);

void WriteConfigurationInfoLog(base::StringPiece log);

void WriteConfigurationInfoLog(
    const std::vector<BrowserWindowInfo>& windows_info);

void WriteConfigurationInfoLog(const FocusAssistStatus& focus_assist_status);

void WriteConfigurationInfoLog(
    const NativeNotificationsStatus& native_notifications_status);

void WriteConfigurationInfoLog(
    const std::vector<WindowParams>& windows_params);

void WriteConfigurationInfoLog(const std::vector<DisplayInfo>& displays_info);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_CONFIGURATION_INFO_LOG_H_
