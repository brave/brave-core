/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/strings/string_util.h"
#include "base/win/wmi.h"
#include "brave/components/rpill/rpill.h"

namespace rpill {

bool validate_os_win() {
#if defined(OS_WIN)
  base::win::WmiComputerSystemInfo info =
      base::win::WmiComputerSystemInfo::Get();
  auto m = info.manufacturer();
  auto mo = info.model();
  const std::string man(m.begin(), m.end());
  const std::string mod(mo.begin(), mo.end());
  const std::string manmod = base::ToLowerASCII(man) + base::ToLowerASCII(mod);

  return manmod.find("vm") != std::string::npos ||
         manmod.find("virtual") != std::string::npos ||
         manmod.find("xen") != std::string::npos ||
         manmod.find("amazon") != std::string::npos;
#else
  return false;
#endif  // defined(OS_WIN)
}

}  // namespace rpill
