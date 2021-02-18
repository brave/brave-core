/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/install_static/install_modes.h"

namespace {

std::wstring GetClientsKeyPathForApp(const wchar_t* app_guid) {
  return std::wstring(L"Software\\BraveSoftware\\Update\\Clients\\")
      .append(app_guid);
}

std::wstring GetClientStateKeyPathForApp(const wchar_t* app_guid) {
  return std::wstring(L"Software\\BraveSoftware\\Update\\ClientState\\")
      .append(app_guid);
}

std::wstring GetClientStateMediumKeyPathForApp(const wchar_t* app_guid) {
  return std::wstring(L"Software\\BraveSoftware\\Update\\ClientStateMedium\\")
      .append(app_guid);
}

}  // namespace

#define GetClientsKeyPathForApp GetClientsKeyPathForApp_ChromiumImpl
#define GetClientStateKeyPathForApp GetClientStateKeyPathForApp_ChromiumImpl
#define GetClientStateMediumKeyPathForApp \
  GetClientStateMediumKeyPathForApp_ChromiumImpl
#define GetClientsKeyPath GetClientsKeyPath_ChromiumImpl
#define GetClientStateKeyPath GetClientStateKeyPath_ChromiumImpl
#define GetClientStateMediumKeyPath GetClientStateMediumKeyPath_ChromiumImpl

#include "../../../../chrome/install_static/install_modes.cc"

#undef GetClientsKeyPath
#undef GetClientStateKeyPath
#undef GetClientStateMediumKeyPath
#undef GetClientsKeyPathForApp
#undef GetClientStateKeyPathForApp
#undef GetClientStateMediumKeyPathForApp

namespace install_static {

std::wstring GetClientsKeyPath(const wchar_t* app_guid) {
#if defined(OFFICIAL_BUILD)
  return GetClientsKeyPathForApp(app_guid);
#else
  return GetClientsKeyPath_ChromiumImpl(app_guid);
#endif
}

std::wstring GetClientStateKeyPath(const wchar_t* app_guid) {
#if defined(OFFICIAL_BUILD)
  return GetClientStateKeyPathForApp(app_guid);
#else
  return GetClientStateKeyPath_ChromiumImpl(app_guid);
#endif
}

std::wstring GetClientStateMediumKeyPath(const wchar_t* app_guid) {
#if defined(OFFICIAL_BUILD)
  return GetClientStateMediumKeyPathForApp(app_guid);
#else
  return GetClientStateMediumKeyPath_ChromiumImpl(app_guid);
#endif
}

}  // namespace install_static
