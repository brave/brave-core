/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_COMPONENT_UPDATER_WIDEVINE_CDM_COMPONENT_INSTALLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_COMPONENT_UPDATER_WIDEVINE_CDM_COMPONENT_INSTALLER_H_

#include "brave/browser/widevine/buildflags.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

#define RegisterWidevineCdmComponent_Prefix \
  RegisterWidevineCdmComponent(                                          \
      ComponentUpdateService* cus,

#if BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)
#define RegisterWidevineCdmComponent_Body \
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
#else
#define RegisterWidevineCdmComponent_Body
#endif

#define RegisterWidevineCdmComponent_Suffix                     \
      base::OnceCallback<void()> callback = base::DoNothing()); \
  void RegisterWidevineCdmComponent_Unused

#define RegisterWidevineCdmComponent                                    \
  RegisterWidevineCdmComponent_Prefix RegisterWidevineCdmComponent_Body \
      RegisterWidevineCdmComponent_Suffix

#include "src/chrome/browser/component_updater/widevine_cdm_component_installer.h"  // IWYU pragma: export

#undef RegisterWidevineCdmComponent
#undef RegisterWidevineCdmComponent_Prefix
#undef RegisterWidevineCdmComponent_Body
#undef RegisterWidevineCdmComponent_Suffix

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_COMPONENT_UPDATER_WIDEVINE_CDM_COMPONENT_INSTALLER_H_
