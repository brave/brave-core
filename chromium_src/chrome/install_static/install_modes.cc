/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/install_static/install_modes.h"

#include "chrome/install_static/buildflags.h"

namespace install_static {

namespace {

#if !defined(OFFICIAL_BUILD)
std::wstring GetUnregisteredKeyPathForProduct() {
  return std::wstring(L"Software\\").append(kProductPathName);
}
#endif

}  // namespace

std::wstring GetClientsKeyPath(const wchar_t* app_guid) {
#if defined(OFFICIAL_BUILD)
  return std::wstring(L"Software\\BraveSoftware\\Update\\Clients\\")
      .append(app_guid);
#else
  return GetUnregisteredKeyPathForProduct();
#endif
}

std::wstring GetClientStateKeyPath(const wchar_t* app_guid) {
#if defined(OFFICIAL_BUILD)
  return std::wstring(L"Software\\BraveSoftware\\Update\\ClientState\\")
      .append(app_guid);
#else
  return GetUnregisteredKeyPathForProduct();
#endif
}

std::wstring GetClientStateMediumKeyPath(const wchar_t* app_guid) {
#if defined(OFFICIAL_BUILD)
  return std::wstring(L"Software\\BraveSoftware\\Update\\ClientStateMedium\\")
      .append(app_guid);
#else
  return GetUnregisteredKeyPathForProduct();
#endif
}

}  // namespace install_static
