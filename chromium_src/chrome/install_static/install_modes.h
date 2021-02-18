/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_INSTALL_STATIC_INSTALL_MODES_H_
#define BRAVE_CHROMIUM_SRC_CHROME_INSTALL_STATIC_INSTALL_MODES_H_

#include <string>

#define GetClientsKeyPath                                  \
  GetClientsKeyPath_ChromiumImpl(const wchar_t* app_guid); \
  std::wstring GetClientsKeyPath

#define GetClientStateKeyPath                                  \
  GetClientStateKeyPath_ChromiumImpl(const wchar_t* app_guid); \
  std::wstring GetClientStateKeyPath

#define GetClientStateMediumKeyPath                                  \
  GetClientStateMediumKeyPath_ChromiumImpl(const wchar_t* app_guid); \
  std::wstring GetClientStateMediumKeyPath

#include "../../../../chrome/install_static/install_modes.h"

#undef GetClientsKeyPath
#undef GetClientStateKeyPath
#undef GetClientStateMediumKeyPath

#endif  // BRAVE_CHROMIUM_SRC_CHROME_INSTALL_STATIC_INSTALL_MODES_H_
