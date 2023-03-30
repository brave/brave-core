/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/rpill/common/rpill.h"

#include <memory>
#include <string>
#include <utility>

#include "base/functional/callback.h"
#include "base/strings/string_util.h"
#include "base/system/sys_info.h"

namespace brave_rpill {

namespace {

bool IsUncertainFuture(const base::SysInfo::HardwareInfo& hardware_info) {
  const std::string device_id =
      base::ToLowerASCII(hardware_info.manufacturer + hardware_info.model);

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

void OnHardwareInfoReady(IsUncertainFutureCallback callback,
                         base::SysInfo::HardwareInfo hardware_info) {
  const bool is_uncertain_future = IsUncertainFuture(hardware_info);
  std::move(callback).Run(is_uncertain_future);
}

}  // namespace

void DetectUncertainFuture(IsUncertainFutureCallback callback) {
  base::SysInfo::GetHardwareInfo(
      base::BindOnce(&OnHardwareInfoReady, std::move(callback)));
}

}  // namespace brave_rpill
