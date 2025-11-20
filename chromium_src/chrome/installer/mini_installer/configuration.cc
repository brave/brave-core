/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// This file contains code that used to be upstream and had to be restored in
// Brave to support delta updates on Windows until we are on Omaha 4. See:
// github.com/brave/brave-core/pull/31937

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(crbug.com/40285824): Remove this and convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include "chrome/installer/mini_installer/configuration.h"

#include <cassert>

#include "build/branding_buildflags.h"
#include "chrome/installer/mini_installer/appid.h"
#include "chrome/installer/mini_installer/mini_string.h"

#define Initialize() Initialize(HMODULE module)

#define kAppGuid \
  kAppGuid;      \
  previous_version_ = nullptr

#if defined(OFFICIAL_BUILD) && !BUILDFLAG(GOOGLE_CHROME_BRANDING)
#undef BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING
#define BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING() (1)
#define NEED_TO_RESET_BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING
#endif

#if defined(OFFICIAL_BUILD)
namespace {
DWORD BraveGetEnvironmentVariableW(const wchar_t* var,
                                   wchar_t* value,
                                   DWORD bufSize) {
  assert(::lstrcmp(var, L"GoogleUpdateIsMachine") == 0);
  return ::GetEnvironmentVariableW(L"BraveSoftwareUpdateIsMachine", value,
                                   bufSize);
}
}  // namespace
#define GetEnvironmentVariableW BraveGetEnvironmentVariableW
#endif

#define BRAVE_READ_RESOURCES ReadResources(module);

#include <chrome/installer/mini_installer/configuration.cc>

#undef BRAVE_READ_RESOURCES

#if defined(OFFICIAL_BUILD)
#undef GetEnvironmentVariableW
#endif

#if defined(NEED_TO_RESET_BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING)
#undef BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING
#define BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING() (0)
#undef NEED_TO_RESET_BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING
#endif

#undef kAppGuid
#undef Initialize

namespace mini_installer {

void Configuration::ReadResources(HMODULE module) {
  HRSRC resource_info_block =
      FindResource(module, MAKEINTRESOURCE(ID_PREVIOUS_VERSION), RT_RCDATA);
  if (!resource_info_block) {
    return;
  }

  HGLOBAL data_handle = LoadResource(module, resource_info_block);
  if (!data_handle) {
    return;
  }

  // The data is a Unicode string, so it must be a multiple of two bytes.
  DWORD version_size = SizeofResource(module, resource_info_block);
  if (!version_size || (version_size & 0x01) != 0) {
    return;
  }

  void* version_data = LockResource(data_handle);
  if (!version_data) {
    return;
  }

  const wchar_t* version_string = reinterpret_cast<wchar_t*>(version_data);
  size_t version_len = version_size / sizeof(wchar_t);

  // The string must be terminated.
  if (version_string[version_len - 1]) {
    return;
  }

  previous_version_ = version_string;
}

}  // namespace mini_installer
