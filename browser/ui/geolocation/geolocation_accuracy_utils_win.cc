/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/geolocation/geolocation_accuracy_utils_win.h"

#include <windows.h>

#include <shellapi.h>
#include <windows.devices.enumeration.h>
#include <windows.foundation.h>
#include <wrl/event.h>

#include "base/logging.h"
#include "base/strings/string_piece.h"
#include "base/threading/scoped_blocking_call.h"
#include "base/threading/scoped_thread_priority.h"
#include "base/win/core_winrt_util.h"

using ABI::Windows::Devices::Enumeration::DeviceAccessStatus;
using ABI::Windows::Devices::Enumeration::DeviceClass;
using ABI::Windows::Devices::Enumeration::IDeviceAccessInformation;
using ABI::Windows::Devices::Enumeration::IDeviceAccessInformationStatics;
using Microsoft::WRL::ComPtr;

// Copied from services/device/geolocation/win/location_provider_winrt.cc
bool IsSystemLocationSettingEnabled() {
  ComPtr<IDeviceAccessInformationStatics> dev_access_info_statics;
  HRESULT hr = base::win::GetActivationFactory<
      IDeviceAccessInformationStatics,
      RuntimeClass_Windows_Devices_Enumeration_DeviceAccessInformation>(
      &dev_access_info_statics);
  if (FAILED(hr)) {
    VLOG(1) << "IDeviceAccessInformationStatics failed: " << hr;
    return true;
  }

  ComPtr<IDeviceAccessInformation> dev_access_info;
  hr = dev_access_info_statics->CreateFromDeviceClass(
      DeviceClass::DeviceClass_Location, &dev_access_info);
  if (FAILED(hr)) {
    VLOG(1) << "IDeviceAccessInformation failed: " << hr;
    return true;
  }

  auto status = DeviceAccessStatus::DeviceAccessStatus_Unspecified;
  dev_access_info->get_CurrentStatus(&status);

  return !(status == DeviceAccessStatus::DeviceAccessStatus_DeniedBySystem ||
           status == DeviceAccessStatus::DeviceAccessStatus_DeniedByUser);
}

void LaunchLocationServiceSettings() {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);

  SHELLEXECUTEINFO sei = {sizeof(sei)};
  sei.nShow = SW_SHOWNORMAL;
  sei.lpVerb = L"open";
  sei.lpFile = L"ms-settings:privacy-location";

  // Mitigate the issues caused by loading DLLs on a background thread
  // (http://crbug/973868).
  SCOPED_MAY_LOAD_LIBRARY_AT_BACKGROUND_PRIORITY();

  ::ShellExecuteExW(&sei);
}
