/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/rpill/rpill.h"

#include <string>
#include <vector>

#include "base/strings/string_util.h"
#include "bat/ads/ads.h"

namespace ads {

namespace {

std::string GetHardware() {
  const std::string manufacturer = base::ToLowerASCII(g_sys_info.manufacturer);
  const std::string model = base::ToLowerASCII(g_sys_info.model);
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

bool IsUncertainFuture() {
  return IsVirtualMachine();
}

}  // namespace ads
