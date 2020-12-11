/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/rpill/rpill_helper.h"

#include <string>
#include <vector>

#include "base/strings/string_util.h"
#include "base/system/sys_info.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/sys_info_helper.h"

namespace ads {

namespace {

std::string GetHardware() {
  const base::SysInfo::HardwareInfo hardware =
      SysInfoHelper::GetInstance()->GetHardware();

  const std::string manufacturer = base::ToLowerASCII(hardware.manufacturer);
  const std::string model = base::ToLowerASCII(hardware.model);

  return manufacturer + model;
}

std::vector<std::string> GetKeywords() {
  return {
    "amazon",
    "virtualbox",
    "vmware",
    "xen"
  };
}

bool IsVirtualMachine() {
  const std::string hardware = GetHardware();
  const std::vector<std::string> keywords = GetKeywords();

  for (const auto& keyword : keywords) {
    if (hardware.find(keyword) != std::string::npos) {
      return true;
    }
  }

  return false;
}

}  // namespace

RPillHelper* g_rpill_helper_for_testing = nullptr;

RPillHelper::RPillHelper() = default;

RPillHelper::~RPillHelper() = default;

void RPillHelper::set_for_testing(
    RPillHelper* rpill_helper) {
  g_rpill_helper_for_testing = rpill_helper;
}

bool RPillHelper::IsUncertainFuture() const {
  return IsVirtualMachine();
}

RPillHelper* RPillHelper::GetInstance() {
  if (g_rpill_helper_for_testing) {
    return g_rpill_helper_for_testing;
  }

  return GetInstanceImpl();
}

RPillHelper* RPillHelper::GetInstanceImpl() {
  return base::Singleton<RPillHelper>::get();
}

}  // namespace ads
