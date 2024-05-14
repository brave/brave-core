/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/geolocation/geolocation_utils.h"

#include <windows.h>

#include <windows.devices.enumeration.h>
#include <windows.foundation.h>
#include <wrl/event.h>

#include <utility>

#include "base/functional/callback.h"
#include "base/logging.h"
#include "base/task/thread_pool.h"
#include "base/win/core_winrt_util.h"

using ABI::Windows::Devices::Enumeration::DeviceAccessStatus;
using ABI::Windows::Devices::Enumeration::DeviceClass;
using ABI::Windows::Devices::Enumeration::IDeviceAccessInformation;
using ABI::Windows::Devices::Enumeration::IDeviceAccessInformationStatics;
using Microsoft::WRL::ComPtr;

namespace geolocation {

namespace {

// Copied from services/device/geolocation/win/location_provider_winrt.cc
bool GetSystemLocationSettingEnabled() {
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

}  // namespace

void IsSystemLocationSettingEnabled(base::OnceCallback<void(bool)> callback) {
  base::ThreadPool::CreateCOMSTATaskRunner({base::MayBlock()})
      ->PostTaskAndReplyWithResult(
          FROM_HERE, base::BindOnce(&GetSystemLocationSettingEnabled),
          std::move(callback));
}

bool CanGiveDetailedGeolocationRequestInfo() {
  return true;
}

}  // namespace geolocation
