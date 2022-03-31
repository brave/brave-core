/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/rpill/common/rpill.h"

#include <memory>
#include <string>
#include <utility>

#include "base/barrier_closure.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/strings/string_util.h"
#include "base/system/sys_info.h"

namespace brave_rpill {

namespace {

struct DeviceInfo {
  std::string manufacturer_name;
  std::string model_name;

  std::string id() const {
    return base::ToLowerASCII(manufacturer_name + model_name);
  }
};

bool IsUncertainFuture(DeviceInfo* device_info_ptr) {
  DCHECK(device_info_ptr);

  const std::string device_id = device_info_ptr->id();

  static const char* const kKeywords[] = {"kvm",
                                          "bochs",
                                          "virtual machine",
                                          "parallels",
                                          "vmware",
                                          "virtualbox",
                                          "amazon",
                                          "hvm domu",
                                          "xen"};

  for (const char* keyword : kKeywords) {
    if (device_id.find(keyword) != std::string::npos) {
      return true;
    }
  }

  return false;
}

void OnDeviceInfoReady(IsUncertainFutureCallback callback,
                       std::unique_ptr<DeviceInfo> device_info) {
  const bool is_uncertain_future = IsUncertainFuture(device_info.get());
  std::move(callback).Run(is_uncertain_future);
}

void OnHardwareInfoReady(DeviceInfo* device_info_ptr,
                         base::ScopedClosureRunner done_closure,
                         base::SysInfo::HardwareInfo hardware_info) {
  DCHECK(device_info_ptr);

  device_info_ptr->manufacturer_name = std::move(hardware_info.manufacturer);
  device_info_ptr->model_name = std::move(hardware_info.model);
}

}  // namespace

void DetectUncertainFuture(IsUncertainFutureCallback callback) {
  std::unique_ptr<DeviceInfo> device_info = std::make_unique<DeviceInfo>();
  DeviceInfo* device_info_ptr = device_info.get();

  base::RepeatingClosure done_closure = base::BarrierClosure(
      /*num_closures=*/1,
      base::BindOnce(&OnDeviceInfoReady, std::move(callback),
                     std::move(device_info)));

  base::SysInfo::GetHardwareInfo(
      base::BindOnce(&OnHardwareInfoReady, device_info_ptr,
                     base::ScopedClosureRunner(done_closure)));
}

}  // namespace brave_rpill
